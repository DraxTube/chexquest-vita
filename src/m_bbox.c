#include "m_bbox.h"
#include <limits.h>

void M_ClearBox(fixed_t *box) {
    box[BOXTOP] = box[BOXRIGHT] = INT_MIN;
    box[BOXBOTTOM] = box[BOXLEFT] = INT_MAX;
}

void M_AddToBox(fixed_t *box, fixed_t x, fixed_t y) {
    if (x < box[BOXLEFT])   box[BOXLEFT] = x;
    if (x > box[BOXRIGHT])  box[BOXRIGHT] = x;
    if (y < box[BOXBOTTOM]) box[BOXBOTTOM] = y;
    if (y > box[BOXTOP])    box[BOXTOP] = y;
}
