/* doomgeneric_vita.c – Chex Quest / DOOM on PS Vita – AUDIO FUNZIONANTE */

#include "doomgeneric.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "d_event.h"
#include "w_wad.h"
#include "sounds.h"
#include "i_sound.h"
#include "z_zone.h"
#include "m_argv.h"
#include "deh_str.h"

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
#include <psp2/audioout.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#define TICRATE      35
#define SCREENWIDTH  320
#define SCREENHEIGHT 200
#define VITA_W       960
#define VITA_H       544

/* ================================================================
   Audio defines
   ================================================================ */
#define AUDIO_RATE        48000
#define AUDIO_GRANULARITY 256
#define MIX_CHANNELS      8

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
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debug_log(buf);
}

/* ================================================================
   Vita display
   ================================================================ */
static void init_display(void)
{
    int sz = (960 * 544 * 4 + 0xFFFFF) & ~0xFFFFF;
    SceDisplayFrameBuf fb;
    int ret;

    fb_memuid = sceKernelAllocMemBlock("framebuffer",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, sz, NULL);
    if (fb_memuid < 0)
        fb_memuid = sceKernelAllocMemBlock("framebuffer",
            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, sz, NULL);
    if (fb_memuid < 0) {
        debug_logf("Alloc failed: 0x%08X", fb_memuid);
        return;
    }

    sceKernelGetMemBlockBase(fb_memuid, &fb_base);
    memset(fb_base, 0, sz);

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
    if (n == kq_r) return;
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
            { SCE_CTRL_SQUARE,   KEY_RALT       },
            { SCE_CTRL_TRIANGLE, KEY_TAB        },
            { SCE_CTRL_RTRIGGER, KEY_FIRE       },
            { SCE_CTRL_LTRIGGER, KEY_RSHIFT     },
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

    analog_axis(pad.ly - 128, KEY_UPARROW,   KEY_DOWNARROW,
                &analog_held[0], &analog_held[1]);
    analog_axis(pad.lx - 128, KEY_STRAFE_L,  KEY_STRAFE_R,
                &analog_held[2], &analog_held[3]);
    analog_axis(pad.rx - 128, KEY_LEFTARROW, KEY_RIGHTARROW,
                &analog_held[4], &analog_held[5]);

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
   AUDIO ENGINE — con mutex e doppio buffer
   ================================================================ */

typedef struct {
    const byte *data;
    int         length;
    int         pos_fixed;    /* 16.16 fixed point */
    int         step_fixed;   /* 16.16 fixed point */
    int         vol_left;     /* 0-256 */
    int         vol_right;    /* 0-256 */
    int         handle;
    int         active;
    int         lumpnum;
} mix_channel_t;

static mix_channel_t  mix_ch[MIX_CHANNELS];
static int            sfx_port       = -1;
static SceUID         sfx_thread_id  = -1;
static SceUID         sfx_mutex      = -1;
static volatile int   sfx_running    = 0;
static volatile int   sfx_master_vol = 15;
static int            next_handle    = 1;
static int            audio_ready    = 0;
static int            sfx_log_count  = 0;

static int16_t __attribute__((aligned(64))) sfx_buf[2][AUDIO_GRANULARITY * 2];
static int sfx_buf_idx = 0;

/* --- SFX lump cache --- */
#define SFX_CACHE_MAX 128
typedef struct {
    int         lumpnum;
    const byte *samples;
    int         length;
    int         samplerate;
} sfx_cache_entry_t;

static sfx_cache_entry_t sfx_cache[SFX_CACHE_MAX];
static int sfx_cache_count = 0;

static sfx_cache_entry_t *sfx_cache_get(int lumpnum)
{
    int    i;
    byte  *raw;
    int    rawlen;
    int    format, rate, nsamples;

    for (i = 0; i < sfx_cache_count; i++) {
        if (sfx_cache[i].lumpnum == lumpnum)
            return &sfx_cache[i];
    }

    rawlen = W_LumpLength(lumpnum);
    if (rawlen < 8) return NULL;

    raw = W_CacheLumpNum(lumpnum, PU_STATIC);
    if (!raw) return NULL;

    format   = raw[0] | (raw[1] << 8);
    rate     = raw[2] | (raw[3] << 8);
    nsamples = raw[4] | (raw[5] << 8) | (raw[6] << 16) | (raw[7] << 24);

    if (format != 3) {
        debug_logf("SFX lump %d: bad format %d", lumpnum, format);
        return NULL;
    }

    if (rate < 4000 || rate > 48000) rate = 11025;

    if (nsamples > rawlen - 8)
        nsamples = rawlen - 8;
    if (nsamples <= 0) return NULL;

    {
        const byte *pcm_start = raw + 8;
        int         pcm_len   = nsamples;

        if (pcm_len > 32) {
            pcm_start += 16;
            pcm_len   -= 32;
        }

        if (sfx_cache_count >= SFX_CACHE_MAX) {
            debug_log("SFX cache full!");
            return NULL;
        }

        i = sfx_cache_count++;
        sfx_cache[i].lumpnum    = lumpnum;
        sfx_cache[i].samples    = pcm_start;
        sfx_cache[i].length     = pcm_len;
        sfx_cache[i].samplerate = rate;
    }

    debug_logf("SFX cached: lump=%d rate=%d len=%d rawlen=%d",
               lumpnum, sfx_cache[i].samplerate,
               sfx_cache[i].length, rawlen);

    return &sfx_cache[i];
}

/* --- Mixing --- */
static void mix_into(int16_t *out, int nsamples)
{
    int i, ch;
    int32_t accum_l, accum_r;
    int     mvol;

    sceKernelLockMutex(sfx_mutex, 1, NULL);

    mvol = sfx_master_vol;
    if (mvol < 0)  mvol = 0;
    if (mvol > 15) mvol = 15;

    for (i = 0; i < nsamples; i++) {
        accum_l = 0;
        accum_r = 0;

        for (ch = 0; ch < MIX_CHANNELS; ch++) {
            mix_channel_t *c = &mix_ch[ch];
            int pos, sample;

            if (!c->active || !c->data) continue;

            pos = c->pos_fixed >> 16;
            if (pos >= c->length) {
                c->active = 0;
                continue;
            }

            sample = ((int)c->data[pos] - 128) * 256;

            accum_l += (sample * c->vol_left)  >> 8;
            accum_r += (sample * c->vol_right) >> 8;

            c->pos_fixed += c->step_fixed;
        }

        accum_l = (accum_l * mvol) / 15;
        accum_r = (accum_r * mvol) / 15;

        if (accum_l >  32767) accum_l =  32767;
        if (accum_l < -32768) accum_l = -32768;
        if (accum_r >  32767) accum_r =  32767;
        if (accum_r < -32768) accum_r = -32768;

        out[i * 2 + 0] = (int16_t)accum_l;
        out[i * 2 + 1] = (int16_t)accum_r;
    }

    sceKernelUnlockMutex(sfx_mutex, 1);
}

/* --- Thread audio --- */
static int sfx_thread_func(SceSize args, void *argp)
{
    (void)args; (void)argp;
    debug_log("Audio thread started");

    while (sfx_running) {
        int16_t *buf = sfx_buf[sfx_buf_idx];
        mix_into(buf, AUDIO_GRANULARITY);
        sceAudioOutOutput(sfx_port, buf);
        sfx_buf_idx ^= 1;
    }

    debug_log("Audio thread exiting");
    return 0;
}

static void start_audio_system(void)
{
    int ret;
    int vols[2];

    if (audio_ready) return;

    debug_log("Starting audio system...");

    memset(mix_ch, 0, sizeof(mix_ch));
    memset(sfx_buf, 0, sizeof(sfx_buf));
    memset(sfx_cache, 0, sizeof(sfx_cache));
    sfx_cache_count = 0;
    sfx_buf_idx = 0;

    sfx_mutex = sceKernelCreateMutex("sfx_mutex", 0, 0, NULL);
    if (sfx_mutex < 0) {
        debug_logf("CreateMutex failed: 0x%08X", sfx_mutex);
        return;
    }

    sfx_port = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_BGM,
        AUDIO_GRANULARITY,
        AUDIO_RATE,
        SCE_AUDIO_OUT_MODE_STEREO
    );

    if (sfx_port < 0) {
        debug_logf("BGM port failed: 0x%08X, trying MAIN...", sfx_port);
        sfx_port = sceAudioOutOpenPort(
            SCE_AUDIO_OUT_PORT_TYPE_MAIN,
            AUDIO_GRANULARITY,
            AUDIO_RATE,
            SCE_AUDIO_OUT_MODE_STEREO
        );
    }

    if (sfx_port < 0) {
        debug_logf("All audio ports failed: 0x%08X", sfx_port);
        sceKernelDeleteMutex(sfx_mutex);
        sfx_mutex = -1;
        return;
    }

    debug_logf("Audio port opened: %d", sfx_port);

    vols[0] = SCE_AUDIO_VOLUME_0DB;
    vols[1] = SCE_AUDIO_VOLUME_0DB;
    ret = sceAudioOutSetVolume(sfx_port,
        SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
    debug_logf("SetVolume: 0x%08X", ret);

    /* Test beep */
    {
        int t;
        int16_t *testbuf = sfx_buf[0];
        for (t = 0; t < AUDIO_GRANULARITY; t++) {
            int16_t val = (t % 64 < 32) ? 8000 : -8000;
            testbuf[t * 2 + 0] = val;
            testbuf[t * 2 + 1] = val;
        }
        ret = sceAudioOutOutput(sfx_port, testbuf);
        debug_logf("Test beep output: 0x%08X", ret);
        memset(sfx_buf[1], 0, sizeof(sfx_buf[1]));
        sceAudioOutOutput(sfx_port, sfx_buf[1]);
    }

    sfx_running = 1;
    sfx_thread_id = sceKernelCreateThread(
        "doom_sfx",
        sfx_thread_func,
        0x10000100,
        0x10000,
        0, 0, NULL
    );

    if (sfx_thread_id < 0) {
        debug_logf("CreateThread failed: 0x%08X", sfx_thread_id);
        sfx_running = 0;
        sceAudioOutReleasePort(sfx_port);
        sfx_port = -1;
        sceKernelDeleteMutex(sfx_mutex);
        sfx_mutex = -1;
        return;
    }

    ret = sceKernelStartThread(sfx_thread_id, 0, NULL);
    if (ret < 0) {
        debug_logf("StartThread failed: 0x%08X", ret);
        sfx_running = 0;
        sceKernelDeleteThread(sfx_thread_id);
        sfx_thread_id = -1;
        sceAudioOutReleasePort(sfx_port);
        sfx_port = -1;
        sceKernelDeleteMutex(sfx_mutex);
        sfx_mutex = -1;
        return;
    }

    audio_ready = 1;
    debug_log("Audio system fully initialized!");
}

/* ================================================================
   DG interface
   ================================================================ */
void DG_Init(void)
{
    debug_log("DG_Init");
    base_time = get_ms();
}

void DG_DrawFrame(void) {}

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
    if (kq_r == kq_w) return 0;
    *pressed = kq[kq_r].pressed;
    *key     = kq[kq_r].key;
    kq_r = (kq_r + 1) % KQUEUE_SZ;
    return 1;
}

