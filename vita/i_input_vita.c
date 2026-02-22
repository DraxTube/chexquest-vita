/*
 * Chex Quest PS Vita - Input Handler
 * Maps PS Vita controls to Doom engine events
 */

#ifdef VITA

#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <stdlib.h>
#include <string.h>
#include "vita_config.h"

// Doom includes
#include "doomdef.h"
#include "d_event.h"
#include "m_argv.h"

// External functions
extern void D_PostEvent(event_t *ev);

// Previous button state for edge detection
static SceCtrlData pad_prev;
static int initialized = 0;

// Analog stick accumulator for smooth turning
static float turn_accumulator = 0.0f;

/*
 * Initialize Vita input subsystem
 */
void I_InitVitaInput(void) {
    // Set analog sampling mode
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

    // Initialize touch screen
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_START);

    memset(&pad_prev, 0, sizeof(SceCtrlData));
    initialized = 1;
}

/*
 * Send a key event to the Doom engine
 */
static void vita_send_key(evtype_t type, int key) {
    event_t event;
    event.type = type;
    event.data1 = key;
    event.data2 = 0;
    event.data3 = 0;
    D_PostEvent(&event);
}

/*
 * Check if a button was just pressed (edge detection)
 */
static inline int button_pressed(unsigned int buttons, unsigned int prev_buttons, unsigned int mask) {
    return (buttons & mask) && !(prev_buttons & mask);
}

/*
 * Check if a button was just released (edge detection)
 */
static inline int button_released(unsigned int buttons, unsigned int prev_buttons, unsigned int mask) {
    return !(buttons & mask) && (prev_buttons & mask);
}

/*
 * Process analog stick input and convert to movement/turning
 */
static void vita_process_sticks(SceCtrlData *pad) {
    event_t event;
    int lx, ly, rx, ry;

    // Center and apply deadzone to left stick
    lx = pad->lx - 128;
    ly = pad->ly - 128;
    if (abs(lx) < VITA_DEADZONE) lx = 0;
    if (abs(ly) < VITA_DEADZONE) ly = 0;

    // Center and apply deadzone to right stick
    rx = pad->rx - 128;
    ry = pad->ry - 128;
    if (abs(rx) < VITA_DEADZONE) rx = 0;
    if (abs(ry) < VITA_DEADZONE) ry = 0;

    // Left stick = movement (forward/backward + strafe)
    if (lx != 0 || ly != 0) {
        event.type = ev_joystick;
        event.data1 = 0; // no buttons from stick
        event.data2 = lx * VITA_MOVE_SENSITIVITY / 128;  // strafe
        event.data3 = -ly * VITA_MOVE_SENSITIVITY / 128;  // forward (inverted)
        D_PostEvent(&event);
    }

    // Right stick = turning
    if (rx != 0) {
        event.type = ev_mouse;
        event.data1 = 0;
        event.data2 = rx * VITA_TURN_SENSITIVITY / 16;  // horizontal = turning
        event.data3 = 0;
        D_PostEvent(&event);
    }
}

/*
 * Main input polling function - called every tic
 */
void I_PollVitaInput(void) {
    SceCtrlData pad;

    if (!initialized) {
        I_InitVitaInput();
    }

    // Read current pad state
    sceCtrlPeekBufferPositive(0, &pad, 1);

    // ========================================
    // D-PAD mapping
    // ========================================
    // D-Up = Move forward
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_UP))
        vita_send_key(ev_keydown, KEY_UPARROW);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_UP))
        vita_send_key(ev_keyup, KEY_UPARROW);

    // D-Down = Move backward
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_DOWN))
        vita_send_key(ev_keydown, KEY_DOWNARROW);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_DOWN))
        vita_send_key(ev_keyup, KEY_DOWNARROW);

    // D-Left = Turn left
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_LEFT))
        vita_send_key(ev_keydown, KEY_LEFTARROW);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_LEFT))
        vita_send_key(ev_keyup, KEY_LEFTARROW);

    // D-Right = Turn right
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_RIGHT))
        vita_send_key(ev_keydown, KEY_RIGHTARROW);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_RIGHT))
        vita_send_key(ev_keyup, KEY_RIGHTARROW);

    // ========================================
    // Face buttons
    // ========================================
    // Cross (X) = Fire / Zorch!
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_CROSS))
        vita_send_key(ev_keydown, KEY_RCTRL);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_CROSS))
        vita_send_key(ev_keyup, KEY_RCTRL);

    // Circle (O) = Use / Open doors
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_CIRCLE))
        vita_send_key(ev_keydown, KEY_SPACE);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_CIRCLE))
        vita_send_key(ev_keyup, KEY_SPACE);

    // Square = Strafe modifier (hold)
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_SQUARE))
        vita_send_key(ev_keydown, KEY_RALT);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_SQUARE))
        vita_send_key(ev_keyup, KEY_RALT);

    // Triangle = Automap toggle
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_TRIANGLE))
        vita_send_key(ev_keydown, KEY_TAB);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_TRIANGLE))
        vita_send_key(ev_keyup, KEY_TAB);

    // ========================================
    // Shoulder buttons
    // ========================================
    // R Trigger = Fire (primary fire button)
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_RTRIGGER))
        vita_send_key(ev_keydown, KEY_RCTRL);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_RTRIGGER))
        vita_send_key(ev_keyup, KEY_RCTRL);

    // L Trigger = Run / Speed
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_LTRIGGER))
        vita_send_key(ev_keydown, KEY_RSHIFT);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_LTRIGGER))
        vita_send_key(ev_keyup, KEY_RSHIFT);

    // ========================================
    // System buttons
    // ========================================
    // Start = Escape (Menu)
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_START))
        vita_send_key(ev_keydown, KEY_ESCAPE);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_START))
        vita_send_key(ev_keyup, KEY_ESCAPE);

    // Select = Enter (confirm in menus)
    if (button_pressed(pad.buttons, pad_prev.buttons, SCE_CTRL_SELECT))
        vita_send_key(ev_keydown, KEY_ENTER);
    if (button_released(pad.buttons, pad_prev.buttons, SCE_CTRL_SELECT))
        vita_send_key(ev_keyup, KEY_ENTER);

    // ========================================
    // Analog sticks
    // ========================================
    vita_process_sticks(&pad);

    // ========================================
    // Touch screen - weapon selection
    // ========================================
    SceTouchData touch;
    sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

    if (touch.reportNum > 0) {
        int tx = touch.report[0].x / 2;  // Touch coords are 1920x1088, screen is 960x544
        int ty = touch.report[0].y / 2;

        // Top area of screen - weapon selection strip
        if (ty < 60) {
            int weapon_zone = tx / (VITA_SCREEN_W / 7);  // 7 weapon slots
            char weapon_key = '1' + weapon_zone;
            if (weapon_key >= '1' && weapon_key <= '7') {
                vita_send_key(ev_keydown, weapon_key);
                // Auto-release next frame
            }
        }
    }

    // Save previous state
    pad_prev = pad;
}

/*
 * Shutdown input
 */
void I_ShutdownVitaInput(void) {
    initialized = 0;
}

#endif /* VITA */
