/* doomgeneric_vita.c – Chex Quest / DOOM on PS Vita */

#include "doomgeneric.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "d_event.h"

/* D_PostEvent è in d_main.h ma dichiariamolo qui per sicurezza */
extern void D_PostEvent(event_t *ev);

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define TICRATE      35
#define SCREENWIDTH  320
#define SCREENHEIGHT 200

#define VITA_W 960
#define VITA_H 544

/* ================================================================
   Globals richiesti dal motore
   ================================================================ */
byte *I_VideoBuffer = NULL;
int   screenvisible = 1;
int   screensaver_mode = 0;
int   vanilla_keyboard_mapping = 0;
int   usegamma = 0;
int   usemouse = 0;
int   snd_musicdevice = 0;
int   mouse_acceleration = 0;
int   mouse_threshold = 0;

/* ================================================================
   Display
   ================================================================ */
static SceUID  fb_memuid;
static void   *fb_base = NULL;
static int     display_ready = 0;
static int     frame_count = 0;

/* Palette: indice 8-bit → colore 32-bit Vita (A8B8G8R8) */
static uint32_t cmap[256];

/* ================================================================
   Timing
   ================================================================ */
static uint32_t base_time = 0;

static uint32_t get_ms(void)
{
    return sceKernelGetProcessTimeLow() / 1000;
}

/* ================================================================
   Debug logging
   ================================================================ */
