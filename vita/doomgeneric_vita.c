/*
 * doomgeneric_vita.c
 * PS Vita platform implementation for doomgeneric
 * Uses raw SceDisplay + SceGxm framebuffer (no external libs needed)
 */

#include "doomgeneric.h"
#include "doomkeys.h"

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
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

/* ============================================================
 * DISPLAY - Direct framebuffer via SceDisplay
 * ============================================================ */
#define VITA_SCREEN_W   960
#define VITA_SCREEN_H   544
#define VITA_FB_STRIDE  1024  /* Must be power of 2 >= VITA_SCREEN_W */
#define DOOM_W          DOOMGENERIC_RESX
#define DOOM_H          DOOMGENERIC_RESY

/* Double-buffered framebuffer */
static uint32_t *vita_fb[2] = { NULL, NULL };
static int vita_fb_index = 0;
static SceUID vita_fb_uid[2];

static uint64_t start_tick = 0;

/* Allocate CDRAM for framebuffer */
static void *vita_gpu_alloc(SceKernelMemBlockType type, unsigned int size, SceUID *uid) {
    void *mem = NULL;
    *uid = sceKernelAllocMemBlock("fb", type, size, NULL);
    if (*uid < 0) return NULL;
    sceKernelGetMemBlockBase(*uid, &mem);
    return mem;
}

static void vita_init_display(void) {
    unsigned int fb_size = VITA_FB_STRIDE * VITA_SCREEN_H * 4;

    /* Allocate two framebuffers in CDRAM */
    vita_fb[0] = (uint32_t *)vita_gpu_alloc(
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, fb_size, &vita_fb_uid[0]);
    vita_fb[1] = (uint32_t *)vita_gpu_alloc(
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, fb_size, &vita_fb_uid[1]);

    if (!vita_fb[0] || !vita_fb[1]) {
        /* Fallback: use regular RAM */
        vita_fb[0] = (uint32_t *)memalign(256 * 1024, fb_size);
        vita_fb[1] = (uint32_t *)memalign(256 * 1024, fb_size);
    }

    /* Clear both buffers */
    memset(vita_fb[0], 0, fb_size);
    memset(vita_fb[1], 0, fb_size);

    /* Set initial framebuffer */
    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size = sizeof(fb);
    fb.base = vita_fb[0];
    fb.pitch = VITA_FB_STRIDE;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = VITA_SCREEN_W;
    fb.height = VITA_SCREEN_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

/* ============================================================
 * INPUT
 * ============================================================ */
#define MAX_KEY_QUEUE 64
#define VITA_DEADZONE 35

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
    if (next == key_queue_read) return;
    key_queue[key_queue_write].pressed = pressed;
    key_queue[key_queue_write].key = key;
    key_queue_write = next;
}

/* Track which analog-simulated keys are currently held */
static int analog_keys_held[4] = {0, 0, 0, 0}; /* up, down, left, right */

static void poll_input(void) {
    SceCtrlData pad;
    int i;

    if (!input_initialized) {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
        sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
        memset(&pad_prev, 0, sizeof(pad_prev));
        input_initialized = 1;
    }

    sceCtrlPeekBufferPositive(0, &pad, 1);

    /* === Digital buttons === */
    struct { unsigned int btn; unsigned char key; } bmap[] = {
        { SCE_CTRL_UP,       KEY_UPARROW   },
        { SCE_CTRL_DOWN,     KEY_DOWNARROW  },
        { SCE_CTRL_LEFT,     KEY_LEFTARROW  },
        { SCE_CTRL_RIGHT,    KEY_RIGHTARROW },
        { SCE_CTRL_CROSS,    KEY_FIRE       },
        { SCE_CTRL_CIRCLE,   KEY_USE        },
        { SCE_CTRL_SQUARE,   KEY_RALT       },  /* strafe modifier */
        { SCE_CTRL_TRIANGLE, KEY_TAB        },
        { SCE_CTRL_RTRIGGER, KEY_FIRE       },
        { SCE_CTRL_LTRIGGER, KEY_RSHIFT     },  /* run */
        { SCE_CTRL_START,    KEY_ESCAPE     },
        { SCE_CTRL_SELECT,   KEY_ENTER      },
        { 0, 0 }
    };

    for (i = 0; bmap[i].btn; i++) {
        int now  = (pad.buttons & bmap[i].btn) != 0;
        int prev = (pad_prev.buttons & bmap[i].btn) != 0;
        if (now && !prev)  queue_key(1, bmap[i].key);
        if (!now && prev)  queue_key(0, bmap[i].key);
    }

    /* === Left analog stick: movement === */
    {
        int lx = (int)pad.lx - 128;
        int ly = (int)pad.ly - 128;

        /* Forward/backward (Y axis) */
        int want_up = (ly < -VITA_DEADZONE);
        int want_down = (ly > VITA_DEADZONE);

        if (want_up && !analog_keys_held[0]) {
            queue_key(1, KEY_UPARROW);
            analog_keys_held[0] = 1;
        } else if (!want_up && analog_keys_held[0]) {
            queue_key(0, KEY_UPARROW);
            analog_keys_held[0] = 0;
        }

        if (want_down && !analog_keys_held[1]) {
            queue_key(1, KEY_DOWNARROW);
            analog_keys_held[1] = 1;
        } else if (!want_down && analog_keys_held[1]) {
            queue_key(0, KEY_DOWNARROW);
            analog_keys_held[1] = 0;
        }

        /* Strafe left/right (X axis) - send as strafe */
        int want_sleft = (lx < -VITA_DEADZONE);
        int want_sright = (lx > VITA_DEADZONE);

        if (want_sleft && !analog_keys_held[2]) {
            queue_key(1, KEY_STRAFE_L);
            analog_keys_held[2] = 1;
        } else if (!want_sleft && analog_keys_held[2]) {
            queue_key(0, KEY_STRAFE_L);
            analog_keys_held[2] = 0;
        }

        if (want_sright && !analog_keys_held[3]) {
            queue_key(1, KEY_STRAFE_R);
            analog_keys_held[3] = 1;
        } else if (!want_sright && analog_keys_held[3]) {
            queue_key(0, KEY_STRAFE_R);
            analog_keys_held[3] = 0;
        }
    }

    /* === Right analog stick: turning === */
    {
        static int rturn_left = 0, rturn_right = 0;
        int rx = (int)pad.rx - 128;

        int want_left = (rx < -VITA_DEADZONE);
        int want_right = (rx > VITA_DEADZONE);

        if (want_left && !rturn_left) {
            queue_key(1, KEY_LEFTARROW);
            rturn_left = 1;
        } else if (!want_left && rturn_left) {
            queue_key(0, KEY_LEFTARROW);
            rturn_left = 0;
        }

        if (want_right && !rturn_right) {
            queue_key(1, KEY_RIGHTARROW);
            rturn_right = 1;
        } else if (!want_right && rturn_right) {
            queue_key(0, KEY_RIGHTARROW);
            rturn_right = 0;
        }
    }

    /* === Touch screen: weapon select (top strip) === */
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
 * DOOMGENERIC INTERFACE - These 5+1 functions are ALL we need
 * ============================================================ */

void DG_Init(void) {
    /* Overclock CPU/GPU */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    /* Init app utilities */
    SceAppUtilInitParam init_param;
    SceAppUtilBootParam boot_param;
    memset(&init_param, 0, sizeof(init_param));
    memset(&boot_param, 0, sizeof(boot_param));
    sceAppUtilInit(&init_param, &boot_param);

    /* Create data directory */
    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 0777);

    /* Init display */
    vita_init_display();

    /* Get start time */
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    start_tick = tick.tick;
}

