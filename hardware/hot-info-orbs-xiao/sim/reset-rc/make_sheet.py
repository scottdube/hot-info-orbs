"""Generate rst_rc.kicad_sch: the R1/C2 display reset, isolated for ngspice.

The board schematic cannot be simulated as-is (the XIAO and GC9A01 symbols
have no SPICE models), so this builds a standalone sheet holding only the reset
network and two supply ramps. Symbol bodies are copied from the KiCad install
so the sheet opens with no project libraries. Re-run after editing; open the
result in KiCad and use Inspect > Simulator.
"""

import re
import uuid
from pathlib import Path

LIB = Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols")
HERE = Path(__file__).resolve().parent
NAME = "rst_rc"
SHEET_UUID = "6f1d2a3c-0b7e-4c1a-9d55-2f6a8e1c4b70"

# KiCad's own PWL field rejects {param} points, so V1/V2 pass raw SPICE through.
# Supply ramp, shared shape for both rails:
#   off until td, linear rise over tr to 3.3 V, held until tcyc, falls over tf,
#   stays off for toff, rises again over tr. tcyc=1 s puts the cycle past the
#   end of the default .tran, so the default run is a plain cold start.
PWL_3V3 = ("0 0 {td3} 0 {td3+tr3} 3.3 {tcyc} 3.3 {tcyc+tf} 0 "
           "{tcyc+tf+toff} 0 {tcyc+tf+toff+tr3} 3.3")
PWL_IOVCC = ("0 0 {tdi} 0 {tdi+tri} 3.3 {tcyc} 3.3 {tcyc+tf} 0 "
             "{tcyc+tf+toff} 0 {tcyc+tf+toff+tri} 3.3")
# 1 V while the panels see RESX asserted: IOVCC inside its 1.65-3.3 V operating
# range (datasheet DC table) and RST below VIL = 0.3*IOVCC.
ASSERTED = "v=((v(IOVCC)>1.65)&&(v(RST)<0.3*v(IOVCC)))?1:0"

DIRECTIVES = [
    ".title GC9A01 RC power-on reset (R1 10k / C2 1u)",
    ".param td3=1u tr3=5m tdi=1u tri=5m",
    ".param tcyc=1 tf=1m toff=50m",
    ".tran 10u 300m",
]

NOTES = """3V3   = XIAO 3V3_OUT, feeds R1 (the pull-up)
IOVCC = the display module's own 3.3 V, made from +5V by the module regulator
        (the GC9A01 VCC pin is on +5V, not 3V3)
I1    = 5 x IIH max 1 uA (GC9A01A DC table): worst case for release
RST_ASSERTED = 1 while the panels are actually held in reset"""


def parse(s):
    toks = re.findall(r'"(?:[^"\\]|\\.)*"|\(|\)|[^\s()"]+', s)
    stack = [[]]
    for t in toks:
        if t == "(":
            stack.append([])
        elif t == ")":
            done = stack.pop()
            stack[-1].append(done)
        else:
            stack[-1].append(t)
    return stack[0][0]


def emit(node, depth=0):
    tab = "\t" * depth
    if not isinstance(node, list):
        return node
    if all(not isinstance(n, list) for n in node):
        return tab + "(" + " ".join(node) + ")"
    head = [n for n in node if not isinstance(n, list)]
    out = [tab + "(" + " ".join(head)]
    for n in node:
        if isinstance(n, list):
            out.append(emit(n, depth + 1))
    out.append(tab + ")")
    return "\n".join(out)


def lib_symbol(lib, name):
    tree = parse((LIB / f"{lib}.kicad_sym").read_text())
    for node in tree:
        if isinstance(node, list) and node[:2] == ["symbol", f'"{name}"']:
            node = list(node)
            node[1] = f'"{lib}:{name}"'
            return node
    raise KeyError(f"{lib}:{name}")


