#!/usr/bin/env python3
"""Upload firmware to an orb over WiFi:  ota_upload.py <orb IP or host> <firmware.bin>

Run by `pio run -e ota -t upload --upload-port <orb IP>`. The app cannot
overwrite itself (one app slot, see partitions.csv), so this:
  1. asks the app to restart into the recovery app (POST /recovery),
  2. waits for recovery's page, recognised by its X-Orbs-Recovery header,
  3. uploads the firmware to recovery, which writes app0 and restarts,
  4. waits for the orbs to answer again and prints the commit they run.
OTA_PASSWORD is read from firmware/config/secrets.h, so no flags are needed.
Standard library only: it runs in PlatformIO's own Python.
"""
import base64
import re
import sys
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path

HEADER = "X-Orbs-Recovery"  # RECOVERY_HEADER in firmware/src/recovery/RecoveryPolicy.h
ROOT = Path(__file__).resolve().parent.parent


def password():
    secrets = ROOT / "firmware" / "config" / "secrets.h"
    if not secrets.exists():
        return None
    m = re.search(r'^\s*#define\s+OTA_PASSWORD\s+"([^"]*)"', secrets.read_text(), re.M)
    return m.group(1) if m else None


def request(url, data=None, headers=None, timeout=5):
    req = urllib.request.Request(url, data=data, headers=dict(headers or {}))
    pw = password()
    if pw:
        req.add_header("Authorization", "Basic " + base64.b64encode(f"admin:{pw}".encode()).decode())
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.headers, r.read().decode(errors="replace")
    except urllib.error.HTTPError as e:
        return e.code, e.headers, e.read().decode(errors="replace")


def state(base):
    """'recovery', 'app', or None when nothing answers."""
    try:
        status, headers, _ = request(base + "/update")
    except (urllib.error.URLError, OSError):
        return None
    if status == 401:
        sys.exit("Orb refused the password. Check OTA_PASSWORD in secrets.h.")
    return "recovery" if headers.get(HEADER) else "app"


def wait_for(base, want, seconds):
    deadline = time.time() + seconds
    while time.time() < deadline:
        if state(base) == want:
            return True
        time.sleep(2)
    return False


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    host, image = sys.argv[1], Path(sys.argv[2])
    base = "http://" + host
    body = image.read_bytes()

    now = state(base)
    if now is None:
        sys.exit(f"Nothing answers at {base}/update. Use the orb's IP; .local is unreliable from the Mac.")
    if now == "app":
        print("Restarting the orb into recovery...")
        status, _, text = request(base + "/recovery", data=b"")
        if status != 200:
            sys.exit(f"The orb would not restart into recovery ({status}): {text.strip()}")
        time.sleep(3)  # let it go down before polling, or the app answers once more
        if not wait_for(base, "recovery", 90):
            sys.exit("Recovery did not answer within 90 s. The orb returns to the app on its own after 10 min.")

    print(f"Uploading {image.name}, {len(body):,} bytes...")
    boundary = uuid.uuid4().hex
    payload = (
        f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="{image.name}"\r\n'
        "Content-Type: application/octet-stream\r\n\r\n"
    ).encode() + body + f"\r\n--{boundary}--\r\n".encode()
    t0 = time.time()
    status, _, text = request(base + "/update", data=payload,
                              headers={"Content-Type": f"multipart/form-data; boundary={boundary}"}, timeout=180)
    print(f"{status} {text.strip()} ({time.time() - t0:.0f} s)")
    if status != 200:
        sys.exit(1)

    time.sleep(3)
    if not wait_for(base, "app", 90):
        sys.exit("The orbs did not come back within 90 s. If recovery answers instead, the update was stopped.")
    _, _, page = request(base + "/update")
    m = re.search(r"commit (\S+), built ([^<]+)", page)
    print("Orbs running " + (f"commit {m.group(1)}, built {m.group(2)}" if m else "(no version on the page)"))


if __name__ == "__main__":
    main()
