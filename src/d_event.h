#ifndef __D_EVENT__
#define __D_EVENT__

#include "doomtype.h"

// Event types
typedef enum {
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_joystick
} evtype_t;

// Event structure
typedef struct {
    evtype_t type;
    int data1;  // keys / buttons
    int data2;  // mouse/joy x
    int data3;  // mouse/joy y
} event_t;

// Button/key codes
typedef enum {
    KEY_RIGHTARROW = 0xae,
    KEY_LEFTARROW  = 0xac,
    KEY_UPARROW    = 0xad,
    KEY_DOWNARROW  = 0xaf,
    KEY_ESCAPE     = 27,
    KEY_ENTER      = 13,
    KEY_TAB        = 9,
    KEY_F1         = (0x80 + 0x3b),
    KEY_F2         = (0x80 + 0x3c),
    KEY_F3         = (0x80 + 0x3d),
    KEY_F4         = (0x80 + 0x3e),
    KEY_F5         = (0x80 + 0x3f),
    KEY_F6         = (0x80 + 0x40),
    KEY_F7         = (0x80 + 0x41),
    KEY_F8         = (0x80 + 0x42),
    KEY_F9         = (0x80 + 0x43),
    KEY_F10        = (0x80 + 0x44),
    KEY_F11        = (0x80 + 0x57),
    KEY_F12        = (0x80 + 0x58),
    KEY_BACKSPACE  = 127,
    KEY_PAUSE      = 0xff,
    KEY_EQUALS     = 0x3d,
    KEY_MINUS      = 0x2d,
    KEY_RSHIFT     = (0x80 + 0x36),
    KEY_RCTRL      = (0x80 + 0x1d),
    KEY_RALT       = (0x80 + 0x38),
    KEY_LALT       = KEY_RALT,
    KEY_SPACE      = 32,
    // Vita-specific keys
    KEY_VITA_CROSS    = 0x100,
    KEY_VITA_CIRCLE   = 0x101,
    KEY_VITA_SQUARE   = 0x102,
    KEY_VITA_TRIANGLE = 0x103,
    KEY_VITA_LTRIGGER = 0x104,
    KEY_VITA_RTRIGGER = 0x105,
    KEY_VITA_START    = 0x106,
    KEY_VITA_SELECT   = 0x107
} keycode_t;

// Game event handling
void D_PostEvent(event_t *ev);

// Tic command
#include "d_ticcmd.h"

#endif
