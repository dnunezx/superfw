# SuperR7 interface redesign history

> Historical record. For current user-facing behavior, see the
> [SuperR7 interface guide](../interface.md). References to `artifacts/` below
> identify ignored local validation outputs rather than published downloads.

## Objective

Replace the basic inherited SuperFW browser presentation with an original handheld-era
card interface while preserving the hardware-proven cover loader, ROM browser,
game launching, saves, settings, and tools.

The approved direction places the selected game's square cover on the left,
seven visible game cards on the right, and primary navigation along the
bottom. The seven-row layout has passed evaluation on a physical 240-by-160
GBA screen.

Reference concept:

- [Generated browser concept](../ui-mockups/cover-browser-concept-v1.png)
- The generated concept establishes the visual direction, not final pixel
  measurements or production-ready assets.

## Current checkpoint

Status as of **August 22, 2026: the Browse-controls and clean boot-handoff build
is the current hardware-validated firmware. Luna 1.1 SD adds its visible build
fingerprint and is awaiting the exact final chain-load before publication**.

- Current hardware-validated firmware:
  `superr7-logo-to-ui-no-white-flash-hardware-test.gba`, 521,728 bytes (2,560
  bytes below the 512 KiB limit), with SHA-256
  `E9AA581CB3D8401D3663646F1D72183B674C753CE0266C692A50BA056D853204`.
  It adds Browse-only A–Z/Z–A sorting, all/GBA/GB/GBC filters, independent
  folder and unknown-extension visibility, Reset Favorites, Reset Recent, and
  a direct VBlank handoff from the boot logo to the first completed UI frame.
  The user confirmed that this exact image works great on physical hardware.
- Luna 1.1 SD release candidate:
  the same source line with `Luna 1.1 SD` displayed on System Information's
  Build row. Its final tagged binary remains pending one exact-image chain-load
  before GitHub publication.
- Immediate hardware-validated rollback:
  `superr7-connected-page-pill-test.gba`, 520,704 bytes (3,584 bytes
  below the 512 KiB limit), with
  SHA-256
  `136C0726E4758460BBE1E7988F4647F8429946F839B0E556A472FC0230482C36`.
  It preserves fixed seven-item pages, accelerated held Browse paging, and the
  dynamic fraction beneath the cover while connecting all four shoulders of
  the rounded outline. The user confirmed that this exact image works great on
  physical hardware.
- Earlier hardware-validated rollback:
  `superr7-dynamic-page-pill-hardware-test.gba`, 520,704 bytes (3,584 bytes
  below the 512 KiB limit), with SHA-256
  `0A64F0A7A528B5469B83EC25CA35795608D022ACC344E849E8B39ED9F565CCB2`.
  It introduced the dynamic Browse page pill and accelerated held paging but
  retains the disconnected shoulder pixels in the pill outline.
- Earlier hardware-validated rollback:
  `superr7-page-navigation-hardware-test.gba`, 520,192 bytes (4,096 bytes
  below the 512 KiB limit), with SHA-256
  `DCD599CD17745FB350A7176C24257377AB9B301E6C5B661B25594E3D53E5C940`.
  It introduced fixed seven-item pages with boundary no-ops and partial final
  pages.
- Earlier hardware-validated Tech Frame rollback: `superr7.gba`, 520,192
  bytes (4,096 bytes below the 512 KiB limit), with SHA-256
  `63EE181F90C0FCACB0014D6F819B6994C26E2677A589C9DCC355718C9F1FAA4F`.
- Earlier hardware-validated rollback:
  `superr7-boot-logo-v2.gba`, 519,168 bytes (5,120 bytes below the 512 KiB
  limit), with SHA-256
  `6DCDEA075A8CF04C8A4FF523F20628D5FF74AFF7126E3234F59A7B9E93A26BFC`.
  It uses the supplied stacked `Super R7` artwork as a 56-by-42 1bpp mask,
  rendered at 2x as a centered 112-by-84 logo on black, with no progress bar.
  The exact ROM passed native mGBA framebuffer checks. The user then confirmed
  that it booted, worked, and looked great on physical hardware.
  The exact tested file is preserved in the workspace with the matching size
  and SHA-256.

- Earlier archived rollback:
  `artifacts/phase13-launch-back-test/superr7-phase13-launch-back-8ca8aaf2.gba`,
  519,168 bytes (5,120 bytes below the 512 KiB limit), with SHA-256
  `8CA8AAF27941BAE9A1DF35D3C3E88C863CEF51B4AA5C35F1F25D780F0C808A2F`.
  It changes the Launch flow's Options, Advanced, and Details footer from
  `B: QUICK` to `B: BACK`. Local checks passed, and the user confirmed that
  this exact image booted, worked correctly, and looked great on physical
  SuperCard SD hardware. It is now the accepted Phase 13 rollback.

- Previous hardware-validated firmware candidate:
  `artifacts/phase11-favorites-hardware/superr7-phase11-favorites-cbdae08b.gba`,
  520,704 bytes (3,584 bytes below the 512 KiB limit), with a 105,899-byte
  ratio-10 compressed main payload and SHA-256
  `CBDAE08B529566E37517776CA336B1FF070AEF4296ED09503E961577972C68B3`.
  It was built from the current working tree with embedded base build ID
  `a27602e6` and archived byte-identically after the hardware pass.
- Favorites persist at `/.superfw/favorites.txt`. Quick Launch centers
  `Add to Favorites` beneath `Launch game` and toggles it to
  `Remove Favorite` after the add succeeds.
- Favorites, Browse, and Recent share the same seven-row navigation: Up/Down
  moves one item, Left/Right selects the first item of the adjacent fixed page,
  boundary page inputs do nothing, and the final page may be partial.
  Favorites uses A for the normal launch flow and Select for confirmed removal.
- The focused Favorites, navigation, Quick Launch, loading, popup, secondary
  screen, Phase 4, and in-game-menu checks pass. The user subsequently
  confirmed that the exact candidate passed physical hardware testing.

- Previous hardware-validated in-game-menu candidate:
  `artifacts/phase10-ingame-menu-hardware/superr7-phase10-ingame-menu-bffca9f.gba`,
  520,192 bytes, SHA-256
  `BFFCA9F38B40F8920FE38BAB47A705B452C4AB4D74A2A518E2C70AAD689F0A9F`.
