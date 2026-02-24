# Chex Quest - PS Vita Port

A port of Chex Quest for PlayStation Vita, based on the doomgeneric source port of the Doom engine. This project brings the classic 1996 non-violent Doom total conversion — originally distributed as a promotional prize inside Chex cereal boxes — to the PS Vita handheld.

## About

Chex Quest is a total conversion of Doom created by Digital Cafe for Ralston Foods in 1996. The player takes the role of the Chex Warrior, using a "zorcher" to send Flemoid invaders back to their dimension. This port runs natively on the PS Vita using the VitaSDK toolchain and direct hardware access (no SDL dependency).

## Current Status

This is an early/work-in-progress port. The following limitations apply:

- The VPK is built without LiveArea icons or images (no icon0.png, bg.png, or startup.png). The game will show the default placeholder icon on the Vita home screen. This is intentional — including custom PNG files causes the VPK to fail installation or crash on boot due to the Vita's strict PNG format requirements. A future update may address this.
- Music playback uses a simplified OPL3 emulation (Nuked-OPL3) with only 9 mono OPL voices and single-voice instrument mode. Two-voice GENMIDI instruments are not fully supported. Music will sound different from the original PC version.
- Pitch bend from MUS data is parsed but not applied to OPL frequency registers.
- Rendering is software-scaled from 320x200 to 960x544 using nearest-neighbor interpolation. There is no bilinear filtering or hardware-accelerated scaling.
- No mouse or external controller support. Input is handled through the Vita's built-in controls only.
- Save/load bypasses the in-game menu system and uses a direct function call approach (see controls below).

## Features

- Fully playable Chex Quest on PS Vita and PSTV
- Sound effects with 8-channel software mixer at 48000 Hz
- OPL3 music synthesis from MUS lump data using the GENMIDI lump
- Dual analog stick support (left stick for movement, right stick for turning)
- Front touchscreen weapon selection
- D-pad weapon cycling while holding L trigger
- Direct quick save/load without navigating the menu
- CPU/GPU/bus clocked to maximum frequencies (444/222/222/166 MHz)
- Debug logging to ux0:/data/chexquest/debug.log

## Requirements

- PlayStation Vita or PSTV with HENkaku/Enso (firmware 3.60 - 3.74)
- VitaShell for file transfer
- A Chex Quest WAD file (chex.wad)

The game also supports doom1.wad and doom.wad as fallback WAD files.

## Installation

1. Download the latest VPK from the Releases page.
2. Transfer the VPK to your Vita using VitaShell (USB or FTP).
3. Install the VPK through VitaShell.
4. Copy your WAD file to:ux0:/data/chexquest/chex.wad
5. Launch Chex Quest from the LiveArea.

The game will create the directory ux0:/data/chexquest/ on first launch if it does not exist. Save files are stored in the same directory.

## Controls

### General

| Action              | Input                        |
|---------------------|------------------------------|
| Move forward/back   | Left stick up/down or D-pad  |
| Strafe left/right   | Left stick left/right        |
| Turn left/right     | Right stick left/right       |
| Fire (Zorch)        | Square or R trigger          |
| Use / Open doors    | Cross                        |
| Run                 | L trigger                    |
| Automap             | Triangle                     |
| Strafe modifier     | Circle                       |
| Menu / Pause        | Start                        |
| Confirm / Enter     | Select                       |

### Weapon Selection

| Method                           | Input                              |
|----------------------------------|------------------------------------|
| Cycle weapon forward             | L trigger + D-pad Up or Right      |
| Cycle weapon backward            | L trigger + D-pad Down or Left     |
| Direct weapon select (1-7)       | Touch top of front touchscreen     |

The front touchscreen is divided into 7 horizontal zones across the top 60 pixels. Touching a zone selects the corresponding weapon slot (1-7 from left to right).

### Quick Save / Load

| Action     | Input              |
|------------|--------------------|
| Quick Save | L + R + D-pad Up   |
| Quick Load | L + R + D-pad Down |

Quick save writes directly to save slot 0 with the description "VITA SAVE". Quick load reads from the same slot. Both have a cooldown of 1 second (35 tics) to prevent accidental repeated triggers. These functions bypass the in-game menu entirely — they call G_SaveGame and G_LoadGame directly.

### Analog Deadzone

The analog stick deadzone is set to 35 (out of 128). Values within this range are ignored.

## Building from Source

### Prerequisites

