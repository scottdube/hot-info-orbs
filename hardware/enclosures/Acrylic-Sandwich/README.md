# Hot Info Orbs — Acrylic Sandwich Enclosure

![Acrylic sandwich enclosure](acrylic%20sandwich.jpg)

The acrylic sandwich is a simple open-frame enclosure for the Hot Info Orbs board (`hot_info_orbs_xiao_46mm`) for a minimalist modern appearance. The PCB, with its five round displays, is clamped between two clear acrylic panels. Four threaded rods, one at each outer mounting hole, pass through both panels and the PCB. Aluminum spacers hold the PCB at a set distance from each panel, and a cap nut on each end of each rod holds the stack together.

## The panels are cut by a plastics shop, not 3D printed

The front and back panels are **6.35 mm (1/4 in) clear acrylic (PMMA) sheet** cut to size by a plastics fabrication shop. They are not 3D printed, because clear, flat, polished acrylic is what makes the displays visible through the front panel.

The only 3D printed part in the enclosure is the **display standoff**. Print it from `display standoff.3mf`. The print requirements are not critical: **PLA or PETG** with your slicer's default settings is fine.

**Attaching the display standoff:** glue the standoff to the front of the PCB with a thin rubber glue. **Avoid using thick double-sided tape.** The standoff is designed to sit flat against the PCB, and the thickness of the tape would push the displays out of position.

Give the shop the two fabrication drawings in this folder. Each drawing lists the material, thickness, outline, hole and notch positions, and edge finish, with every dimension in mm and inches. All outside edges are to be polished.

For shops that cut from CAD files, DXF cut files of both panels are also included (`front panel - DXF cut profile.dxf`, `back panel - DXF cut profile.dxf`). Each contains only the cut geometry, in mm: one closed outline (with the USB notch on the back panel) and the four Ø3.205 mm holes. `back panel - DXF cut profile.dxf` is drawn in the same orientation as its fabrication drawing. The front panel DXFs come out of Fusion rotated 180° from the drawing's outside view. The part is identical; rotate it 180° to compare it with the drawing.

**Optional display cutouts:** `front panel - DXF with display cutouts.dxf` is a variant of the front panel with five round openings for the displays, for anyone who wants them. Their diameter is set by the Fusion user parameter **disp_hole_dia** (currently 35.5 mm [1.398 in]), and they are centered on the displays. The standard `front panel - DXF cut profile.dxf` and the front panel drawing have **no** display openings.

**Re-exporting the DXFs after a design change:** in `acrylic sandwich.f3d`, each panel component has a hidden sketch named **front panel - DXF cut profile** / **back panel - DXF cut profile**. It is projected from the panel body, so it updates with the model. In the Browser, right-click that sketch and choose **Save As DXF**. Save the file under the sketch's name, as the files in this folder are. For the display-cutout variant, export **front panel - DXF with display cutouts** the same way. Do not export the other panel sketches (**front panel**, **back panel**): they contain reference geometry and artwork that is not cut.

| Panel | Size | Features |
|---|---|---|
| Front | 254.00 × 82.78 × 6.35 mm [10.000 × 3.259 × 0.250 in] | 4 × Ø3.205 mm [0.1262 in] through-holes |
| Back | 254.00 × 82.78 × 6.35 mm [10.000 × 3.259 × 0.250 in] | 4 × Ø3.205 mm through-holes, plus a 15.00 × 17.00 mm [0.591 × 0.669 in] notch in the bottom edge for the USB cable |

## Files in this folder

| File | Purpose |
|---|---|
| `README.md` | This file: overview, file list, and parts list. |
| `acrylic sandwich.f3d` | The Autodesk Fusion design of the complete enclosure: PCB assembly, both panels, display standoff, and all hardware. It is a single self-contained design with no links to other Fusion files. Open it with **File → Open → Open from my computer** (or upload it to a Fusion project) to view or change the enclosure. |
| `display standoff.3mf` | 3D print file for the display standoff, the only printed part. Open it in your slicer. |
| `acrylic sandwich.jpg` | A rendered picture of the assembled enclosure, for reference. |
| `Acrylic Sandwich - Front Panel Fabrication Drawing.pdf` | Fabrication drawing for the **front panel**, to send to the plastics shop. |
| `Acrylic Sandwich - Back Panel Fabrication Drawing.pdf` | Fabrication drawing for the **back panel**, including the USB notch, to send to the plastics shop. |
| `front panel - DXF cut profile.dxf` | DXF cut file (mm) for the **front panel**: outline and four holes, for shops that cut from CAD files. Exported from the **front panel - DXF cut profile** sketch. |
| `front panel - DXF with display cutouts.dxf` | Optional DXF variant of the front panel with five round display openings (diameter = `disp_hole_dia`, currently 35.5 mm), plus the outline and four holes. Exported from the **front panel - DXF with display cutouts** sketch. |
| `back panel - DXF cut profile.dxf` | DXF cut file (mm) for the **back panel**: outline with USB notch and four holes, for shops that cut from CAD files. Exported from the **back panel - DXF cut profile** sketch. |

## Parts list (other than the PCB and the acrylic panels)

| Part | Qty per enclosure | McMaster-Carr P/N | Description |
|---|---|---|---|
| Threaded rod | 4 | 93805A130 | Threaded rod, M3, 50 mm long. One at each outer PCB mounting hole, through both panels. |
| Spacer, 15 mm | 4 | 94669A316 | Aluminum unthreaded spacer for M3, 6 mm OD, 3.2 mm ID, 15 mm long. Sits between the **front** panel and the PCB. |
| Spacer, 16 mm | 4 | 94669A021 | Aluminum unthreaded spacer for M3, 6 mm OD, 3.2 mm ID, 16 mm long. Sits between the PCB and the **back** panel. |
| Cap nut | 8 | 94000A330 | 18-8 stainless steel cap nut, M3 × 0.5 mm, DIN 1587, 5.5 mm wide, 6 mm high. Sold in packs of 10; one pack covers one enclosure. Goes on each end of each rod, against the panel faces. |
| Display standoff | 1 |n/a (3D printed) | Strip that sits between the displays and the front of the PCB, with a brow around each end display. Print it from `display standoff.3mf`; it is also modeled in `acrylic sandwich.f3d`. Glue it to the PCB with a thin rubber glue, not thick double-sided tape. Not a McMaster-Carr part. |

Notes:
* The cap nut description is McMaster-Carr's own listing for 94000A330. The descriptions for 93805A130, 94669A316 and 94669A021 were measured from their models in the Fusion design. Check them on mcmaster.com, including the pack quantities, before ordering.
* The rods are 50 mm long, so about 2.3 mm sticks out past each panel face for the cap nuts. The cap nuts take 3.8 mm of thread.
* Assembly stack on each rod, from front to back: cap nut → front panel → 15 mm spacer → PCB → 16 mm spacer → back panel → cap nut.
