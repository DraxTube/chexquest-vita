/*
 * doomgeneric_vita.c
 * 
 * PS Vita platform implementation for doomgeneric
 * This is the ONLY file needed to port Doom to Vita via doomgeneric.
 * 
 * doomgeneric requires implementing these functions:
 *   DG_Init()         - Initialize display/input
 *   DG_DrawFrame()    - Blit framebuffer to screen
 *   DG_SleepMs()      - Sleep
 *   DG_GetTicksMs()   - Get milliseconds
 *   DG_GetKey()       - Get input events
 *   DG_SetWindowTitle() - Set title (no-op on Vita)
 */

#include "doomgeneric.h"
#include "doomkeys.h"

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/power.h>
#include <psp2/apputil.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================
 * DISPLAY
 * ============================================================ */
#define VITA_SCREEN_W  960
#define VITA_SCREEN_H  544
#define DOOM_W         DOOMGENERIC_RESX   /* 640 in doomgeneric */
#define DOOM_H         DOOMGENERIC_RESY   /* 400 in doomgeneric */

/* We'll use vita2d for rendering */
#include <vita2d.h>

static vita2d_texture *doom_texture = NULL;
static uint64_t start_tick = 0;

/* ============================================================
 * INPUT
 * ============================================================ */
#define MAX_KEY_QUEUE 32
#define VITA_DEADZONE 30

typedef struct {
    int      pressed;
    unsigned key;
} key_event_t;

static key_event_t key_queue[MAX_KEY_QUEUE];
static int key_queue_read = 0;
static int key_queue_write = 0;

static SceCtrlData pad_prev;
static int input_initialized = 0;

static void queue_key(int pressed, unsigned key) {
    int next = (key_queue_write + 1) % MAX_KEY_QUEUE;
    if (next == key_queue_read) return;  /* queue full */
    key_queue[key_queue_write].pressed = pressed;
    key_queue[key_queue_write].key = key;
    key_queue_write = next;
}

/* Button-to-Doom-key mapping table */
typedef struct {
    unsigned int vita_btn;
    unsigned char doom_key;
} btn_map_t;

static const btn_map_t button_map[] = {
    { SCE_CTRL_UP,       KEY_UPARROW   },
    { SCE_CTRL_DOWN,     KEY_DOWNARROW  },
    { SCE_CTRL_LEFT,     KEY_LEFTARROW  },
    { SCE_CTRL_RIGHT,    KEY_RIGHTARROW },
    { SCE_CTRL_CROSS,    KEY_FIRE       },  /* X = Fire (Zorch!) */
    { SCE_CTRL_CIRCLE,   KEY_USE        },  /* O = Use/Open */
    { SCE_CTRL_SQUARE,   KEY_STRAFE_L   },  /* [] = Strafe Left */
    { SCE_CTRL_TRIANGLE, KEY_TAB        },  /* Triangle = Automap */
    { SCE_CTRL_RTRIGGER, KEY_FIRE       },  /* R = Fire (alt) */
    { SCE_CTRL_LTRIGGER, KEY_RSHIFT     },  /* L = Run */
    { SCE_CTRL_START,    KEY_ESCAPE     },  /* Start = Menu */
    { SCE_CTRL_SELECT,   KEY_ENTER      },  /* Select = Confirm */
    { 0, 0 }
};

static void poll_input(void) {
    SceCtrlData pad;
    const btn_map_t *m;

    if (!input_initialized) {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
        sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                                SCE_TOUCH_SAMPLING_STATE_START);
        memset(&pad_prev, 0, sizeof(pad_prev));
        input_initialized = 1;
    }

    sceCtrlPeekBufferPositive(0, &pad, 1);

    /* Check each mapped button */
    for (m = button_map; m->vita_btn; m++) {
        int now  = (pad.buttons & m->vita_btn) != 0;
        int prev = (pad_prev.buttons & m->vita_btn) != 0;
        if (now && !prev)  queue_key(1, m->doom_key);
        if (!now && prev)  queue_key(0, m->doom_key);
    }

    /* Right analog stick -> turning */
    {
        int rx = (int)pad.rx - 128;
        if (abs(rx) > VITA_DEADZONE) {
            /* Simulate left/right arrow presses for turning */
            if (rx > 0) {
                queue_key(1, KEY_RIGHTARROW);
                queue_key(0, KEY_RIGHTARROW);
            } else {
                queue_key(1, KEY_LEFTARROW);
                queue_key(0, KEY_LEFTARROW);
            }
        }
    }

    /* Left analog stick Y -> forward/backward */
    {
        int ly = (int)pad.ly - 128;
        if (abs(ly) > VITA_DEADZONE) {
            if (ly < 0) {
                queue_key(1, KEY_UPARROW);
                queue_key(0, KEY_UPARROW);
            } else {
                queue_key(1, KEY_DOWNARROW);
                queue_key(0, KEY_DOWNARROW);
            }
        }
    }

    /* Left analog stick X -> strafe */
    {
        int lx = (int)pad.lx - 128;
        if (abs(lx) > VITA_DEADZONE) {
            if (lx > 0) {
                queue_key(1, KEY_STRAFE_R);
                queue_key(0, KEY_STRAFE_R);
            } else {
                queue_key(1, KEY_STRAFE_L);
                queue_key(0, KEY_STRAFE_L);
            }
        }
    }

    /* Touch screen top strip -> weapon selection */
    {
        SceTouchData touch;
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
        if (touch.reportNum > 0) {
            int ty = touch.report[0].y / 2;
            if (ty < 60) {
                int tx = touch.report[0].x / 2;
                int slot = tx / (VITA_SCREEN_W / 7);
                if (slot >= 0 && slot < 7) {
                    queue_key(1, '1' + slot);
                    queue_key(0, '1' + slot);
                }
            }
        }
    }

    pad_prev = pad;
}