- Earlier Phase 9 boot-logo candidate:
  `releases/archive/2026-08-07-gothic-boot/superr7-phase9-gothic-boot-1eec14f.gba`,
  518,144 bytes, SHA-256
  `93F774D81C6DEF17125587A66B121A8CF126D87256F7F547389ACD482F49A1E5`.
- The Browse-controls and clean-handoff image is the current hardware-validated
  firmware. The connected-page-pill, original dynamic-pill, and fixed-page
  navigation images are its first three rollbacks, followed by the Tech Frame,
  boot-logo-v2, Phase 13 Launch Back, and archived Phase 11 Favorites images.
- Earlier branded, Phase 9, Phase 10, Phase 11, Phase 13, boot-logo-v2, Tech
  Frame, fixed-page, and original dynamic-pill images remain preserved behind
  the accepted connected-page-pill candidate.

- The exact cleaned Phase 5 candidate passed physical SuperCard SD validation
  and is now frozen as the functional baseline for the independent **SuperR7**
  fork, maintained by Danny Nunez (dnunezx).
- Frozen baseline: `releases/archive/2026-08-06-phase5-baseline/superr7-phase5-baseline.gba`,
  520,192 bytes, SHA-256
  `15A88B4F0F25B057ED4B93B4B0D855E7F3CFE67C0E7D0B7ADBA01261A6667A92`.
- SuperR7 branding and recognition are applied after that frozen checkpoint.
  Branded builds are validated separately and never overwrite the exact
  hardware-passed binary.
- First post-checkpoint SuperR7 build from source commit `3deb361`: 214,232-byte
  main binary, 104,768-byte compressed main payload, 220,752 of 257,024 EWRAM
  bytes, 11,176 of 32,768 IWRAM bytes, and a 518,144-byte final image with
  6,144 bytes free. Its SHA-256 is
  `72FCA4B89E329B9D2A4E21D5E4BB6C083E997A214C2BB0AE8373FCC0367AF61B`.
- The user chain-loaded that exact branded build and confirmed it passed the
  physical hardware test. It is now the accepted branded rollback baseline.

- Phase 0 baseline protection, Phase 1 native specification, the Phase 2
  emulator renderer, and Phase 3 Browse integration are complete. The user
  successfully chain-loaded the seven-row, native 76-by-76 version 3 build on
  physical hardware and confirmed that it boots and works correctly.
- The user has now also chain-loaded the complete Phase 4 candidate and
  confirmed that its dock, Recent integration, Appearance screen, presets,
  custom colors, and contrast modes work correctly on hardware. A problem was
  subsequently found in the Stars wallpaper: it drew only sparse pixels at the
  extreme screen edges instead of a recognizable star field.
- The user subsequently confirmed that the dock-contrast revision works
  wonderfully on hardware. Phase 4 is therefore hardware-accepted.
- A corrected Stars implementation was evaluated, but its limited visibility
  did not justify the firmware cost. By user decision, Stars has been removed
  from the renderer, settings list, emulator script, and visual checks.
- The completed UI work is being checkpointed locally as the start of
  `superr7/main`; publication to a new hosted repository remains separate.
- The inherited normal SD target still excludes the redesigned UI. The frozen,
  hardware-tested Phase 5 image remains the rollback source of truth.
- Version 3 is now the redesign's default candidate. It uses seven complete
  rows, native 76-by-76 artwork in a 78-by-78 frame, and a 16-pixel bottom
  dock. The version 2 path and its 72-by-72 files remain unchanged.
- Row icons and the selection arrow are removed. Icons appear only in the dock,
  whose visible order is Favorites, Recent, Browse, and Tools.
- The dock uses a dedicated 5-by-7 uppercase font so the full `FAVORITE` label
  fits its 60-pixel cell. Its icons are a star, clock, folder, and crossed
  tools, each verified as a distinct 8-by-8 native-resolution silhouette.
- Appearance provides user-facing presets, independent Background, Accent, and
  Selection controls, contrast handling, theme reset, and five procedural
  wallpaper choices: None, Weave, Grid, Circuit, and Tech Frame.
- The preset set is now ELECTRIC BLUE, MUTANT GREEN, STEALTH BLACK, and CHROME
  SILVER. ELECTRIC BLUE is the source default.
- Hardware feedback showed that Accent and Selection colors were being muted
  too heavily by background blending. Selection fill now uses a 50-percent
  color mix, while selection and accent shadows retain 75 percent of their
  chosen colors. Selected borders and active dock highlights use the full
  chosen colors.
- A second hardware contrast report found that unselected dock labels became
  difficult to read with Purple, Amber, Green, White, Slate, and Cyan
  backgrounds. The dock now has a dedicated text color derived from the actual
  darker dock surface rather than the general muted-text color. Auto chooses
  light or dark dock text for contrast, while the explicit Dark and Light
  overrides remain respected.
- The former Lime option is now Green, using a more natural vivid green rather
  than yellow-lime. MUTANT GREEN uses the updated color for both Accent and
  Selection.
- The local WSL Ubuntu environment now contains GNU Make, ARM GCC 13.2,
  Python/Pillow, and the scripted mGBA headless test runner.
- All 23 local Python checks, the theme host tests, and the complete Phase 4
  and Phase 5 mGBA visual suites pass.
- A separate `superfw-ui.gba` target now connects the seven-row renderer to the
  real SD Browse model without enabling it in the normal `superfw.gba` target.
- Phase 5 now applies the same card renderer to Interface, Global settings,
  System information, launch options, patch options, save and file actions,
  firmware update, confirmation dialogs, RTC editing, and alerts.
- The refined Phase 5 candidate replaces the crowded launch screen with a
  progressive Quick Launch flow. Its full-width title bar centers the filename,
  `Launch game` is the single primary action, and Options, Advanced, and Details
  separate the remaining information without changing the existing handlers.
- The loading screen now follows Background, Accent, Selection, wallpaper, and
  contrast choices. It adds a theme-colored progress bar, moving highlight,
  percentage, and clean status footer while retaining the unobstructed native
  76-by-76 cover.
