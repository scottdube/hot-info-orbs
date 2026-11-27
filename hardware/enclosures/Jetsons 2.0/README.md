# Jetsons-Inspired Five-Orb Display Enclosure

**Community-ready 3D-print files — final physically fitted front: P30.**

This project is a retro-futuristic enclosure for a PCB with **five circular displays at 46.00 mm center-to-center spacing**. The final P30 front panel was confirmed by the builder to align correctly after the display opening coordinates were mirrored horizontally. **Do not substitute P19, P26, P27, P29 or the P28 test gauges for the final P30 front.** The rear enclosure is the previously validated P23.

> This package contains mechanical 3D-print files and PCB Gerber reference/manufacturing files; it does **not** include display firmware, a schematic, source electronics design, a PCB bill of materials, or a guaranteed ready-to-slice printer profile. Printer settings below are starting recommendations, not measured settings from the builder's final successful print.

## Print bill of materials

All dimensions in millimeters. **Each STL contains one modeled part; duplicate in the slicer to the quantity below.**

| File under `STL/` | Qty | Suggested color / finish | Function |
|---|---:|---|---|
| `01_Enclosure/Front_P30_FINAL.stl` | 1 | Turquoise or teal, matte PLA | **Final corrected front**, 5 aligned openings |
| `01_Enclosure/Rear_P23.stl` | 1 | Matching turquoise/teal, matte PLA | Rear case, PCB posts, switch holes, sockets |
| `02_Display_Bezels/Display_Bezel_44mm_PRINT_5.stl` | 5 | Metallic silver, silk PLA or silver PLA | Display trim: 44 mm OD, 35.5 mm ID |
| `03_Buttons/Button_Snap_Bezel_PRINT_3.stl` | 3 | Metallic silver PLA | Stationary button trim |
| `03_Buttons/Button_P25A_13mm_PRINT_3.stl` | 3 | Orange PLA | Guided pushbuttons with 13 mm reach |
| `03_Buttons/Button_P24_C_Retainer_PRINT_3.stl` | 3 | Dark gray PLA or PETG | Side-install button retainers |
| `04_Feet/Forward_Stability_Foot_PRINT_2.stl` | 2 | Dark gray PLA/PETG | Forward-stability feet, corrected integral peg |
| `05_Decorative/Left_Fin_PRINT_1.stl` | 1 | Matching turquoise/teal PLA | Decorative fin |
| `05_Decorative/Antenna_PRINT_1.stl` | 1 | Silver PLA | Decorative antenna |
| `05_Decorative/Accent_Rail_PRINT_4.stl` | 4 | Silver PLA | Optional decorative accent rails |

**Total: 24 printed pieces** with the listed accessories. The final design assembles without glue; connector notch covers are not part of this release. The builder confirmed the final P30 display alignment. Printer profiles remain recommended starting points.

## Filament recommendations

The intended palette is a Jetsons-style **turquoise body, silver trim, orange buttons, dark gray feet/retainers, **. These are color suggestions, not exact manufacturer SKU matches.

- **Main front and rear:** quality matte PLA or PLA+; consistent low-warp behavior and a clean painted-like surface. Bambu Lab PLA Matte in a suitable turquoise/teal shade is a reasonable option if currently available.
- **Display bezels and rails:** silver silk PLA for a shiny chrome-like appearance, or ordinary metallic silver PLA for more dimensional consistency. Silk PLA can be brittle; handle small pegs carefully.
- **Buttons:** bright orange PLA/PLA+; favor dimensional accuracy and smooth sliding over appearance.
- **C-retainers and feet:** PETG is an optional tougher choice, but PETG fit can differ from PLA. For a first build, PLA is simplest.
- **Fin:** same turquoise as the shell. **Antenna:** silver PLA.

No AMS is required: the parts are separate color-specific STLs. Print each color as a separate plate, or combine parts by filament.

## Recommended Bambu Studio / FDM starting settings

Choose your **actual printer model, nozzle, build plate, and filament manufacturer's profile** first. These are suggested baselines for a **0.4 mm nozzle**, not an exported or tested Bambu Studio preset.

| Setting | Front / rear | Bezels / buttons / small trim | Feet |
|---|---|---|---|
| Layer height | 0.16–0.20 mm | 0.12–0.16 mm | 0.20 mm |
| Walls / perimeters | 3–4 | 3–4 | 4 |
| Top / bottom layers | 4–5 | 4–5 where applicable | 4–5 |
| Infill | 15–20% gyroid | 20–30% gyroid | 25–35% gyroid |
| Supports | Only where slicer preview requires | Avoid supports on pins/snap fits if possible | Usually none if placed flat |
| Brim | Optional 5–8 mm if adhesion is poor | Optional for tall/thin parts | Usually none |
| Seam | Place on a less-visible edge | Away from mating surfaces | Anywhere unobtrusive |

