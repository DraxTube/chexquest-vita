#ifndef __DOOMDATA__
#define __DOOMDATA__

#include "doomtype.h"

// Map vertex
typedef struct {
    short x;
    short y;
} mapvertex_t;

// Map sidedef
typedef struct {
    short textureoffset;
    short rowoffset;
    char  toptexture[8];
    char  bottomtexture[8];
    char  midtexture[8];
    short sector;
} mapsidedef_t;

// Map linedef
typedef struct {
    short v1;
    short v2;
    short flags;
    short special;
    short tag;
    short sidenum[2];
} maplinedef_t;

// Linedef flags
#define ML_BLOCKING      1
#define ML_BLOCKMONSTERS 2
#define ML_TWOSIDED      4
#define ML_DONTPEGTOP    8
#define ML_DONTPEGBOTTOM 16
#define ML_SECRET        32
#define ML_SOUNDBLOCK    64
#define ML_DONTDRAW      128
#define ML_MAPPED        256

// Map sector
typedef struct {
    short floorheight;
    short ceilingheight;
    char  floorpic[8];
    char  ceilingpic[8];
    short lightlevel;
    short special;
    short tag;
} mapsector_t;

// Map subsector
typedef struct {
    short numsegs;
    short firstseg;
} mapsubsector_t;

// Map seg
typedef struct {
    short v1;
    short v2;
    short angle;
    short linedef;
    short side;
    short offset;
} mapseg_t;

// Map node
typedef struct {
    short x, y, dx, dy;
    short bbox[2][4];
    unsigned short children[2];
} mapnode_t;

#define NF_SUBSECTOR 0x8000

// Map thing
typedef struct {
    short x;
    short y;
    short angle;
    short type;
    short options;
} mapthing_t;

// Thing options
#define MTF_EASY    1
#define MTF_NORMAL  2
#define MTF_HARD    4
#define MTF_AMBUSH  8
#define MTF_MULTI   16

#endif