- The cleaned refined candidate is 520,192 bytes with a 105,745-byte ratio-10
  compressed main payload, leaving 4,096 bytes below the 512 KiB limit. Its
  SHA-256 is
  `15A88B4F0F25B057ED4B93B4B0D855E7F3CFE67C0E7D0B7ADBA01261A6667A92`.
- Candidate-only dead UI baggage has been removed from the v3 SD build: the
  classic Browse/Recent renderers, old OBJ icon atlas and palette, selector
  sprite queue, and unused classic drawing helpers are no longer linked. They
  remain available behind the normal-firmware and NOR fallback guards.
- Phase 5 is complete and its refined launch and loading screens passed
  physical chain-load validation.

### Seven-row hardware readability checkpoint

- Native row bounds are `y=2..138`, using seven 17-pixel cards on a 20-pixel
  step. The final row remains fully above the dock and retains its selection
  treatment when the list window advances.
- The dock occupies `y=144..159`, leaving a three-pixel safety gap below the
  final card's shadow/glow.
- The accepted version 3 path uses strict native 76-by-76 `.sfcov` artwork and
  copies it with the established DMA path without runtime scaling.
- The automated native-frame capture verifies all seven rows, clipped and
  scrolling names, ready/pending/missing/invalid/folder covers, stable settled
  frames, dock highlights, and red/cyan palette-only selection variants.
- Hardware acceptance passed: the user successfully chain-loaded the version 3
  candidate and confirmed that the 76-by-76 cover path boots and works.

### Version 3 76-by-76 cover checkpoint

- Version 3 uses a strict 76-by-76 `.sfcov` header and 5,776-byte pixel
  payload. It does not reinterpret or overwrite version 2 artwork.
- The separate `superfw-ui-v3.gba` and `cover-demo-v3.gba` targets enable the
  redesign without changing the normal firmware or version 2 UI targets.
- The native layout places version 3 artwork at `x=4..79`, `y=33..108`, with
  its 78-by-78 frame at `x=3..80`, `y=32..109`.
- Converter round trips, version isolation, overwrite protection, firmware
  validation, and the complete scripted mGBA visual suite pass locally.
- See [the production cover format](../cover-format.md). Physical
  chain-load verification passed.

### Completed glow selection

- Cyan, lime, and ice-white pixel-glow mockups were generated in
  [glow comparison mockups](../ui-mockups/glow-comparison/).
- All three variants now use the C renderer's one-to-two-pixel bands and were
  captured from mGBA at 240 by 160 after GBA BGR555 conversion.
- The verifier checks exact converted palette values and proves that only the
  palette, not geometry or cover data, changes between variants.
- Cyan is selected because it preserves the clearest selected/unselected
  hierarchy without competing with the lime file badges or flattening the
  pale cards. Lime is too dominant and ice-white loses hierarchy.
- The demo starts on cyan; Select cycles cyan, lime, and ice-white. Production
  candidate code always uses cyan.

## Non-negotiable rules

- Target the SuperCard SD build first.
- Keep cover files, parsing, caching, and delayed SD reads unchanged during the
  initial renderer work.
- Keep ROM launch, save, patch, cheat, and file-manager behavior unchanged.
- Use original icons and decoration rather than artwork copied from another
  game's interface.
- Test UI changes in the standard-GBA mGBA demo before chain-loading them.
- Chain-load on hardware before considering another internal-flash update.
- Never exceed the 512 KiB SD firmware image limit.
- Five visible game rows is the minimum acceptance target.

## Proposed native layout

The current hardware prototype is:

| Region | Proposed bounds | Purpose |
| --- | --- | --- |
| Main content | `y=0..143` | Cover and seven-card game list |
| Cover frame | `x=3..80`, `y=32..109` | 78-by-78 selected frame around version 3 cover data |
| Cover artwork | `x=4..79`, `y=33..108` | Native 76-by-76 version 3 `.sfcov` artwork |
| Browse page pill | centered at `x=42`, `y=121..135`; width `31..74` | Dynamic `current/total` fraction with a connected theme-colored outline |
| Game list | `x=85..236`, cards at `y=2..138` | Seven stacked 17-pixel cards |
| Navigation dock | `y=144..159` | Compact Favorites, Recent, Browse, and Tools bar |

The selected filename may scroll horizontally. Unselected rows use a static
clipped name. Folder, parent-directory, unsupported-file, missing-cover, and
invalid-cover states remain visually distinct.

The first navigation dock maps existing functionality into four primary areas:

1. **Favorites** - persistent seven-row library with Quick Launch add/remove,
   normal launching, confirmed removal, and an approved empty state.
2. **Recent** - recently launched games.
3. **Browse** - ROM and file browser.
4. **Tools** - advanced tools, settings, and firmware information in Phase 4.

## Phase 0: Protect and record the baseline

Status: **Complete locally**

- Hardware-verified source commit: `90de4fb54255dcd7e42e08bbfe70f678915bfd29`.
- Local baseline tag: `cover-art-baseline-r7`.
- UI development branch: `feature/ui-browser`.
- Final SD firmware: 518,144 of 524,288 bytes.
- Remaining final-image space: 6,144 bytes.
- Main firmware binary: 219,380 bytes.
- Compressed main firmware: 105,129 bytes.
- Main firmware EWRAM allocation: 225,308 of 257,024 bytes (87.66%).
- Main firmware IWRAM allocation: 11,672 of 32,768 bytes (35.62%).
- Firmware SHA-256:
  `93EDC36DCF92D054834CE101DA735049EF3A48FA7794EF8FAC185189950FBA01`.
- R7 boots from internal flash, reads covers exclusively from the canonical
  organized folder, displays covers in Browse and Recent, launches games, and
  preserves save behavior on physical hardware.

The tag and branch are local until the standalone repository decision is
completed. The verified R7 firmware and build reports remain archived outside
the checkout under `phase6-hardware-test/R7_ARTIFACT`.

## Phase 1: Native-resolution specification

Status: **Complete**

