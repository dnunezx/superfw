# SuperR7 tools

## User-facing tools

- `cover_converter.py`: production 76-by-76 SuperR7 cover converter.
- `cover_converter_v2.py`: legacy 72-by-72 SuperFW compatibility converter.
- `update_translations.py`: download a Crowdin translation bundle.

## Build and test tools

- `finalize_gba_image.py`: pad a GBA image and finalize its header, size, and
  firmware hash fields.
- `verify_firmware_image.py`: reject oversized, corrupt, or wrong-board
  release images and print their exact SHA-256.
- `patch_ingame_menu_test_rom.py`: inject the in-game-menu payload into a test
  ROM that already contains the required IRQ patch.
- `mgba-*.lua`: scripted native-resolution emulator captures and regression
  fixtures. Phase or `v3` suffixes on these internal scripts identify the
  historical UI checkpoint or cover-format version they verify.

The Makefile and automated tests are the authoritative examples for internal
tool invocation.