void DG_SetWindowTitle(const char *t) { (void)t; }

/* ================================================================
   I_* base
   ================================================================ */
void I_Init(void) {}

void I_Quit(void)
{
    sfx_running = 0;
    if (sfx_thread_id >= 0) {
        sceKernelWaitThreadEnd(sfx_thread_id, NULL, NULL);
        sceKernelDeleteThread(sfx_thread_id);
    }
    if (sfx_port >= 0) sceAudioOutReleasePort(sfx_port);
    if (sfx_mutex >= 0) sceKernelDeleteMutex(sfx_mutex);
    sceKernelExitProcess(0);
}

void I_Error(char *error, ...)
{
    char buf[512];
    va_list args;
    va_start(args, error);
    vsnprintf(buf, sizeof(buf), error, args);
    va_end(args);
    debug_log(buf);
    sfx_running = 0;
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
   VIDEO
   ================================================================ */
void I_InitGraphics(void)
{
    int i;
    debug_log("I_InitGraphics");
    I_VideoBuffer = (byte *)calloc(SCREENWIDTH * SCREENHEIGHT, 1);
    for (i = 0; i < 256; i++)
        cmap[i] = 0xFF000000u | ((uint32_t)i << 16)
                               | ((uint32_t)i << 8)
                               |  (uint32_t)i;
}

void I_SetPalette(byte *doompalette)
{
    int i;
    for (i = 0; i < 256; i++) {
        uint32_t r = doompalette[i * 3 + 0];
        uint32_t g = doompalette[i * 3 + 1];
        uint32_t b = doompalette[i * 3 + 2];
        cmap[i] = 0xFF000000u | (b << 16) | (g << 8) | r;
    }
}

void I_FinishUpdate(void)
{
    uint32_t *dst;
    int x, y, step_x, step_y, src_y_fixed;
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
    frame_count++;
}

void I_ShutdownGraphics(void) {}
void I_StartFrame(void)       {}

void I_StartTic(void)
{
    event_t event;
    do_poll_input();
    while (kq_r != kq_w) {
        event.type  = kq[kq_r].pressed ? ev_keydown : ev_keyup;
        event.data1 = kq[kq_r].key;
        event.data2 = kq[kq_r].key;
        event.data3 = 0;
        D_PostEvent(&event);
        kq_r = (kq_r + 1) % KQUEUE_SZ;
    }
}

void I_UpdateNoBlit(void) {}
void I_ReadScreen(byte *scr)
{
    if (I_VideoBuffer) memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}
void I_EnableLoadingDisk(void)      {}
void I_BeginRead(void)              {}
void I_EndRead(void)                {}
void I_SetWindowTitle(char *title)  { (void)title; }
void I_BindVideoVariables(void)     {}
int  I_GetPaletteIndex(int r, int g, int b)
    { (void)r; (void)g; (void)b; return 0; }
void I_InitScale(void) {}

/* Input stubs */
void I_InitInput(void)          {}
void I_ShutdownInput(void)      {}
void I_InitJoystick(void)       {}
void I_ShutdownJoystick(void)   {}
void I_UpdateJoystick(void)     {}
void I_BindJoystickVariables(void) {}

/* ================================================================
   SOUND – signatures matching i_sound.h exactly
   ================================================================ */

void I_SetChannels(void)
{
    debug_log("I_SetChannels");
}

void I_SetSfxVolume(int volume)
{
    sfx_master_vol = volume;
    debug_logf("I_SetSfxVolume: %d", volume);
}

int I_GetSfxLumpNum(sfxinfo_t *sfx)
{
    char namebuf[16];
    int  lumpnum;

    if (!sfx || !sfx->name || sfx->name[0] == '\0') return -1;

    snprintf(namebuf, sizeof(namebuf), "ds%s", DEH_String(sfx->name));

    lumpnum = W_CheckNumForName(namebuf);

    if (sfx_log_count < 30) {
        debug_logf("I_GetSfxLumpNum: '%s' -> lump %d", namebuf, lumpnum);
        sfx_log_count++;
    }

    return lumpnum;
}

void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    int i, lumpnum;

    debug_logf("I_PrecacheSounds: %d sounds", num_sounds);

    for (i = 0; i < num_sounds; i++) {
        lumpnum = I_GetSfxLumpNum(&sounds[i]);
        if (lumpnum >= 0) {
            sfx_cache_get(lumpnum);
        }
    }

    debug_logf("SFX cache loaded: %d entries", sfx_cache_count);
}

