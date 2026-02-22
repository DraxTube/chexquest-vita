#!/bin/bash
set -e

echo "=== Preparing Chex Quest Vita Build ==="

CHOCO_DIR="chocolate-doom"
VITA_DIR="vita"

# -------------------------------------------------------
# 1) Verify Chocolate Doom was cloned
# -------------------------------------------------------
if [ ! -d "$CHOCO_DIR" ]; then
    echo "ERROR: Chocolate Doom directory not found!"
    exit 1
fi
echo "[1/4] Chocolate Doom found."

# -------------------------------------------------------
# 2) Copy Vita platform layer into Chocolate Doom src
# -------------------------------------------------------
echo "[2/4] Copying Vita platform files..."
cp -v ${VITA_DIR}/i_video_vita.c    ${CHOCO_DIR}/src/
cp -v ${VITA_DIR}/i_input_vita.c    ${CHOCO_DIR}/src/
cp -v ${VITA_DIR}/i_sound_vita.c    ${CHOCO_DIR}/src/
cp -v ${VITA_DIR}/i_system_vita.c   ${CHOCO_DIR}/src/
cp -v ${VITA_DIR}/vita_config.h     ${CHOCO_DIR}/src/
cp -v ${VITA_DIR}/vita_main.c       ${CHOCO_DIR}/src/

# -------------------------------------------------------
# 3) Generate config.h for Chocolate Doom
# -------------------------------------------------------
echo "[3/4] Generating config.h..."
cat > ${CHOCO_DIR}/config.h << 'CONFIG_EOF'
/* config.h - Generated for PS Vita build */
#ifndef CHOCOLATE_DOOM_CONFIG_H
#define CHOCOLATE_DOOM_CONFIG_H

#define PACKAGE_NAME "chex-quest-vita"
#define PACKAGE_TARNAME "chex-quest-vita"
#define PACKAGE_VERSION "1.0.0"
#define PACKAGE_STRING "chex-quest-vita 1.0.0"
#define PACKAGE "chex-quest-vita"
#define VERSION "1.0.0"

#define PROGRAM_PREFIX ""

/* PS Vita platform */
#define VITA 1
#define __vita__ 1

/* We have SDL2 */
#define HAVE_LIBSDL2 1
#define HAVE_LIBSDL2_MIXER 1
#define HAVE_LIBSDL2_NET 1

/* Standard headers */
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1

/* No POSIX stuff on Vita */
/* #undef HAVE_DECL_STRCASECMP */
/* #undef HAVE_DECL_STRNCASECMP */

#endif /* CHOCOLATE_DOOM_CONFIG_H */
CONFIG_EOF

# -------------------------------------------------------
# 4) List what we have
# -------------------------------------------------------
echo "[4/4] Verifying files..."
echo "=== Chocolate Doom src/ contents ==="
ls -la ${CHOCO_DIR}/src/*.c ${CHOCO_DIR}/src/*.h 2>/dev/null | head -30
echo "..."
echo "=== Chocolate Doom src/doom/ contents ==="
ls -la ${CHOCO_DIR}/src/doom/*.c 2>/dev/null | head -20
echo "..."

echo ""
echo "=== Preparation complete! ==="
