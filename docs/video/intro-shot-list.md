# HOT Info Orbs — intro video shot list

Follows [intro-script.md](intro-script.md). Shot numbers (S01…) follow script
order; the **setups** below group them in the order to film, so the camera
moves as few times as possible.

**Sources:** CAM = camera · PHONE = phone screen recording · LAPTOP = laptop
screen recording · STILL = image or render to gather · CARD = title or text
card made in the edit.

---

## Shots in script order

| # | Seg | Source | Shot | Length | Notes |
|---|---|---|---|---|---|
| S01 | 1 | CAM | Wide: five orbs on the desk cycling Clock → Weather → Stocks | 8 s | Locked off. Room tone only. Roll a full rotation (45 s at a 15 s cycle) and pick the best 8 s. |
| S02 | 1 | CARD | Title: HOT Info Orbs + Hands-On Tech logo over S01 | 4 s | Logo: `docs/assets/hot-smcomp-logo.png` |
| S03 | 2 | CAM | Slow push-in on the running orbs | 8 s | Slider or slow zoom. |
| S04 | 2 | CAM | The board from directly above, running | 8 s | Same as setup C, overhead. |
| S05 | 3 | STILL | Brett Turner's Info Orbs project page | 6 s | Screenshot, credit on screen. |
| S06 | 3 | STILL | HackerBoxes #0129 "Orbital" product image | 5 s | Screenshot of the product page, credit on screen. |
| S07 | 3 | CAM | A member's built HackerBox kit, running | 8 s | Borrow one if possible; otherwise reuse S03. |
| S08 | 4 | CAM | Clock page, standard face, full frame | 5 s | |
| S09 | 4 | CAM | Finger presses the middle button → nixie face | 5 s | Close enough to see the finger and the face change in one shot. |
| S10 | 4 | CAM | Weather page, full frame | 5 s | |
| S11 | 4 | CAM | Tempest page, full frame | 5 s | The station label shows; no station ID appears on the orb. |
| S12 | 4 | CAM | Close-up: the weather-icon orb, sunrise/sunset on top, moon below | 5 s | Tight; the moon drawing is 40 px. |
| S13 | 4 | CAM | Stocks page, full frame | 5 s | **Weekday, market hours**, so prices move. |
| S14 | 4 | CAM | Hand stepping right, right, left through the pages | 6 s | Over-the-shoulder or 3/4 angle. |
| S15 | 4 | LAPTOP | Home Assistant dashboard | 4 s | Crop out the URL bar and any addresses. |
| S16 | 5 | CAM | Macro: ESP32-S3 SuperMini in its socket | 5 s | Orb off or on, whichever reads better. |
| S17 | 5 | CAM | Slow pan across the five displays, then the three buttons | 6 s | |
| S18 | 5 | CAM | Bare v1.1 carrier board | 4 s | One of the 10 from JLCPCB, if one is unbuilt. Otherwise skip to S19. |
| S19 | 5 | LAPTOP | KiCad: the v1.1 board, 2D then 3D view | 5 s | `hardware/hot-info-orbs/` on main. |
| S20 | 5 | CAM | USB cable plugs in; the orbs light on the boot screen | 6 s | Boot picture set to the HOT hand first (S28). |
| S21 | 5 | LAPTOP | GitHub repo page, scrolling the README | 4 s | `github.com/scottdube/hot-info-orbs` |
| S22 | 6 | LAPTOP + CAM | The first USB flash: upload progress on the laptop, the orb beside it | 6 s | Staged: re-run the same USB flash on camera. |
| S23 | 6 | PHONE + CAM | Split screen: settings page on the phone, the orb beside it; change the town or the tickers, Save, the orb restarts | 12 s | Scroll **past** the Tempest station fields. |
| S24 | 6 | PHONE + CAM | Update page on the phone; the orb shows "Updating nn%" | 8 s | Send the same build over Wi-Fi again just for this shot. |
| S25 | 6 | CAM | The orbs dimming | 4 s | Set the dim hours to start a minute ahead, then roll. |
| S26 | 6 | CAM | Blank screens; a finger presses a button and they wake | 5 s | The backlight stays on, so film in a lit room and don't call it dark. |
| S27 | 6 | CAM | Boot screen with the Hands-On Tech hand | 4 s | |
| S28 | 6 | PHONE | Uploading the boot picture from the phone | 6 s | Optional insert under S27's narration. |
| S29 | 7 | STILL | KiCad 3D render of the XIAO board | 6 s | Render needed (see Assets). Label: "Design: not built yet". |
| S30 | 7 | STILL | Winged Speedster enclosure concept | 6 s | `docs/assets/enclosure-concept-3-winged-speedster.png` on main. Label: "Concept". |
| S31 | 8 | CARD | GitHub URL + feature-sheet URL | 4 s | |
| S32 | 8 | CARD | Credits: Brett Turner — Info Orbs · HackerBoxes — #0129 Orbital · Yang — board design · the members who built the first kit | 5 s | |
| S33 | 8 | CAM + CARD | End card: logo over the orbs running | 3 s | Reuse S01 footage. |

The listed lengths add up to 3:06, and the narration runs about 3:30. Every
shot is cut to the narration, so roll at least twice the listed length.

---

## Setups (order to film)

**A. Desk wide (tripod, slider)**: S01, S03, S08, S10, S11, S13, plus extra
footage of the orbs running for the end card. Lock exposure on the screens.

**B. Close-ups at the desk**: S09, S12, S14, S25, S26, S27. Same light as A.

**C. Board on the table (overhead and macro)**: S04, S16, S17, S18, S20.

**D. Phone beside the orb**: S23, S24, S28. Screen-record the phone while the
camera films the orb; sync on the Save tap.

**E. Laptop captures**: S15, S19, S21, S22. S22 is a staged USB flash of the
same build. Settings live in NVS and survive it, but the orb restarts, so do
S22 before setups A and B or after them, not in between.

**F. Gather and make**: S02, S05, S06, S29, S30, S31, S32, S33.

---

## Before the shoot

1. The orb firmware went back on over USB on 2026-10-17, off camera. For S22,
   re-run the same flash on camera:
   `cd ~/code/hot-info-orbs-main && pio run -e esp32-s3-devkitc-1 -t upload`.
2. On `/settings`: re-tick every page; set the boot picture to the HOT hand.
3. Check a Tempest fetch shows ✔ for station 1 before filming S11.
4. Weekday, market hours for S13.
5. Phone: Do Not Disturb on, full battery, screen recording on.

## Assets to gather

- Screenshot: Brett Turner's Info Orbs project page (S05).
- Screenshot: HackerBoxes #0129 "Orbital" product page (S06).
- **Render to make:** XIAO board 3D view (S29). Only the STEP exists
  (`hardware/hot-info-orbs-xiao/3d-assembly/`); render from KiCad.
- On hand: HOT logos (`docs/assets/hot-smcomp-logo.png`,
  `images/hot-logo.jpg`); enclosure concept (main,
  `docs/assets/enclosure-concept-3-winged-speedster.png`).

## Camera notes

- Round TFTs glare. Light from the side, not in front; check each frame for a
  window or lamp in the glass.
- Expose for the screens and let the desk fall a little dark; auto exposure
  will blow out the digits.
- Keep color settings fixed between setups A and B so the orbs match when
  cut together.
