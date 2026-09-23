# PlatformIO pre-build script: writes firmware/include/build_id.h with the git
# commit and build time, so the OTA update page can say which image is running.
# __DATE__/__TIME__ were tried first and rejected: they only change when the
# file that uses them recompiles, so after a main.cpp-only change the page
# showed the previous build's time. The header is rewritten only when its
# content changes, so it recompiles just the files that include it.
import subprocess
from datetime import datetime, timezone
from pathlib import Path

Import("env")  # noqa: F821 - provided by PlatformIO

root = Path(env["PROJECT_DIR"])  # noqa: F821
try:
    commit = subprocess.check_output(
        ["git", "describe", "--always", "--dirty", "--exclude=*"], cwd=root, text=True
    ).strip()
except Exception:
    commit = "unknown"
stamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

header = root / "firmware" / "include" / "build_id.h"
content = f'#pragma once\n#define BUILD_COMMIT "{commit}"\n#define BUILD_TIME "{stamp}"\n'
if not header.exists() or header.read_text() != content:
    header.write_text(content)