def q(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def uid():
    return f'"{uuid.uuid4()}"'


def prop(key, value, x, y, hide=False):
    hide_s = "\n\t\t\t(hide yes)" if hide else ""
    return (f"\t\t(property {q(key)} {q(value)}\n\t\t\t(at {x:.2f} {y:.2f} 0){hide_s}\n"
            f"\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t)\n"
            f"\t\t\t\t(justify left)\n\t\t\t)\n\t\t)")


def place(lib_id, ref, value, x, y, pins, sim=None):
    props = [prop("Reference", ref, x + 2.54, y - 1.27),
             prop("Value", value, x + 2.54, y + 1.27),
             prop("Footprint", "", x, y, hide=True),
             prop("Datasheet", "", x, y, hide=True)]
    for i, (k, v) in enumerate((sim or {}).items()):
        props.append(prop(k, v, x + 2.54, y + 3.81 + 2.54 * i, hide=(k != "Sim.Params")))
    pin_s = "\n".join(f'\t\t(pin "{p}"\n\t\t\t(uuid {uid()})\n\t\t)' for p in pins)
    return (f"\t(symbol\n\t\t(lib_id {q(lib_id)})\n\t\t(at {x:.2f} {y:.2f} 0)\n\t\t(unit 1)\n"
            f"\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n\t\t(in_bom yes)\n\t\t(on_board yes)\n"
            f"\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid {uid()})\n"
            + "\n".join(props) + "\n" + pin_s +
            f"\n\t\t(instances\n\t\t\t(project {q(NAME)}\n\t\t\t\t(path \"/{SHEET_UUID}\"\n"
            f"\t\t\t\t\t(reference {q(ref)})\n\t\t\t\t\t(unit 1)\n\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)")


def label(net, x, y):
    return (f"\t(global_label {q(net)}\n\t\t(shape passive)\n\t\t(at {x:.2f} {y:.2f} 0)\n"
            f"\t\t(fields_autoplaced yes)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n"
            f"\t\t\t)\n\t\t\t(justify left)\n\t\t)\n\t\t(uuid {uid()})\n\t)")


def ground(n, x, y):
    return place("Simulation_SPICE:0", f"#GND0{n}", "0", x, y, ["1"])


def text(body, x, y):
    return (f"\t(text {q(body)}\n\t\t(exclude_from_sim no)\n\t\t(at {x:.2f} {y:.2f} 0)\n"
            f"\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n"
            f"\t\t\t(justify left bottom)\n\t\t)\n\t\t(uuid {uid()})\n\t)")


def two_pin(lib_id, ref, value, x, y, top_net, bot_net, half, sim=None, gnd_n=None):
    """Place a vertical two-pin part; pin 1 at y-half, pin 2 at y+half."""
    items = [place(lib_id, ref, value, x, y, ["1", "2"], sim), label(top_net, x, y - half)]
    items.append(ground(gnd_n, x, y + half) if bot_net == "0" else label(bot_net, x, y + half))
    return items


def main():
    libs = [lib_symbol("Device", "R"), lib_symbol("Device", "C"),
            lib_symbol("Simulation_SPICE", "VPWL"), lib_symbol("Simulation_SPICE", "IDC"),
            lib_symbol("Simulation_SPICE", "BSOURCE"), lib_symbol("Simulation_SPICE", "0"),
            lib_symbol("power", "PWR_FLAG")]
    v = 5.08
    rc = 3.81
    items = []
    items += two_pin("Simulation_SPICE:VPWL", "V1", "3V3", 50.8, 76.2, "3V3", "0", v,
                     {"Sim.Device": "SPICE",
                      "Sim.Params": f'type="V" model="PWL({PWL_3V3})"'}, 1)
    items += two_pin("Device:R", "R1", "10k", 76.2, 63.5, "3V3", "RST", rc)
    items += two_pin("Device:C", "C2", "1u", 76.2, 81.28, "RST", "0", rc, gnd_n=2)
    items += two_pin("Simulation_SPICE:IDC", "I1", "5u", 101.6, 81.28, "RST", "0", v,
                     {"Sim.Device": "I", "Sim.Type": "DC", "Sim.Pins": "1=+ 2=-",
                      "Sim.Params": "dc=5u"}, 3)
    items += two_pin("Simulation_SPICE:VPWL", "V2", "IOVCC", 127.0, 76.2, "IOVCC", "0", v,
                     {"Sim.Device": "SPICE",
                      "Sim.Params": f'type="V" model="PWL({PWL_IOVCC})"'}, 4)
    items += two_pin("Simulation_SPICE:BSOURCE", "B1", "asserted", 165.1, 76.2,
                     "RST_ASSERTED", "0", v,
                     {"Sim.Device": "SPICE", "Sim.Params": f'type="B" model="{ASSERTED}"'}, 5)
    items.append(place("power:PWR_FLAG", "#FLG01", "PWR_FLAG", 50.8, 81.28, ["1"]))
    items.append(text("\n".join(DIRECTIVES), 38.1, 30.48))
    items.append(text(NOTES, 38.1, 114.3))

    out = ["(kicad_sch", "\t(version 20260306)", '\t(generator "eeschema")',
           '\t(generator_version "10.0")', f'\t(uuid "{SHEET_UUID}")', '\t(paper "A4")',
           "\t(lib_symbols"]
    out += [emit(s, 2) for s in libs]
    out.append("\t)")
    out += items
    out += ['\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)',
            "\t(embedded_fonts no)", ")"]
    (HERE / f"{NAME}.kicad_sch").write_text("\n".join(out) + "\n")
    pro = HERE / f"{NAME}.kicad_pro"
    if not pro.exists():
        pro.write_text('{\n  "meta": {\n    "filename": "%s.kicad_pro",\n    "version": 3\n  }\n}\n' % NAME)


if __name__ == "__main__":
    main()
