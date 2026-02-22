#include "m_fixed.h"

fixed_t FixedMul(fixed_t a, fixed_t b) {
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FRACBITS);
}

fixed_t FixedDiv(fixed_t a, fixed_t b) {
    if ((abs(a) >> 14) >= abs(b))
        return (a ^ b) < 0 ? INT32_MIN : INT32_MAX;
    return FixedDiv2(a, b);
}

fixed_t FixedDiv2(fixed_t a, fixed_t b) {
    int64_t result = ((int64_t)a << FRACBITS) / (int64_t)b;
    return (fixed_t)result;
}
