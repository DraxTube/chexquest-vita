/* doomgeneric_vita.c – Chex Quest / DOOM on PS Vita – CON AUDIO */

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
#include <psp2/audioout.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#define TICRATE      35
#define SCREENWIDTH  320
#define SCREENHEIGHT 200

#define VITA_W 960
#define VITA_H 544

/* ================================================================
   Audio defines
   ================================================================ */
#define SND_RATE         22050      /* Doom nativo: 11025, upsampling a 22050 */
#define SND_SAMPLES      1024      /* campioni per grain (per porta audio)   */
#define MIX_CHANNELS     8         /* canali di mixing simultanei            */
#define MUS_SAMPLES      1024      /* campioni per grain musica              */

#define SFX_VOL_SCALE    16        /* Doom volume 0-127 → scala interna     */

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
   AUDIO ENGINE – SFX mixing + output via sceAudioOut
   ================================================================ */

/*  Struttura per un canale di mixing attivo.
    Doom SFX sono campioni 8-bit unsigned, mono, tipicamente 11025 Hz.
    Il formato nel lump WAD è:
      offset 0-1: format (3)
      offset 2-3: sample rate (uint16 LE)
      offset 4-7: length in samples (uint32 LE)
      offset 8-23: padding (16 byte)
      offset 24+: sample data (unsigned 8-bit, center=128)             */

typedef struct {
    const byte *data;       /* puntatore ai campioni 8-bit unsigned     */
    int         length;     /* lunghezza totale in campioni              */
    int         pos_fixed;  /* posizione corrente (16.16 fixed point)   */
    int         step_fixed; /* step per campione output (16.16)         */
    int         vol_left;   /* volume canale sinistro (0-255)           */
    int         vol_right;  /* volume canale destro   (0-255)           */
    int         handle;     /* handle univoco per Doom                  */
    int         active;     /* 1 = in riproduzione                      */
} mix_channel_t;

static mix_channel_t mix_ch[MIX_CHANNELS];
static int           sfx_port = -1;           /* porta audio Vita SFX  */
static SceUID        sfx_thread_id = -1;
static volatile int  sfx_running = 0;
static int           sfx_master_vol = 127;    /* 0-127                 */
static int           next_handle = 1;

/* Buffer di output stereo 16-bit (due buffer per double-buffering) */
static int16_t sfx_buf[2][SND_SAMPLES * 2];   /* *2 = stereo          */
static int     sfx_buf_idx = 0;

/* Mixing: mixa tutti i canali attivi in un buffer stereo S16 */
static void mix_into(int16_t *out, int nsamples)
{
    int i, ch;
    int32_t accum_l, accum_r;

    /* Azzera il buffer */
    memset(out, 0, nsamples * 2 * sizeof(int16_t));

    for (ch = 0; ch < MIX_CHANNELS; ch++) {
        mix_channel_t *c = &mix_ch[ch];
        if (!c->active) continue;

        for (i = 0; i < nsamples; i++) {
            int pos = c->pos_fixed >> 16;

            if (pos >= c->length) {
                c->active = 0;
                break;
            }

            /* Campione 8-bit unsigned → signed 16-bit */
            {
                int sample = ((int)c->data[pos] - 128) * 256;

                /* Applica volume per canale (stereo panning) */
                int sl = (sample * c->vol_left)  >> 8;
                int sr = (sample * c->vol_right) >> 8;

                /* Accumula nel buffer (somma con clamp dopo) */
                accum_l = (int32_t)out[i * 2 + 0] + sl;
                accum_r = (int32_t)out[i * 2 + 1] + sr;

                /* Clamp a int16 */
                if (accum_l >  32767) accum_l =  32767;
                if (accum_l < -32768) accum_l = -32768;
                if (accum_r >  32767) accum_r =  32767;
                if (accum_r < -32768) accum_r = -32768;

                out[i * 2 + 0] = (int16_t)accum_l;
                out[i * 2 + 1] = (int16_t)accum_r;
            }

            c->pos_fixed += c->step_fixed;
        }
    }

    /* Applica master volume */
    if (sfx_master_vol < 127) {
        for (i = 0; i < nsamples * 2; i++) {
            out[i] = (int16_t)(((int32_t)out[i] * sfx_master_vol) / 127);
        }
    }
}

