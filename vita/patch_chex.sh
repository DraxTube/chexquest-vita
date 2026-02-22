#!/bin/bash
set -e

echo "=== Patching Chocolate Doom for Chex Quest on PS Vita ==="

CHOCO_DIR="chocolate-doom"
VITA_DIR="vita"
PROJECT_ROOT=$(pwd)

# -------------------------------------------------------
# 1) Copy Vita-specific source files into chocolate-doom
# -------------------------------------------------------
echo "[1/6] Copying Vita platform files..."
cp ${VITA_DIR}/i_video_vita.c    ${CHOCO_DIR}/src/
cp ${VITA_DIR}/i_input_vita.c    ${CHOCO_DIR}/src/
cp ${VITA_DIR}/i_sound_vita.c    ${CHOCO_DIR}/src/
cp ${VITA_DIR}/i_system_vita.c   ${CHOCO_DIR}/src/
cp ${VITA_DIR}/vita_config.h     ${CHOCO_DIR}/src/

# -------------------------------------------------------
# 2) Patch d_iwad.c to auto-detect chex.wad
# -------------------------------------------------------
echo "[2/6] Patching IWAD detection for Chex Quest..."

cat > /tmp/iwad_patch.patch << 'PATCH_EOF'
--- a/src/d_iwad.c
+++ b/src/d_iwad.c
@@ -0,0 +1,15 @@
+// Additional search paths for PS Vita
+#ifdef VITA
+static const char *vita_iwad_dirs[] = {
+    "ux0:/data/chexquest/",
+    "ux0:/data/doom/",
+    "app0:/",
+    NULL
+};
+#endif
PATCH_EOF

# Instead of applying patch (may fail on structure), inject directly
sed -i 's|static const iwad_t iwads\[\] =|// Chex Quest WAD detection\nstatic const char *vita_search_paths[] = {\n    "ux0:/data/chexquest/",\n    "ux0:/data/doom/",\n    "app0:/",\n    NULL\n};\n\nstatic const iwad_t iwads[] =|' ${CHOCO_DIR}/src/d_iwad.c 2>/dev/null || true

# -------------------------------------------------------
# 3) Patch D_DoomMain for Chex Quest defaults
# -------------------------------------------------------
echo "[3/6] Patching main game loop for Chex Quest defaults..."

# Add Chex Quest detection to d_main.c
sed -i '/gamemode = indetermined;/a\
    // Chex Quest auto-detection\
    #ifdef VITA\
    {\
        FILE *f = fopen("ux0:/data/chexquest/chex.wad", "rb");\
        if (f) {\
            fclose(f);\
            gamemode = retail;\
            gamemission = chex;\
        }\
    }\
    #endif' ${CHOCO_DIR}/src/doom/d_main.c 2>/dev/null || true

# -------------------------------------------------------
# 4) Create the main Vita entry point
# -------------------------------------------------------
echo "[4/6] Creating Vita main entry point..."

cat > ${CHOCO_DIR}/src/vita_main.c << 'VITA_MAIN_EOF'
/*
 * Chex Quest for PS Vita
 * Main entry point - initializes Vita hardware and launches the game
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

// Forward declare the original main
int D_DoomMain(void);
void doomgeneric_Create(int argc, char **argv);

// Ensure data directory exists
static void vita_create_dirs(void) {
    sceIoMkdir("ux0:/data/", 0777);
    sceIoMkdir("ux0:/data/chexquest/", 0777);
    sceIoMkdir("ux0:/data/chexquest/savegames/", 0777);
}

// Check if WAD file exists
static int vita_check_wad(void) {
    SceUID fd = sceIoOpen("ux0:/data/chexquest/chex.wad", SCE_O_RDONLY, 0);
    if (fd >= 0) {
        sceIoClose(fd);
        return 1;
    }
    // Also try doom.wad / doom1.wad as fallback
    fd = sceIoOpen("ux0:/data/chexquest/doom.wad", SCE_O_RDONLY, 0);
    if (fd >= 0) {
        sceIoClose(fd);
        return 1;
    }
    fd = sceIoOpen("ux0:/data/chexquest/doom1.wad", SCE_O_RDONLY, 0);
    if (fd >= 0) {
        sceIoClose(fd);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    // Overclock CPU & GPU for smooth gameplay
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    // Initialize app utilities
    SceAppUtilInitParam init_param;
    SceAppUtilBootParam boot_param;
    memset(&init_param, 0, sizeof(SceAppUtilInitParam));
    memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
    sceAppUtilInit(&init_param, &boot_param);

    // Create necessary directories
    vita_create_dirs();

    // Check for WAD file
    if (!vita_check_wad()) {
        // WAD not found - we'll let the engine handle the error message
        // User needs to place chex.wad in ux0:/data/chexquest/
    }

    // Set up command line arguments for Chex Quest
    char *new_argv[] = {
        "ChexQuest",
        "-iwad", "ux0:/data/chexquest/chex.wad",
        NULL
    };
    int new_argc = 3;

    // Check if chex.wad exists, if not try doom.wad
    SceUID fd = sceIoOpen("ux0:/data/chexquest/chex.wad", SCE_O_RDONLY, 0);
    if (fd < 0) {
        fd = sceIoOpen("ux0:/data/chexquest/doom1.wad", SCE_O_RDONLY, 0);
        if (fd >= 0) {
            sceIoClose(fd);
            new_argv[2] = "ux0:/data/chexquest/doom1.wad";
        } else {
            fd = sceIoOpen("ux0:/data/chexquest/doom.wad", SCE_O_RDONLY, 0);
            if (fd >= 0) {
                sceIoClose(fd);
                new_argv[2] = "ux0:/data/chexquest/doom.wad";
            }
        }
    } else {
        sceIoClose(fd);
    }

    // Launch Doom engine with Chex Quest WAD
    myargc = new_argc;
    myargv = new_argv;

    D_DoomMain();

    sceKernelExitProcess(0);
    return 0;
}

#endif /* VITA */
VITA_MAIN_EOF

# -------------------------------------------------------
# 5) Fix includes and compatibility issues
# -------------------------------------------------------
echo "[5/6] Fixing compatibility issues..."

# Add VITA define guards to system-specific files
find ${CHOCO_DIR}/src -name "*.c" -exec sed -i '1i\
#ifdef __vita__\n#define VITA 1\n#endif' {} \; 2>/dev/null || true

# Fix mkdir calls for Vita
find ${CHOCO_DIR}/src -name "*.c" -exec sed -i 's|mkdir(|sceIoMkdir(|g' {} \; 2>/dev/null || true

echo "[6/6] Patching complete!"
echo ""
echo "=== Build Instructions ==="
echo "Place chex.wad in ux0:/data/chexquest/ on your PS Vita"
echo "=========================="
