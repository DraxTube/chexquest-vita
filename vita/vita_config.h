/*
 * Chex Quest PS Vita - Configuration Header
 * Defines Vita-specific constants and mappings
 */

#ifndef __VITA_CONFIG_H__
#define __VITA_CONFIG_H__

#ifdef VITA

#include <psp2/ctrl.h>
#include <psp2/touch.h>

// =====================================================
// DISPLAY CONFIGURATION
// =====================================================
#define VITA_SCREEN_W      960
#define VITA_SCREEN_H      544
#define VITA_RENDER_W      320   // Doom's native width
#define VITA_RENDER_H      200   // Doom's native height
#define VITA_SCALE_X       (VITA_SCREEN_W / VITA_RENDER_W)  // 3x
#define VITA_SCALE_Y       (VITA_SCREEN_H / VITA_RENDER_H)  // ~2.7x

// Use integer scaling with letterboxing for pixel-perfect look
#define VITA_SCALED_W      (VITA_RENDER_W * 3)   // 960
#define VITA_SCALED_H      (VITA_RENDER_H * 2)   // 400, centered vertically
#define VITA_OFFSET_Y      ((VITA_SCREEN_H - VITA_SCALED_H) / 2)  // 72px top/bottom

// Or stretch to fill (looks slightly stretched but fills screen)
#define VITA_STRETCH_FULL  1

// =====================================================
// CONTROLLER MAPPING - Chex Quest optimized
// =====================================================
//
// PS Vita Button Layout for Chex Quest:
//
//  L Trigger = Strafe             R Trigger = Fire (Zorch!)
//
//       [D-Up]                        Triangle = Automap
//  [D-Left] [D-Right]           Square = Use    Circle = Strafe
//      [D-Down]                       Cross = Fire (alt)
//
//  Left Stick = Move/Strafe      Right Stick = Look/Turn
//
//  Start  = Menu/Pause
//  Select = Toggle Map
//
// =====================================================

// Button mappings to Doom keys
typedef struct {
    int vita_button;
    int doom_key;
    const char *description;
} vita_button_map_t;

#define VITA_DEADZONE      30    // Analog stick dead zone (0-128)
#define VITA_STICK_SPEED   1200  // Turning speed multiplier
#define VITA_MOVE_SPEED    80    // Movement speed from stick

// Analog stick sensitivity
#define VITA_TURN_SENSITIVITY    6
#define VITA_LOOK_SENSITIVITY    4
#define VITA_MOVE_SENSITIVITY    50

// =====================================================
// AUDIO CONFIGURATION
// =====================================================
#define VITA_AUDIO_FREQ    44100
#define VITA_AUDIO_FORMAT  AUDIO_S16SYS
#define VITA_AUDIO_CHANNELS 2
#define VITA_AUDIO_SAMPLES 1024
#define VITA_MUSIC_VOLUME  96    // 0-128
#define VITA_SFX_VOLUME    96    // 0-128

// =====================================================
// FILE PATHS
// =====================================================
#define VITA_DATA_PATH     "ux0:/data/chexquest/"
#define VITA_SAVE_PATH     "ux0:/data/chexquest/savegames/"
#define VITA_CONFIG_FILE   "ux0:/data/chexquest/chexquest.cfg"
#define VITA_WAD_FILE      "ux0:/data/chexquest/chex.wad"
#define VITA_WAD_DOOM1     "ux0:/data/chexquest/doom1.wad"
#define VITA_WAD_DOOM      "ux0:/data/chexquest/doom.wad"

// =====================================================
// PERFORMANCE
// =====================================================
#define VITA_TARGET_FPS    35    // Doom's native ticrate
#define VITA_CPU_FREQ      444   // MHz (max)
#define VITA_GPU_FREQ      222   // MHz (max)
#define VITA_BUS_FREQ      222   // MHz (max)

#endif /* VITA */
#endif /* __VITA_CONFIG_H__ */
