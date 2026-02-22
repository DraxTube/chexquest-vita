#include "m_argv.h"
#include <string.h>
#include <ctype.h>

int myargc;
char **myargv;

int M_CheckParm(const char *check) {
    int i;
    for (i = 1; i < myargc; i++) {
        if (!strcasecmp(check, myargv[i]))
            return i;
    }
    return 0;
}
