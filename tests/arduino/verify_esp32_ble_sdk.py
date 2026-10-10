"""Read-only pin for the official S3 BLE SDK used by the release matrix."""
import argparse
import hashlib
from pathlib import Path

# Official Arduino-ESP32 3.3.11 / IDF 5.5.5. This does not install, patch or
# replace a user's SDK. The CLI verifies release archive checksums on install.
PINNED_FILES = {
    'packages/esp32/hardware/esp32/3.3.11/platform.txt':
        '82d29b4d98b14b0db490bf351c1772849c57b48c1c44f84fb635cd68afaf20f5',
    'packages/esp32/tools/esp32s3-libs/3.3.11/sdkconfig':
        '534f3457aaf3ffbc82c45dc21d6b3ac57388c82d0d9a24bd2b8e6cadc3e48d9d',
    'packages/esp32/tools/esp32s3-libs/3.3.11/lib/libbt.a':
        'ea1169a7357639aab5810fb6a86451d898624b59274abd8166ddf3c0ad6cff0f',
}


def verify_sdk(data_dir: Path) -> None:
    for relative, expected in PINNED_FILES.items():
        path = data_dir / relative
        if not path.is_file():
            raise ValueError(f'official ESP32S3 BLE SDK input missing: {relative}')
        with path.open('rb') as source:
            actual = hashlib.file_digest(source, 'sha256').hexdigest()
        if actual != expected:
            raise ValueError(f'official ESP32S3 BLE SDK input changed: {relative}')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-dir', type=Path, required=True)
    args = parser.parse_args()
    verify_sdk(args.data_dir)
    print('Official ESP32S3 3.3.11 NimBLE SDK pin passed (IPC 2048 B, Host 5120 B)')


if __name__ == '__main__':
    main()
