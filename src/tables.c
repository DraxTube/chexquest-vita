#include "tables.h"
#include <math.h>

fixed_t finesine[10240];
fixed_t *finecosine = &finesine[FINEANGLES / 4];
fixed_t finetangent[4096];
angle_t tantoangle[2049];

// Generate tables at startup instead of huge static arrays
void R_InitTables(void) {
    int i;
    double angle;

    // Generate fine sine table
    for (i = 0; i < 10240; i++) {
        angle = (double)i * 3.14159265358979323846 * 2.0 / (double)FINEANGLES;
        finesine[i] = (fixed_t)(sin(angle) * (double)FRACUNIT);
    }

    // Generate fine tangent table
    for (i = 0; i < 4096; i++) {
        angle = (2048.5 - (double)i) * 3.14159265358979323846 / 4096.0;
        finetangent[i] = (fixed_t)(tan(angle) * (double)FRACUNIT);
    }

    // Generate angle-to-tangent table
    for (i = 0; i <= 2048; i++) {
        double f = atan((double)i / 2048.0) / (3.14159265358979323846 * 2.0);
        tantoangle[i] = (angle_t)(0xFFFFFFFF * f);
    }
}

int SlopeDiv(unsigned num, unsigned den) {
    unsigned ans;
    if (den < 512)
        return SLOPERANGE;
    ans = (num << 3) / (den >> 8);
    return ans <= SLOPERANGE ? ans : SLOPERANGE;
}
