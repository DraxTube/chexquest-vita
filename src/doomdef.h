#ifndef __DOOMDEF__
#define __DOOMDEF__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#ifdef VITA
#include <psp2/kernel/processmgr.h>
#endif

// Versione del gioco
#define VERSION 109

// Modalità di gioco
typedef enum {
    shareware,
    registered,
    commercial,
    retail,
    indetermined
} GameMode_t;

// Missione
typedef enum {
    doom,
    doom2,
    pack_tnt,
    pack_plut,
    chex,       // Chex Quest!
    none
} GameMission_t;

// Lingua
typedef enum {
    english,
    french,
    german,
    unknown
} Language_t;

// Skill levels
typedef enum {
    sk_baby,
    sk_easy,
    sk_medium,
    sk_hard,
    sk_nightmare
} skill_t;

// Game state
typedef enum {
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN
} gamestate_t;

// Game action
typedef enum {
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_playdemo,
    ga_completed,
    ga_victory,
    ga_worlddone,
    ga_screenshot
} gameaction_t;

// Weapon types
typedef enum {
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,
    NUMWEAPONS,
    wp_nochange
} weapontype_t;

// Ammo types
typedef enum {
    am_clip,
    am_shell,
    am_cell,
    am_misl,
    NUMAMMO,
    am_noammo
} ammotype_t;

// Power types
typedef enum {
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,
    NUMPOWERS
} powertype_t;

// Card/key types
typedef enum {
    it_bluecard,
    it_yellowcard,
    it_redcard,
    it_blueskull,
    it_yellowskull,
    it_redskull,
    NUMCARDS
} card_t;

// Rendering constants
#define SCREENWIDTH  320
#define SCREENHEIGHT 200
#define FRACBITS     16
#define FRACUNIT     (1 << FRACBITS)

// Limiti mappa
#define MAXPLAYERS   4
#define TICRATE      35

// Boolean
#ifndef boolean
typedef int boolean;
#endif
#define true  1
#define false 0

// Byte
typedef unsigned char byte;
typedef int32_t fixed_t;

// Angoli
#define ANG45   0x20000000
#define ANG90   0x40000000
#define ANG180  0x80000000
#define ANG270  0xC0000000
#define FINEANGLES 8192
#define FINEMASK   (FINEANGLES - 1)
#define ANGLETOFINESHIFT 19

typedef unsigned angle_t;

// Lookup tables size
#define SLOPERANGE 2048
#define SLOPEBITS  11
#define DBITS      (FRACBITS - SLOPEBITS)

// Map limits
#define MAXRADIUS   (32 * FRACUNIT)
#define MAXHEALTH   100
#define VIEWHEIGHT  (41 * FRACUNIT)

// Stato
typedef enum {
    PST_LIVE,
    PST_DEAD,
    PST_REBORN
} playerstate_t;

// Cheat check
#define MAXWADFILES 20

// Forward declarations
struct player_s;
struct mobj_s;
struct line_s;
struct sector_s;
struct subsector_s;
struct node_s;
struct seg_s;
struct side_s;

// Function declarations
void I_Error(const char *error, ...);

#endif
