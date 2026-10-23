"""Sweep supply ramps through the rst_rc sheet with KiCad's bundled ngspice.

kicad-cli has no simulate command and there is no standalone ngspice here, so
this exports the sheet's SPICE netlist with kicad-cli (the sheet is the single
source of the circuit) and drives libngspice from the KiCad app bundle.

For each case it reports, while IOVCC is within its 1.65-3.3 V operating range:
  low_ms     how long RST sat below VIL = 0.3*IOVCC (RESX asserted)
  release_ms when RST last rose through VIH = 0.7*IOVCC (reset released)
  min_ratio  lowest RST/IOVCC seen; under 0.3 means a reset was seen at all
"""

import ctypes
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
KICAD = Path("/Applications/KiCad/KiCad.app/Contents")
CLI = KICAD / "MacOS/kicad-cli"
LIBNGSPICE = KICAD / "Frameworks/libngspice.0.dylib"
VDD_MIN = 1.65

COLD = [dict(name=f"cold, both rails {t} ms", tr3=f"{t}m", tri=f"{t}m")
        for t in (0.1, 1, 2, 5, 10, 15, 16, 17, 18, 19, 20, 30, 50, 100)]
SKEW = [
    dict(name="IOVCC 1 ms, 3V3 30 ms", tri="1m", tr3="30m"),
    dict(name="IOVCC 1 ms, 3V3 100 ms", tri="1m", tr3="100m"),
    dict(name="IOVCC 30 ms, 3V3 1 ms", tri="30m", tr3="1m"),
    dict(name="IOVCC 1 ms late, both 5 ms", tdi="1m", tri="5m", tr3="5m"),
    dict(name="IOVCC 2 ms late, both 5 ms", tdi="2m", tri="5m", tr3="5m"),
    dict(name="IOVCC 5 ms late, both 5 ms", tdi="5m", tri="5m", tr3="5m"),
    dict(name="3V3 5 ms late, both 5 ms", td3="5m", tri="5m", tr3="5m"),
]
CYCLE = [dict(name=f"replug, off {t} ms", tr3="1m", tri="1m", tcyc="100m", tf="1m",
              toff=f"{t}m", window=0.101)
         for t in (1, 2, 5, 10, 15, 20, 30, 50)]

CB_CHAR = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_void_p)
CB_STAT = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_void_p)
CB_EXIT = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_bool, ctypes.c_bool,
                           ctypes.c_int, ctypes.c_void_p)
LOG = []
_keep = [CB_CHAR(lambda s, i, p: LOG.append(s.decode(errors="replace")) or 0),
         CB_STAT(lambda s, i, p: 0),
         CB_EXIT(lambda *a: 0)]


def netlist():
    out = Path(tempfile.mkdtemp()) / "rst_rc.cir"
    subprocess.run([str(CLI), "sch", "export", "netlist", "--format", "spice", "-o", str(out),
                    str(HERE / "rst_rc.kicad_sch")], check=True, capture_output=True)
    return out.read_text().splitlines()


def spice():
    ng = ctypes.CDLL(str(LIBNGSPICE))
    ng.ngSpice_Init(_keep[0], _keep[1], _keep[2], None, None, None, None)
    return ng


def run(ng, lines, case, tmp, c2=None, diode=False):
    p = dict(td3="1u", tr3="5m", tdi="1u", tri="5m", tcyc="1", tf="1m", toff="50m")
    p.update({k: v for k, v in case.items() if k in p})
    body = [l for l in lines if not l.lower().startswith((".param", ".tran", ".end"))]
    if c2:
        body = [f"C2 RST 0 {c2}" if l.startswith("C2 ") else l for l in body]
    if diode:   # discharge diode across R1, cathode to 3V3; generic small-signal model
        body += ["D1 RST 3V3 DSIG", ".model DSIG D(IS=2.52n N=1.752 RS=0.568 CJO=4p)"]
    body.append(".param " + " ".join(f"{k}={v}" for k, v in p.items()))
    body += [".tran 5u 400m 0 20u", ".end"]
    arr = (ctypes.c_char_p * (len(body) + 1))(*[l.encode() for l in body], None)
    ng.ngSpice_Circ(arr)
    ng.ngSpice_Command(b"run")
    data = tmp / "out.txt"
    ng.ngSpice_Command(f"wrdata {data} v(rst) v(iovcc) v(3v3)".encode())
    ng.ngSpice_Command(b"remcirc")
    rows = [list(map(float, l.split())) for l in data.read_text().splitlines() if l.strip()]
    return [(r[0], r[1], r[3]) for r in rows]          # time, v(rst), v(iovcc)


def judge(samples, t0=0.0):
    """Reset metrics for the power-up that starts at t0."""
    low = 0.0
    release = None
    min_ratio = 9.0
    prev = None
    for t, rst, vdd in samples:
        if t < t0:
            continue
        if vdd >= VDD_MIN:
            min_ratio = min(min_ratio, rst / vdd)
            if prev is not None and rst < 0.3 * vdd:
                low += t - prev
            if rst >= 0.7 * vdd and release is None:
                release = t
            elif rst < 0.7 * vdd:
                release = None
        prev = t
    return low * 1e3, (release - t0) * 1e3 if release is not None else None, min_ratio


def main():
    """Optional arguments: a C2 value to try instead of the sheet's (e.g. 4.7u),
    and "diode" to add a discharge diode across R1."""
    args = sys.argv[1:]
    diode = "diode" in args
    c2 = next((a for a in args if a != "diode"), None)
    lines = netlist()
    ng = spice()
    tmp = Path(tempfile.mkdtemp())
    print(f"C2 = {c2 or 'as drawn (1u)'}" + (", diode across R1" if diode else ""))
    print(f"{'case':34} {'low_ms':>8} {'release_ms':>11} {'min_ratio':>10}")
    for group in (COLD, SKEW, CYCLE):
        for case in group:
            s = run(ng, lines, case, tmp, c2, diode)
            low, rel, ratio = judge(s, case.get("window", 0.0))
            rel_s = f"{rel:11.2f}" if rel is not None else f"{'never':>11}"
            print(f"{case['name']:34} {low:8.3f} {rel_s} {ratio:10.3f}")
        print()


if __name__ == "__main__":
    main()