void DG_DrawFrame(void) {
    uint32_t *fb = vita_fb[vita_fb_index];
    uint32_t *src = (uint32_t *)DG_ScreenBuffer;
    int x, y;

    /* Calculate scaling */
    /* Doom renders at DOOM_W x DOOM_H (typically 640x400) */
    /* Vita screen is 960x544 */
    /* We'll scale with nearest-neighbor */

    float scale_x = (float)DOOM_W / (float)VITA_SCREEN_W;
    float scale_y = (float)DOOM_H / (float)VITA_SCREEN_H;

    for (y = 0; y < VITA_SCREEN_H; y++) {
        int src_y = (int)(y * scale_y);
        if (src_y >= DOOM_H) src_y = DOOM_H - 1;

        uint32_t *fb_row = fb + y * VITA_FB_STRIDE;
        uint32_t *src_row = src + src_y * DOOM_W;

        for (x = 0; x < VITA_SCREEN_W; x++) {
            int src_x = (int)(x * scale_x);
            if (src_x >= DOOM_W) src_x = DOOM_W - 1;

            /* Doom outputs XRGB8888, Vita wants ABGR8888 */
            uint32_t pixel = src_row[src_x];
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;

            fb_row[x] = (0xFF << 24) | (b << 16) | (g << 8) | r;
        }
    }

    /* Swap buffers */
    SceDisplayFrameBuf fb_param;
    memset(&fb_param, 0, sizeof(fb_param));
    fb_param.size = sizeof(fb_param);
    fb_param.base = fb;
    fb_param.pitch = VITA_FB_STRIDE;
    fb_param.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb_param.width = VITA_SCREEN_W;
    fb_param.height = VITA_SCREEN_H;
    sceDisplaySetFrameBuf(&fb_param, SCE_DISPLAY_SETBUF_NEXTFRAME);

    sceDisplayWaitVblankStart();

    vita_fb_index ^= 1;

    /* Poll input */
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
        return 0;

    *pressed  = key_queue[key_queue_read].pressed;
    *doom_key = key_queue[key_queue_read].key;
    key_queue_read = (key_queue_read + 1) % MAX_KEY_QUEUE;
    return 1;
}

void DG_SetWindowTitle(const char *title) {
    (void)title;
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char **argv) {
    /* Find WAD file */
    const char *wad = "ux0:/data/chexquest/chex.wad";
    SceUID fd;

    fd = sceIoOpen(wad, SCE_O_RDONLY, 0);
    if (fd < 0) {
        wad = "ux0:/data/chexquest/doom1.wad";
        fd = sceIoOpen(wad, SCE_O_RDONLY, 0);
        if (fd < 0) {
            wad = "ux0:/data/chexquest/doom.wad";
            fd = sceIoOpen(wad, SCE_O_RDONLY, 0);
        }
    }
    if (fd >= 0) sceIoClose(fd);

    char *new_argv[] = { "ChexQuest", "-iwad", (char *)wad, NULL };

    doomgeneric_Create(3, new_argv);

    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}
