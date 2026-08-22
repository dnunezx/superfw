# SuperR7

<p align="center">
  <img src="res/superr7-boot-logo-source.png" alt="SuperR7 logo" width="620">
</p>

> **YOUR SUPERCARD. POWERED UP.**
>
> Box art. Favorites. Seven-game pages. Page-at-a-glance. Custom colors. Same
> proven SuperFW engine underneath.

SuperR7 is an independent, GPL-licensed fork of
[SuperFW](https://github.com/davidgfnet/superfw) for **SuperCard SD GBA flash
carts**. It keeps the serious firmware technology and gives it a new
game-library experience built for the GBA's 240-by-160 screen.

Developed and hardware-tested by **Danny Nunez (dnunezx)** on SuperCard SD.

## The power-up

- **Big cover art** — native 76-by-76 `.sfcov` covers.
- **Seven games at once** — cover-focused rows with long-title support.
- **Page controls and position** — Up/Down moves one game; Left/Right changes
  seven-game pages and selects the first game. Browse adds accelerated held
  paging and a theme-colored `current/total` pill beneath the cover.
- **Four-item dock** — Favorites, Recent, Browse, and Tools.
- **Real Favorites** — save up to 200 games and launch them normally.
- **Browse your way** — sort A–Z or Z–A, filter for GBA/GB/GBC, and hide
  folders or unknown extensions from Browse.
- **Safe list maintenance** — Reset Favorites and Reset Recent never remove
  ROM files.
- **Quick Launch** — launch now or open Options, Advanced, and Details.
- **Make it yours** — four presets, custom colors, contrast control, and five
  wallpapers: None, Weave, Grid, Circuit, and Tech Frame.
- **Matching in-game menu** — your selected colors and wallpaper follow you
  into the game.

## See it in action

| Cover-powered library | Quick Launch and Favorites |
| :---: | :---: |
| ![SuperR7 seven-row cover library](docs/screenshots/library-browser.png) | ![SuperR7 Quick Launch Add to Favorites action](docs/screenshots/quick-launch-favorites.png) |
| **Tech Frame Appearance** | **Matching in-game menu** |
| ![SuperR7 Tech Frame Appearance settings](docs/screenshots/appearance-tech-frame.png) | ![SuperR7 matching in-game menu](docs/screenshots/in-game-menu.png) |

SuperFW supplies the proven foundation. SuperR7 turns it into a cover-powered
library.

SuperR7 keeps SuperFW's features, GPL license, credits, and compatible
`/.superfw/` SD-card layout. The projects have different interfaces,
roadmaps, and releases.

## Add box art with SuperCover

[**SuperCover**](https://github.com/dnunezx/SuperCover) makes the cover setup
easy:

1. Pick your GBA ROM folder.
2. Review the artwork matches.
3. Export to `/.superfw/covers/` on your SuperCard SD.

That's it. SuperCover scans your ROMs, creates SuperR7's
76-by-76 cover files, and gives them the filenames the firmware expects.

Want full manual control? Use the included
[cover converter](docs/cover-converter.md).

## Original power under the hood

SuperR7 retains SuperFW's major features:

- SDHC and exFAT support.
- WAITCNT, save, IRQ, and RTC patching.
- SRAM save protection and optional Direct-Saving.
- In-game saves, savestates, cheats, reset, RTC, and return-to-menu.
- Game Boy and Game Boy Color emulation through Goomba Color.
- Per-game settings, patch cache, cheat files, and emulator support.

## Controls

| Button | Library action |
| --- | --- |
| Up / Down | Previous or next game |
| Left / Right | Previous or next seven-game page; hold to accelerate in Browse |
| A | Open Quick Launch |
| B | Go back |
| Start | Open View & Sort in Browse |
| Select | Remove a game from Favorites after confirmation |

## Hardware checkpoint

The latest confirmed functional build passed physical **SuperCard SD** testing
on August 22, 2026.

- Firmware size: **521,728 bytes**
- Flash limit: **524,288 bytes**
- Free space: **2,560 bytes**
- SHA-256:
  `E9AA581CB3D8401D3663646F1D72183B674C753CE0266C692A50BA056D853204`
- Tested features include fixed-page navigation, the connected page pill,
  Browse View & Sort, Reset Favorites/Recent, and the direct boot-logo-to-UI
  handoff without the white transition frame.

Current source identifies the SD release line in System Information with the
build fingerprint **Luna 1.1 SD**. The final fingerprinted image remains a
candidate until that exact binary completes its last chain-load check.

Every new binary is a new candidate. Chain-load it before flashing it. Never
flash an unverified development build. Exact hashes and rollback images are in
the [hardware-validation record](docs/hardware-validation.md).

## Get it

Versioned firmware downloads belong on
[GitHub Releases](https://github.com/dnunezx/SuperR7/releases). Verify the
published checksum, chain-load the `.gba` file, and test it on your hardware
before considering an internal-flash installation.

Need installation or recovery details? Use the inherited
[SuperFW installation guide](https://superfw.davidgf.net/docs/install/flash/).

## Build it

```sh
make BOARD=sd COMPRESSION_RATIO=10 superr7.gba
```

Output: `superr7.gba`. Building is not hardware validation—test every new
image.

## Learn more

- [Interface and controls](docs/interface.md)
- [Cover format](docs/cover-format.md)
- [Documentation index](docs/README.md)
- [Development history](docs/history/README.md)

## Credits and license

SuperR7-specific work is copyright (C) 2026 **Danny Nunez (dnunezx)**.
SuperR7 is based on SuperFW, primarily written by **David Guillen Fandos
(davidgf)**. Upstream authorship and copyright notices are preserved.

Licensed under the **GNU General Public License, version 3 or later**. See
[LICENSE](LICENSE) and [CREDITS.md](CREDITS.md).
