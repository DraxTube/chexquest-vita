#ifndef __TABLES__
#define __TABLES__

#include "doomdef.h"

#define FINEANGLES 8192
#define FINEMASK   (FINEANGLES - 1)
#define ANGLETOFINESHIFT 19

// Trigonometric LUT
extern fixed_t finesine[10240];
extern fixed_t *finecosine;
extern fixed_t finetangent[4096];
extern angle_t tantoangle[2049];

// Binary angle to degree
#define ANG1 (ANG45 / 45)

int SlopeDiv(unsigned num, unsigned den);

#endif