- VitaSDK installed and configured (https://vitasdk.org/)
- cmake (3.10 or later)
- make
- git

### Build Steps

```bash
git clone https://github.com/DraxTube/chexquest-vita.git
cd chexquest-vita

# Clone doomgeneric
git clone --depth 1 https://github.com/ozkl/doomgeneric.git

# Download Nuked-OPL3
curl -sL https://raw.githubusercontent.com/nukeykt/Nuked-OPL3/master/opl3.c \
-o doomgeneric/doomgeneric/opl3.c
curl -sL https://raw.githubusercontent.com/nukeykt/Nuked-OPL3/master/opl3.h \
-o doomgeneric/doomgeneric/opl3.h

# Copy Vita platform source into doomgeneric
cp doomgeneric_vita.c doomgeneric/doomgeneric/doomgeneric_vita.c

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
The build produces a VPK file in the build directory. The VPK is built without LiveArea image assets to avoid compatibility issues with the Vita's PNG validation.

CI/CD
The project includes a GitHub Actions workflow (.github/workflows/build.yml) that automates the full build process. The workflow:

Checks out the source and verifies required files exist
Installs VitaSDK via vdpm
Clones doomgeneric and downloads Nuked-OPL3
Builds the VPK
Repacks the VPK without compression for maximum compatibility
Uploads the VPK as a build artifact
Technical Details
Architecture
This port does not use SDL or any middleware. All platform interfaces are implemented directly against the Vita system libraries:

Display: Direct framebuffer allocation via sceKernelAllocMemBlock (CDRAM preferred, user RAM fallback) and sceDisplaySetFrameBuf
Input: sceCtrlPeekBufferPositive for buttons/sticks, sceTouchPeek for touchscreen
Audio: sceAudioOutOpenPort with a dedicated mixing thread running at elevated priority (0x10000100)
Timing: sceKernelGetProcessTimeLow for millisecond-resolution timing
Audio Engine
The audio system runs in a separate thread and mixes two sources into a single stereo output at 48000 Hz with a buffer granularity of 256 samples:

SFX: 8-channel software mixer. Doom-format sound lumps (format 3, 8-bit unsigned PCM) are decoded and resampled using fixed-point stepping. Stereo separation is applied per-channel. Sound data is cached (up to 128 entries) to avoid repeated WAD lookups.
Music: OPL3 FM synthesis via Nuked-OPL3. MUS format data is parsed and played through 9 OPL voices using instrument definitions from the GENMIDI lump. The OPL chip is ticked at 140 Hz. Only the first voice of each GENMIDI instrument is used; two-voice instruments are not fully rendered.
Both sources are mixed into a shared 32-bit accumulator buffer, clipped to 16-bit range, and output through sceAudioOutOutput.

Video
The game renders internally at 320x200 (standard Doom resolution) using an 8-bit paletted framebuffer. Each frame, the palette-indexed buffer is converted to 32-bit ABGR and scaled to 960x544 using fixed-point nearest-neighbor sampling, then presented via sceDisplaySetFrameBuf with vsync.

Memory
The Doom zone allocator is initialized with 16 MB (falling back to 8 MB if allocation fails). The framebuffer uses a separate CDRAM allocation.

WAD Search Order
On startup, the following paths are checked in order:

ux0:/data/chexquest/chex.wad
ux0:/data/chexquest/doom1.wad
ux0:/data/chexquest/doom.wad
The first file found is used as the IWAD. If no WAD is found, the game logs an error and exits after 5 seconds.

Title ID
The Vita application title ID is CHEX00001 (version 01.00).

Project Structure
text

chexquest-vita/
├── .github/
│   └── workflows/
│       └── build.yml              # GitHub Actions CI workflow
├── doomgeneric_vita.c             # Vita platform layer (display, input, audio, main)
├── CMakeLists.txt                 # Build configuration
└── README.md
The doomgeneric engine source and Nuked-OPL3 are fetched at build time and are not included in this repository.

Known Issues
The VPK has no LiveArea icons or images. The Vita home screen will show a default placeholder. Including custom PNGs causes installation failures due to the Vita's strict PNG chunk validation.
OPL3 music is simplified compared to the original PC output. Some instruments may sound wrong or be missing entirely.
Pitch bend events in MUS data are read but have no audible effect.
No gamma correction controls are exposed (usegamma is hardcoded to 0).
The game does not cleanly handle missing WAD files beyond logging and exiting.
Nearest-neighbor scaling may produce visible pixel artifacts, especially on diagonal lines and text.
The in-game save/load menu is not functional. Use the quick save/load button combo (L+R+Up / L+R+Down) instead.
Credits
Digital Cafe — Original Chex Quest developers (1996)
id Software — Original Doom engine
ozkl — doomgeneric source port (https://github.com/ozkl/doomgeneric)
Nuke.YKT — Nuked-OPL3 emulator (https://github.com/nukeykt/Nuked-OPL3)
VitaSDK team — Vita homebrew development toolchain
License
This project is based on the Doom source code released under the GNU General Public License v2.0. See the LICENSE file for details.

Chex Quest is a trademark of its respective owners. This is an unofficial fan port.