- Rebuild the concept at exactly 240 by 160 pixels.
- Produce five-row and six-row variants at actual display size.
- View both variants without enlargement and select the most readable one.
- Fix exact cover, list, card, footer, icon, and text bounds.
- Allocate the background palette around the existing cover range.
- Define selected, unselected, folder, empty, missing, invalid, and disabled
  states.
- Define long-name movement and page-scroll behavior.

Acceptance criteria:

- At least five complete game rows are visible.
- Long titles remain identifiable.
- The cover remains square and unobstructed.
- The navigation dock is understandable without memorizing its icons.
- The design remains attractive at native GBA size, not only when enlarged.

Phase 1 result:

- Produced deterministic native 240-by-160 five-row and six-row variants.
- Selected six rows for the Phase 2 emulator prototype because the 19-pixel
  cards and bitmap text remain readable while increasing visible games.
- Reduced the navigation dock from 28 pixels to 20 pixels and placed each icon
  beside its label.
- Retained five 24-pixel rows as a compile-time fallback for physical-screen
  testing.
- Fixed cover, list, dock, row, icon, and text bounds.
- Defined a complete 20-color UI palette that preserves indices 20-239 for
  cover artwork.
- Defined ready, pending, folder, missing, invalid, SD-error, unsupported,
  selected, unselected, and disabled states.
- Documented filename clipping and existing selected-name scrolling behavior.
- See [the native Phase 1 specification](ui-phase-1-spec.md).

## Phase 2: Static mGBA renderer prototype

Status: **Complete locally**

- Add the new renderer behind an emulator/demo-only build flag.
- Use synthetic entries and existing valid, missing, and corrupt cover fixtures.
- Draw stripes, frames, and highlights procedurally where this saves space.
- Use small original sprite icons with their own constrained palette.
- Capture selected-row positions, both cover states, folders, and each dock item.
- Add visual assertions for bounds, stable frame swaps, and palette separation.

Acceptance criteria:

- The emulator screen matches the approved native specification.
- No new UI element draws over the cover or outside its assigned bounds.
- Six-row navigation works at every visible position.
- Production firmware behavior is unchanged because integration is not active.

Phase 2 result:

- Added a native Mode 4 renderer with six 19-pixel game cards, a square
  72-by-72 cover, procedural blue stripes, and a compact 20-pixel dock.
- Kept the renderer behind `UI_BROWSER_V2`, which was enabled only by the
  `cover-demo.gba` target.
- Kept background palette indices 0-19 separate from cover indices 20-239.
- Verified all six selected-row positions, ready, pending, missing, invalid,
  folder, and long-filename states in scripted mGBA captures.
- Verified Browse, Recent, Setup, and Tools dock highlights and Browse/Recent
  cover consistency.
- Passed all 19 local Python checks and the mGBA framebuffer verifier.
- Confirmed the normal SD build is byte-for-byte identical to hardware-tested
  R7: 518,144 bytes with SHA-256
  `93EDC36DCF92D054834CE101DA735049EF3A48FA7794EF8FAC185189950FBA01`.
- Reference captures are stored in
  [Phase 2 mockups](../ui-mockups/phase2/).

## Phase 3: Browse integration

Status: **Complete; version 3 chain-load validation passed**

- Connect the renderer to the real ROM browser without changing its file model.
- Replace the current eight-row presentation with a five-or-more-row card
  window.
- Preserve folders, parent navigation, file types, selection state, and long
  filenames.
- Reuse the current cover request, delay, validation, and EWRAM cache paths.
- Preserve every existing Browse button action.

Acceptance criteria:

- Large directories remain responsive during rapid scrolling.
- The list window advances correctly above and below every boundary.
- The selected cover follows the selected filename without stale frames.
- Existing popups, launching, and returning to Browse still work.

Phase 3 implementation result so far:

- Added a separate `superfw-ui.gba` SD target; `superfw.gba` does not link the
  new renderer.
- Connected the selected entry, six-entry window, cover cache state, cover
  pixels, hidden attribute, and animation counter to the existing browser
  model without changing SD reads, cache behavior, or launch actions.
- Changed Browse paging and boundary calculations from eight visible rows to
  six only when `UI_BROWSER_V2` is enabled.
- Added distinct game, folder, parent, save, firmware, generic-file, hidden,
  and unsupported visual treatments.
- Preserved delayed selected-name scrolling and clipped unselected names.
- Verified the sixth row, an eight-entry window shift, scrolling long name,
  cover states, and cyan/lime/ice palettes in scripted mGBA frames.
- Ratio-10 build measurements: 224,068-byte main binary, 108,775-byte
  compressed payload, 229,996 of 257,024 EWRAM bytes, 11,672 of 32,768 IWRAM
  bytes, and a 522,240-byte final candidate with 2,048 bytes free.
- The user successfully chain-loaded the seven-row version 3 candidate and
  confirmed that it boots and works with native 76-by-76 covers.

## Phase 4: Recent and navigation dock integration

Status: **Complete; hardware validation passed**

- Apply the same card list to Recent Games.
- Replace the top icon strip with the bottom navigation dock.
- Use L/R for primary-area switching.
- Route current UI/language settings through Settings.
- Route current Info through Tools.
- Preserve each area's last selection when switching.

Acceptance criteria:

- Browse and Recent show identical covers for the same ROM.
- Every existing top-level function remains reachable.
- The selected dock item and button hints are always unambiguous.

Phase 4 implementation result so far:

- Applied the same seven-card renderer and cover path to Recent Games.
- Replaced primary navigation with the compact four-item dock: Favorite,
  Recent, Browse, and Tools. L/R changes primary destinations, while each
  destination retains its own selection state.
- Routed Appearance, Interface & language, Global settings, and System
  information through Tools so the existing top-level functions remain
  reachable.
- Added an Appearance screen with the exact user-facing controls: Preset,
  Wallpaper, Background, Accent, Selection, Contrast, and Reset theme.
- Added ELECTRIC BLUE, MUTANT GREEN, STEALTH BLACK, and CHROME SILVER presets
  plus None, Weave, Grid, Circuit, and Tech Frame wallpapers. The selected
  values persist in settings.
- Background automatically derives the screen, cards, dock, stripes, edges,
  and shadows. Accent controls the active dock item and highlights. Selection
  controls the selected game's border, fill, and shadow.
