#!/usr/bin/env python3
"""BOM for the XIAO carrier board -> bom/hot_info_orbs_xiao_bom.xlsx + .csv

  python3 -m venv /tmp/bomvenv && /tmp/bomvenv/bin/pip install openpyxl
  /tmp/bomvenv/bin/python tools/make_bom.py

Schematic parts (refs, values, footprints) are from
`kicad-cli sch export bom`; everything else (sockets, headers, screws,
the board) is not in the schematic and is listed by hand. Every line has
Link 1 filled; Link 2 is a second source where one was found, else blank.
No prices: none were checked live (house rule: an unverified price gets
+40% and an estimate mark, which is noise on a BOM for someone else).
Links were taken from search results on 2026-10-17, not opened one by one.
"""
import csv
from pathlib import Path
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font, PatternFill
from openpyxl.utils import get_column_letter

HERE = Path(__file__).resolve().parent.parent
OUT = HERE / "bom"
OUT.mkdir(exist_ok=True)

COLS = ["Item", "Refs", "Qty / board", "Part", "Spec", "Footprint", "Mfr part no.",
        "Link 1", "Link 2", "Notes"]
DK = "https://www.digikey.com/en/products/detail/"
ROWS = [
    ["U2", 1, "Seeed Studio XIAO ESP32-S3 Plus", "ESP32-S3R8, 16 MB flash, 8 MB PSRAM; SKU 102010671",
     "XIAO-ESP32-S3-SOCKET", "102010671",
     "https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html",
     "https://thepihut.com/products/seeed-xiao-esp32s3-plus",
     "Plus boards ship WITHOUT pin headers: see the header and socket lines below. "
     "Mounted on the back, on sockets."],
    ["DS1-DS5", 5, "1.28in round TFT, GC9A01, 240x240, 7-pin SPI",
     "7-pin 2.54 mm header, order VCC GND SCL SDA DC CS RST",
     "_Displays:GC9A01_SOCKET", "generic",
     "https://www.amazon.com/MTDELE-Display-Compatible-Arduino-Interfac/dp/B0DJ13VW73",
     "https://www.amazon.com/AITRIP-1-28-Display-Multicolored-Arduino/dp/B0H6RQ4C54",
     "Check the listing's pin order before buying; the footprint is pad 1 = VCC (+5V) .. pad 7 = RST. "
     "The module's own male header plugs into the female socket below."],
    ["J (displays)", 5, "Female socket 1x07, 2.54 mm, straight", "8.5 mm body, through hole",
     "PinSocket_1x07_P2.54mm_Vertical (in GC9A01_SOCKET)", "Sullins PPTC071LFBN-RC",
     DK + "sullins-connector-solutions/PPTC071LFBN-RC/810146", "",
     "Not a schematic part: modeled inside the display footprint. 8.5 mm is the lift the 3D assumes."],
    ["J (XIAO)", 2, "Female socket 1x07, 2.54 mm, straight", "8.5 mm body, through hole",
     "in XIAO-ESP32-S3-SOCKET", "Sullins PPTC071LFBN-RC",
     DK + "sullins-connector-solutions/PPTC071LFBN-RC/810146", "",
     "Same part as the display sockets; 7 per board in all."],
    ["P (XIAO)", 2, "Male header 1x07, 2.54 mm", "cut from a 1x40 breakaway strip (1 strip = 5 sets)",
     "in XIAO-ESP32-S3-SOCKET", "Sullins PRPC040SAAN-RC",
     DK + "sullins-connector-solutions/PRPC040SAAN-RC/2775214", "",
     "Soldered to the XIAO, plugs into the XIAO sockets."],
    ["SW1-SW3", 3, "Tactile switch, right angle, 6x6 mm", "SPST-NO, through hole, side actuated",
     "SW_Tactile_SPST_Angled_PTS645Vx39-2LFS", "C&K PTS645VL39-2 LFS",
     DK + "c-k/PTS645VL39-2-LFS/1146765",
     "https://lcsc.com/product-detail/Tactile-Switches_C-K-PTS645VL392LFS_C285523.html",
     "Footprint is the C&K part. The uxcell 6x6x8 right-angle switches in the README are NOT "
     "verified to fit this footprint."],
    ["C1", 1, "Electrolytic capacitor, +5V bulk", "VALUE NOT IN SCHEMATIC - suggested 100 uF 16 V, 6.3 mm dia, 2.5 mm pitch",
     "CP_Radial_D6.3mm_P2.50mm", "Panasonic EEU-FC1C101H (suggested)",
     DK + "panasonic-electronic-components/EEU-FC1C101H/2689573",
     DK + "panasonic-electronic-components/EEU-FC1C101/356102",
     "Schematic Value field holds the symbol name only - pick and record a value. "
     "EEU-FC1C101H is 12.2 mm tall on the back side; check enclosure clearance."],
    ["C2", 1, "Ceramic capacitor 1 uF", "50 V X7R, radial, 5.08 mm lead spacing",
     "C_Disc_D5.0mm_W2.5mm_P5.00mm", "KEMET C330C105K5R5TA7301",
     DK + "kemet/C330C105K5R5TA7301/399-9886-1-ND/3726180",
     DK + "kyocera-avx/SE015C105KAR/2060741",
     "Display RST hold-off with R1."],
    ["R1", 1, "Resistor 10k", "1/4 W 5 % carbon film, axial, 10.16 mm pitch",
     "R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal", "Stackpole CF14JT10K0",
     DK + "stackpole-electronics-inc/CF14JT10K0/1741265",
     DK + "stackpole-electronics-inc/CFM14JT10K0/1742061",
     "Display RST pull-up to 3V3."],
    ["H1-H7", 7, "M3 screw (mounting)", "M3-0.5, socket head; 3.2 mm holes",
     "Edge.Cuts circles, no footprint", "M3 x 6 DIN 912 (length depends on the enclosure)",
     "https://www.aspenfasteners.com/m3-0-5-x-6mm-din-912-iso-4762-metric-socket-head-cap-screws-hex-socket-drive-class-12-9-steel-plain-unbrako/",
     "", "Standoffs/length set by the enclosure, not chosen yet."],
    ["PCB", 1, "Bare board", "2 layer, 1.6 mm, 198.82 x 47.78 mm, WHITE solder mask, BLACK silkscreen",
     "hot_info_orbs_xiao.kicad_pcb", "-",
     "https://jlcpcb.com/quote/pcbOrderFaq/PCB%20Color", "https://www.pcbway.com/",
     "Order the colors explicitly: the stackup colors only drive the 3D/STEP, not the gerbers."],
]
assert all(len(r) == len(COLS) - 1 for r in ROWS)
assert all(r[6] for r in ROWS), "every line needs Link 1"

