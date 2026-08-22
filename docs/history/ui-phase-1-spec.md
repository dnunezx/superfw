# Historical Phase 1 interface specification

> This is the original design checkpoint. See the current
> [SuperR7 interface guide](../interface.md) for production behavior.

## Production evolution

This document preserves the original Phase 1 design target. Production
SuperR7 now uses seven rows, native 76-by-76 covers, a 16-pixel dock, and the
dock order Favorites, Recent, Browse, and Tools. The August 10, 2026 Favorites
build keeps the same card, cover, title-scroll, and list-navigation behavior
across Favorites, Browse, and Recent.

Favorites persist at `/.superfw/favorites.txt`. Quick Launch centers
`Add to Favorites` beneath `Launch game` and toggles it to `Remove Favorite`.
The archived Phase 11 build used Up/Down for one item and Left/Right for a
seven-item offset, retaining a full final seven-row window. Focused emulator
checks and physical SuperCard SD verification passed with that candidate. The
August 14 production behavior supersedes that navigation model with fixed
pages, boundary no-ops, and a potentially partial final page. The original
empty Favorites presentation remains unchanged. The August 21 and 22 Browse
follow-ups add accelerated held Left/Right paging plus a dynamic page fraction
beneath the cover; the August 22 image connects the pill's rounded outline.
The later Luna 1.1 SD line adds Browse-only sorting and Game Boy-family filters,
confirmed Favorites/Recent reset tools, a clean logo-to-UI VBlank handoff, and
the visible `Luna 1.1 SD` System Information build fingerprint.

The Launch flow's Options, Advanced, and Details screens now consistently use
`B: BACK`. The exact archived Phase 13 image passed physical SuperCard SD boot
and visual testing on August 11, 2026. It became the immediate rollback for
the boot-logo-v2 firmware at that checkpoint.

On August 12, 2026, `superr7-boot-logo-v2.gba` passed physical hardware
testing and became the current, most recent hardware-validated SuperR7
firmware. It replaces the historical Gothic splash and progress bar with a
centered 112-by-84 stacked `Super R7` logo. Phase 13 is its immediate archived
rollback for that historical checkpoint. Later hardware-validated Tech Frame,
fixed-page navigation, dynamic page-pill, and connected page-pill images now
follow it in the accepted lineage.

## Decision

Use the **six-row layout with a compact 20-pixel dock** as the Phase 2 emulator
target. It preserves the approved card appearance while showing one more game
than the five-row minimum. The five-row layout remains the fallback if
physical-screen testing later shows that six rows are uncomfortable to read.

Both variants use the existing 72-by-72 cover size and a 20-pixel bottom dock.
All reference images are exact 240-by-160 PNGs; the `-4x` copies use
nearest-neighbor enlargement for inspection only.

## Reference images

- [Five-row native screen](../ui-mockups/phase1/browser-native-5-row.png)
- [Five-row enlarged inspection copy](../ui-mockups/phase1/browser-native-5-row-4x.png)
- [Six-row native screen](../ui-mockups/phase1/browser-native-6-row.png)
- [Six-row enlarged inspection copy](../ui-mockups/phase1/browser-native-6-row-4x.png)
- [Folder and unsupported-file states](../ui-mockups/phase1/browser-native-states.png)
- [Enlarged state inspection copy](../ui-mockups/phase1/browser-native-states-4x.png)

The images are generated deterministically by
`tools/render_ui_phase1.py`. The script accepts a square preview image and does
not embed or distribute cover artwork.

## Production target geometry

| Element | Bounds |
| --- | --- |
| GBA framebuffer | `x=0..239`, `y=0..159` |
| Main content | `x=0..239`, `y=0..139` |
| Cover outer frame and shadow | `x=4..81`, `y=32..109` |
| Cover pixels | `x=6..77`, `y=34..105` |
| Game-card column | `x=86..235` |
| Bottom navigation dock | `x=0..239`, `y=140..159` |
| Dock cells | Four cells, 60 pixels wide |

### Selected six-row geometry

Each card is 19 pixels high with a three-pixel gap:

| Visible row | Vertical bounds |
| ---: | --- |
| 1 | `y=4..22` |
| 2 | `y=26..44` |
| 3 | `y=48..66` |
| 4 | `y=70..88` |
| 5 | `y=92..110` |
| 6 | `y=114..132` |

Rows leave seven clear pixels before the dock divider. Filename text begins at
`x=108`; the area at `x=92..101` is reserved for a compact file-state icon.

### Five-row fallback geometry

Each fallback card is 24 pixels high with a three-pixel gap:

| Visible row | Vertical bounds |
| ---: | --- |
| 1 | `y=4..27` |
| 2 | `y=31..54` |
| 3 | `y=58..81` |
| 4 | `y=85..108` |
| 5 | `y=112..135` |

## Background palette allocation

The browser renderer may use background palette indices 0-19. Existing cover
art continues to own indices 20-239. Indices 240-255 remain outside this UI
specification so existing system behavior can be preserved.

| Index | Purpose | Reference RGB |
| ---: | --- | --- |
| 0 | Primary text/navy | `#061A4A` |
| 1 | Deep-blue structure | `#07336E` |
| 2 | Main blue background | `#07509A` |
| 3 | Background stripe | `#0B68B8` |
| 4 | Light background stripe | `#1380CC` |
| 5 | Normal card fill | `#CDEFF3` |
| 6 | Selected card/cover-frame fill | `#F1FFFF` |
| 7 | Card shadow | `#05295D` |
| 8 | Normal card edge | `#4BB4D4` |
| 9 | Selected edge and pointer | `#FF6B2C` |
| 10 | Selected shadow | `#A9371E` |
| 11 | Active cyan accent | `#1FC4D4` |
| 12 | Dark cyan dock fill | `#087E9B` |
| 13 | Valid-game accent | `#A5D92B` |
| 14 | Bright text | `#FFFFFF` |
| 15 | Inactive text/icon | `#78A9B8` |
| 16 | Folder | `#FFD050` |
| 17 | Invalid/error | `#E44A4A` |
| 18 | Disabled/unsupported | `#718493` |
| 19 | Reserved black | `#000000` |

These RGB values are design references. Production code must convert them to
GBA BGR555 and reuse existing theme/palette installation paths where possible.
Footer icons should use the separate OBJ palette if that reduces background
palette pressure without conflicting with the existing selector sprites.

## Required visual states

### Cover panel

- **Ready:** draw the validated 72-by-72 cover inside the selected frame.
- **Pending:** immediately hide stale artwork and show a neutral loading state.
- **Folder or parent:** show an original folder illustration.
- **Missing:** show a neutral `NO COVER` treatment.
- **Invalid:** show a distinct error treatment without using cover palette data.
- **SD error:** show a distinct storage-error treatment and keep navigation live.
- **Unsupported or empty:** show an empty framed panel.

### Game cards

- **Selected game:** bright fill, orange edge/shadow, and left pointer.
- **Unselected game:** pale-cyan fill with blue edge/shadow.
- **Folder:** yellow folder icon and normal readable text.
- **Parent directory:** folder icon and explicit parent label.
- **Unsupported file:** muted text and an error-marked document icon.
- **Disabled action:** muted card without an orange selection treatment.

### Filenames

- The selected filename retains the existing delayed horizontal-scroll behavior.
- Unselected filenames are clipped with `...` inside the card text bounds.
- The file-state icon is never allowed to overlap filename text.
- Directory and parent indicators are icons, not filename prefixes alone.

### Navigation dock

The four 60-pixel cells are Browse, Recent, Setup, and Tools. Each cell has an
original compact icon beside a visible text label. The active cell uses cyan
fill and bright text; inactive cells use the navy surface and muted text. Setup
maps to the complete Settings area. L/R switching behavior remains for the
later integration phase.

## Phase 2 handoff constraints

- Implement the six-row renderer only in the emulator/demo path first.
- Keep the five-row renderer available through one compile-time layout constant
  until physical readability is confirmed.
- Do not modify browser storage, cover lookup, game launch, or save behavior.
- Generate screenshots for every row position and all required cover states.
- Compare all drawing bounds against this document in automated visual tests.