static void debug_log(const char *msg)
{
    FILE *f = fopen("ux0:/data/chexquest/debug.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static void debug_logf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debug_log(buf);
}

/* ================================================================
   Vita display init
   ================================================================ */
static void init_display(void)
{
    int sz = (960 * 544 * 4 + 0xFFFFF) & ~0xFFFFF;
    SceDisplayFrameBuf fb;
    int ret;

    fb_memuid = sceKernelAllocMemBlock("framebuffer",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, sz, NULL);

    if (fb_memuid < 0) {
        fb_memuid = sceKernelAllocMemBlock("framebuffer",
            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, sz, NULL);
    }

    if (fb_memuid < 0) {
        debug_logf("Alloc failed: 0x%08X", fb_memuid);
        return;
    }

    sceKernelGetMemBlockBase(fb_memuid, &fb_base);
    memset(fb_base, 0, sz);
    debug_logf("Framebuffer at %p", fb_base);

    memset(&fb, 0, sizeof(fb));
    fb.size        = sizeof(fb);
    fb.base        = fb_base;
    fb.pitch       = 960;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width       = 960;
    fb.height      = 544;

    ret = sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    debug_logf("SetFrameBuf: 0x%08X", ret);
    sceDisplayWaitVblankStart();
    display_ready = 1;
}

/* Schermata colorata (debug) */
static void show_color(uint32_t color)
{
    int i;
    uint32_t *p;
    SceDisplayFrameBuf fb;

    if (!fb_base) return;

    p = (uint32_t *)fb_base;
    for (i = 0; i < 960 * 544; i++) p[i] = color;

    memset(&fb, 0, sizeof(fb));
    fb.size        = sizeof(fb);
    fb.base        = fb_base;
    fb.pitch       = 960;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width       = 960;
    fb.height      = 544;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();
    sceDisplayWaitVblankStart();
}

/* ================================================================
   Input
   ================================================================ */
#define KQUEUE_SZ 64
#define DEADZONE  35

static struct { int pressed; unsigned char key; } kq[KQUEUE_SZ];
static int kq_r = 0, kq_w = 0;
static SceCtrlData pad_prev;
static int input_init = 0;
static int analog_held[6];

static void kq_push(int p, unsigned char k)
{
    int n = (kq_w + 1) % KQUEUE_SZ;
    if (n == kq_r) return;        /* coda piena, scarta */
    kq[kq_w].pressed = p;
    kq[kq_w].key     = k;
    kq_w = n;
}

static void analog_axis(int val, int neg_key, int pos_key,
                         int *neg_held, int *pos_held)
{
    int want_neg = val < -DEADZONE;
    int want_pos = val >  DEADZONE;
    if ( want_neg && !*neg_held) { kq_push(1, neg_key); *neg_held = 1; }
    if (!want_neg &&  *neg_held) { kq_push(0, neg_key); *neg_held = 0; }
    if ( want_pos && !*pos_held) { kq_push(1, pos_key); *pos_held = 1; }
    if (!want_pos &&  *pos_held) { kq_push(0, pos_key); *pos_held = 0; }
}

static void do_poll_input(void)
{
    SceCtrlData pad;
    int i;

    if (!input_init) {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
        sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                                SCE_TOUCH_SAMPLING_STATE_START);
        memset(&pad_prev, 0, sizeof(pad_prev));
        memset(analog_held, 0, sizeof(analog_held));
        input_init = 1;
    }

    sceCtrlPeekBufferPositive(0, &pad, 1);

    {
        struct { unsigned btn; unsigned char key; } bm[] = {
            { SCE_CTRL_UP,       KEY_UPARROW   },
            { SCE_CTRL_DOWN,     KEY_DOWNARROW  },
            { SCE_CTRL_LEFT,     KEY_LEFTARROW  },
            { SCE_CTRL_RIGHT,    KEY_RIGHTARROW },
            { SCE_CTRL_CROSS,    KEY_FIRE       },
            { SCE_CTRL_CIRCLE,   KEY_USE        },
            { SCE_CTRL_SQUARE,   KEY_RALT       },   /* strafe modifier */
            { SCE_CTRL_TRIANGLE, KEY_TAB        },   /* automap */
            { SCE_CTRL_RTRIGGER, KEY_FIRE       },
            { SCE_CTRL_LTRIGGER, KEY_RSHIFT     },   /* run */
            { SCE_CTRL_START,    KEY_ESCAPE     },
            { SCE_CTRL_SELECT,   KEY_ENTER      },
            { 0, 0 }
        };

        for (i = 0; bm[i].btn; i++) {
            int now = (pad.buttons  & bm[i].btn) != 0;
            int was = (pad_prev.buttons & bm[i].btn) != 0;
            if ( now && !was) kq_push(1, bm[i].key);
            if (!now &&  was) kq_push(0, bm[i].key);
        }
    }

    /* Analog sticks */
    analog_axis(pad.ly - 128, KEY_UPARROW,   KEY_DOWNARROW,
                &analog_held[0], &analog_held[1]);
    analog_axis(pad.lx - 128, KEY_STRAFE_L,  KEY_STRAFE_R,
                &analog_held[2], &analog_held[3]);
    analog_axis(pad.rx - 128, KEY_LEFTARROW, KEY_RIGHTARROW,
                &analog_held[4], &analog_held[5]);

    /* Touch → weapon select */
    {
        SceTouchData touch;
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
        if (touch.reportNum > 0 && touch.report[0].y / 2 < 60) {
            int slot = (touch.report[0].x / 2) / (VITA_W / 7);
            if (slot >= 0 && slot < 7) {
                kq_push(1, '1' + slot);
                kq_push(0, '1' + slot);
            }
        }
    }

    pad_prev = pad;
}

/* ================================================================
   DG interface (chiamato da doomgeneric.c)
   ================================================================ */
void DG_Init(void)
{
    debug_log("DG_Init");
    base_time = get_ms();
    debug_logf("base_time = %u ms", base_time);
}

void DG_DrawFrame(void)
{
    /* Il rendering vero è in I_FinishUpdate().
       DG_DrawFrame può essere chiamato da doomgeneric_Tick,
       non deve fare nulla di grafico. */
}

void DG_SleepMs(uint32_t ms)
{
    sceKernelDelayThread(ms * 1000);
}

uint32_t DG_GetTicksMs(void)
{
    return get_ms() - base_time;
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    /* Non più usato – I_StartTic drena la coda direttamente.
       Manteniamo per compatibilità con doomgeneric.c */
    if (kq_r == kq_w) return 0;
    *pressed = kq[kq_r].pressed;
    *key     = kq[kq_r].key;
    kq_r = (kq_r + 1) % KQUEUE_SZ;
    return 1;
}