/* ============================================================
 * DOOMGENERIC INTERFACE
 * ============================================================ */

void DG_Init(void) {
    /* Overclock */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    /* Init app util */
    SceAppUtilInitParam init_param;
    SceAppUtilBootParam boot_param;
    memset(&init_param, 0, sizeof(init_param));
    memset(&boot_param, 0, sizeof(boot_param));
    sceAppUtilInit(&init_param, &boot_param);

    /* Create directories */
    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 0777);

    /* Init vita2d */
    vita2d_init();
    vita2d_set_clear_color(RGBA8(0, 0, 0, 255));
    vita2d_set_vblank_wait(1);

    /* Create texture for Doom's framebuffer */
    doom_texture = vita2d_create_empty_texture_format(
        DOOM_W, DOOM_H, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);

    if (!doom_texture) {
        sceKernelExitProcess(0);
        return;
    }

    /* Get start time */
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    start_tick = tick.tick;
}

void DG_DrawFrame(void) {
    /* DG_ScreenBuffer is Doom's 32-bit XRGB framebuffer */
    /* Copy it to the vita2d texture */
    uint32_t *tex_data = (uint32_t *)vita2d_texture_get_datap(doom_texture);
    uint32_t *src = (uint32_t *)DG_ScreenBuffer;
    int stride = vita2d_texture_get_stride(doom_texture) / 4;
    int x, y;

    for (y = 0; y < DOOM_H; y++) {
        for (x = 0; x < DOOM_W; x++) {
            /* Convert XRGB (Doom) to ABGR (Vita GXM) */
            uint32_t pixel = src[y * DOOM_W + x];
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            tex_data[y * stride + x] = RGBA8(r, g, b, 255);
        }
    }

    /* Draw to screen */
    vita2d_start_drawing();
    vita2d_clear_screen();

    /* Scale to fill Vita screen */
    float scale_x = (float)VITA_SCREEN_W / (float)DOOM_W;
    float scale_y = (float)VITA_SCREEN_H / (float)DOOM_H;

    vita2d_draw_texture_scale(doom_texture, 0, 0, scale_x, scale_y);

    vita2d_end_drawing();
    vita2d_swap_buffers();

    /* Poll input after drawing */
    poll_input();
}

void DG_SleepMs(uint32_t ms) {
    sceKernelDelayThread(ms * 1000);
}

uint32_t DG_GetTicksMs(void) {
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    return (uint32_t)((tick.tick - start_tick) / 1000);
}

int DG_GetKey(int *pressed, unsigned char *doom_key) {
    if (key_queue_read == key_queue_write)
        return 0;  /* no events */

    *pressed  = key_queue[key_queue_read].pressed;
    *doom_key = key_queue[key_queue_read].key;
    key_queue_read = (key_queue_read + 1) % MAX_KEY_QUEUE;
    return 1;
}

void DG_SetWindowTitle(const char *title) {
    (void)title;  /* no-op on Vita */
}

/* ============================================================
 * MAIN - Vita entry point
 * ============================================================ */
int main(int argc, char **argv) {
    /* Set up command line for Chex Quest WAD */
    char *new_argv[] = {
        "ChexQuest",
        "-iwad", "ux0:/data/chexquest/chex.wad",
        NULL
    };
    int new_argc = 3;

    /* Check if chex.wad exists, fallback to doom1.wad */
    SceUID fd = sceIoOpen("ux0:/data/chexquest/chex.wad", SCE_O_RDONLY, 0);
    if (fd < 0) {
        fd = sceIoOpen("ux0:/data/chexquest/doom1.wad", SCE_O_RDONLY, 0);
        if (fd >= 0) {
            sceIoClose(fd);
            new_argv[2] = "ux0:/data/chexquest/doom1.wad";
        } else {
            fd = sceIoOpen("ux0:/data/chexquest/doom.wad", SCE_O_RDONLY, 0);
            if (fd >= 0) {
                sceIoClose(fd);
                new_argv[2] = "ux0:/data/chexquest/doom.wad";
            }
        }
    } else {
        sceIoClose(fd);
    }

    doomgeneric_Create(new_argc, new_argv);

    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}
