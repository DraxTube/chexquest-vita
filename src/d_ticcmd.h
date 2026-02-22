#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "doomdef.h"

typedef struct {
    signed char forwardmove;
    signed char sidemove;
    short       angleturn;
    short       consistancy;
    byte        chatchar;
    byte        buttons;
} ticcmd_t;

#define BT_ATTACK      1
#define BT_USE         2
#define BT_SPECIAL     128
#define BT_SPECIALMASK 3
#define BT_CHANGE      4
#define BT_WEAPONMASK  (8+16+32)
#define BT_WEAPONSHIFT 3
#define BTS_PAUSE      1
#define BTS_SAVEGAME   2
#define BTS_SAVEMASK   (4+8+16)
#define BTS_SAVESHIFT  2

#endif