with open(OUT / "hot_info_orbs_xiao_bom.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(COLS)
    for i, r in enumerate(ROWS, 1):
        w.writerow([i] + r)

wb = Workbook()
ws = wb.active
ws.title = "BOM"
ws.append(COLS)
for i, r in enumerate(ROWS, 1):
    ws.append([i] + r)
head = PatternFill("solid", fgColor="D9D9D9")
for c in ws[1]:
    c.font = Font(bold=True)
    c.fill = head
widths = [6, 13, 10, 34, 42, 34, 30, 40, 40, 60]
for j, wd in enumerate(widths, 1):
    ws.column_dimensions[get_column_letter(j)].width = wd
for row in ws.iter_rows(min_row=2):
    for c in row:
        c.alignment = Alignment(wrap_text=True, vertical="top")
    for c in (row[7], row[8]):            # Link 1, Link 2
        if c.value:
            c.hyperlink = c.value
            c.font = Font(color="0563C1", underline="single")
    if "NOT IN SCHEMATIC" in str(row[4].value):
        for c in row:
            c.fill = PatternFill("solid", fgColor="FFF2CC")
ws.freeze_panes = "B2"
ws.auto_filter.ref = ws.dimensions
wb.save(OUT / "hot_info_orbs_xiao_bom.xlsx")
print("wrote", len(ROWS), "lines to", OUT)
