#!/usr/bin/env python3
"""Validate a finalized SuperR7/SuperFW firmware image."""

import argparse
import hashlib
import struct
from pathlib import Path


HEADER_CHECKSUM_OFFSET = 0xBD
FW_SIZE_OFFSET = 0xCC
HW_VARIANT_OFFSET = 0xD0
FW_DIGEST_OFFSET = 0xE0
FW_DIGEST_SIZE = 16
FW_MAGIC_OFFSET = 0xF0
FW_MAGIC = b"SUPERFW~DAVIDGF\0"


def verify_image(path: Path, max_bytes: int, flavour: str) -> tuple[int, str]:
    image = path.read_bytes()
    errors: list[str] = []

    minimum_size = FW_MAGIC_OFFSET + len(FW_MAGIC)
    if len(image) < minimum_size:
        raise ValueError(
            f"image is {len(image)} bytes; firmware header requires {minimum_size}"
        )

    if len(image) > max_bytes:
        errors.append(f"image is {len(image)} bytes; limit is {max_bytes}")
    if len(image) % 512:
        errors.append("image is not padded to a 512-byte boundary")

    expected_header = (-(0x19 + sum(image[0xA0:0xBD]))) & 0xFF
    if image[HEADER_CHECKSUM_OFFSET] != expected_header:
        errors.append("GBA header checksum is invalid")

    embedded_size = struct.unpack_from("<I", image, FW_SIZE_OFFSET)[0]
    if embedded_size != len(image):
        errors.append(
            f"embedded size is {embedded_size}; actual size is {len(image)}"
        )

    embedded_flavour = image[HW_VARIANT_OFFSET : HW_VARIANT_OFFSET + 4]
    expected_flavour = flavour.encode("ascii").ljust(4, b"\0")
    if embedded_flavour != expected_flavour:
        errors.append(
            f"embedded flavour is {embedded_flavour!r}; expected {expected_flavour!r}"
        )

    if image[FW_MAGIC_OFFSET : FW_MAGIC_OFFSET + len(FW_MAGIC)] != FW_MAGIC:
        errors.append("SuperFW-compatible firmware signature is missing")

    stored_digest = image[FW_DIGEST_OFFSET : FW_DIGEST_OFFSET + FW_DIGEST_SIZE]
    digest_input = bytearray(image)
    digest_input[FW_DIGEST_OFFSET : FW_DIGEST_OFFSET + FW_DIGEST_SIZE] = bytes(
        FW_DIGEST_SIZE
    )
    computed_digest = hashlib.sha256(digest_input).digest()[:FW_DIGEST_SIZE]
    if stored_digest != computed_digest:
        errors.append("embedded firmware digest is invalid")

    if errors:
        raise ValueError("; ".join(errors))

    return len(image), hashlib.sha256(image).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("--max-bytes", type=int, required=True)
    parser.add_argument("--flavour", choices=("SD", "Lite", "Chis"), required=True)
    args = parser.parse_args()

    try:
        size, digest = verify_image(args.image, args.max_bytes, args.flavour)
    except (OSError, ValueError, IndexError, struct.error) as exc:
        raise SystemExit(f"firmware verification failed: {exc}") from exc

    print(f"verified {args.image}: {size} bytes, sha256={digest}")


if __name__ == "__main__":
    main()
