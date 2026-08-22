# Copyright (C) 2026 Danny Nunez (dnunezx)

from __future__ import annotations

from pathlib import Path
import os

from PIL import Image, ImageChops


SCREENSHOT_DIR = Path(os.environ.get("COVER_DEMO_SCREENSHOTS", "artifacts/cover-demo"))
PANEL_BOX = (1, 30, 82, 111)
PANEL_IMAGE = (6, 35, 78, 107)
DOCK_BOX = (0, 144, 240, 160)
FRAME_SIZE = 512 + 240 * 160
GLOW_VARIANTS = {
    "red": (0xFF2D45, 0xBF2030),
    "cyan": (0x00E5FF, 0x00A8B8),
}


def render_frame(path: Path) -> Image.Image:
    data = path.read_bytes()
    if len(data) != FRAME_SIZE:
        raise AssertionError(f"{path.name} is {len(data)} bytes, expected {FRAME_SIZE}")

    palette = []
    for offset in range(0, 512, 2):
        color = int.from_bytes(data[offset : offset + 2], "little")
        palette.append(
            (
                (color & 0x1F) * 255 // 31,
                ((color >> 5) & 0x1F) * 255 // 31,
                ((color >> 10) & 0x1F) * 255 // 31,
            )
        )

    image = Image.new("RGB", (240, 160))
    image.putdata([palette[index] for index in data[512:]])
    return image


def load(name: str) -> Image.Image:
    path = SCREENSHOT_DIR / f"{name}.png"
    frame_path = SCREENSHOT_DIR / f"{name}.frame"
    if frame_path.exists():
        image = render_frame(frame_path)
        image.save(path)
    elif path.exists():
        image = Image.open(path).convert("RGB")
    else:
        raise AssertionError(f"missing emulator frame dump: {frame_path}")
    if image.size != (240, 160):
        raise AssertionError(f"{path.name} is {image.size}, expected native 240x160")
    return image


def changed_pixels(first: Image.Image, second: Image.Image) -> int:
    difference = ImageChops.difference(first, second)
    return sum(pixel != (0, 0, 0) for pixel in difference.getdata())


