# PlatformIO pre-build script: writes firmware/include/build_id.h with the git
# commit, build time and board environment, so the OTA update page can say
# which image is running and which board it was built for. On 2026-10-22 the
# XIAO image went onto the SuperMini prototype over OTA, booted, and reported
# its commit; nothing on the page said it carried the wrong pin map.
# __DATE__/__TIME__ were tried first and rejected: they only change when the
# file that uses them recompiles, so after a main.cpp-only change the page
# showed the previous build's time. The header is rewritten only when its
# content changes, so it recompiles just the files that include it.
import configparser
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

# The board env, not the env that was built: [env:ota] only extends the board
# env, so printing "ota" would hide the one thing the page is there to show.
ini = configparser.ConfigParser(interpolation=None)
ini.read(root / "platformio.ini")
board_env = env["PIOENV"]  # noqa: F821
seen = set()
while board_env not in seen and ini.has_option(f"env:{board_env}", "extends"):
    seen.add(board_env)
    board_env = ini.get(f"env:{board_env}", "extends").strip().removeprefix("env:")

header = root / "firmware" / "include" / "build_id.h"
content = f'#pragma once\n#define BUILD_COMMIT "{commit}"\n#define BUILD_TIME "{stamp}"\n#define BUILD_ENV "{board_env}"\n'
if not header.exists() or header.read_text() != content:
    header.write_text(content)
