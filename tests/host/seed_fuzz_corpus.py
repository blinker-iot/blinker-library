#!/usr/bin/env python3
"""Add stable protocol-aware seeds to a persistent libFuzzer corpus."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


SEEDS = (
    bytes((
        0x00, 0x42, 0x4B, 0x02, 0x11, 0x00,
        0x0A, 0x00, 0x01, 0x00, 0x01, 0xA0,
    )),
    bytes((
        0x01, 0xA2, 0x01, 0x65,
        ord("h"), ord("e"), ord("l"), ord("l"), ord("o"),
        0x02, 0x82, 0xF5, 0xF6,
    )),
    bytes((
        0x02, 0xB2, 0x13, 0x01, 0x00,
        0x42, 0x4B, 0x02, 0x11, 0x00,
        0x0A, 0x00, 0x01, 0x00, 0x01, 0xA0,
    )),
    # selector 0x0a + canonical DeviceKey RateLimited error body
    bytes((
        0x0A, 0xA4, 0x00, 0x01, 0x01, 0x19,
        0x0D, 0xAC, 0x02, 0x02, 0x03, 0x01,
    )),
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("corpus", type=Path)
    arguments = parser.parse_args()
    arguments.corpus.mkdir(parents=True, exist_ok=True)
    for seed in SEEDS:
        name = hashlib.sha1(seed).hexdigest()
        destination = arguments.corpus / name
        if not destination.exists():
            destination.write_bytes(seed)
    print(f"protocol fuzz corpus ready: {arguments.corpus}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