/*
 * Matches: int I_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep);
 * NO pitch parameter in this engine version.
 */
int I_StartSound(sfxinfo_t *sfx, int channel, int vol, int sep)
{
    int               lumpnum;
    sfx_cache_entry_t *entry;
    mix_channel_t     *c;
    int               handle;
    int               best, i;
    int               oldest_handle;

    (void)channel;

    if (!audio_ready || !sfx) return 0;

    lumpnum = sfx->lumpnum;
    if (lumpnum < 0) {
        lumpnum = I_GetSfxLumpNum(sfx);
        if (lumpnum < 0) return 0;
    }

    entry = sfx_cache_get(lumpnum);
    if (!entry || entry->length <= 0) return 0;

    sceKernelLockMutex(sfx_mutex, 1, NULL);

    /* Find free channel or steal oldest */
    best = 0;
    oldest_handle = 0x7FFFFFFF;
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (!mix_ch[i].active) { best = i; goto found; }
        if (mix_ch[i].handle < oldest_handle) {
            oldest_handle = mix_ch[i].handle;
            best = i;
        }
    }
found:
    c = &mix_ch[best];

    c->data       = entry->samples;
    c->length     = entry->length;
    c->pos_fixed  = 0;
    c->lumpnum    = lumpnum;

    /* Resampling step: source_hz -> 48000 Hz, 16.16 fixed */
    c->step_fixed = (int)(((int64_t)entry->samplerate << 16) / AUDIO_RATE);
    if (c->step_fixed <= 0) c->step_fixed = (11025 << 16) / AUDIO_RATE;

    /* Volume and stereo panning
       vol: 0-127, sep: 0=left, 128=center, 255=right */
    {
        int vl, vr;

        if (sep < 0)   sep = 128;
        if (sep > 255) sep = 128;

        vr = (sep * 256) / 255;
        vl = 256 - vr;

        if (vl < 0)   vl = 0;
        if (vl > 256) vl = 256;
        if (vr < 0)   vr = 0;
        if (vr > 256) vr = 256;

        c->vol_left  = (vl * vol) / 127;
        c->vol_right = (vr * vol) / 127;

        if (c->vol_left  > 256) c->vol_left  = 256;
        if (c->vol_right > 256) c->vol_right = 256;
    }

    handle = next_handle++;
    if (next_handle > 0x7FFFFF00) next_handle = 1;

    c->handle = handle;
    c->active = 1;

    sceKernelUnlockMutex(sfx_mutex, 1);

    if (sfx_log_count < 60) {
        debug_logf("SND play: lump=%d rate=%d len=%d vol=%d sep=%d "
                   "step=0x%X ch=%d hnd=%d",
                   lumpnum, entry->samplerate, entry->length,
                   vol, sep, c->step_fixed, best, handle);
        sfx_log_count++;
    }

    return handle;
}

