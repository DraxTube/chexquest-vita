/*
 * Chex Quest for PS Vita
 * Main entry point
 */

#ifdef VITA

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/apputil.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* From Chocolate Doom */
extern int myargc;
extern char **myargv;
extern void D_DoomMain(void);

static void vita_create_dirs(void) {
    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 0777);
    sceIoMkdir("ux0:/data/chexquest/savegames/", 0777);
}

static const char *vita_find_wad(void) {
    static const char *wad_paths[] = {
        "ux0:/data/chexquest/chex.wad",
        "ux0:/data/chexquest/doom1.wad",
        "ux0:/data/chexquest/doom.wad",
        "ux0:/data/doom/chex.wad",
        "ux0:/data/doom/doom1.wad",
        "ux0:/data/doom/doom.wad",
        NULL
    };

    for (int i = 0; wad_paths[i] != NULL; i++) {
        SceUID fd = sceIoOpen(wad_paths[i], SCE_O_RDONLY, 0);
        if (fd >= 0) {
            sceIoClose(fd);
            return wad_paths[i];
        }
    }
    return "ux0:/data/chexquest/chex.wad"; /* default, will error nicely */
}

int main(int argc, char **argv) {
    /* Overclock for smooth gameplay */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    /* Init app utilities */
    SceAppUtilInitParam init_param;
    SceAppUtilBootParam boot_param;
    memset(&init_param, 0, sizeof(SceAppUtilInitParam));
    memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
    sceAppUtilInit(&init_param, &boot_param);

    vita_create_dirs();

    const char *wad_path = vita_find_wad();

    /* Build argv for Doom engine */
    static char *new_argv[] = {
        "ChexQuest",
        "-iwad", NULL,
        NULL
    };
    new_argv[2] = (char *)wad_path;

    myargc = 3;
    myargv = new_argv;

    D_DoomMain();

    sceKernelExitProcess(0);
    return 0;
}

#endif /* VITA */
