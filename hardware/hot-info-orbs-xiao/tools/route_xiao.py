#!/usr/bin/env python3
"""Autoroute hot_info_orbs_xiao.kicad_pcb with Freerouting (KiCad closed).

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
import json, subprocess, sys
from pathlib import Path
import pcbnew

HERE = Path(__file__).resolve().parent.parent
PCB = HERE / "hot_info_orbs_xiao.kicad_pcb"
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
print("dsn", DSN.stat().st_size, "bytes")

r = subprocess.run([JAVA, "-jar", str(JAR), "-de", str(DSN), "-do", str(SES), "-mp", passes, "-mt", "1"],   # -mt 1: Freerouting 2.4.1 warns its multi-threaded optimizer is broken
                   capture_output=True, text=True, timeout=1800)
print("\n".join(l for l in (r.stdout + r.stderr).strip().splitlines()[-4:] if "WARNING" not in l))
assert SES.exists() and SES.stat().st_size > 0, "no SES produced: Freerouting writes a 0-byte file when nets are left unrouted - read its log above"

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
