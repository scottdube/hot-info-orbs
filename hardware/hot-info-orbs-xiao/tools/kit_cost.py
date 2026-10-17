"""Kit cost for a 25-35 kit run, AliExpress-first (Scott, 2026-10-17).

Every price was read live on 2026-10-17, first logged out, then re-read SIGNED
IN to Scott's AliExpress account. Signed in, each listing shows a
"One-time offer: applies to one item only" (Max. 1 pcs/shopper); that price
is NOT used. The price used is the regular one a repeat buyer pays for a
whole run, before coins (1-2%) and store coupons ($1 off $18/$25). Signed
in, only C1 changed (a 50% sale, $4.96 -> $2.48). Shipping is excluded
except on the PCB, where JLCPCB's DHL estimate is shown.

Writes bom/hot_info_orbs_xiao_kit_cost.csv and prints the per-kit curve.
"""
import csv
import math
from pathlib import Path

ALI = "https://www.aliexpress.us/item/{}.html"

# (line, per kit, lot size, lot price, vendor, link, note)
LINES = [
    ("XIAO ESP32-S3 (U2)", 1, 1, 7.49, "Seeed direct",
     "https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html",
     "Seeed's 10+ tier is also $7.49; AliExpress lists the S3 at $15-44, so Seeed stays. Bulk quote available on request."),
    ("1.28in GC9A01 display (DS1-DS5)", 5, 1, 5.23, "AliExpress", ALI.format(3256805914388478),
     "Option '1.28 TFT Round'. 5,000+ sold, 297 reviews. Signed-in one-time offer $2.40 is 1 pc/shopper. Stock for 175 is NOT shown: ask the seller before ordering. Sold singly; the 10-pack listing (3256811824930283) is worse at $6.46 each. Pin order not checked against VCC GND SCL SDA DC CS RST."),
    ("Female socket 1x7 (J, 5 display + 2 XIAO)", 7, 100, 5.22, "AliExpress", ALI.format(2251832784633600),
     "Option '7Pin 100Pcs'. 2,000+ sold. Body height not stated; the 3D assumes 8.5 mm."),
    ("Male header 1x40 strip (P, XIAO)", 0.4, 30, 2.59, "AliExpress", ALI.format(3256806779943089),
     "30 strips per lot; 1 strip = 5 1x7 pieces = 2.5 kits. Skip if the XIAO ships pre-soldered."),
    ("Right-angle tactile 6x6 (SW1-SW3)", 3, 20, 3.17, "AliExpress", ALI.format(2255800088886713),
     "Option 6x6x8mm, 'Right Angle With stent'. 700+ sold. $2.91/lot at 10+ lots (not reached by 35 kits). FIT NOT VERIFIED against the C&K PTS645VL39 footprint. The BOM's current AliExpress link (3256810140983087) is a 4-pin VERTICAL switch."),
    ("C1 100uF 16V electrolytic", 1, 50, 2.48, "AliExpress", ALI.format(3256807605290784),
     "Option '16V100UF 50PCS'. Signed-in sale price, 50% off $4.96; 29 lots in stock. Can size NOT stated. The size-stated Nichicon 6.3x7 is 20 for $10.99 (3256812824344961): 2 lots for 25-35 kits, +$19.50 a run vs the sale price (+$0.56-0.78/kit)."),
    ("C2 1uF ceramic, 5.08 mm", 1, 100, 3.54, "AliExpress", ALI.format(3256811560978404),
     "Options 1UF + 100PCS. Same listing as the BOM."),
    ("R1 10k 1/4 W", 1, 100, 1.44, "AliExpress", ALI.format(3256807898146236),
     "Option 10K. Signed in: $1.44 (50% off $2.88), no one-time offer, 7 lots left."),
    ("M3 x 6 socket head (H1-H7)", 7, 50, 2.05, "AliExpress", ALI.format(3256805704775238),
     "Options 'M3 50pcs' + 6mm. Length still depends on the enclosure."),
]

# JLCPCB, 198.82 x 47.78 mm, 2 layer, 1.6 mm, white mask, black silk: (boards, price, DHL)
PCB_TIERS = [(25, 26.90, 33.03), (30, 31.50, 37.60), (50, 46.10, 40.54)]

GLASS_LENS = 3.08  # each, AliExpress 3256806569387664, optional


def pcb_for(n):
    for boards, price, ship in PCB_TIERS:
        if boards >= n:
            return boards, price, ship
    raise ValueError(n)


def run(n):
    rows, parts = [], 0.0
    for name, per, lot, price, vendor, link, note in LINES:
        need = math.ceil(per * n - 1e-9)
        lots = math.ceil(need / lot)
        cost = round(lots * price, 2)
        parts += cost
        rows.append((name, need, lot, price, lots, cost, lots * lot - need))
    boards, pcb, ship = pcb_for(n)
    return rows, round(parts, 2), boards, pcb, ship


def main():
    out = Path(__file__).resolve().parent.parent / "bom" / "hot_info_orbs_xiao_kit_cost.csv"
    curve = []
    for n in range(25, 36):
        rows, parts, boards, pcb, ship = run(n)
        total = parts + pcb + ship
        curve.append((n, parts, boards, pcb, ship, round(total, 2), round(total / n, 2),
                      round((parts + pcb) / n, 2)))
    with out.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Kits", "Parts", "PCB boards", "PCB", "PCB ship (DHL)", "Total", "Per kit", "Per kit, no PCB ship"])
        w.writerows(curve)
        w.writerow([])
        w.writerow(["Line", "Per kit", "Lot", "Lot price", "Vendor", "Link", "Note"])
        for name, per, lot, price, vendor, link, note in LINES:
            w.writerow([name, per, lot, price, vendor, link, note])
    for c in curve:
        print(*c, sep="\t")
    print()
    for n in (25, 30, 35):
        rows, *_ = run(n)
        print(n, [(r[0][:14], r[4], r[5], r[6]) for r in rows])
    print("wrote", out)


if __name__ == "__main__":
    main()
