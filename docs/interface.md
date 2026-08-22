# SuperR7 interface

SuperR7 uses a native 240-by-160 card interface designed for the SuperCard SD.
The current layout presents a 76-by-76 cover beside a seven-row game list and a
four-item dock: Favorites, Recent, Browse, and Tools.

## Library navigation

- Up and Down move one item.
- Left and Right change to the previous or next fixed seven-item page and
  select its first item. Paging at the beginning or end of a list does nothing;
  the final page may contain fewer than seven items.
- In Browse, holding Left or Right repeats page changes and accelerates while
  the direction remains held. Releasing the button, reversing direction,
  pressing another direction, opening a popup, or leaving Browse resets the
  repeat sequence.
- Browse shows the current and total page as a compact fraction such as `1/2`
  in a rounded pill beneath the cover. Its connected outline uses the active
  Selection color, its fill uses the dock background, and its numbers use the
  dock's 5-by-7 text scale.
- A opens the normal launch flow.
- Start opens the Browse `View & sort` popup. Filename order can be `A-Z` or
  `Z-A`; Game type can show all files, all Game Boy-family ROMs (`.gba`, `.gb`,
  and `.gbc`), or only one of those three formats. Folders and unknown
  extensions can be shown or hidden independently. These view choices last
  until reboot.
- Long titles scroll inside the selected row.
- Browse, Recent, and Favorites share the same list and cover behavior.

Favorites are stored in `/.superfw/favorites.txt` and support up to 200 ROM
paths. Quick Launch provides `Add to Favorites` or `Remove Favorite`. In the
Favorites tab, Select requests confirmation before removing the selected item.

The Tools screen includes `Reset Favorites` and `Reset Recent`. Each action
asks for confirmation, clears only its saved list, and never deletes ROMs.

System Information displays the build fingerprint `Luna 1.1 SD` on its Build
row for the Luna 1.1 SuperCard SD release line.

During startup, SuperR7 keeps the completed boot logo visible while it prepares
Browse on the hidden framebuffer. The UI palette and completed first page are
then revealed together on VBlank, avoiding the inherited white transition
between the logo and the library.

## Launch flow

Selecting a GBA game opens Quick Launch. The main action starts the game;
Options, Advanced, and Details expose the remaining controls without crowding
the first screen. Secondary screens use `B: BACK` consistently.

## Appearance

Appearance includes four presets, independent Background, Accent, and
Selection colors, contrast handling, and five procedural wallpapers: None,
Weave, Grid, Circuit, and Tech Frame. The selected palette and wallpaper are
passed to the in-game menu when a game launches.

## Compatibility paths

SuperR7 retains the `/.superfw/` directory so existing SuperFW settings, saves,
patches, cheats, and emulator files remain usable. The inherited path is a
compatibility contract, not the public project name.

For exact cover encoding, see the [cover format](cover-format.md). For the
accepted firmware lineage, see [hardware validation](hardware-validation.md).
