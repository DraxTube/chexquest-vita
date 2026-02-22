#ifndef __D_STATE__
#define __D_STATE__

#include "doomdef.h"
#include "d_player.h"

// Game mode - set in d_main.c
extern GameMode_t     gamemode;
extern GameMission_t  gamemission;
extern Language_t     language;

// Game state
extern gamestate_t  gamestate;
extern gameaction_t gameaction;

// Flags
extern boolean paused;
extern boolean netgame;
extern boolean deathmatch;
extern boolean respawnmonsters;
extern boolean automapactive;
extern boolean menuactive;
extern boolean nodrawers;
extern boolean noblit;
extern boolean viewactive;
extern boolean singledemo;
extern boolean precache;
extern boolean usergame;
extern boolean devparm;
extern boolean modifiedgame;
extern boolean singletics;

// Game info
extern skill_t gameskill;
extern int     gameepisode;
extern int     gamemap;
extern int     gametic;
extern int     levelstarttic;
extern int     totalkills;
extern int     totalitems;
extern int     totalsecret;
extern int     leveltime;
extern int     consoleplayer;
extern int     displayplayer;

// Players
extern player_t players[MAXPLAYERS];
extern boolean  playeringame[MAXPLAYERS];

// Demos
extern boolean demoplayback;
extern boolean demorecording;

// Wad file names
extern char *wadfiles[MAXWADFILES];

// Screen
extern int screenblocks;
extern int showMessages;

// Timer
extern boolean timelimit;

// Savegame
#define SAVEGAMESIZE 0x2C000
extern int savegameslot;

// Network
extern int maketic;
extern int nettics[MAXPLAYERS];

// Misc
extern int bodyqueslot;

#endif
