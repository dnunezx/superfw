import hashlib
import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "tools" / "verify_firmware_image.py"
SPEC = importlib.util.spec_from_file_location("verify_firmware_image", MODULE_PATH)
verifier = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(verifier)


def make_image(flavour="Chis"):
    image = bytearray(512)
    struct.pack_into("<I", image, verifier.FW_SIZE_OFFSET, len(image))
    image[verifier.HW_VARIANT_OFFSET : verifier.HW_VARIANT_OFFSET + 4] = (
        flavour.encode("ascii").ljust(4, b"\0")
    )
    image[
        verifier.FW_MAGIC_OFFSET : verifier.FW_MAGIC_OFFSET + len(verifier.FW_MAGIC)
    ] = verifier.FW_MAGIC
    image[verifier.HEADER_CHECKSUM_OFFSET] = (
        -(0x19 + sum(image[0xA0:0xBD]))
    ) & 0xFF
    digest = hashlib.sha256(image).digest()[: verifier.FW_DIGEST_SIZE]
    image[
        verifier.FW_DIGEST_OFFSET : verifier.FW_DIGEST_OFFSET
        + verifier.FW_DIGEST_SIZE
    ] = digest
    return image


class FirmwareImageTest(unittest.TestCase):
    def verify(self, image, flavour="Chis", max_bytes=2 * 1024 * 1024):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "firmware.gba"
            path.write_bytes(image)
            return verifier.verify_image(path, max_bytes, flavour)

    def test_valid_image(self):
        image = make_image()
        size, digest = self.verify(image)
        self.assertEqual(512, size)
        self.assertEqual(hashlib.sha256(image).hexdigest(), digest)

    def test_rejects_wrong_board(self):
        with self.assertRaisesRegex(ValueError, "embedded flavour"):
            self.verify(make_image(), flavour="SD")

    def test_rejects_corruption(self):
        image = make_image()
        image[300] ^= 1
        with self.assertRaisesRegex(ValueError, "embedded firmware digest"):
            self.verify(image)

    def test_rejects_oversized_image(self):
        with self.assertRaisesRegex(ValueError, "limit is 511"):
            self.verify(make_image(), max_bytes=511)

    def test_rejects_truncated_image(self):
        with self.assertRaisesRegex(ValueError, "firmware header requires"):
            self.verify(bytes(128))


if __name__ == "__main__":
    unittest.main()
