/*
 * doomgeneric_vita.c - PS Vita platform for doomgeneric
 * Provides: display, input, timing, AND stubs for removed modules
 */

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

/* ============================================================
 * DISPLAY
 * ============================================================ */
#define VITA_W       960
#define VITA_H       544
#define VITA_STRIDE  1024
#define DOOM_W       DOOMGENERIC_RESX
#define DOOM_H       DOOMGENERIC_RESY

static uint32_t *vita_fb[2] = { NULL, NULL };
static int vita_fb_idx = 0;
static SceUID vita_fb_uid[2];
static uint64_t start_tick = 0;

static void *alloc_cdram(unsigned int size, SceUID *uid) {
    void *mem = NULL;
    *uid = sceKernelAllocMemBlock("fb", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, size, NULL);
    if (*uid < 0) return NULL;
    sceKernelGetMemBlockBase(*uid, &mem);
    return mem;
}

static void init_display(void) {
    unsigned int sz = VITA_STRIDE * VITA_H * 4;
    vita_fb[0] = (uint32_t *)alloc_cdram(sz, &vita_fb_uid[0]);
    vita_fb[1] = (uint32_t *)alloc_cdram(sz, &vita_fb_uid[1]);
    if (!vita_fb[0] || !vita_fb[1]) {
        vita_fb[0] = (uint32_t *)calloc(VITA_STRIDE * VITA_H, 4);
        vita_fb[1] = (uint32_t *)calloc(VITA_STRIDE * VITA_H, 4);
    }
    memset(vita_fb[0], 0, sz);
    memset(vita_fb[1], 0, sz);

    SceDisplayFrameBuf fb = {0};
    fb.size = sizeof(fb);
    fb.base = vita_fb[0];
    fb.pitch = VITA_STRIDE;
    fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    fb.width = VITA_W;
    fb.height = VITA_H;
    sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
}

/* ============================================================
 * INPUT
 * ============================================================ */
#define KQUEUE_SZ 64
#define DEADZONE  35

static struct { int pressed; unsigned char key; } kq[KQUEUE_SZ];
static int kq_r = 0, kq_w = 0;
static SceCtrlData pad_prev;
static int input_init = 0;
static int analog_held[6]; /* up down sleft sright tleft tright */

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

    /* Left stick: move */
    analog_axis(pad.ly - 128, KEY_UPARROW, KEY_DOWNARROW, &analog_held[0], &analog_held[1]);
    analog_axis(pad.lx - 128, KEY_STRAFE_L, KEY_STRAFE_R, &analog_held[2], &analog_held[3]);
    /* Right stick: turn */
    analog_axis(pad.rx - 128, KEY_LEFTARROW, KEY_RIGHTARROW, &analog_held[4], &analog_held[5]);

    /* Touch: weapon select */
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    if (touch.reportNum > 0 && touch.report[0].y / 2 < 60) {
        int slot = (touch.report[0].x / 2) / (VITA_W / 7);
        if (slot >= 0 && slot < 7) { kq_push(1, '1'+slot); kq_push(0, '1'+slot); }
    }

    pad_prev = pad;
}

/* ============================================================
 * DOOMGENERIC INTERFACE
 * ============================================================ */

void DG_Init(void) {
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    SceAppUtilInitParam ip; SceAppUtilBootParam bp;
    memset(&ip, 0, sizeof(ip)); memset(&bp, 0, sizeof(bp));
    sceAppUtilInit(&ip, &bp);

    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 