void I_StopSound(int handle)
{
    int i;
    if (sfx_mutex < 0) return;
    sceKernelLockMutex(sfx_mutex, 1, NULL);
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (mix_ch[i].active && mix_ch[i].handle == handle) {
            mix_ch[i].active = 0;
            break;
        }
    }
    sceKernelUnlockMutex(sfx_mutex, 1);
}

/*
 * Matches: boolean I_SoundIsPlaying(int channel);
 */
boolean I_SoundIsPlaying(int handle)
{
    int i;
    boolean result = false;
    if (sfx_mutex < 0) return false;
    sceKernelLockMutex(sfx_mutex, 1, NULL);
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (mix_ch[i].active && mix_ch[i].handle == handle) {
            result = true;
            break;
        }
    }
    sceKernelUnlockMutex(sfx_mutex, 1);
    return result;
}

void I_UpdateSound(void)
{
    /* Audio thread handles everything */
}

void I_UpdateSoundParams(int handle, int vol, int sep)
{
    int i;
    if (sfx_mutex < 0) return;
    sceKernelLockMutex(sfx_mutex, 1, NULL);
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (mix_ch[i].active && mix_ch[i].handle == handle) {
            int vl, vr;

            if (sep < 0)   sep = 128;
            if (sep > 255) sep = 128;

            vr = (sep * 256) / 255;
            vl = 256 - vr;
            if (vl < 0)   vl = 0;
            if (vl > 256) vl = 256;
            if (vr < 0)   vr = 0;
            if (vr > 256) vr = 256;

            mix_ch[i].vol_left  = (vl * vol) / 127;
            mix_ch[i].vol_right = (vr * vol) / 127;
            if (mix_ch[i].vol_left  > 256) mix_ch[i].vol_left  = 256;
            if (mix_ch[i].vol_right > 256) mix_ch[i].vol_right = 256;
            break;
        }
    }
    sceKernelUnlockMutex(sfx_mutex, 1);
}

