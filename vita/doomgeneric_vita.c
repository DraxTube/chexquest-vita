#include "doomgeneric.h"
#include "doomkeys.h"
#include "doomtype.h"

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
#include <stdarg.h>

#define TICRATE 35
#define SCREENWIDTH 320
#define SCREENHEIGHT 200

#define VITA_W       960
#define VITA_H       544
#define VITA_STRIDE  960
#define DOOM_W       DOOMGENERIC_RESX
#define DOOM_H       DOOMGENERIC_RESY

/* Globals the engine needs */
byte *I_VideoBuffer = NULL;
int screenvisible = 1;
int screensaver_mode = 0;
int vanilla_keyboard_mapping = 0;
int usegamma = 0;
int usemouse = 0;
int snd_musicdevice = 0;
int mouse_acceleration = 0;
int mouse_threshold = 0;

/* Framebuffer - single buffer, simpler */
static uint32_t vita_framebuffer[960 * 544] __attribute__((aligned(256)));
static uint64_t start_tick = 0;
static int display_ready = 0;

static void debug_log(const char *msg) {
    FILE *f = fopen("ux0:/data/chexquest/debug.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static void debug_logf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debug_log(buf);
}

static void show_color(uint32_t color) {
    int i;
    for (i = 0; i < 960 * 544; i++)
        vita_framebuffer[i] = color;

    SceDisplayFrameBuf fb;
    memset(&fb, 0, sizeof(fb));
    fb.size = sizeof(fb);
    fb.base = vita_framebuffer;
    fb.pitch = VITA_W;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = VITA_W;
    fb.height = VITA_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
}

/* Input */
#define KQUEUE_SZ 64
#define DEADZONE  35

static struct { int pressed; unsigned char key; } kq[KQUEUE_SZ];
static int kq_r = 0, kq_w = 0;
static SceCtrlData pad_prev;
static int input_init = 0;
static int analog_held[6];

static void kq_push(int p, unsigned char k) {
    int n = (kq_w + 1) % KQUEUE_SZ;
    if (n == kq_r) return;
    kq[kq_w].pressed = p;
    kq[kq_w].key = k;
    kq_w = n;
}

static void analog_axis(int val, int neg_key, int pos_key, int *neg_held, int *pos_held) {
    int want_neg = val < -DEADZONE;
    int want_pos = val > DEADZONE;
    if (want_neg && !*neg_held) { kq_push(1, neg_key); *neg_held = 1; }
    if (!want_neg && *neg_held) { kq_push(0, neg_key); *neg_held = 0; }
    if (want_pos && !*pos_held) { kq_push(1, pos_key); *pos_held = 1; }
    if (!want_pos && *pos_held) { kq_push(0, pos_key); *pos_held = 0; }
}

static void poll_input(void) {
    SceCtrlData pad;
    int i;

    if (!input_init) {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
        sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
        memset(&pad_prev, 0, sizeof(pad_prev));
        memset(analog_held, 0, sizeof(analog_held));
        input_init = 1;
    }

    sceCtrlPeekBufferPositive(0, &pad, 1);

    struct { unsigned btn; unsigned char key; } bm[] = {
        {SCE_CTRL_UP,       KEY_UPARROW},
        {SCE_CTRL_DOWN,     KEY_DOWNARROW},
        {SCE_CTRL_LEFT,     KEY_LEFTARROW},
        {SCE_CTRL_RIGHT,    KEY_RIGHTARROW},
        {SCE_CTRL_CROSS,    KEY_FIRE},
        {SCE_CTRL_CIRCLE,   KEY_USE},
        {SCE_CTRL_SQUARE,   KEY_RALT},
        {SCE_CTRL_TRIANGLE, KEY_TAB},
        {SCE_CTRL_RTRIGGER, KEY_FIRE},
        {SCE_CTRL_LTRIGGER, KEY_RSHIFT},
        {SCE_CTRL_START,    KEY_ESCAPE},
        {SCE_CTRL_SELECT,   KEY_ENTER},
        {0, 0}
    };

    for (i = 0; bm[i].btn; i++) {
        int now = (pad.buttons & bm[i].btn) != 0;
        int was = (pad_prev.buttons & bm[i].btn) != 0;
        if (now && !was) kq_push(1, bm[i].key);
        if (!now && was) kq_push(0, bm[i].key);
    }

    analog_axis(pad.ly - 128, KEY_UPARROW, KEY_DOWNARROW, &analog_held[0], &analog_held[1]);
    analog_axis(pad.lx - 128, KEY_STRAFE_L, KEY_STRAFE_R, &analog_held[2], &analog_held[3]);
    analog_axis(pad.rx - 128, KEY_LEFTARROW, KEY_RIGHTARROW, &analog_held[4], &analog_held[5]);

    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    if (touch.reportNum > 0 && touch.report[0].y / 2 < 60) {
        int slot = (touch.report[0].x / 2) / (VITA_W / 7);
        if (slot >= 0 && slot < 7) { kq_push(1, '1'+slot); kq_push(0, '1'+slot); }
    }

    pad_prev = pad;
}

/* DG interface */
void DG_Init(void) {
    debug_log("DG_Init called");
    /* Display already initialized in main() */
    display_ready = 1;
    show_color(0xFFFFFF00); /* cyan = DG_Init OK */
    sceKernelDelayThread(500000);
}

void DG_DrawFrame(void) {
    if (!display_ready) return;

    uint32_t *src = (uint32_t *)DG_ScreenBuffer;
    if (!src) return;

    int x, y;
    int step_x = (DOOM_W << 16) / VITA_W;
    int step_y = (DOOM_H << 16) / VITA_H;

    int src_y_fixed = 0;
    for (y = 0; y < VITA_H; y++) {
        int srcy = src_y_fixed >> 16;
        if (srcy >= DOOM_H) srcy = DOOM_H - 1;
        uint32_t *dst_row = vita_framebuffer + y * VITA_W;
        uint32_t *src_row = src + srcy * DOOM_W;

        int src_x_fixed = 0;
        for (x = 0; x < VITA_W; x++) {
            int srcx = src_x_fixed >> 16;
            if (srcx >= DOOM_W) srcx = DOOM_W - 1;
            uint32_t p = src_row[srcx];
            uint8_t r = (p >> 16) & 0xFF;
            uint8_t g = (p >> 8) & 0xFF;
            uint8_t b = p & 0xFF;
            dst_row[x] = 0xFF000000 | (b << 16) | (g << 8) | r;
            src_x_fixed += step_x;
        }
        src_y_fixed += step_y;
    }

    SceDisplayFrameBuf dfb;
    memset(&dfb, 0, sizeof(dfb));
    dfb.size = sizeof(dfb);
    dfb.base = vita_framebuffer;
    dfb.pitch = VITA_W;
    dfb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    dfb.width = VITA_W;
    dfb.height = VITA_H;
    sceDisplaySetFrameBuf(&dfb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();

    poll_input();
}

void DG_SleepMs(uint32_t ms) { sceKernelDelayThread(ms * 1000); }

uint32_t DG_GetTicksMs(void) {
    SceRtcTick t; sceRtcGetCurrentTick(&t);
    return (uint32_t)((t.tick - start_tick) / 1000);
}

int DG_GetKey(int *pressed, unsigned char *key) {
    if (kq_r == kq_w) return 0;
    *pressed = kq[kq_r].pressed;
    *key = kq[kq_r].key;
    kq_r = (kq_r + 1) % KQUEUE_SZ;
    return 1;
}

void DG_SetWindowTitle(const char *t) { (void)t; }

/* === ALL STUBS === */
void I_Init(void) {}
void I_Quit(void) { sceKernelExitProcess(0); }
void I_Error(char *error, ...) {
    char buf[512];
    va_list args;
    va_start(args, error);
    vsnprintf(buf, sizeof(buf), error, args);
    va_end(args);
    debug_log(buf);
    show_color(0xFF0000FF); /* red */
    sceKernelDelayThread(5000000);
    sceKernelExitProcess(0);
}
void I_WaitVBL(int count) { sceKernelDelayThread(count * 14286); }
int I_GetTime(void) { return DG_GetTicksMs() * TICRATE / 1000; }
void I_Sleep(int ms) { DG_SleepMs(ms); }
byte *I_ZoneBase(int *size) {
    *size = 16 * 1024 * 1024;
    byte *ptr = (byte *)malloc(*size);
    if (!ptr) { *size = 8 * 1024 * 1024; ptr = (byte *)malloc(*size); }
    if (!ptr) { *size = 4 * 1024 * 1024; ptr = (byte *)malloc(*size); }
    debug_logf("ZoneBase: allocated %d bytes at %p", *size, ptr);
    return ptr;
}
void I_Tactile(int on, int off, int total) { (void)on; (void)off; (void)total; }
int I_ConsoleStdout(void) { return 0; }
boolean I_GetMemoryValue(unsigned int offset, void *value, int size) {
    (void)offset; (void)value; (void)size; return 0;
}
void I_AtExit(void (*func)(void), boolean run_on_error) { (void)func; (void)run_on_error; }
void I_PrintBanner(const char *msg) { (void)msg; }
void I_PrintDivider(void) {}
void I_PrintStartupBanner(const char *gamedescription) { (void)gamedescription; }
void I_DisplayFPSDots(boolean dots_on) { (void)dots_on; }
void I_CheckIsScreensaver(void) {}
void I_GraphicsCheckCommandLine(void) {}
void I_SetGrabMouseCallback(void (*func)(boolean grab)) { (void)func; }
int I_GetTime_RealTime(void) { return DG_GetTicksMs() * TICRATE / 1000; }
int I_GetTimeMS(void) { return DG_GetTicksMs(); }
void I_InitTimer(void) {}
void I_InitGraphics(void) {
    debug_log("I_InitGraphics");
    I_VideoBuffer = (byte *)calloc(SCREENWIDTH * SCREENHEIGHT, 1);
    debug_logf("I_VideoBuffer = %p", I_VideoBuffer);
}
void I_ShutdownGraphics(void) {}
void I_StartFrame(void) {}
void I_StartTic(void) {}
void I_UpdateNoBlit(void) {}
void I_FinishUpdate(void) {}
void I_ReadScreen(byte *scr) {
    if (I_VideoBuffer) memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}
void I_SetPalette(byte *palette) { (void)palette; }
void I_EnableLoadingDisk(void) {}
void I_BeginRead(void) {}
void I_EndRead(void) {}
void I_SetWindowTitle(char *title) { (void)title; }
void I_BindVideoVariables(void) {}
int I_GetPaletteIndex(int r, int g, int b) { (void)r; (void)g; (void)b; return 0; }
void I_PrecacheSounds(void *sounds, int num_sounds) { (void)sounds; (void)num_sounds; }
void I_InitInput(void) {}
void I_ShutdownInput(void) {}
void I_InitJoystick(void) {}
void I_ShutdownJoystick(void) {}
void I_UpdateJoystick(void) {}
void I_BindJoystickVariables(void) {}
int I_GetSfxLumpNum(void *sfxinfo) { (void)sfxinfo; return 0; }
void I_SetChannels(void) {}
void I_SetSfxVolume(int volume) { (void)volume; }
void I_SetMusicVolume(int volume) { (void)volume; }
int I_StartSound(void *sfxinfo, int channel, int vol, int sep, int pitch) {
    (void)sfxinfo; (void)channel; (void)vol; (void)sep; (void)pitch; return 0;
}
void I_StopSound(int channel) { (void)channel; }
int I_SoundIsPlaying(int channel) { (void)channel; return 0; }
void I_UpdateSound(void) {}
void I_UpdateSoundParams(int channel, int vol, int sep) {
    (void)channel; (void)vol; (void)sep;
}
void I_ShutdownSound(void) {}
void I_InitSound(int use_sfx_prefix) { (void)use_sfx_prefix; }
void I_ShutdownMusic(void) {}
void I_InitMusic(void) {}
void I_PlaySong(void *handle, int looping) { (void)handle; (void)looping; }
void I_PauseSong(void) {}
void I_ResumeSong(void) {}
void I_StopSong(void) {}
void I_UnRegisterSong(void *handle) { (void)handle; }
void *I_RegisterSong(void *data, int len) { (void)data; (void)len; return NULL; }
int I_MusicIsPlaying(void) { return 0; }
void I_BindSoundVariables(void) {}
int I_CDMusInit(void) { return 0; }
void I_CDMusShutdown(void) {}
void I_CDMusUpdate(void) {}
void I_CDMusStop(void) {}
int I_CDMusPlay(int track) { (void)track; return 0; }
void I_CDMusSetVolume(int vol) { (void)vol; }
int I_CDMusFirstTrack(void) { return 0; }
int I_CDMusLastTrack(void) { return 0; }
int I_CDMusTrackLength(int track) { (void)track; return 0; }
void I_Endoom(byte *data) { (void)data; }
void I_InitScale(void) {}
char *gus_patch_path = "";
int gus_ram_kb = 0;

/* MAIN */
int main(int argc, char **argv) {
    /* Basic init */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    SceAppUtilInitParam ip; SceAppUtilBootParam bp;
    memset(&ip, 0, sizeof(ip)); memset(&bp, 0, sizeof(bp));
    sceAppUtilInit(&ip, &bp);

    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 0777);

    /* Clear old log */
    sceIoRemove("ux0:/data/chexquest/debug.log");
    debug_log("=== Chex Quest Vita ===");

    /* Init framebuffer with static array (no alloc needed) */
    memset(vita_framebuffer, 0, sizeof(vita_framebuffer));

    /* Show GREEN = we are alive */
    debug_log("Showing green screen");
    show_color(0xFF00FF00);
    sceKernelDelayThread(2000000); /* 2 seconds */

    /* Get start tick */
    SceRtcTick t;
    sceRtcGetCurrentTick(&t);
    start_tick = t.tick;

    /* Find WAD */
    const char *wad = NULL;
    const char *paths[] = {
        "ux0:/data/chexquest/chex.wad",
        "ux0:/data/chexquest/doom1.wad",
        "ux0:/data/chexquest/doom.wad",
        "ux0:/data/chexquest/CHEX.WAD",
        "ux0:/data/chexquest/DOOM1.WAD",
        "ux0:/data/chexquest/DOOM.WAD",
        NULL
    };

    int i;
    for (i = 0; paths[i]; i++) {
        SceUID fd = sceIoOpen(paths[i], SCE_O_RDONLY, 0);
        if (fd >= 0) {
            SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
            sceIoClose(fd);
            wad = paths[i];
            debug_logf("Found: %s (%d bytes)", wad, (int)size);
            break;
        }
    }

    if (!wad) {
        debug_log("NO WAD FOUND!");
        /* YELLOW = no wad */
        show_color(0xFF00FFFF);
        sceKernelDelayThread(10000000);
        sceKernelExitProcess(0);
        return 1;
    }

    /* CYAN = starting engine */
    debug_log("Starting engine...");
    show_color(0xFFFFFF00);
    sceKernelDelayThread(1000000);

    char *nargv[] = { "ChexQuest", "-iwad", (char*)wad, NULL };
    debug_log("Calling doomgeneric_Create");
    doomgeneric_Create(3, nargv);
    debug_log("doomgeneric_Create returned OK");

    /* MAGENTA = entering game loop */
    show_color(0xFFFF00FF);
    sceKernelDelayThread(500000);

    debug_log("Entering main loop");
    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}
