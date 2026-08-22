# Copyright (C) 2026 Danny Nunez (dnunezx)

from __future__ import annotations

from pathlib import Path

from PIL import ImageChops

import cover_demo_visual as visual


visual.SCREENSHOT_DIR = Path("artifacts/phase4-v3")
visual.PANEL_BOX = (1, 30, 83, 112)
visual.PANEL_IMAGE = (4, 33, 80, 109)


def frame(name: str) -> bytes:
    return (visual.SCREENSHOT_DIR / f"{name}.frame").read_bytes()


def palette(name: str, index: int) -> int:
    data = frame(name)
    return int.from_bytes(data[index * 2 : index * 2 + 2], "little")


def assert_palette(name: str, index: int, rgb: int) -> None:
    actual = palette(name, index)
    expected = visual.gba_bgr555(rgb)
    if actual != expected:
        raise AssertionError(
            f"{name} palette {index} is {actual:#06x}, expected {expected:#06x}"
        )


def palette_luminance(name: str, index: int) -> int:
    color = palette(name, index)
    return (color & 31) * 3 + ((color >> 5) & 31) * 6 + ((color >> 10) & 31)


def assert_seven_rows(name: str, selected_row: int = 0, x: int = 89) -> None:
    data = frame(name)[512:]

    def pixel(x: int, y: int) -> int:
        return data[y * 240 + x]

    for row, y in enumerate((10, 30, 50, 70, 90, 110, 130)):
        expected = 6 if row == selected_row else 5
        if pixel(x, y) != expected:
            raise AssertionError(
                f"{name} row {row + 1} uses palette {pixel(x, y)}, expected {expected}"
            )
    for y in (21, 41, 61, 81, 101, 121, 141):
        if pixel(x, y) in (5, 6):
            raise AssertionError(f"{name} row gap at y={y} was filled by a card")


def assert_browse_page_pill() -> None:
    data = frame("browse-aurora-ready")[512:]

    def pixel(x: int, y: int) -> int:
        return data[y * 240 + x]

    if pixel(42, 121) != pixel(150, 145):
        raise AssertionError("page pill edge does not match the active dock edge")
    if pixel(30, 125) != pixel(30, 146):
        raise AssertionError("page pill fill does not match the dock background")
    if pixel(27, 121) == pixel(42, 121) or pixel(57, 121) == pixel(42, 121):
        raise AssertionError("page pill top edge is not rounded")
    if pixel(27, 125) != pixel(42, 121) or pixel(57, 125) != pixel(42, 121):
        raise AssertionError("page pill did not size itself to the page number")
    for y in (122, 134):
        for x in (31, 32, 52, 53):
            if pixel(x, y) != pixel(42, 121):
                raise AssertionError("page pill outline is disconnected at a shoulder")


def assert_wallpapers() -> None:
    names = (
        "wallpaper-none",
        "wallpaper-weave",
        "wallpaper-grid",
        "wallpaper-circuit",
        "wallpaper-tech-frame",
    )
    buffers = [frame(name)[512:] for name in names]
    if len(set(buffers)) != len(names):
        raise AssertionError("wallpaper modes did not produce five distinct frames")

    background = 2
    none = buffers[0]
    background_pixels = [i for i, value in enumerate(none) if value == background]
    for name, candidate in zip(names[1:], buffers[1:]):
        changed = sum(candidate[i] != background for i in background_pixels)
        if changed < 8:
            raise AssertionError(f"{name} lacks a distinct procedural pattern")

    dynamic_roles = {2, 3, 4, 8, 12}
    for name, candidate in zip(names[4:], buffers[4:]):
        pattern_roles = {
            candidate[i] for i in background_pixels if candidate[i] != background
        }
        if not pattern_roles or not pattern_roles <= dynamic_roles:
            raise AssertionError(
                f"{name} uses non-dynamic wallpaper colors: {sorted(pattern_roles)}"
            )

