# Chex Quest - PS Vita Port

**Port of Chex Quest (1996) to PlayStation Vita using the doomgeneric engine.**
No SDL. All video, audio, and input are implemented directly against the Vita system libraries.

---

### 🔥 ALSO AVAILABLE ON PSP!
**I have just released a native port for the PlayStation Portable!**
It features a custom OPL2 synthesizer, native resolution, and full optimization for the PSP hardware.
👉 **[Check out the Chex Quest PSP Port here](https://github.com/DraxTube/Chex-Quest-PSP)**

---

## 📅 Update (Feb 25)
**THANKS TO THE WORK OF LATINWIZARD99 THE GAME NOW HAS COMPLETE ASSETS.**

## 🛠️ How It Works
The project takes the `doomgeneric` source port of Doom, replaces all platform-specific code with a single Vita backend file (`doomgeneric_vita.c`), and adds OPL3 FM music synthesis via `Nuked-OPL3`. The result is a standalone VPK that runs Chex Quest (or any compatible Doom IWAD) natively on the Vita.

## 📋 Requirements
*   **PS Vita or PSTV** running HENkaku/Enso (firmware 3.60-3.74)
*   **VitaShell**
*   `chex.wad` (the original Chex Quest WAD file)
*   *Note: `doom1.wad` and `doom.wad` are also supported as fallbacks.*

## 📥 How to Install
1.  Download the **VPK** from the [Releases](../../releases) page.
2.  Transfer it to the Vita and install it using **VitaShell**.
3.  Place your WAD file at:
    ```
    ux0:/data/chexquest/chex.wad
    ```
4.  Launch the game from the LiveArea.
    *   *Save files and debug logs are written to the same directory.*

## 🎮 Controls

| Action | Input |
| :--- | :--- |
| **Move** | Left Stick |
| **Turn** | Right Stick |
| **Fire** | Square **OR** R Trigger |
| **Use / Open** | Cross (X) |
| **Run** | L Trigger |
| **Automap** | Triangle |
| **Strafe Modifier** | Circle |
| **Menu** | Start |
| **Confirm** | Select |
| **Weapon Cycle** | D-Pad Left / Right |

### Quick Save / Load
*   **Quick Save:** `D-Pad Up` (Saves to slot 0)
*   **Quick Load:** `D-Pad Down` (Loads from slot 0)
*   *Note: Both have a 1-second cooldown. The in-game save/load menu does not work, please use these combos instead.*

## ⚠️ Known Issues
*   **Music is simplified:** Only 9 OPL voices, single-voice instruments only. Two-voice GENMIDI patches are not rendered correctly. Music sounds noticeably different from the original PC version.
*   **Pitch Bend:** Parsed but ignored. Notes play at fixed pitch regardless of MUS pitch bend events.
*   **No Gamma Control:** `usegamma` is hardcoded to 0.
*   **Scaling:** Nearest-neighbor scaling. The 320x200 framebuffer is scaled to 960x544 without filtering. Pixels are visible and diagonal lines look rough.
*   **Input:** No external controller or mouse support.
*   **Error Handling:** If no WAD file is found, the game logs an error and exits after 5 seconds with no on-screen message.

## 🙏 Credits
*   **id Software** for Doom
*   **Digital Café** for Chex Quest
*   **ozkl** for doomgeneric
*   **LatinWizard99** for completing the assets and testing
*   **DraxTube** for the porting work