- Increased visible color separation after hardware feedback: selected fills
  use a stronger 50-percent Selection mix, selected and accent shadows preserve
  75 percent of their chosen colors, game highlights use full Accent, and the
  bright Cyan, Blue, Purple, Red, Amber, and Green source colors are more
  saturated.
- Added a dedicated dock-text palette role without increasing the 20-color UI
  palette. Unselected dock labels now contrast against the derived dock shade;
  regression tests cover Purple, Amber, Green, White, Slate, and Cyan.
- Contrast defaults to Auto with Dark and Light overrides. Readable text and
  muted text are derived automatically; folder amber, error red, and disabled
  gray remain protected rather than exposed as user controls.
- Reset theme restores the current preset's full values.
- Favorites is a working dock destination backed by
  `/.superfw/favorites.txt`. It keeps the approved empty state, uses the same
  seven-row cover/list presentation as Browse and Recent, and supports normal
  launch and confirmed removal.
- Scripted mGBA checks cover all five wallpapers, all four presets, every
  independent color control, contrast overrides, reset behavior, seven-row
  layouts, all four dock selections, and Browse/Recent cover consistency.
- Ratio-10 measurements: 226,976-byte main binary, 110,196-byte compressed
  payload, 233,496 of 257,024 EWRAM bytes, 11,704 of 32,768 IWRAM bytes, and a
  523,264-byte final candidate with 1,024 bytes free.
- Candidate SHA-256:
  `597ABCB25DC95E7EBDC7FA4C999F32541A6B96483B4F16CD98CEEA00DE06C22E`.
- Physical result: the user chain-loaded this exact Phase 4 candidate and
  confirmed that its dock, theme settings, and wallpaper selection controls
  work. The Stars rendering defect was found afterward.
- Final Stars decision at that checkpoint: remove the option rather than spend
  additional firmware space on a wallpaper with little visible area. The later
  Tech Frame wallpaper now occupies saved wallpaper value 4.
- Vibrant candidate measurements: 226,824-byte main binary, 110,150-byte
  compressed payload, and a 523,264-byte final image with 1,024 bytes free.
- Vibrant candidate SHA-256:
  `C60A095B73F4F207C8F0516CA0C4DF3604783FD51A04982542B8D468E075573F`.
- The vibrant candidate passes the full native 240-by-160 mGBA sequence;
  hardware feedback identified the unselected dock-label contrast issue.
- Dock-contrast candidate measurements: 226,952-byte main binary, 110,213-byte
  compressed payload, and a 523,264-byte final image with 1,024 bytes free.
- Dock-contrast candidate SHA-256:
  `B24DDBCBCA78DD1764058BAFDD715EBF984C681056BB9D67DB703862BDD6E39D`.
- Host contrast tests and the complete native 240-by-160 mGBA sequence pass.
  The user confirmed the dock-contrast revision works correctly on hardware.
- Favorites result: persistent storage, add/remove actions, shared
  navigation, focused emulator checks, and physical SuperCard SD verification
  are complete.

## Phase 5: Secondary-screen visual consistency

Status: **Complete; hardware validation passed**

- Restyle launch options, confirmations, errors, settings, tools, and firmware
  information using the same cards, colors, and spacing.
- Keep each screen's underlying actions unchanged.
- Retain safe fallback rendering for low-memory or error states.

Acceptance criteria:

- Normal use does not unexpectedly mix old and new visual systems.
- Every confirmation and error remains readable and clearly actionable.

Phase 5 implementation result:

- Replaced the legacy Interface, Global settings, Tools information, launch,
  save, file-manager, firmware-update, confirmation, RTC, and alert layouts
  with the same full-width cards, theme palette, wallpaper, and compact dock.
- Removed the obsolete legacy Theme color row from Interface because the
  dedicated Appearance screen now owns preset and color selection.
- Global Settings section headings remain visible but are skipped by the
  selector. Saving Interface or Global settings now returns focus to the first
  actionable row rather than a heading or removed option.
- System Information is organized into four A-cycled card pages: SuperR7,
  flash information, patch database, and SD card.
- Game launch information, load/save policy, patch controls, DirectSave,
  in-game menu, RTC, cheats, and remember/build actions remain connected to
  their existing input handlers.
- The standard SD UI uses the new dialogs throughout normal use. Legacy popup
  rendering remains available as a compile-time fallback for NOR-capable
  builds that are outside this first SD-target redesign.
- Exact 240-by-160 scripted captures verify secondary screens plus launch,
  patch, save, file, firmware, confirmation No/Yes, RTC field, and alert
  states. The refined Quick Launch view uses a full-width centered title and a
  single centered `Launch game` action. Options, Advanced, and read-only Details
  pages progressively disclose the remaining controls and status information.
- The loading page is theme-aware and adds an Accent fill, Selection border and
  glow, moving bright pulse, and percentage without drawing over the cover.
  Scripted Electric Blue, Mutant Green, and Chrome Silver captures verify that
  each palette role changes independently and that B returns to Quick Launch.
- All four Phase 4/5 native visual assertion suites and all 23 Python checks
  pass.
- Final cleaned measurements: 214,176-byte main binary, 105,745-byte compressed
  main payload, 220,696 of 257,024 EWRAM bytes, 11,176 of 32,768 IWRAM bytes,
  and a 520,192-byte firmware image with 4,096 bytes free.
- Candidate SHA-256:
  `15A88B4F0F25B057ED4B93B4B0D855E7F3CFE67C0E7D0B7ADBA01261A6667A92`.
- Fresh mGBA captures from the cleaned build pass the complete Phase 4 theme,
  browser, and dock suite plus the Phase 5 secondary-screen, popup, refined
  launch, file-action, and themed-loading suites. Host theme tests also pass.
- Packaged chain-load candidate:
  `artifacts/phase5-v4-launch-loading-hardware/superfw-phase5-v4-76-launch-loading.gba`.
- Hardware result: the user chain-loaded this exact candidate and accepted the
  complete cleaned Phase 5 experience. It is preserved byte-for-byte in
  `releases/archive/2026-08-06-phase5-baseline/`.
