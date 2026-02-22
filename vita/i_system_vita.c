/*
 * Chex Quest PS Vita - System interface
 * Handles timing, error reporting, memory, and Vita-specific system calls
 */

#ifdef VITA

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/rtc.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "vita_config.h"
#include "doomdef.h"
#include "m_argv.h"

// Timer
static uint32_t start_time = 0;

/*
 * Initialize system
 */
void I_InitVita(void) {
    // Set CPU/GPU frequencies for optimal performance
    scePowerSetArmClockFrequency(VITA_CPU_FREQ);
    scePowerSetBusClockFrequency(VITA_BUS_FREQ);
    scePowerSetGpuClockFrequency(VITA_GPU_FREQ);
    scePowerSetGpuXbarClockFrequency(166);

    // Create necessary directories
    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir(VITA_DATA_PATH, 0777);
    sceIoMkdir(VITA_SAVE_PATH, 0777);

    start_time = SDL_GetTicks();
}

/*
 * Get current time in tics (1/35 second units)
 * This is the heart of Doom's timing system
 */
int I_GetTime(void) {
    uint32_t now = SDL_GetTicks();
    return (int)((now - start_time) * TICRATE / 1000);
}

/*
 * Get time in microseconds (for more precise timing)
 */
uint64_t I_GetTimeUS(void) {
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    return tick.tick;
}

/*
 * Wait for vertical blank
 */
void I_WaitVBL(int count) {
    SDL_Delay((count * 1000) / 70);
}

/*
 * Sleep for a number of milliseconds
 */
void I_Sleep(int ms) {
    SDL_Delay(ms);
}

/*
 * Fatal error - display message and exit
 */
void I_Error(const char *error, ...) {
    char msg[1024];
    va_list args;

    va_start(args, error);
    vsnprintf(msg, sizeof(msg), error, args);
    va_end(args);

    // Write error to file for debugging
    FILE *f = fopen("ux0:/data/chexquest/error.log", "a");
    if (f) {
        fprintf(f, "ERROR: %s\n", msg);
        fclose(f);
    }

    // Print to stderr (visible in debug console)
    fprintf(stderr, "Error: %s\n", msg);

    // Shutdown everything
    extern void I_ShutdownGraphicsVita(void);
    extern void I_ShutdownSoundVita(void);
    I_ShutdownGraphicsVita();
    I_ShutdownSoundVita();

    // Give user time to see the error (in case of debug output)
    SDL_Delay(3000);

    // Exit
    sceKernelExitProcess(0);
}

/*
 * Quit cleanly
 */
void I_Quit(void) {
    extern void I_ShutdownGraphicsVita(void);
    extern void I_ShutdownSoundVita(void);
    extern void I_ShutdownVitaInput(void);

    I_ShutdownSoundVita();
    I_ShutdownGraphicsVita();
    I_ShutdownVitaInput();

    sceKernelExitProcess(0);
}

/*
 * Allocate memory from the zone
 * On Vita we have plenty of RAM, so use standard malloc
 */
void *I_ZoneBase(int *size) {
    // Allocate 16MB zone (Vita has 512MB+ available)
    *size = 16 * 1024 * 1024;
    void *ptr = malloc(*size);
    if (!ptr) {
        // Try smaller
        *size = 8 * 1024 * 1024;
        ptr = malloc(*size);
    }
    if (!ptr) {
        I_Error("I_ZoneBase: failed to allocate %d bytes", *size);
    }
    return ptr;
}

/*
 * Check disk space (always OK on Vita)
 */
int I_CheckDiskSpace(void) {
    return 1;
}

/*
 * File path handling for Vita
 */
const char *I_GetSavePath(void) {
    return VITA_SAVE_PATH;
}

const char *I_GetDataPath(void) {
    return VITA_DATA_PATH;
}

const char *I_GetConfigPath(void) {
    return VITA_CONFIG_FILE;
}

/*
 * Create the save game directory if it doesn't exist
 */
void I_CreateSaveDir(void) {
    sceIoMkdir(VITA_SAVE_PATH, 0777);
}

/*
 * Process system events
 */
void I_ProcessSystemEvents(void) {
    // Check if Vita wants us to suspend or anything
    // The OS handles this automatically for the most part
}

/*
 * Get Vita-specific system info
 */
void I_PrintVitaSystemInfo(void) {
    int battery = scePowerGetBatteryLifePercent();
    int temp = scePowerGetArmClockFrequency();

    FILE *f = fopen("ux0:/data/chexquest/sysinfo.log", "w");
    if (f) {
        fprintf(f, "Chex Quest PS Vita\n");
        fprintf(f, "Battery: %d%%\n", battery);
        fprintf(f, "CPU Clock: %d MHz\n", temp);
        fprintf(f, "Screen: %dx%d\n", VITA_SCREEN_W, VITA_SCREEN_H);
        fprintf(f, "Render: %dx%d\n", VITA_RENDER_W, VITA_RENDER_H);
        fclose(f);
    }
}

#endif /* VITA */