/* Thread audio SFX: gira in background, invia dati alla porta audio */
static int sfx_thread_func(SceSize args, void *argp)
{
    (void)args; (void)argp;

    debug_log("SFX thread started");

    while (sfx_running) {
        int16_t *buf = sfx_buf[sfx_buf_idx];

        mix_into(buf, SND_SAMPLES);

        sceAudioOutOutput(sfx_port, buf);

        sfx_buf_idx ^= 1;
    }

    debug_log("SFX thread exit");
    return 0;
}

/* ================================================================
   MUSIC ENGINE – Minimal PCM playback
   Doom invia dati MUS/MIDI. Senza un sintetizzatore MIDI completo,
   implementiamo la struttura per future espansioni.
   Per ora: silent placeholder con la struttura pronta.
   ================================================================ */

/* Struttura per musica registrata */
typedef struct {
    byte   *data;      /* dati grezzi MUS/MIDI dal WAD */
    int     length;    /* dimensione */
    int     valid;     /* 1 se registrato */
} mus_track_t;

#define MAX_MUS_TRACKS 64
static mus_track_t mus_tracks[MAX_MUS_TRACKS];
static int         mus_playing = 0;
static int         mus_looping = 0;
static int         mus_current = -1;
static int         mus_volume  = 127;

/* ================================================================
   DG interface
   ================================================================ */