- The first branded SuperR7 successor carries the `SUPERR7` GBA title and
  `3deb361a` Build ID. All Phase 4/5 native visual suites, all 23 Python checks,
  and the host theme test pass. Its separate release note is under
  `releases/archive/2026-08-06-initial-superr7/`; physical validation of the branded successor is
  recommended before it replaces the frozen baseline.

## Phase 6: Firmware-size and performance gate

- Compare the final image, compressed payload, EWRAM, and IWRAM with Phase 0.
- Target no more than 2 KiB of compressed final-image growth before optimization.
- Do not add another cover-sized cache or framebuffer.
- Reuse procedural shapes, shared icons, and existing font infrastructure.
- Run the SD build, host tests, converter tests, and mGBA visual tests.

Acceptance criteria:

- Final SD image remains below 524,288 bytes with useful repair headroom.
- Menu input and rapid scrolling remain responsive.
- Cover rendering and palette installation remain unchanged and stable.

## Phase 7: Physical SuperCard SD verification

Status: **Complete for the cleaned Phase 5 baseline; hardware validation passed**

- Chain-load the UI build first.
- Test directories smaller than and larger than the visible row count.
- Test every row position, rapid scrolling, long names, folders, and all dock
  destinations.
- Test missing and corrupt covers, popups, settings, tools, game launch, saves,
  Recent Games, and returning to the menu.
- Perform repeated cold boots and extended browsing.
- The cleaned Phase 5 baseline passed the full-system hardware gate. Any later
  branded or functional build repeats the regression gate before replacing it.

Acceptance criteria:

- The five-or-more-row layout is readable on the physical display.
- No corruption, hang, stale cover, launch regression, or save regression is
  observed.
- A verified rollback image remains available before flashing.

## Phase 8: In-game menu card integration and legacy-theme cleanup

Status: **Complete; hardware validation passed**

- Replaced the inherited logo, flat text rows, popup bands, and OBJ selection
  bar with full-width SuperR7 cards, selected-card glow, accent rails, a compact
  header, card dialogs, and dedicated RTC and savestate layouts.
- Aligned the menu text baselines with their card and field bounds, and removed
  the `A SELECT / B BACK` footer dock.
- The menu keeps the LCD forced blank while the first complete framebuffer is
  rendered and while the game framebuffer is restored, then reveals each menu
  page on VBlank to prevent a corrupted transition frame.
- The updated in-game-menu candidate passed physical hardware validation,
  including text alignment, footer removal, menu activation, and return to the
  game.
- Passed all 20 Appearance palette roles and the selected wallpaper into the
  in-game payload when a game is launched. Electric Blue, Mutant Green,
  Stealth Black, Chrome Silver, custom colors, and contrast overrides therefore
  use the same derived colors as the main interface.
- Added an emulator-only in-game menu ROM and scripted native captures for the
  main, reset, save, RTC, selected update, and confirmation-dialog states.
- Removed the obsolete `menu_theme` variable, `theme=` settings-file output,
  hidden Interface row, five unused alternative legacy palettes, English
  message key, and all fourteen localized `Theme color` entries only after the
  replacement renderer passed its dedicated visual suite.
- The exact hardware candidate was built from source checkpoint `e696196`
  (embedded build ID `e6961964`): 49,512-byte in-game payload, 214,236-byte
  main binary, 104,835-byte ratio-10
  compressed main payload, 220,756 of 257,024 EWRAM bytes, 11,176 of 32,768
  IWRAM bytes, and a 518,144-byte final image with 6,144 bytes free.
- Hardware image:
  `artifacts/phase8-ingame-cards-hardware/superr7-phase8-ingame-cards-e696196.gba`
  (`SHA-256 DA86376417C34FC002970A4B55406C4EEC736173DC5A1EC4D2C4DBF50CF1E8F8`).
- Hardware-validated follow-up image:
  `artifacts/phase10-ingame-menu-hardware/superr7-phase10-ingame-menu-bffca9f.gba`
  (`SHA-256 BFFCA9F38B40F8920FE38BAB47A705B452C4AB4D74A2A518E2C70AAD689F0A9F`).
- All four Phase 4/5 native visual suites, the new in-game visual suite, all 23
  Python checks, the host theme test, and the normal-firmware compatibility
  build pass.

Hardware acceptance criteria:

- The in-game menu opens reliably with the configured hotkey.
- Main, reset, save, savestate, RTC, cheat, and confirmation screens remain
  readable and responsive with the active Appearance preset.
- Resuming, saving, resetting, returning to firmware, RTC updates, savestates,
  and cheats retain their existing behavior.
- Returning to the game restores its display and input state without a new
  regression.

## Phase 9: Historical SuperR7 Gothic boot splash

Status: **Complete; hardware validation passed; superseded August 12, 2026**

This section records the archived Phase 9 candidate. Its Gothic wordmark and
four-stage bar are no longer the active boot splash; the current replacement
is documented in the August 12 boot-logo refresh below.

- Replaced the inherited 31-by-7 SuperFW mark with the approved monochrome
  `SuperR7` Gothic wordmark, ornamental swashes, pure-black background, and a
  four-stage grayscale progress bar.
- Preserved the approved source artwork at the time and credited Danny Nunez (dnunezx)
  directly beside the compact boot-logo data. The old source remains in Git
  history; `res/superr7-boot-logo-source.png` now contains the active stacked
  replacement artwork.
- Stored the wordmark as a 72-by-22 1bpp mask and rendered it at 2x scale so
  the extra detail does not consume the bootloader safety margin.
- Shared one renderer between the GBA and NDS paths. The first stage remains
  428 of 1,024 bytes; the complete bootloader is 2,968 of 3,072 bytes, leaving
  104 bytes free.
- Rebuilt the complete ratio-10 SuperR7 payload from checkpoint `1eec14f`
  (embedded build ID `1eec14fd`): 214,236-byte main binary, 104,786-byte
  compressed main payload, 220,756 of 257,024 EWRAM bytes, 11,176 of 32,768
  IWRAM bytes, and a 518,144-byte final image with 6,144 bytes free.
- Hardware image:
  `releases/archive/2026-08-07-gothic-boot/superr7-phase9-gothic-boot-1eec14f.gba`
  (`SHA-256 93F774D81C6DEF17125587A66B121A8CF126D87256F7F547389ACD482F49A1E5`).