void DG_SetWindowTitle(const char *t) { (void)t; }

/* ================================================================
   I_* – Implementazioni per Chocolate Doom
   ================================================================ */
void I_Init(void) {}

void I_Quit(void) { sceKernelExitProcess(0); }

void I_Error(char *error, ...)
{
    char buf[512];
    va_list args;
    va_start(args, error);
    vsnprintf(buf, sizeof(buf), error, args);
    va_end(args);
    debug_log(buf);
    if (fb_base) { show_color(0xFF0000FF); sceKernelDelayThread(5000000); }
    sceKernelExitProcess(0);
}

void I_WaitVBL(int count) { sceKernelDelayThread(count * 14286); }

int I_GetTime(void)
{
    uint32_t ms = get_ms() - base_time;
    return (int)(ms * TICRATE / 1000);
}

void I_Sleep(int ms) { sceKernelDelayThread(ms * 1000); }

byte *I_ZoneBase(int *size)
{
    byte *ptr;
    *size = 16 * 1024 * 1024;
    ptr = (byte *)malloc(*size);
    if (!ptr) { *size = 8 * 1024 * 1024; ptr = (byte *)malloc(*size); }
    debug_logf("ZoneBase: %d at %p", *size, ptr);
    return ptr;
}

void I_Tactile(int on, int off, int total)
    { (void)on; (void)off; (void)total; }

int I_ConsoleStdout(void) { return 0; }

boolean I_GetMemoryValue(unsigned int offset, void *value, int size)
    { (void)offset; (void)value; (void)size; return 0; }

void I_AtExit(void (*func)(void), boolean run_on_error)
    { (void)func; (void)run_on_error; }

void I_PrintBanner(const char *msg)       { (void)msg; }
void I_PrintDivider(void)                 {}
void I_PrintStartupBanner(const char *g)  { (void)g; }
void I_DisplayFPSDots(boolean d)          { (void)d; }
void I_CheckIsScreensaver(void)           {}
void I_GraphicsCheckCommandLine(void)     {}
void I_SetGrabMouseCallback(void (*f)(boolean g)) { (void)f; }

int I_GetTime_RealTime(void)
{
    uint32_t ms = get_ms() - base_time;
    return (int)(ms * TICRATE / 1000);
}

int I_GetTimeMS(void) { return (int)(get_ms() - base_time); }

void I_InitTimer(void)
{
    base_time = get_ms();
    debug_logf("I_InitTimer: base=%u", base_time);
}

/* ================================================================
   VIDEO – Palette + Rendering diretto su framebuffer Vita
   ================================================================ */

void I_InitGraphics(void)
{
    int i;
    debug_log("I_InitGraphics");

    I_VideoBuffer = (byte *)calloc(SCREENWIDTH * SCREENHEIGHT, 1);
    debug_logf("I_VideoBuffer = %p", I_VideoBuffer);

    /* Palette grayscale di fallback */
    for (i = 0; i < 256; i++) {
        cmap[i] = 0xFF000000u | ((uint32_t)i << 16)
                               | ((uint32_t)i << 8)
                               |  (uint32_t)i;
    }
}

void I_SetPalette(byte *doompalette)
{
    /* Doom ci passa 256 triplette R,G,B (768 byte).
       Formato Vita A8B8G8R8: 0xAA BB GG RR */
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t r = doompalette[i * 3 + 0];
        uint32_t g = doompalette[i * 3 + 1];
        uint32_t b = doompalette[i * 3 + 2];
        cmap[i] = 0xFF000000u | (b << 16) | (g << 8) | r;
    }

    if (frame_count < 3) {
        debug_logf("I_SetPalette: [0]=%08X [1]=%08X [2]=%08X",
                   cmap[0], cmap[1], cmap[2]);
    }
}

