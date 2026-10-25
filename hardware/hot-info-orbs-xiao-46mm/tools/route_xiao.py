#!/usr/bin/env python3
"""Autoroute hot_info_orbs_xiao_46mm.kicad_pcb with Freerouting (KiCad closed).

Run with KiCad's own Python:
  /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 tools/route_xiao.py [passes]

  1. export Specctra DSN from the board WITHOUT the GND pour (own process)
  2. java -jar freerouting-2.4.1.jar -de x.dsn -do x.ses -mp N   (headless)
  3. import the SES into the real board (pour intact), save, then
       kicad-cli pcb drc --schematic-parity --severity-all --refill-zones --save-board
     and print the counts.  0 unconnected is the pass criterion.

Why the pour is left out of the export: with it, Freerouting treats GND as
a plane, routes nothing for it, and the new tracks slice the pour into
islands (first run: 3 starved thermals, 2 unconnected GND pads). Routing
GND as tracks and pouring on top afterwards gives a pour that only helps.

Why three processes: pcbnew.LoadBoard() a second time in one interpreter,
and the board after ImportSpecctraSES, hand back objects whose methods are
missing (bare SwigPyObject), so export and import each run as a child.

Why the footprint was changed first (tools/merge_xiao_pads.py): with
Seeed's two pads per pin Freerouting reported 14 violations, stubbed every
pin and left one net unrouted every run (CS4, then MOSI once CS4 was
pre-routed by hand). One oval pad per pin: 0 violations, 0 unrouted.
"""
import json, math, re, subprocess, sys
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao_46mm.kicad_pcb"
WORK = Path("/tmp/xiao_route"); WORK.mkdir(exist_ok=True)
DSN, SES = WORK / "xiao.dsn", WORK / "xiao.ses"
JAR = Path.home() / "Documents/KiCad/10.0/3rdparty/plugins/app_freerouting_kicad-plugin/jar/freerouting-2.4.1.jar"
JAVA = "/opt/homebrew/opt/openjdk/bin/java"
KICAD_CLI = "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli"

if len(sys.argv) > 1 and sys.argv[1] == "--export":
    b = pcbnew.LoadBoard(str(PCB))
    pours = [z for z in b.Zones() if not z.GetIsRuleArea()]   # zones before tracks: Zones()
    for z in pours:                                           # breaks once tracks are removed
        b.Remove(z)
    for tr in list(b.GetTracks()):
        b.Remove(tr)
    assert pcbnew.ExportSpecctraDSN(b, str(DSN)), "DSN export failed"
    sys.exit(0)
if len(sys.argv) > 1 and sys.argv[1] == "--import":
    b = pcbnew.LoadBoard(str(PCB))
    for tr in list(b.GetTracks()):          # start from bare copper every run
        b.Remove(tr)
    assert pcbnew.ImportSpecctraSES(b, str(SES)), "SES import failed"
    pcbnew.SaveBoard(str(PCB), b)
    sys.exit(0)

passes = sys.argv[1] if len(sys.argv) > 1 else "20"
subprocess.run([sys.executable, __file__, "--export"], check=True, capture_output=True)
# The mounting holes export as keepouts at the bare 1.6 mm radius and
# Freerouting keeps only its 0.2 mm clearance from them; KiCad wants 0.5 mm
# to any Edge.Cuts (2026-10-17: two copper_edge_clearance errors). Grow each
# keepout radially by 0.4 mm about its centroid.
def _grow(m, grow=400.0):
    nums = [float(v) for v in m.group(2).split()]
    pts = list(zip(nums[0::2], nums[1::2]))
    cx = sum(p[0] for p in pts[:-1]) / (len(pts) - 1)
    cy = sum(p[1] for p in pts[:-1]) / (len(pts) - 1)
    out = []
    for x, y in pts:
        k = (math.hypot(x - cx, y - cy) + grow) / math.hypot(x - cx, y - cy)
        out.append(f"{cx + (x - cx) * k:.1f} {cy + (y - cy) * k:.1f}")
    return m.group(1) + "  ".join(out) + "))"
dsn, n = re.subn(r'(\(keepout "" \(polygon signal 0\s+)([-0-9.\s]+?)\)\)', _grow, DSN.read_text())
DSN.write_text(dsn)
print("dsn", DSN.stat().st_size, "bytes,", n, "hole keepouts grown 0.4 mm")

# A 0-byte SES is NOT "nets left unrouted" (that was this script's old reading):
# 2026-10-17 a 0-unrouted run wrote 0 bytes and an identical rerun wrote the
# file - Freerouting 2.4.1 exits before the save flushes. Unrouted count comes
# from its log; an empty file just means run it again.
for attempt in range(5):
    SES.unlink(missing_ok=True)
    r = subprocess.run([JAVA, "-jar", str(JAR), "-de", str(DSN), "-do", str(SES), "-mp", passes, "-mt", "1"],   # -mt 1: Freerouting 2.4.1 warns its multi-threaded optimizer is broken
                       capture_output=True, text=True, timeout=1800)
    log = (r.stdout + r.stderr).splitlines()
    print("\n".join(l[:200] for l in log if "final score" in l and "Auto-routing stage" in l or l.startswith("  Net '")))
    # 2026-10-20: a run can also end "Auto-routing stage interrupted ... (44
    # unrouted and 0 violations)" after 0.14 s and still write a small SES;
    # the identical DSN routed to 0 unrouted by hand. Treat that like the
    # empty file: rerun.
    done = any("Auto-routing stage completed" in l and "(0 unrouted" in l for l in log)
    if SES.exists() and SES.stat().st_size > 0 and done:
        break
    print(f"{'empty SES' if not SES.exists() or SES.stat().st_size == 0 else 'stage interrupted'} (attempt {attempt + 1}), rerunning")
assert SES.exists() and SES.stat().st_size > 0 and done, "Freerouting failed five times running"

subprocess.run([sys.executable, __file__, "--import"], check=True, capture_output=True)
txt = PCB.read_text()
print("tracks+vias after import:", txt.count("\n\t(segment") + txt.count("\n\t(via"))

out = WORK / "drc.json"
subprocess.run([KICAD_CLI, "pcb", "drc", "--schematic-parity", "--severity-all", "--refill-zones",
                "--save-board", "--format", "json", "-o", str(out), str(PCB)], check=True, capture_output=True)
d = json.loads(out.read_text())
errs = [v for v in d["violations"] if v["severity"] == "error"]
print("DRC: errors", len(errs), "/ warn", len(d["violations"]) - len(errs),
      "/ unconnected", len(d["unconnected_items"]), "/ parity", len(d["schematic_parity"]))
for v in errs[:20] + d["unconnected_items"][:10]:
    print("  ", v["type"], "|", v["description"][:100], [i["description"][:40] for i in v["items"]])