- The real candidate was captured in headless mGBA; its 240-by-160 framebuffer
  contains the expected black background, white logo, four grayscale bar
  stages, and no unexpected colors. All Phase 4/5 visual suites, the in-game
  visual suite, all 23 Python checks, and the normal-firmware compatibility
  link also pass.

Historical hardware acceptance criteria:

- The full wordmark and ornamental curls remain readable on the physical GBA
  display during cold boot and chain-load boot.
- The progress bar shows four distinct grayscale stages on the black field.
- Booting continues normally on the SuperCard SD with no delay, corruption,
  or regression before the main menu appears.

## Phase 11: Persistent Favorites and shared list navigation

Status: **Complete; hardware validation passed**

- Added a persistent Favorites library at `/.superfw/favorites.txt`, limited
  to 200 ROM paths.
- Added the centered `Add to Favorites` Quick Launch action immediately below
  `Launch game`; it toggles to `Remove Favorite` for games already in the list.
- Kept the approved empty Favorites screen. A populated Favorites tab uses the
  same seven-row cards, covers, title scrolling, and launch flow as Browse.
- Unified Favorites, Browse, and Recent navigation through one list helper:
  Up/Down moves one item, Left/Right moves seven, lists of seven or fewer stay
  on one page, and longer lists retain a full final seven-row window.
- A opens the normal game launch flow. Select opens a confirmation before
  removing the selected favorite.
- Copies paths out of cartridge SDRAM before FatFs or launch operations and
  uses a stable removal callback, avoiding the unsafe path and delayed-callback
  behavior found in the earlier experimental implementation.
- Focused host and native mGBA checks cover empty, add, toggle, populated,
  confirmed removal, Quick Launch layout, and shared navigation behavior.
- Hardware-validated image:
  `artifacts/phase11-favorites-hardware/superr7-phase11-favorites-cbdae08b.gba`,
  520,704 bytes, SHA-256
  `CBDAE08B529566E37517776CA336B1FF070AEF4296ED09503E961577972C68B3`.
  It is byte-identical to the root image supplied for testing.
- The user confirmed that this exact candidate passed physical SuperCard SD
  hardware testing. It is now the accepted Phase 11 rollback source.

## Phase 13: Consistent Launch Back footer

Status: **Complete; hardware validation passed**

- Replaced `B: QUICK` with `B: BACK` on the Launch flow's Options, Advanced,
  and Details screens so they match the main Launch screen.
- All 23 host tests and `git diff --check` passed before packaging.
- Hardware-validated image:
  `artifacts/phase13-launch-back-test/superr7-phase13-launch-back-8ca8aaf2.gba`,
  519,168 bytes (5,120 bytes below the 512 KiB limit), SHA-256
  `8CA8AAF27941BAE9A1DF35D3C3E88C863CEF51B4AA5C35F1F25D780F0C808A2F`.
- The root and archived images are byte-identical. On August 11, 2026, the
  user confirmed that this exact image booted, worked correctly, and looked
  great on physical SuperCard SD hardware. It is now the accepted Phase 13
  rollback; Phase 11 remains preserved as the previous rollback.

## August 12, 2026: Stacked Super R7 boot-logo refresh

Status: **Complete; emulator and physical hardware validation passed**

- Replaced the Phase 9 Gothic wordmark with the supplied stacked `Super R7`
  artwork and removed the four-stage grayscale progress bar.
- Preserved the supplied 1376-by-768 source at
  `res/superr7-boot-logo-source.png`. The conversion crops the surrounding
  black field and unrelated lower-right sparkle before producing the compact
  boot mask.
- Stored the active logo as a 56-by-42 1bpp mask and rendered it at 2x, giving
  a centered 112-by-84 monochrome mark on the native 240-by-160 GBA screen.
- Kept the shared GBA/NDS renderer and reduced the boot palette to black and
  white. The first-stage loader remains 428 of 1,024 bytes; the complete
  bootloader is 3,000 of 3,072 bytes, leaving 72 bytes free.
- Validated the exact ROM in mGBA across 12 captured frames. Every framebuffer
  matched, used only palette indices 0 and 1 (`0x0000` and `0x7fff`), contained
  no progress-bar pixels, and placed the visible logo at `x=64..175` and
  `y=38..121`.
- Hardware-validated image: `superr7-boot-logo-v2.gba`, 519,168 bytes,
  SHA-256
  `6DCDEA075A8CF04C8A4FF523F20628D5FF74AFF7126E3234F59A7B9E93A26BFC`.
- On August 12, 2026, the user confirmed that this exact image booted, worked,
  and looked great on physical hardware. No additional firmware build was
  requested or produced after that confirmation.
- The exact tested binary is preserved in the workspace with the matching
  size and SHA-256. It became the current SuperR7 firmware on August 12;
  subsequent validated images preserve it as an earlier rollback.

## August 13, 2026: Dynamic Tech Frame wallpaper

Status: **Complete; emulator and physical hardware validation passed**

- Added Tech Frame as the fifth Appearance wallpaper alongside None, Weave,
  Grid, and Circuit. It is drawn procedurally at the native 240-by-160
  resolution and uses the active Background, Accent, and derived palette roles,
  so it follows every preset and custom color choice without storing a large
  bitmap.
- Passed the selected wallpaper and its dynamic palette through to the in-game
  menu, preserving the same-color appearance between the firmware and menu.
- Recreated Ribbons and Slashes as exact native-size concepts and evaluated
  compact 2bpp masks. Their visual result and firmware cost were not acceptable,
  so both options, their render paths, packed assets, temporary encodings, and
  related checks were removed completely. They do not appear in either menu and
  require no fallback behavior.
- After removal, the ratio-10 SD image is 520,192 bytes, leaving 4,096 bytes
  below the 512 KiB limit instead of 512 bytes. The theme host test, all 23
  Python checks, the complete Phase 4 native mGBA visual suite, and
  `git diff --check` pass.
- Hardware-validated root image: `superr7.gba`, SHA-256
  `63EE181F90C0FCACB0014D6F819B6994C26E2677A589C9DCC355718C9F1FAA4F`.
  On August 13, 2026, the user confirmed that this exact build works great on
  physical hardware. It became the current accepted SuperR7 firmware and is
  now the immediate rollback for the August 14 fixed-page navigation build.

