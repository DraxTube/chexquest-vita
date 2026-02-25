# Chex Quest - PS Vita Port

Port of Chex Quest (1996) to PlayStation Vita using the doomgeneric engine. No SDL. All video, audio, and input are implemented directly against the Vita system libraries.

## How It Works

The project takes the doomgeneric source port of Doom, replaces all platform-specific code with a single Vita backend file (doomgeneric_vita.c), and adds OPL3 FM music synthesis via Nuked-OPL3. The result is a standalone VPK that runs Chex Quest (or any compatible Doom IWAD) natively on the Vita.

## Requirements

- PS Vita or PSTV running HENkaku/Enso (firmware 3.60-3.74)
- VitaShell
- chex.wad (the original Chex Quest WAD file)

doom1.wad and doom.wad are also supported as fallbacks.

## How to Install

1. Download the VPK from the Releases page.
2. Transfer it to the Vita and install it with VitaShell.
3. Place your WAD file at:ux0:/data/chexquest/chex.wad
4. Launch the game.

Save files and debug logs are written to the same directory.

## Controls

| Action            | Input                       |
|-------------------|-----------------------------|
| Move              | Left stick or D-pad         |
| Turn              | Right stick                 |
| Fire              | Square or R trigger         |
| Use / Open        | Cross                       |
| Run               | L trigger                   |
| Automap           | Triangle                    |
| Strafe modifier   | Circle                      |
| Menu              | Start                       |
| Confirm           | Select                      |

Weapon cycling: hold L trigger and press D-pad Up/Down/Left/Right.

Direct weapon select: touch the top of the front touchscreen (divided into 7 zones, left to right = weapons 1-7).

Quick save: L + R + D-pad Up (saves to slot 0).
Quick load: L + R + D-pad Down (loads from slot 0).
Both have a 1 second cooldown. The in-game save/load menu does not work, use these combos instead.

## Known Issues

-  ̶N̶o̶ ̶L̶i̶v̶e̶A̶r̶e̶a̶ ̶i̶c̶o̶n̶s̶.̶ ̶T̶h̶e̶ ̶V̶P̶K̶ ̶s̶h̶i̶p̶s̶ ̶w̶i̶t̶h̶o̶u̶t̶ ̶i̶c̶o̶n̶0̶.̶p̶n̶g̶,̶ ̶b̶g̶.̶p̶n̶g̶,̶ ̶o̶r̶ ̶s̶t̶a̶r̶t̶u̶p̶.̶p̶n̶g̶.̶ ̶I̶n̶c̶l̶u̶d̶i̶n̶g̶ ̶c̶u̶s̶t̶o̶m̶ ̶P̶N̶G̶s̶ ̶c̶a̶u̶s̶e̶s̶ ̶t̶h̶e̶ ̶V̶P̶K̶ ̶t̶o̶ ̶f̶a̶i̶l̶ ̶o̶n̶ ̶i̶n̶s̶t̶a̶l̶l̶ ̶d̶u̶e̶ ̶t̶o̶ ̶t̶h̶e̶ ̶V̶i̶t̶a̶'̶s̶ ̶s̶t̶r̶i̶c̶t̶ ̶P̶N̶G̶ ̶f̶o̶r̶m̶a̶t̶ ̶r̶e̶q̶u̶i̶r̶e̶m̶e̶n̶t̶s̶.̶ ̶T̶h̶e̶ ̶g̶a̶m̶e̶ ̶s̶h̶o̶w̶s̶ ̶a̶ ̶d̶e̶f̶a̶u̶l̶t̶ ̶p̶l̶a̶c̶e̶h̶o̶l̶d̶e̶r̶ ̶o̶n̶ ̶t̶h̶e̶ ̶h̶o̶m̶e̶ ̶s̶c̶r̶e̶e̶n̶.̶
Thanks to the work of LatinWizard99 the game has asset complete
- Music is simplified. Only 9 OPL voices, single-voice instruments only. Two-voice GENMIDI patches are not rendered correctly. Music sounds noticeably different from the original PC version.
- Pitch bend is parsed but ignored. Notes play at fixed pitch regardless of MUS pitch bend events.
- No gamma control. usegamma is hardcoded to 0.
- Nearest-neighbor scaling. The 320x200 framebuffer is scaled to 960x544 without filtering. Pixels are visible and diagonal lines look rough.
- No external controller or mouse support.
- If no WAD file is found the game logs an error and exits after 5 seconds with no on-screen message.