def gba_color(color: int) -> tuple[int, int, int]:
    return tuple(((color >> shift) & 0xFF) // 8 * 255 // 31 for shift in (16, 8, 0))


def gba_bgr555(color: int) -> int:
    return ((color >> 19) & 0x1F) | ((color >> 6) & 0x3E0) | ((color << 7) & 0x7C00)


def assert_glow_variants() -> None:
    images = []
    framebuffers = []
    for name, (edge, shadow) in GLOW_VARIANTS.items():
        image = load(f"glow-{name}")
        data = (SCREENSHOT_DIR / f"glow-{name}.frame").read_bytes()
        palette_edge = int.from_bytes(data[18:20], "little")
        palette_shadow = int.from_bytes(data[20:22], "little")
        if palette_edge != gba_bgr555(edge) or palette_shadow != gba_bgr555(shadow):
            raise AssertionError(f"{name} glow did not use the requested GBA colors")
        if image.getpixel((90, 2)) != gba_color(edge):
            raise AssertionError(f"{name} selected-card edge has the wrong color")
        if image.getpixel((83, 0)) != gba_color(shadow):
            raise AssertionError(f"{name} outer glow band has the wrong color")
        if image.getpixel((123, 145)) != gba_color(edge):
            raise AssertionError(f"{name} dock highlight has the wrong color")
        images.append(image)
        framebuffers.append(data[512:])

    if len(set(framebuffers)) != 1:
        raise AssertionError("glow variants changed geometry instead of palette only")
    if len({image.getpixel((90, 2)) for image in images}) != len(images):
        raise AssertionError("glow variants are not visually distinct after BGR555 conversion")


def assert_page_pill() -> None:
    data = (SCREENSHOT_DIR / "browse-aurora-ready.frame").read_bytes()[512:]

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
    white = pixel(35, 125)
    if white != pixel(145, 148):
        raise AssertionError("page number does not use the dock text color")
    for y in range(125, 132):
        if not any(pixel(x, y) == white for x in range(34, 51)):
            raise AssertionError("page number does not use the dock's 5x7 scale")
    if any(pixel(x, 132) == white for x in range(34, 51)):
        raise AssertionError("page number exceeds the dock's seven-pixel height")


def assert_outside_panel_equal(first: Image.Image, second: Image.Image) -> None:
    regions = (
        (0, 0, 240, PANEL_BOX[1]),
        (0, PANEL_BOX[3], 240, 160),
        (0, PANEL_BOX[1], PANEL_BOX[0], PANEL_BOX[3]),
        (PANEL_BOX[2], PANEL_BOX[1], 240, PANEL_BOX[3]),
    )
    for region in regions:
        if ImageChops.difference(first.crop(region), second.crop(region)).getbbox():
            raise AssertionError(f"framebuffer changed outside cover panel at {region}")


def assert_seven_row_layout(image: Image.Image) -> None:
    row_centers = (10, 30, 50, 70, 90, 110, 130)
    selected_fill = image.getpixel((89, row_centers[0]))
    normal_fill = image.getpixel((89, row_centers[1]))
    if selected_fill == normal_fill:
        raise AssertionError("selected card is not visually distinct")
    for row, y in enumerate(row_centers[1:], start=2):
        if image.getpixel((89, y)) != normal_fill:
            raise AssertionError(f"row {row} does not use the normal card fill")

    for y in (21, 41, 61, 81, 101, 121, 141):
        if image.getpixel((89, y)) == normal_fill:
            raise AssertionError(f"row gap at y={y} was filled by a card")

    dock = image.crop(DOCK_BOX)
    if dock.size != (240, 16):
        raise AssertionError(f"navigation dock is {dock.size}, expected 240x16")
    if image.getpixel((123, 145)) == image.getpixel((63, 145)):
        raise AssertionError("active and inactive dock cells are indistinguishable")


def assert_dock_selection(images: tuple[tuple[int, Image.Image], ...]) -> None:
    samples = (3, 63, 123, 183)
    active = images[0][1].getpixel((samples[images[0][0]], 145))
    inactive = images[0][1].getpixel((samples[0], 145))
    if active == inactive:
        raise AssertionError("dock active color matches inactive color")
    for selected, image in images:
        for index, x in enumerate(samples):
            expected = active if index == selected else inactive
            if image.getpixel((x, 145)) != expected:
                raise AssertionError(
                    f"dock item {selected + 1} does not have the expected highlight"
                )


def assert_dock_icons_and_favorite_label(image: Image.Image) -> None:
    origins = (1, 67, 127, 190)
    muted = image.getpixel((11, 148))
    active = image.getpixel((137, 148))
    colors = (muted, muted, active, muted)
    masks = []
    for origin, color in zip(origins, colors):
        mask = tuple(
            image.getpixel((origin + x, 148 + y)) == color
            for y in range(8)
            for x in range(8)
        )
        if sum(mask) < 12:
            raise AssertionError(f"dock icon at x={origin} is too sparse")
        masks.append(mask)
    if len(set(masks)) != 4:
        raise AssertionError("dock icons are not visually distinct")

    star_rows = (0x18, 0x18, 0xFF, 0x7E, 0x3C, 0x7E, 0x66, 0x00)
    expected_star = tuple(
        bool(star_rows[y] & (0x80 >> x))
        for y in range(8)
        for x in range(8)
    )
    if masks[0] != expected_star:
        raise AssertionError("Favorites dock icon is not the approved star")

    # The final E in FAVORITE occupies x=53..57 in the compact dock font.
    favorite_tail = image.crop((53, 148, 58, 155))
    if not any(pixel == muted for pixel in favorite_tail.getdata()):
        raise AssertionError("full FAVORITE label did not fit in its dock cell")


def main() -> None:
    assert_glow_variants()
    assert_page_pill()
    aurora = load("browse-aurora-ready")
    pending = load("browse-checker-pending")
    checker = load("browse-checker-ready")
    stable = load("browse-checker-stable")
    missing = load("browse-missing")
    invalid = load("browse-invalid")
    folder = load("browse-folder")
    long_name_start = load("browse-long-name-start")
    long_name_scrolled = load("browse-long-name-scrolled")
    last_row = load("browse-last-row")
    shifted = load("browse-window-shift")
    recent = load("recent-aurora-ready")
    tools = load("dock-tools")

    assert_seven_row_layout(aurora)

    aurora_panel = aurora.crop(PANEL_IMAGE)
    checker_panel = checker.crop(PANEL_IMAGE)
    if len(aurora_panel.getcolors(maxcolors=4096) or ()) < 6:
        raise AssertionError("Aurora cover did not render its expected color range")
    if len(checker_panel.getcolors(maxcolors=4096) or ()) < 7:
        raise AssertionError("Checker cover did not render its expected color range")
    if changed_pixels(aurora_panel, checker_panel) < 2500:
        raise AssertionError("navigation did not replace the first cover with the second")

    assert_outside_panel_equal(pending, checker)
    if ImageChops.difference(checker, stable).getbbox():
        raise AssertionError("two settled frames differ; possible partial update or flicker")

    missing_panel = missing.crop(PANEL_IMAGE)
    invalid_panel = invalid.crop(PANEL_IMAGE)
    if len(missing_panel.getcolors(maxcolors=32) or ()) > 3:
        raise AssertionError("missing-cover placeholder contains unexpected palette colors")
    if len(invalid_panel.getcolors(maxcolors=32) or ()) > 3:
        raise AssertionError("invalid-cover placeholder contains unexpected palette colors")
    if changed_pixels(missing_panel, invalid_panel) < 10:
        raise AssertionError("missing and corrupt covers did not produce distinct placeholders")

    folder_panel = folder.crop(PANEL_IMAGE)
    if len(folder_panel.getcolors(maxcolors=32) or ()) < 3:
        raise AssertionError("folder cover panel lacks its distinct illustration")

    selected_fill = aurora.getpixel((89, 10))
    if last_row.getpixel((89, 130)) != selected_fill:
        raise AssertionError("seventh visible row did not receive the selection treatment")
    if changed_pixels(long_name_start.crop((91, 102, 232, 119)),
                      long_name_scrolled.crop((91, 102, 232, 119))) < 20:
        raise AssertionError("selected long filename did not scroll after its delay")
    if shifted.getpixel((89, 10)) != selected_fill:
        raise AssertionError("selection did not start on the first row of the next fixed page")
    if changed_pixels(last_row.crop((83, 0, 240, 144)),
                      shifted.crop((83, 0, 240, 144))) < 100:
        raise AssertionError("eight-entry navigation did not advance the seven-row window")

    if ImageChops.difference(aurora_panel, recent.crop(PANEL_IMAGE)).getbbox():
        raise AssertionError("Recent Games did not render the same cached-format Aurora cover")

    assert_dock_selection(((2, aurora), (1, recent), (3, tools)))
    assert_dock_icons_and_favorite_label(aurora)

    print("mGBA cover-art visual checks passed")


if __name__ == "__main__":
    main()