void DG_Init(void)
{
    debug_log("DG_Init");
    base_time = get_ms();
    debug_logf("base_time = %u ms", base_time);
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
   I_* – Implementazioni per Chocolate Doom
   ================================================================ */
void I_Init(void) {}

void I_Quit(void)
{
    sfx_running = 0;
    if (sfx_thread_id >= 0) {
        sceKernelWaitThreadEnd(sfx_thread_id, NULL, NULL);
        sceKernelDeleteThread(sfx_thread_id);
    }
    if (sfx_port >= 0) {
        sceAudioOutReleasePort(sfx_port);
    }
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
    debug_logf("I_VideoBuffer = %p", I_VideoBuffer);

    for (i = 0; i < 256; i++) {
        cmap[i] = 0xFF000000u | ((uint32_t)i << 16)
                               | ((uint32_t)i << 8)
                               |  (uint32_t)i;
    }
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

    if (frame_count < 3) {
        debug_logf("I_SetPalette: [0]=%08X [1]=%08X [2]=%08X",
                   cmap[0], cmap[1], cmap[2]);
    }
}

void I_FinishUpdate(void)
{
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
   INPUT → D_PostEvent
   ================================================================ */
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
   SOUND – Effetti sonori con mixing reale
   ================================================================ */

void I_SetChannels(void)
{
    debug_log("I_SetChannels");
}

void I_SetSfxVolume(int volume)
{
    sfx_master_vol = volume;   /* 0-127, usato dal mixer */
}

/*  I_GetSfxLumpNum: Doom ci passa un puntatore a sfxinfo_t.
    Il campo "name" contiene il nome del lump (es. "pistol").
    Noi restituiamo il numero del lump con prefisso "DS".
    
    NOTA: la struttura sfxinfo_t ha layout diverso tra versioni.
    In doomgeneric/Chocolate Doom, il primo campo utile è "name"
    (char* o char[]) a offset 0 o dopo "link".
    Per massima compatibilità, usiamo l'approccio di Chocolate Doom. */

/* Include per W_GetNumForName */
extern int W_GetNumForName(const char *name);
extern int W_CheckNumForName(const char *name);
extern int W_LumpLength(int lump);
extern void *W_CacheLumpNum(int lump, int tag);

/* PU_STATIC tag per la cache */
#define PU_STATIC 1

/* sfxinfo_t – prendi solo il nome.
   In Chocolate Doom / doomgeneric, sfxinfo_t inizia con:
     char *name;          (o simile)
   Facciamo il cast a char** per prendere il primo campo. */

int I_GetSfxLumpNum(void *sfxinfo)
{
    /* sfxinfo->name è il primo campo (char *) nella struct */
    char   lumpname[16];
    char  *name;
    int    lumpnum;

    if (!sfxinfo) return 0;

    /* Leggi il primo campo della struct come char* */
    name = *((char **)sfxinfo);
    if (!name) return 0;

    snprintf(lumpname, sizeof(lumpname), "ds%s", name);

    lumpnum = W_CheckNumForName(lumpname);
    if (lumpnum < 0) {
        debug_logf("SFX lump not found: %s", lumpname);
        return 0;
    }

    return lumpnum;
}

void I_PrecacheSounds(void *sounds, int num_sounds)
{
    /* Pre-carichiamo i lump SFX in cache */
    (void)sounds;
    (void)num_sounds;
    debug_logf("I_PrecacheSounds: %d sounds", num_sounds);
}

/*  I_StartSound: avvia la riproduzione di un effetto sonoro.
    
    Parametri (da Chocolate Doom):
      sfxinfo  - puntatore a sfxinfo_t (contiene lumpnum, ecc.)
      channel  - canale Doom (0-7)
      vol      - volume (0-127)
      sep      - separazione stereo (0=right, 128=center, 255=left)
      pitch    - pitch (128 = normale, Doom range ~0-255)
    
    Ritorna un handle univoco. */

int I_StartSound(void *sfxinfo, int channel, int vol, int sep, int pitch)
{
    int       lumpnum;
    byte     *lumpdata;
    int       lumplen;
    int       samplerate;
    int       samplelen;
    const byte *samples;
    int       handle;
    mix_channel_t *c;

    (void)channel; /* usiamo il nostro sistema di canali */

    if (!sfxinfo) return 0;

    /* Trova il lump */
    lumpnum = I_GetSfxLumpNum(sfxinfo);
    if (lumpnum <= 0) return 0;

    lumplen  = W_LumpLength(lumpnum);
    lumpdata = (byte *)W_CacheLumpNum(lumpnum, PU_STATIC);

    if (!lumpdata || lumplen < 28) return 0;

    /* Parsa l'header del lump SFX di Doom:
       Byte 0-1: format (should be 3)
       Byte 2-3: sample rate (LE uint16)
       Byte 4-7: number of samples (LE uint32)
       Byte 8-23: padding (16 bytes)
       Byte 24+: unsigned 8-bit PCM data */
    {
        int format = lumpdata[0] | (lumpdata[1] << 8);
        if (format != 3) {
            debug_logf("SFX bad format: %d (lump %d)", format, lumpnum);
            return 0;
        }
    }

    samplerate = lumpdata[2] | (lumpdata[3] << 8);
    samplelen  = lumpdata[4] | (lumpdata[5] << 8) |
                 (lumpdata[6] << 16) | (lumpdata[7] << 24);

    /* Sanity checks */
    if (samplerate < 4000 || samplerate > 48000) samplerate = 11025;
    if (samplelen <= 0 || samplelen > lumplen - 24) samplelen = lumplen - 24;
    if (samplelen <= 0) return 0;

    /* Salta il padding di 16 byte + 8 byte header = offset 24 */
    samples = lumpdata + 24;

    /* Se ci sono pad bytes all'inizio/fine del campione (Doom aggiunge
       qualche byte di padding), tronchiamo */
    if (samplelen > 2) {
        /* Rimuovi i byte di padding iniziale e finale tipici di Doom */
        samples += 16;          /* skip 16 byte di pre-padding nel campione */
        samplelen -= 32;        /* remove 16+16 padding */
        if (samplelen <= 0) {
            samples = lumpdata + 24;
            samplelen = lumplen - 24;
        }
    }

    /* Trova un canale di mixing libero (o ruba il più vecchio) */
    {
        int best = -1;
        int oldest_handle = 0x7FFFFFFF;
        int i;

        for (i = 0; i < MIX_CHANNELS; i++) {
            if (!mix_ch[i].active) { best = i; break; }
            if (mix_ch[i].handle < oldest_handle) {
                oldest_handle = mix_ch[i].handle;
                best = i;
            }
        }
        if (best < 0) best = 0;

        c = &mix_ch[best];
    }

    /* Calcola step per il resampling: da samplerate → 48000 Hz (porta Vita) */
    /* step = (samplerate << 16) / 48000 */
    c->data       = samples;
    c->length     = samplelen;
    c->pos_fixed  = 0;
    c->step_fixed = (samplerate << 16) / 48000;

    /* Pitch adjustment: Doom usa 128 come pitch normale.
       step *= pitch / 128 */
    if (pitch > 0 && pitch != 128) {
        c->step_fixed = (c->step_fixed * pitch) / 128;
    }

    /* Volume e panning.
       sep: 0 = tutto a destra, 128 = centro, 255 = tutto a sinistra.
       vol: 0-127 */
    {
        int vl, vr;
        /* Converti sep (0-255) in volume L/R */
        vr = 255 - sep;    /* sep=0 → vr=255, sep=255 → vr=0 */
        vl = sep;           /* sep=0 → vl=0,   sep=255 → vl=255 */

        /* Normalizza: sep=128 → entrambi a ~128 */
        /* Ma in realtà sep=128 dovrebbe essere uguale L/R */
        /* Ricalcoliamo: */
        if (sep < 128) {
            vl = 255;
            vr = sep * 2;
        } else {
            vl = (255 - sep) * 2;
            vr = 255;
        }

        /* Applica volume del canale */
        vl = (vl * vol) / 127;
        vr = (vr * vol) / 127;

        if (vl > 255) vl = 255;
        if (vr > 255) vr = 255;

        c->vol_left  = vl;
        c->vol_right = vr;
    }

    handle = next_handle++;
    c->handle = handle;
    c->active = 1;

    return handle;
}

void I_StopSound(int handle)
{
    int i;
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (mix_ch[i].active && mix_ch[i].handle == handle) {
            mix_ch[i].active = 0;
            break;
        }
    }
}

int I_SoundIsPlaying(int handle)
{
    int i;
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (mix_ch[i].active && mix_ch[i].handle == handle)
            return 1;
    }
    return 0;
}