**Orientation:** Place each enclosure half on its largest stable planar surface; inspect the sliced preview for bridge spans, unsupported bosses, and finish of the visible front face. Print bezels with the flat ring surface on the plate and pins facing up *if that matches the STL's actual orientation in the slicer*. Place feet sole-flat. Orient small buttons and retainers to minimize supports on functional contact surfaces.

**Quality tips:** Use a clean plate, dry filament, slow outer walls for visible surfaces, and inspect the first layer. Do not use aggressive elephant-foot compensation without checking snap-fit dimensions. Silk PLA often benefits from the manufacturer's slower recommended profile. For the display bezels, inspect that all **three pins** are actually fused to the ring in the sliced toolpath. If not, repair/union the mesh in CAD or the slicer before printing; do not rely solely on a successful STL import.

## Hardware and tools

**Final assembly uses mechanical fits and fasteners only—no glue. The connector notch covers from earlier prototypes are obsolete and are intentionally excluded.**


- Compatible five-round-display PCB with **46 mm display pitch**. The PCB's exact purchasing details and electronics are not specified in this package.
- **Four M3 enclosure screws** were recommended in earlier fit tests (enclosure hardware, distinct from PCB mounting). **Length, head style and engagement depth must be checked against the printed bosses before installation.** Do not force screws or crack the printed plastic.
- **PCB mounting screws: M2.5**, confirmed by the builder. Screw length and head type are not yet specified; check engagement depth in the PCB posts before tightening.
- 3 pushbutton switches already fitted to the compatible electronics/PCB.
- Flush cutters, a small screwdriver, and a deburring tool. **No glue or adhesive is required.**

## Suggested print order and assembly

1. **Print the P30 front**, not an older front. Dry-fit it to the PCB and P23 rear before installing decoration. The P30 horizontal mirror is the final confirmed fix.
2. Print **one** 44 mm display bezel. Check that its 35.5 mm opening hides the PCB edges without covering active pixels; check all three pins and right-side rail clearance. Then print four more.
3. Print **one** stationary silver button bezel, one P25A button, and one P24 C-retainer. Confirm the button moves and actuates the switch without preload. Repeat for all three buttons.
4. Fit the PCB to the seven P23 posts using hardware verified for your board. Do not overtighten.
5. Assemble front and rear; check the four alignment features and enclosure screws. Confirm no display glass or PCB components contact the enclosure under load.
6. Install two feet, the optional fin and antenna, and four accent rails using their modeled attachment features. **Do not use glue.** Check trim clearance; earlier prototypes had bezel/rail interference.
7. Verify switch travel, USB/power access, display visibility, and enclosure stability.

## Dimensions and design history

- Front width: **270 mm**; overall face height: **82.5 mm**.
- Display pitch: **46.00 mm**.
- **Final P30 opening X centers in CAD:** `-93.7, -47.7, -1.7, 44.3, 90.3 mm`; common Z center: `-2 mm`.
- P29 used the opposite X order and did **not** align when assembled normally. Rotating P29 180° aligned horizontally but inverted vertical alignment. P30 corrects **only the X coordinates** and was confirmed to work.
- Display opening nominal diameter in front CAD: **37.0 mm**. Bezel: **44 mm OD / 35.5 mm ID**.
- Rear: **P23**, with seven PCB posts. Original decorative rail locations retained.
- Buttons: P25A 13 mm reach, with P24 side-install C-retainers.

## Project structure

```text
STL/                 Final printable parts (only P30 front and 44 mm bezel)
CAD/                 Editable OpenSCAD sources for the main modeled parts
README.md            This guide
bambu/.              Bambu Slicer Plates with parts already placed
```

### Editing and regenerating files

Open `.scad` files in **OpenSCAD**. The final front source is `CAD/Front_P30_FINAL.scad`. It defaults to `part="front"`; render with F6 and export as STL. Some legacy sources include multiple module definitions and may need the correct `part` selector. The STL files are the recommended source of truth for printing.

**Important source limitation:** The original `Display_Bezel_P10.scad` is a **superseded 45 mm design** and does not recreate the final 44 mm / 35.5 mm bezel. It is preserved only for historical reference. The final bezel STL is provided; an exact editable source for that revised bezel was not present in the original release files. The button-bezel original was supplied as a 3MF rather than a matching OpenSCAD source. These limitations are disclosed rather than presenting older source as final.

## Release and safety notes

- This is a hobby enclosure, **not** an electrical safety certification. Check insulation, thermal clearance, power supply ratings, strain relief and exposed wiring for your own build.
- A 3D-print STL is printer-independent; fit can change with nozzle, material, calibration, and shrinkage.
- No exact manufacturer color SKUs, proven 3MF slicing profile, firmware, or BOM for the electronics were supplied.
