#!/usr/bin/env python3
"""Package the release build into the two files the README quick start names.

Run after `pio run -e release`, from the repo root:

    python3 tools/make_release_bins.py

Writes:
  release/info_orbs_fw_combo.bin  bootloader + partition table + boot_app0 +
                                  app, padded to the whole 8 MB flash. Flashed at
                                  0x0 it wipes everything: WiFi, settings, the
                                  uploaded start-up picture. First install, or a
                                  clean start.
  release/firmware.bin            the app alone, flashed at 0x10000 (app0 in
                                  partitions.csv). Keeps WiFi and settings. Same
                                  file the /update page takes over WiFi.
  release/SHA256SUMS.txt

Padding to 8 MB is what makes the combo a factory reset without asking anyone
to tick "Erase entire flash" in the web flasher: every byte past the app,
including NVS at 0x9000, is written as 0xFF (erased). Rejected: an unpadded
merge, which leaves old NVS behind and so is not a reset. The S3 Plus has
16 MB; its upper 8 MB is unused by this table and left alone.

The ESP32-S3 boot ROM loads the second-stage bootloader from 0x0 (the classic
ESP32 uses 0x1000), so these offsets are S3-only.
"""

import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / ".pio" / "build" / "release"
OUT = ROOT / "release"
PIO = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
ESPTOOL = PIO / "packages" / "tool-esptoolpy" / "esptool.py"
BOOT_APP0 = PIO / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin"
PYTHON = PIO / "penv" / "bin" / "python"

COMBO = "info_orbs_fw_combo.bin"
APP_OFFSET = 0x10000  # app0 in partitions.csv


def main() -> int:
    parts = {
        0x0: BUILD / "bootloader.bin",
        0x8000: BUILD / "partitions.bin",
        0xE000: BUILD / "boot_app0.bin" if (BUILD / "boot_app0.bin").exists() else BOOT_APP0,
        APP_OFFSET: BUILD / "firmware.bin",
    }
    missing = [str(p) for p in parts.values() if not p.exists()]
    if missing or not ESPTOOL.exists():
        print("Missing (run `pio run -e release` first):", *missing, ESPTOOL if not ESPTOOL.exists() else "", sep="\n  ")
        return 1

    OUT.mkdir(exist_ok=True)
    cmd = [str(PYTHON if PYTHON.exists() else sys.executable), str(ESPTOOL), "--chip", "esp32s3", "merge_bin",
           "-o", str(OUT / COMBO), "--fill-flash-size", "8MB"]
    for off, path in sorted(parts.items()):
        cmd += [hex(off), str(path)]
    subprocess.run(cmd, check=True)
    shutil.copyfile(BUILD / "firmware.bin", OUT / "firmware.bin")

    sums = []
    for name in (COMBO, "firmware.bin"):
        data = (OUT / name).read_bytes()
        sums.append(f"{hashlib.sha256(data).hexdigest()}  {name}")
        print(f"{name}: {len(data):,} bytes")
    (OUT / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n")
    print(f"Wrote {OUT}/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