void I_FinishUpdate(void)
{
    /*  Scala I_VideoBuffer (320×200, 8-bit indicizzato)
        direttamente sul framebuffer Vita (960×544, 32-bit)
        usando cmap[] per la conversione palette.
        NON usa DG_ScreenBuffer → nessun problema di stride! */

    uint32_t *dst;
    int x, y;
    int step_x, step_y;
    int src_y_fixed;
    SceDisplayFrameBuf dfb;

    if (!display_ready || !I_VideoBuffer || !fb_base) return;

    dst    = (uint32_t *)fb_base;
    step_x = (SCREENWIDTH  << 16) / VITA_W;
    step_y = (SCREENHEIGHT << 16) / VITA_H;

    src_y_fixed = 0;
    for (y = 0; y < VITA_H; y++) {
        int srcy = src_y_fixed >> 16;
        uint32_t *dst_row;
        byte     *src_row;
        int       src_x_fixed;

        if (srcy >= SCREENHEIGHT) srcy = SCREENHEIGHT - 1;

        dst_row = dst + y * 960;
        src_row = I_VideoBuffer + srcy * SCREENWIDTH;

        src_x_fixed = 0;
        for (x = 0; x < VITA_W; x++) {
            int srcx = src_x_fixed >> 16;
            if (srcx >= SCREENWIDTH) srcx = SCREENWIDTH - 1;
            /* PALETTE LOOKUP: indice 8-bit → colore 32-bit */
            dst_row[x] = cmap[src_row[srcx]];
            src_x_fixed += step_x;
        }
        src_y_fixed += step_y;
    }

    memset(&dfb, 0, sizeof(dfb));
    dfb.size        = sizeof(dfb);
    dfb.base        = fb_base;
    dfb.pitch       = 960;
    dfb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    dfb.width       = 960;
    dfb.height      = 544;
    sceDisplaySetFrameBuf(&dfb, SCE_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();

    if (frame_count < 10) {
        debug_logf("I_FinishUpdate %d: cmap[0]=%08X vid[0]=%u vid[100]=%u time=%d",
                   frame_count, cmap[0],
                   (unsigned)I_VideoBuffer[0],
                   (unsigned)I_VideoBuffer[100],
                   I_GetTime());
    }
    frame_count++;
}

void I_ShutdownGraphics(void) {}
void I_StartFrame(void)       {}

/* ================================================================
   INPUT → D_PostEvent (LA FIX PRINCIPALE PER I CONTROLLI)
   ================================================================ */
void I_StartTic(void)
{
    event_t event;

    do_poll_input();

    /* Drena la coda e invia ogni evento al motore Doom */
    while (kq_r != kq_w) {
        event.type  = kq[kq_r].pressed ? ev_keydown : ev_keyup;
        event.data1 = kq[kq_r].key;
        event.data2 = kq[kq_r].key;   /* per input testo (weapon select) */
        event.data3 = 0;
        D_PostEvent(&event);
        kq_r = (kq_r + 1) % KQUEUE_SZ;
    }
}

void I_UpdateNoBlit(void) {}

void I_ReadScreen(byte *scr)
{
    if (I_VideoBuffer)
        memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

void I_EnableLoadingDisk(void)      {}
void I_BeginRead(void)              {}
void I_EndRead(void)                {}
void I_SetWindowTitle(char *title)  { (void)title; }
void I_BindVideoVariables(void)     {}
int  I_GetPaletteIndex(int r, int g, int b)
    { (void)r; (void)g; (void)b; return 0; }
void I_InitScale(void) {}

/* ================================================================
   Input stubs
   ================================================================ */
void I_InitInput(void)          {}
void I_ShutdownInput(void)      {}
void I_InitJoystick(void)       {}
void I_ShutdownJoystick(void)   {}
void I_UpdateJoystick(void)     {}
void I_BindJoystickVariables(void) {}

/* ================================================================
   Sound stubs (tutto disabilitato)
   ================================================================ */
void I_PrecacheSounds(void *s, int n) { (void)s; (void)n; }
int  I_GetSfxLumpNum(void *s)   { (void)s; return 0; }
void I_SetChannels(void)        {}
void I_SetSfxVolume(int v)      { (void)v; }
void I_SetMusicVolume(int v)    { (void)v; }

int I_StartSound(void *s, int c, int v, int sep, int p)
    { (void)s; (void)c; (void)v; (void)sep; (void)p; return 0; }

void I_StopSound(int c)          { (void)c; }
int  I_SoundIsPlaying(int c)     { (void)c; return 0; }
void I_UpdateSound(void)         {}
void I_UpdateSoundParams(int c, int v, int s)
    { (void)c; (void)v; (void)s; }
void I_ShutdownSound(void)       {}
void I_InitSound(int u)          { (void)u; }
void I_ShutdownMusic(void)       {}
void I_InitMusic(void)           {}
void I_PlaySong(void *h, int l)  { (void)h; (void)l; }
void I_PauseSong(void)           {}
void I_ResumeSong(void)          {}
void I_StopSong(void)            {}
void I_UnRegisterSong(void *h)   { (void)h; }
void *I_RegisterSong(void *d, int l)
    { (void)d; (void)l; return NULL; }
int  I_MusicIsPlaying(void)      { return 0; }
void I_BindSoundVariables(void)  {}

int  I_CDMusInit(void)           { return 0; }
void I_CDMusShutdown(void)       {}
void I_CDMusUpdate(void)         {}
void I_CDMusStop(void)           {}
int  I_CDMusPlay(int t)          { (void)t; return 0; }
void I_CDMusSetVolume(int v)     { (void)v; }
int  I_CDMusFirstTrack(void)     { return 0; }
int  I_CDMusLastTrack(void)      { return 0; }
int  I_CDMusTrackLength(int t)   { (void)t; return 0; }

void I_Endoom(byte *d) { (void)d; }

char *gus_patch_path = "";
int   gus_ram_kb = 0;

/* ================================================================
   MAIN
   ================================================================ */
int main(int argc, char **argv)
{
    int i;
    const char *wad = NULL;

    const char *paths[] = {
        "ux0:/data/chexquest/chex.wad",
        "ux0:/data/chexquest/doom1.wad",
        "ux0:/data/chexquest/doom.wad",
        NULL
    };

    SceAppUtilInitParam ip;
    SceAppUtilBootParam bp;

    (void)argc; (void)argv;

    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    memset(&ip, 0, sizeof(ip));
    memset(&bp, 0, sizeof(bp));
    sceAppUtilInit(&ip, &bp);

    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 0777);

    sceIoRemove("ux0:/data/chexquest/debug.log");
    debug_log("=== Chex Quest Vita ===");
    debug_logf("Initial ms = %u", get_ms());

    init_display();
    if (!display_ready) {
        debug_log("FATAL: no display");
        sceKernelExitProcess(0);
        return 1;
    }

    base_time = get_ms();
    debug_logf("base_time = %u", base_time);
    debug_log("Display OK");

    show_color(0xFF00FF00);   /* verde = display OK */
    sceKernelDelayThread(1000000);

    for (i = 0; paths[i]; i++) {
        SceUID fd = sceIoOpen(paths[i], SCE_O_RDONLY, 0);
        if (fd >= 0) {
            SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
            sceIoClose(fd);
            wad = paths[i];
            debug_logf("WAD: %s (%d bytes)", wad, (int)size);
            break;
        }
    }

    if (!wad) {
        debug_log("NO WAD!");
        show_color(0xFF00FFFF);
        sceKernelDelayThread(10000000);
        sceKernelExitProcess(0);
        return 1;
    }

    show_color(0xFFFFFF00);   /* ciano = WAD trovato */
    sceKernelDelayThread(500000);

    base_time = get_ms();
    debug_logf("Engine start base_time = %u", base_time);

    {
        char *nargv[] = { "ChexQuest", "-iwad", (char *)wad, NULL };
        debug_log("doomgeneric_Create...");
        doomgeneric_Create(3, nargv);
        debug_logf("Create OK, time=%d ms=%u",
                   I_GetTime(), get_ms() - base_time);
    }

    debug_log("Main loop");

    {
        int tick_count = 0;
        while (1) {
            if (tick_count < 10) {
                debug_logf("tick %d: I_GetTime=%d ms=%u",
                           tick_count, I_GetTime(), get_ms() - base_time);
                tick_count++;
            }
            doomgeneric_Tick();
        }
    }

    return 0;
}