/*
 * Matches: void I_InitSound(boolean use_sfx_prefix);
 */
void I_InitSound(boolean use_sfx_prefix)
{
    (void)use_sfx_prefix;
    debug_log("I_InitSound");
    start_audio_system();
}

void I_ShutdownSound(void)
{
    debug_log("I_ShutdownSound");
    sfx_running = 0;
    if (sfx_thread_id >= 0) {
        sceKernelWaitThreadEnd(sfx_thread_id, NULL, NULL);
        sceKernelDeleteThread(sfx_thread_id);
        sfx_thread_id = -1;
    }
    if (sfx_port >= 0) {
        sceAudioOutReleasePort(sfx_port);
        sfx_port = -1;
    }
    if (sfx_mutex >= 0) {
        sceKernelDeleteMutex(sfx_mutex);
        sfx_mutex = -1;
    }
    audio_ready = 0;
}

void I_BindSoundVariables(void) {}

/* ================================================================
   MUSIC – stubs matching i_sound.h signatures
   ================================================================ */
void    I_InitMusic(void)             { debug_log("I_InitMusic (stub)"); }
void    I_ShutdownMusic(void)         {}
void    I_SetMusicVolume(int v)       { (void)v; }
void    I_PauseSong(void)             {}
void    I_ResumeSong(void)            {}
void    I_StopSong(void)              {}

