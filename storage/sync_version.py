#!/usr/bin/env python3
"""Synchronize the firmware version macro from the root VERSION file.

Run from anywhere inside the repository:

    python storage/sync_version.py

The source of truth is the repository root VERSION file. This script mirrors
that value into libraries/engine/src/config.hpp as EDUBOX_HUB_PANEL_FIRMWARE_VERSION.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


VERSION_DEFINE_PATTERN = re.compile(
    r'(#define\s+EDUBOX_HUB_PANEL_FIRMWARE_VERSION\s+)".*?"'
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def version_file(root: Path) -> Path:
    return root / "VERSION"


def engine_config(root: Path) -> Path:
    return root / "libraries" / "engine" / "src" / "config.hpp"


def read_version(root: Path) -> str:
    version = version_file(root).read_text(encoding="utf-8").strip()
    if not version:
        raise ValueError("VERSION is empty.")
    if not re.fullmatch(r"\d+\.\d+\.\d+\.\d+", version):
        raise ValueError(f"Invalid VERSION '{version}' (expected x.y.z.b).")
    return version


def sync_version(dry_run: bool = False) -> int:
    root = repo_root()
    version = read_version(root)
    config_path = engine_config(root)
    config_text = config_path.read_text(encoding="utf-8")

    if not VERSION_DEFINE_PATTERN.search(config_text):
        raise ValueError("EDUBOX_HUB_PANEL_FIRMWARE_VERSION define not found in config.hpp.")

    updated_text = VERSION_DEFINE_PATTERN.sub(
        rf'\1"{version}"',
        config_text,
        count=1,
    )

    if dry_run:
        print(f"Would update {config_path.relative_to(root)} to version={version}")
        return 1

    config_path.write_text(updated_text, encoding="utf-8")
    print(f"Updated {config_path.relative_to(root)} version={version}")
    return 1


def main() -> None:
    parser = argparse.ArgumentParser(description="Synchronize firmware version macro from VERSION.")
    parser.add_argument("--dry-run", action="store_true", help="List changes without writing.")
    args = parser.parse_args()

    count = sync_version(dry_run=args.dry_run)
    print(f"Done. Targets: {count}")


if __name__ == "__main__":
    main()