void I_UpdateSound(void)
{
    /* Il mixing avviene nel thread audio, nulla da fare qui */
}

void I_UpdateSoundParams(int handle, int vol, int sep)
{
    int i;
    for (i = 0; i < MIX_CHANNELS; i++) {
        if (mix_ch[i].active && mix_ch[i].handle == handle) {
            int vl, vr;

            if (sep < 128) {
                vl = 255;
                vr = sep * 2;
            } else {
                vl = (255 - sep) * 2;
                vr = 255;
            }

            vl = (vl * vol) / 127;
            vr = (vr * vol) / 127;

            if (vl > 255) vl = 255;
            if (vr > 255) vr = 255;

            mix_ch[i].vol_left  = vl;
            mix_ch[i].vol_right = vr;
            break;
        }
    }
}

void I_InitSound(int use_sfx_prefix)
{
    int i;
    (void)use_sfx_prefix;

    debug_log("I_InitSound: initializing audio");

    /* Azzera canali di mixing */
    memset(mix_ch, 0, sizeof(mix_ch));
    memset(sfx_buf, 0, sizeof(sfx_buf));

    /* Apri porta audio Vita.
       SCE_AUDIO_OUT_PORT_TYPE_MAIN = output principale (speaker/cuffie)
       48000 Hz, stereo, 16-bit signed                                    */
    sfx_port = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_MAIN,
        SND_SAMPLES,                     /* granularità in campioni */
        48000,                           /* sample rate             */
        SCE_AUDIO_OUT_MODE_STEREO        /* stereo                  */
    );

    if (sfx_port < 0) {
        debug_logf("sceAudioOutOpenPort failed: 0x%08X", sfx_port);
        /* Proviamo con BGM port come fallback */
        sfx_port = sceAudioOutOpenPort(
            SCE_AUDIO_OUT_PORT_TYPE_BGM,
            SND_SAMPLES,
            48000,
            SCE_AUDIO_OUT_MODE_STEREO
        );
        if (sfx_port < 0) {
            debug_logf("BGM port also failed: 0x%08X", sfx_port);
            return;
        }
        debug_log("Using BGM audio port");
    }

    /* Volume al massimo sulla porta */
    {
        int vols[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
        sceAudioOutSetVolume(sfx_port, SCE_AUDIO_VOLUME_FLAG_L_CH |
                             SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
    }

    debug_logf("Audio port opened: %d", sfx_port);

    /* Avvia thread di mixing */
    sfx_running = 1;

    sfx_thread_id = sceKernelCreateThread(
        "sfx_mixer",
        sfx_thread_func,
        0x10000100,          /* priorità (default user) */
        0x10000,             /* stack 64KB              */
        0,                   /* attributi               */
        0,                   /* cpu affinity mask (any) */
        NULL
    );

    if (sfx_thread_id < 0) {
        debug_logf("CreateThread failed: 0x%08X", sfx_thread_id);
        sfx_running = 0;
        return;
    }

    i = sceKernelStartThread(sfx_thread_id, 0, NULL);
    if (i < 0) {
        debug_logf("StartThread failed: 0x%08X", i);
        sfx_running = 0;
        sceKernelDeleteThread(sfx_thread_id);
        sfx_thread_id = -1;
        return;
    }

    debug_log("SFX mixer thread started OK");
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
}

void I_BindSoundVariables(void) {}

/* ================================================================
   MUSIC – MUS/MIDI (struttura pronta, output silenzioso)
   La PS Vita non ha un sintetizzatore MIDI hardware accessibile
   facilmente. Per avere musica servirebbe un softsynth (es. 
   Timidity, FluidSynth) con soundfont. Per ora la struttura
   è tutta pronta: basta sostituire I_PlaySong con un decoder.
   ================================================================ */

void I_InitMusic(void)
{
    debug_log("I_InitMusic (placeholder)");
    memset(mus_tracks, 0, sizeof(mus_tracks));
}

void I_ShutdownMusic(void)
{
    int i;
    debug_log("I_ShutdownMusic");
    for (i = 0; i < MAX_MUS_TRACKS; i++) {
        if (mus_tracks[i].data) {
            free(mus_tracks[i].data);
            mus_tracks[i].data = NULL;
        }
        mus_tracks[i].valid = 0;
    }
    mus_playing = 0;
}

void I_SetMusicVolume(int volume)
{
    mus_volume = volume;  /* 0-127 */
}

void I_PauseSong(void)
{
    mus_playing = 0;
}

void I_ResumeSong(void)
{
    if (mus_current >= 0)
        mus_playing = 1;
}

void *I_RegisterSong(void *data, int len)
{
    int i;

    if (!data || len <= 0) return NULL;

    /* Trova slot libero */
    for (i = 0; i < MAX_MUS_TRACKS; i++) {
        if (!mus_tracks[i].valid) {
            mus_tracks[i].data = (byte *)malloc(len);
            if (!mus_tracks[i].data) return NULL;
            memcpy(mus_tracks[i].data, data, len);
            mus_tracks[i].length = len;
            mus_tracks[i].valid  = 1;

            debug_logf("I_RegisterSong: slot %d, %d bytes, "
                       "header: %02X %02X %02X %02X",
                       i, len,
                       ((byte*)data)[0], ((byte*)data)[1],
                       ((byte*)data)[2], ((byte*)data)[3]);

            /* Ritorna handle = indice + 1 (0 = invalid) */
            return (void *)(intptr_t)(i + 1);
        }
    }

    debug_log("I_RegisterSong: no free slots!");
    return NULL;
}

void I_UnRegisterSong(void *handle)
{
    int idx = (int)(intptr_t)handle - 1;

    if (idx < 0 || idx >= MAX_MUS_TRACKS) return;

    if (mus_current == idx) {
        mus_playing = 0;
        mus_current = -1;
    }

    if (mus_tracks[idx].data) {
        free(mus_tracks[idx].data);
        mus_tracks[idx].data = NULL;
    }
    mus_tracks[idx].valid = 0;
}

void I_PlaySong(void *handle, int looping)
{
    int idx = (int)(intptr_t)handle - 1;

    if (idx < 0 || idx >= MAX_MUS_TRACKS || !mus_tracks[idx].valid) {
        debug_logf("I_PlaySong: invalid handle %d", idx + 1);
        return;
    }

    mus_current = idx;
    mus_looping = looping;
    mus_playing = 1;

    debug_logf("I_PlaySong: track %d, loop=%d, %d bytes",
               idx, looping, mus_tracks[idx].length);

    /* TODO: Qui andrebbe avviato il decoder MUS→PCM.
       Per implementare la musica vera, opzioni:
       1) Convertire MUS→MIDI→PCM con un softsynth (FluidSynth + .sf2)
       2) Usare una libreria MUS player dedicata
       3) Pre-convertire la musica in OGG e riprodurla            */
}

void I_StopSong(void)
{
    mus_playing = 0;
    mus_current = -1;
}

int I_MusicIsPlaying(void)
{
    return mus_playing;
}

/* ================================================================
   CD Music stubs
   ================================================================ */
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
    debug_log("=== Chex Quest Vita (with audio) ===");
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
