#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum { dr_false, dr_true } dr_boolean;
#endif

// Fixed endianness for Vita (ARM, little-endian)
#ifdef VITA
#define SHORT(x) (x)
#define LONG(x)  (x)
#else
#define SHORT(x) (x)
#define LONG(x)  (x)
#endif

#endif