## August 14, 2026: Fixed-page list navigation

Status: **Complete; host and physical hardware validation passed**

- Replaced inherited item-offset paging with fixed seven-item pages across
  Favorites, Recent, and Browse.
- Left and Right select the first item of the previous or next page. Paging at
  the beginning or end of a list does nothing, and the final page may contain
  fewer than seven items.
- Up and Down cross page boundaries cleanly. Horizontal page input takes
  priority over an accidental diagonal, and folder returns plus Recent or
  Favorites deletion realign the selection to a fixed page boundary.
- The focused navigation test passed with strict compiler warnings and with
  address and undefined-behavior sanitizers. The SuperR7 UI source passed ARM
  syntax checking, all 23 Python host tests passed, and `git diff --check`
  passed before packaging.
- Hardware-validated image: `superr7-page-navigation-hardware-test.gba`,
  520,192 bytes (4,096 bytes below the 512 KiB limit), SHA-256
  `DCD599CD17745FB350A7176C24257377AB9B301E6C5B661B25594E3D53E5C940`.
- On August 14, 2026, the user confirmed that this exact image works great on
  physical SuperCard SD hardware. It became the current accepted SuperR7
  firmware at that checkpoint and is preserved as a rollback for the later
  page-pill images.

## August 21, 2026: Dynamic Browse page pill and held paging

Status: **Complete; host, emulator, and physical hardware validation passed**

- Added a rounded page-position pill beneath the Browse cover, displaying only
  the dynamic current and total page fraction such as `1/2`.
- Sized the pill from its dock-scale 5-by-7 text, with a 31-pixel minimum and
  74-pixel maximum. Its outline uses the active Selection color and its fill
  uses the dock's deep background color.
- Added frame-based held Left/Right paging in Browse: a 400 ms initial delay,
  133 ms repeat interval before 1.3 seconds, and 67 ms thereafter. Releasing,
  reversing, conflicting directional input, leaving Browse, or opening a popup
  resets the repeat state.
- All 23 Python checks, the focused navigation test with strict warnings and
  sanitizers, the v3 cover-demo build, and native mGBA visual checks passed.
- Hardware-validated image: `superr7-dynamic-page-pill-hardware-test.gba`,
  520,704 bytes (3,584 bytes below the 512 KiB limit), SHA-256
  `0A64F0A7A528B5469B83EC25CA35795608D022ACC344E849E8B39ED9F565CCB2`.
- The user confirmed this exact image on physical SuperCard SD hardware. It
  became the current image at that checkpoint and is now the immediate rollback
  for the connected-outline follow-up.

## August 22, 2026: Connected page-pill outline

Status: **Complete; rendered and physical hardware validation passed**

- Closed the four visible raster gaps between the pill's horizontal caps and
  curved side pixels while preserving its dimensions, placement, palette
  roles, and dynamic page text.
- Added visual regression assertions for the upper and lower shoulder pixels.
- Rebuilt the native v3 emulator ROM. All 23 Python checks, the v3 cover-art
  visual suite, and the focused rendered pill-continuity check passed. The full
  Phase 4 replay reached and passed the pill assertion before a separate
  seven-row navigation-capture assertion remained outstanding.
- Hardware-validated image: `superr7-connected-page-pill-test.gba`, 520,704
  bytes (3,584 bytes below the 512 KiB limit), SHA-256
  `136C0726E4758460BBE1E7988F4647F8429946F839B0E556A472FC0230482C36`.
- Its GBA header checksum is valid. The user confirmed that this exact image
  works great on physical SuperCard SD hardware, making it the current accepted
  SuperR7 firmware at that checkpoint.

## August 22, 2026: Browse controls and clean startup handoff

Status: **Feature-complete; functional image passed physical hardware testing;
fingerprinted Luna 1.1 SD package pending exact-image confirmation**

- Added a Browse-only Start popup with A–Z/Z–A ordering; All Files, All Games,
  GBA Only, GB Only, and GBC Only modes; and independent folder and unknown-file
  visibility. A changes the selected value; L and R are not advertised or
  handled as popup controls. All four choices persist through
  `/.superfw/ui-settings.txt` and survive a reboot. Favorites, Recent, and
  Tools do not open this popup.
- Added confirmed Reset Favorites and Reset Recent actions to Tools. Each clears
  only its own persisted list and never deletes ROM files.
- Preserved the boot logo while Browse initializes on the hidden Mode 4 page,
  then installs the UI palette and flips to the completed page together on
  VBlank. This removes the visible white frame between the logo and main UI.
- Optimized the compact boot-logo expansion loop without changing any rendered
  logo pixels. The first captured logo frame has forced blank disabled.
- Focused sanitizer tests cover list resets, sort direction, filters, and the
  `Luna 1.1 SD` build fingerprint. The Browse-only mGBA flow and full ratio-10
  SD build pass.
- Hardware-confirmed functional image:
  `superr7-logo-to-ui-no-white-flash-hardware-test.gba`, 521,728 bytes (2,560
  bytes below the 512 KiB limit), SHA-256
  `E9AA581CB3D8401D3663646F1D72183B674C753CE0266C692A50BA056D853204`.
- The fingerprinted Luna 1.1 SD binary is a new candidate because changing the
  visible Build value changes the ROM. It must be chain-loaded once before the
  `Luna1.1` tag and GitHub Release are published.


Retained regression checklist:

- Add and remove a game through the centered Quick Launch action.
- Launch a game from Favorites and return to the firmware normally.
- Remove a favorite through the Favorites Select confirmation.
- Preserve additions, removals, and ordering across reboot.
- Keep Up/Down and Left/Right behavior coherent with more than seven favorites,
  including a partially filled final page.

## Definition of done

The redesign is complete when the approved original card interface is used by
Favorites, Browse, Recent, Settings, Tools, the in-game menu, and their normal
secondary screens; at least five game rows remain readable; the SD firmware
stays within its size and memory limits; and emulator plus physical-hardware
testing show no regression in cover art, navigation, launching, or saves.
