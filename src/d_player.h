#ifndef __D_PLAYER__
#define __D_PLAYER__

#include "d_ticcmd.h"
#include "doomdef.h"
#include "d_think.h"

// Player sprite
typedef enum {
    ps_weapon,
    ps_flash,
    NUMPSPRITES
} psprnum_t;

// Player sprite state
typedef struct {
    void *state;  // state_t pointer
    int   tics;
    fixed_t sx;
    fixed_t sy;
} pspdef_t;

// Extended player info
typedef struct player_s {
    struct mobj_s *mo;
    playerstate_t  playerstate;
    ticcmd_t       cmd;

    fixed_t viewz;
    fixed_t viewheight;
    fixed_t deltaviewheight;
    fixed_t bob;

    int health;
    int armorpoints;
    int armortype;

    int powers[NUMPOWERS];
    boolean cards[NUMCARDS];
    boolean backpack;

    int frags[MAXPLAYERS];
    weapontype_t readyweapon;
    weapontype_t pendingweapon;

    boolean weaponowned[NUMWEAPONS];
    int ammo[NUMAMMO];
    int maxammo[NUMAMMO];

    int attackdown;
    int usedown;

    int cheats;
    int refire;

    int killcount;
    int itemcount;
    int secretcount;

    char *message;

    int damagecount;
    int bonuscount;

    struct mobj_s *attacker;

    int extralight;
    int fixedcolormap;

    int colormap;
    pspdef_t psprites[NUMPSPRITES];

    boolean didsecret;
} player_t;

// Cheat flags
#define CF_NOCLIP    1
#define CF_GODMODE   2
#define CF_NOMOMENTUM 4

#endif