/*
 * Matches: boolean I_MusicIsPlaying(void);
 */
boolean I_MusicIsPlaying(void)        { return false; }

void *I_RegisterSong(void *data, int len)
{
    if (data && len > 4) {
        byte *d = (byte *)data;
        debug_logf("I_RegisterSong: %d bytes, hdr: %02X%02X%02X%02X",
                   len, d[0], d[1], d[2], d[3]);
    }
    return (void *)1;
}

void I_UnRegisterSong(void *handle)   { (void)handle; }

/*
 * Matches: void I_PlaySong(void *handle, boolean looping);
 */
void I_PlaySong(void *handle, boolean looping)
{
    (void)handle;
    (void)looping;
}

/* CD stubs */
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
    debug_log("=== Chex Quest Vita (audio v4 fixed sigs) ===");

    init_display();
    if (!display_ready) {
        debug_log("FATAL: no display");
        sceKernelExitProcess(0);
        return 1;
    }

    base_time = get_ms();
    debug_log("Display OK");

    show_color(0xFF00FF00);
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

    show_color(0xFFFFFF00);
    sceKernelDelayThread(500000);

    base_time = get_ms();

    {
        char *nargv[] = { "ChexQuest", "-iwad", (char *)wad, NULL };
        debug_log("doomgeneric_Create...");
        doomgeneric_Create(3, nargv);
        debug_logf("Create OK, time=%d", I_GetTime());
    }

    debug_log("Entering main loop");

    while (1) {
        doomgeneric_Tick();
    }

    return 0;
}
