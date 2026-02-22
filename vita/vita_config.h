#ifndef __VITA_CONFIG_H__
#define __VITA_CONFIG_H__

#ifdef VITA

#include <psp2/ctrl.h>
#include <psp2/touch.h>

/* Display */
#define VITA_SCREEN_W      960
#define VITA_SCREEN_H      544
#define VITA_RENDER_W      320
#define VITA_RENDER_H      200
#define VITA_SCALED_W      (VITA_RENDER_W * 3)
#define VITA_SCALED_H      (VITA_RENDER_H * 2)
#define VITA_OFFSET_Y      ((VITA_SCREEN_H - VITA_SCALED_H) / 2)
#define VITA_STRETCH_FULL  1

/* Controller */
#define VITA_DEADZONE          30
#define VITA_TURN_SENSITIVITY  6
#define VITA_MOVE_SENSITIVITY  50

/* Audio */
#define VITA_AUDIO_FREQ     44100
#define VITA_AUDIO_FORMAT   AUDIO_S16SYS
#define VITA_AUDIO_CHANNELS 2
#define VITA_AUDIO_SAMPLES  1024
#define VITA_MUSIC_VOLUME   96
#define VITA_SFX_VOLUME     96

/* Paths */
#define VITA_DATA_PATH     "ux0:/data/chexquest/"
#define VITA_SAVE_PATH     "ux0:/data/chexquest/savegames/"
#define VITA_CONFIG_FILE   "ux0:/data/chexquest/chexquest.cfg"

/* Performance */
#define VITA_TARGET_FPS    35
#define VITA_CPU_FREQ      444
#define VITA_GPU_FREQ      222
#define VITA_BUS_FREQ      222

#endif
#endif
