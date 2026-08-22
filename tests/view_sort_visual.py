# Copyright (C) 2026 Danny Nunez (dnunezx)

from __future__ import annotations

from pathlib import Path

import cover_demo_visual as visual


CAPTURES = Path("artifacts/view-sort-v3")
FRAME_SIZE = 512 + 240 * 160
POPUP_MODES = (
    "popup-all-files",
    "popup-all-games",
    "popup-gba-only",
    "popup-gb-only",
    "popup-gbc-only",
)


def frame(name: str) -> bytes:
    data = (CAPTURES / f"{name}.frame").read_bytes()
    if len(data) != FRAME_SIZE:
        raise AssertionError(f"{name} frame has {len(data)} bytes")
    return data


def pixel(data: bytes, x: int, y: int) -> int:
    return data[512 + y * 240 + x]


def main() -> None:
    default = frame("popup-all-files")
    reversed_order = frame("popup-z-a")
    if default[512:] == reversed_order[512:]:
        raise AssertionError("A-Z and Z-A popup states are identical")

    modes = [frame(name)[512:] for name in POPUP_MODES]
    if len(set(modes)) != len(modes):
        raise AssertionError("game-type selector did not render five distinct modes")

    configured = frame("popup-gba-hidden")
    if pixel(configured, 6, 100) != 9:
        raise AssertionError("Unknown files is not the selected popup row")
    if configured[512:] != frame("popup-after-left")[512:]:
        raise AssertionError("L changed a View & Sort value; only A should change it")
    if configured[512:] != frame("popup-after-right")[512:]:
        raise AssertionError("R changed a View & Sort value; only A should change it")

    filtered = frame("browse-gba-z-a")
    selected = pixel(filtered, 89, 10)
    if selected != 6:
        raise AssertionError("the filtered descending list lost its selection")
    for y in (30, 50, 70, 90):
        if pixel(filtered, 89, y) != 5:
            raise AssertionError("filtered GBA row layout is incomplete")
    if pixel(filtered, 89, 110) in (5, 6):
        raise AssertionError("non-GBA or hidden entries remained in the list")

    if frame("recent-before-start")[512:] != frame("recent-after-start")[512:]:
        raise AssertionError("Start opened View & Sort outside Browse")

    for name in (*POPUP_MODES, "popup-z-a", "popup-gba-hidden",
                 "popup-after-left",
                 "popup-after-right",
                 "browse-gba-z-a", "recent-before-start",
                 "recent-after-start"):
        visual.render_frame(CAPTURES / f"{name}.frame").save(
            CAPTURES / f"{name}.png"
        )

    print("SuperR7 View & Sort visual checks passed")


if __name__ == "__main__":
    main()
