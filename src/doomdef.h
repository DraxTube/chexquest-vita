#ifndef __DOOMDEF__
#define __DOOMDEF__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>

#ifdef VITA
#include <psp2/types.h>
#endif

#define VERSION 110

typedef int boolean;
#define true  1
#define false 0

typedef unsigned char byte;
typedef int32_t fixed_t;
typedef unsigned int angle_t;

#define FRACBITS   16
#define FRACUNIT   (1 << FRACBITS)

#define ANG45    0x20000000u
#define ANG90    0x40000000u
#define ANG180   0x80000000u
#define ANG270   0xC0000000u
#define FINEANGLES 8192
#define FINEMASK   (FINEANGLES - 1)
#define ANGLETOFINESHIFT 19

#define SLOPERANGE 2048
#define SLOPEBITS  11
#define DBITS      (FRACBITS - SLOPEBITS)

#define SCREENWIDTH  320
#define SCREENHEIGHT 200
#define MAXPLAYERS   4
#define TICRATE      35

#define MAXWADFILES  20
#define SAVESTRINGSIZE 24
#define SAVEGAMESIZE   0x2C000

#define MAXHEALTH   100
#define VIEWHEIGHT  (41 * FRACUNIT)
#define MAXRADIUS   (32 * FRACUNIT)

typedef enum {
    shareware, registered, commercial, retail, indetermined
} GameMode_t;

typedef enum {
    doom, doom2, pack_tnt, pack_plut, chex, none_mission
} GameMission_t;

typedef enum {
    english, french, german, unknown_lang
} Language_t;

typedef enum {
    sk_baby, sk_easy, sk_medium, sk_hard, sk_nightmare
} skill_t;

typedef enum {
    GS_LEVEL, GS_INTERMISSION, GS_FINALE, GS_DEMOSCREEN
} gamestate_t;

typedef enum {
    ga_nothing, ga_loadlevel, ga_newgame, ga_loadgame,
    ga_savegame, ga_playdemo, ga_completed, ga_victory,
    ga_worlddone, ga_screenshot
} gameaction_t;

typedef enum {
    wp_fist, wp_pistol, wp_shotgun, wp_chaingun, wp_missile,
    wp_plasma, wp_bfg, wp_chainsaw, wp_supershotgun,
    NUMWEAPONS, wp_nochange
} weapontype_t;

typedef enum {
    am_clip, am_shell, am_cell, am_misl, NUMAMMO, am_noammo
} ammotype_t;

typedef enum {
    pw_invulnerability, pw_strength, pw_invisibility,
    pw_ironfeet, pw_allmap, pw_infrared, NUMPOWERS
} powertype_t;

typedef enum {
    it_bluecard, it_yellowcard, it_redcard,
    it_blueskull, it_yellowskull, it_redskull, NUMCARDS
} card_t;

typedef enum {
    PST_LIVE, PST_DEAD, PST_REBORN
} playerstate_t;

void I_Error(const char *error, ...);

#endif