def assert_presets_and_controls() -> None:
    # Dynamic UI palette: text=0, background=2, selection=9, accent=11.
    assert_palette("appearance-electric-blue-grid", 2, 0x071424)
    assert_palette("appearance-electric-blue-grid", 9, 0x00E5FF)
    assert_palette("appearance-electric-blue-grid", 11, 0x3B82FF)
    assert_palette("preset-mutant-green", 2, 0x171A21)
    assert_palette("preset-mutant-green", 9, 0x39FF70)
    assert_palette("preset-mutant-green", 11, 0x39FF70)
    assert_palette("preset-stealth-black", 2, 0x08090D)
    assert_palette("preset-stealth-black", 9, 0xD7DCE5)
    assert_palette("preset-stealth-black", 11, 0x171A21)
    assert_palette("preset-chrome-silver", 2, 0xD7DCE5)
    assert_palette("preset-chrome-silver", 9, 0xFFFFFF)
    assert_palette("preset-chrome-silver", 11, 0x171A21)

    if palette("custom-background", 2) == palette("preset-mutant-green", 2):
        raise AssertionError("Background control did not change the screen base")
    if palette("custom-accent", 11) == palette("custom-background", 11):
        raise AssertionError("Accent control did not change general highlights")
    if palette("custom-selection", 9) == palette("custom-accent", 9):
        raise AssertionError("Selection control did not change the selected border")
    if palette("contrast-dark", 0) == palette("contrast-light", 0):
        raise AssertionError("Dark and Light contrast overrides use the same text")

    custom = frame("custom-background")[512:]
    if custom[148 * 240 + 11] != 13:
        raise AssertionError("unselected dock label does not use dock-aware text")
    dock_difference = abs(
        palette_luminance("custom-background", 13) -
        palette_luminance("custom-background", 1)
    )
    if dock_difference < 120:
        raise AssertionError("unselected dock text lacks contrast on Slate")

    # Folders and errors stay safety colors regardless of the preset.
    for name in (
        "appearance-electric-blue-grid",
        "preset-mutant-green",
        "preset-stealth-black",
        "preset-chrome-silver",
    ):
        assert_palette(name, 16, 0xFFD45A)
        assert_palette(name, 17, 0xFF4655)

    if frame("theme-reset")[:40] != frame("preset-mutant-green")[:40]:
        raise AssertionError("Reset theme did not restore the selected preset palette")


def main() -> None:
    aurora = visual.load("browse-aurora-ready")
    pending = visual.load("browse-checker-pending")
    checker = visual.load("browse-checker-ready")
    stable = visual.load("browse-checker-stable")
    missing = visual.load("browse-missing")
    invalid = visual.load("browse-invalid")
    folder = visual.load("browse-folder")
    long_start = visual.load("browse-long-name-start")
    long_scrolled = visual.load("browse-long-name-scrolled")
    last_row = visual.load("browse-last-row")
    shifted = visual.load("browse-window-shift")
    recent = visual.load("recent-aurora-ready")
    favorite = visual.load("favorites-empty")
    tools = visual.load("tools-home")
    appearance = visual.load("appearance-electric-blue-grid")

    assert_seven_rows("browse-aurora-ready")
    assert_browse_page_pill()
    assert_seven_rows("tools-home", x=6)
    assert_seven_rows("appearance-electric-blue-grid", x=6)
    visual.assert_dock_selection(
        ((2, aurora), (0, favorite), (1, recent), (3, tools))
    )
    visual.assert_dock_icons_and_favorite_label(aurora)

    aurora_panel = aurora.crop(visual.PANEL_IMAGE)
    checker_panel = checker.crop(visual.PANEL_IMAGE)
    if visual.changed_pixels(aurora_panel, checker_panel) < 2500:
        raise AssertionError("Browse navigation did not replace the cover")
    visual.assert_outside_panel_equal(pending, checker)
    if ImageChops.difference(checker, stable).getbbox():
        raise AssertionError("settled cover frames differ")
    if visual.changed_pixels(
        missing.crop(visual.PANEL_IMAGE), invalid.crop(visual.PANEL_IMAGE)
    ) < 10:
        raise AssertionError("missing and invalid covers are not distinct")
    if len(folder.crop(visual.PANEL_IMAGE).getcolors(maxcolors=32) or ()) < 3:
        raise AssertionError("folder placeholder lost its fixed amber treatment")
    if ImageChops.difference(aurora_panel, recent.crop(visual.PANEL_IMAGE)).getbbox():
        raise AssertionError("Browse and Recent do not show the same cover")

    selected_fill = aurora.getpixel((89, 10))
    if last_row.getpixel((89, 130)) != selected_fill:
        raise AssertionError("seventh row is not selectable")
    if visual.changed_pixels(
        long_start.crop((91, 102, 232, 119)),
        long_scrolled.crop((91, 102, 232, 119)),
    ) < 20:
        raise AssertionError("long selected filename did not scroll")
    if shifted.getpixel((89, 130)) != selected_fill:
        raise AssertionError("seven-row window did not advance correctly")

    assert_wallpapers()
    assert_presets_and_controls()
    print("Phase 4 mGBA visual checks passed")


if __name__ == "__main__":
    main()
