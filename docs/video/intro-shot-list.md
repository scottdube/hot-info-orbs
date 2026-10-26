# HOT Info Orbs — intro video shot list

Follows [intro-script.md](intro-script.md). Shot numbers (S01…) follow script
order, so the edit can cut in number order. The shoot won't run that way, so
this file is laid out for filming:

1. **[Work list](#work-list)**: what to do, in the order to do it, with boxes to tick.
2. **[Shots by type](#shots-by-type)**: each shot with its setup and why it's in the video.
3. **[Script-order index](#script-order-index)**: number → segment → length, for the edit.

**Sources:**

| Code | Meaning |
|---|---|
| CAM | camera |
| PHONE | phone screen recording |
| LAPTOP | laptop screen recording |
| STILL | image or render, gathered ahead |
| CARD | text card made in the edit |

---

## Work list

The order is set by what each block changes. Some blocks restart the orb or
change its settings, so they go after the shots that need it undisturbed.

**Any time before the shoot (no camera)**
- [ ] Stills, renders and draft cards collected into `~/Movies/hot-info-orbs-intro/assets/`
      (outside the repo: third-party screenshots). See [Assets](#assets).
- [ ] Watch the MQTT and Web Data pages on the orb for a day; adjust labels,
      units or colors (see Before the shoot, item 2).
- [x] HackerBox kit for S07: Donald Champion sent four photos of his build
      (2026-10-18), credited to him on screen.
- [ ] Check for an unbuilt v1.1 board (S18) and whether the XIAO boards have
      arrived (S29).

**Shoot day, before the camera comes out**
- [ ] Work through [Before the shoot](#before-the-shoot), items 1–6.

**Session 6, 2026-10-26: the last shoot, then it's a wrap (Scott, 2026-10-25).**
Reshoot the four shots still cut from the session-2 desk-wide take
(IMG_4778, bench-top clutter in frame). Every other slot is final. **The VO
stays as read 2** (the good mic); the slot lengths below do not change, so it
needs no re-record or re-cut.

| Shot | Page | Slot in the cut | Now |
|---|---|---|---|
| S10 | Weather (first weather page) | 5.6 s at 1:00.7 | IMG_4778 39.5 s |
| S11 | Tempest / second weather page | 6.1 s at 1:06.3 | IMG_4778 49 s |
| S15 | MQTT | 10.6 s at 1:31.9 | IMG_4778 69 s, slowed to 79.5 % to fill |
| S15a | Web Data sample | 5.1 s at 1:42.4 | IMG_4778 59 s |

- [x] **Seconds per page to 15** on `/settings` before rolling. At 5 s a page
      is up too briefly for the 10.6 s S15 slot. Put it back afterward.
- [x] MQTT reads ✔ Connected and Web Data ✔ (Before the shoot, item 2).
- [x] Both weather pages show a town you are happy to have in a public video
      (the orb was left on Orlando, FL after S23).
- [x] Bench cleared. Frame like S14 (orb row about 68% of the width) so it
      cuts with S13/S14. Side light, exposure locked on the screens.
- [x] One locked-off take, no hands, at least two full rotations, so each of
      the four pages comes up twice.

Afterward Claude overwrites the four slots (same lengths), drops the S15
slow-down, exports a review MP4, and the cut is done.

**Shot 2026-10-26 (IMG_0037) and cut the same day:** S10 and S11 (the same two
towns as before; framed tighter than S13/S14, about
80 % of the width, left at 100 %), S15a Web Data and S15 MQTT are in, S15 at 100 %. Review export
`review/hot-info-orbs-intro-review-2026-10-26.mp4`. **One gap: S15.** The
prototype orb has one MQTT page, now fed a local events list, so the shot
shows that under a VO about "live values from Home Assistant". The two-page
firmware is on `feat/two-mqtt-pages` and is not in demo-typed. To close it,
put the five home rows back on that page and reshoot S15 (10.6 s, MQTT
page only), or keep the events list. Put Seconds per page back.

**Block 1 — Laptop (setup E):** S22, then S19. (S21 is done: a full-page
pan, no filming.)
S22 restarts the orb, so do it first, before the orb is framed.

**Block 2 — Desk wide (setup A):** S01, S03, S08, S10, S11, S15, S15a, S13, plus
a minute of extra running footage for S33.
- S11: only after Tempest shows ✔.
- S15: only after MQTT shows ✔ Connected.
- S13: only on a weekday, in market hours. If the shoot isn't, come back to this
  setup for S13 alone.

**Block 3 — Close-ups at the desk (setup B):** S09, S12, S14, S27, then S25, S26.
S25 and S26 change the dim settings, so they come last. Put the dim hours back
afterward.

**Block 4 — Phone beside the orb (setup D):** S23, S23a, S24, S28.
These change settings.
- After S23, change the town or tickers back.
- After S23a, re-tick Stocks.

**Block 5 — Board on the table (setup C):** S04, S17, S18.
This is last because it means moving the orb off the desk set.

**Edit:** S02, S31, S32, S33 cards; S05, S06, S20a, S20b, S29, S30 stills;
S16, S18, S20, S29 Fusion animations.

---

## Shots by type

Each shot has **Setup** (how to get it) and **Why** (what it does for the
video). The audience runs from first-time builders to seasoned tinkerers, so
some shots are there for each end.

### Orbs on camera: desk wide (setup A)

Tripod, plus a slider if you have one. Lock exposure on the screens.

- **S01, Wide, five orbs cycling** (8 s).
  - *Setup:* locked off, room tone only. Roll a full rotation (about 45 s) and
    pick the best 8 s.
  - *Why:* running-orbs footage for the title plate and B-roll. **No longer
    a silent cold open** (cut 2026-10-21): an 8 s hold with no narration read
    as a yawn, so the video opens on the title card with the VO from frame 1.
- **S03, Slow push-in** (8 s).
  - *Setup:* slider or slow zoom toward the center orb.
  - *Why:* gives the "what it is" narration something alive to sit on, and
    shows scale.
- **S08, Clock, standard face** (5 s).
  - *Setup:* full frame, all five orbs.
  - *Why:* the first page and the most familiar. It shows the five screens
    working as one display.
- **S10, Weather** (5 s).
  - *Setup:* full frame.
  - *Why:* the page most people want.
  - The town name shows on screen and the video is public, so pick a town
    you're happy to show.
- **S11, Tempest** (5 s).
  - *Setup:* full frame. Only after Tempest shows ✔ on `/settings`. The page
    label shows; no station ID appears on the orb.
  - *Why:* a Tempest owner in the audience sees their station used. For
    others, it shows the orb isn't limited to one weather source.
- **S13, Stocks** (5 s).
  - *Setup:* full frame, **weekday, market hours**.
  - *Why:* moving prices prove the data is live. A frozen after-hours price
    looks like a mock-up.
- **S15, MQTT page** (5 s).
  - *Setup:* full frame. Five live values from Home Assistant: Outside,
    Humidity, Pool, Power, Today (fed every 30 s, see Before the shoot).
  - *Why:* the advanced tinkerer's hook. "It shows anything from my Home
    Assistant" is what turns a clock into a dashboard.
  - Generic labels only: no room or place names.
- **S15a, Web Data sample** (4 s).
  - *Setup:* full frame. The sample draws five colored orbs.
  - *Why:* shows the JSON route, for people who write a little code but don't
    run Home Assistant. The colors make it read as "your own page" at a
    glance.
- **S33 footage, End-card plate** (3 s used).
  - *Setup:* a minute of clean running footage from the S01 position, with no
    hands in shot.
  - *Why:* a plate to lay the logo over. It's cheaper to shoot now than to set
    up again.

### Orbs on camera: close-ups at the desk (setup B)

Same light and color settings as A, so the two cut together.

- **S09, Middle button → nixie face** (5 s).
  - *Setup:* finger and face change in one frame, 3/4 angle.
  - *Why:* the first time a hand touches it. It shows the buttons do
    something, and the nixie face is the eye candy.
- **S12, Weather-icon orb: sunrise/sunset and moon** (5 s).
  - *Setup:* tight on one orb; the moon drawing is only 40 px, so go close
    and focus by hand.
  - *Why:* a detail people only notice up close. It rewards anyone who pauses
    the video.
- **S14, Hand stepping through pages** (6 s).
  - *Setup:* over the shoulder or 3/4: right, right, left.
  - *Why:* shows how simple daily use is, three buttons and nothing else, for
    the first-time builder.
- **S27, Boot screen with the HOT hand** (4 s).
  - *Setup:* power-cycle the orb, then roll before it comes up. The boot
    picture is set to the HOT hand beforehand.
  - *Why:* club branding, and it sets up "any picture you like".
- **S25, Dimming** (4 s).
  - *Setup:* dim hours are whole hours only, and the orb checks the hour every
    loop, so the change lands exactly on the hour. A few minutes before the
    hour, note the current From/Until, set From to the coming hour, and Save
    (the orb restarts). Roll from :59:30 through :00:10. Setting From to the
    current hour doesn't work: the orb restarts and comes up already dim, so
    there's no change on camera.
  - *Why:* answers "will it light up my bedroom?", which matters for a gadget
    that runs all day.
- **S26, Screens off → a button wakes them** (5 s).
  - *Setup:* film in a lit room. Screens-off isn't black: the backlight glows,
    so don't frame it as dark.
  - *Why:* shows the bedtime behavior and that any button brings it back.

### Orbs on camera: board on the table (setup C)

Overhead rig and a macro lens. Light from low on one side, so the parts cast
shadows.

- **S04, Board from directly above, running** (8 s).
  - *Setup:* overhead, square to the board.
  - *Why:* the first look at the hardware. The layout reads clearly from
    above.
- **S16, Animation: the XIAO ESP32-S3 lifts out of its sockets and plugs
  back in** (10 s, ANIM; was a 5 s macro camera shot of the module until
  2026-10-22).
  - *Setup:* `S16-xiao-plug-dark.mp4` / `-transparent.mov` in the asset
    folder, rendered from Fusion by `sources/fusion_plug.py`. Seated 1.5 s,
    out over 2 s, hold, back in over 2 s, hold; loops. Oblique back view so
    the pins visibly leave the sockets.
  - *Why:* "the brain" needs a face, and "it sits in a socket, so it lifts
    out" is literally shown, which reassures beginners worried about
    soldering it wrong. An animation shows the lift cleanly; on camera the
    pull would need a hand in a macro frame.
- **S17, Pan across the five displays, then the buttons** (6 s).
  - *Setup:* slow lateral move at board height.
  - *Why:* pictures "five displays share one connection" and puts the three
    buttons in context.
- **S18, Bare v1.1 board** (4 s).
  - *Setup:* an unbuilt board from the batch of 10, on its own. If none is
    left unbuilt, skip it and lean on S19.
  - *Why:* credits Yang and Scott's design. Experienced builders want to see the bare
    copper.
- **S20, Animation: a right-angle USB-C cable plugs in, the orbs light on
  the boot screens** (6 s, ANIM; was a camera shot until 2026-10-25, when
  Scott found the plug-in too awkward to film on the board).
  - *Setup:* `S20-usb-plug-dark.mp4` / `-transparent.mov` in the asset
    folder, rendered from Fusion by `sources/fusion_usb.py`. Back view from
    below: the 90° plug rises into the XIAO's port with its cable running
    straight back, then the camera swings round to the front and the five
    screens light with the real boot screens (Welcome, HOT Info Orbs, boot
    picture, Connecting → Success, IP address), lifted from the 10-22
    restart take by `sources/fusion_usb_screens.py`.
  - *Why:* it pictures "runs off a USB cable": nothing to wire, no power
    brick.

### Phone + orb (setup D)

Screen-record the phone while the camera films the orb. Sync on the Save tap.
Do Not Disturb on.

- **S23, Settings change → orb restarts** (12 s).
  - *Setup:* split screen. Change the town or the tickers, Save, the orb
    restarts with them. Scroll **past** the Tempest station fields.
    **Shot on the laptop, not the phone** (2026-10-25, FC Record,
    take-20261025-160952): Safari full screen and zoomed, "Boston, MA" →
    "Orlando, FL", camera inset on the orb. The Tempest box sits under the
    Weather box in the typing frames, so the cut is cropped to the Weather box.
  - *Why:* the biggest worry for a first-time builder is "do I have to edit
    code?". This shot answers no.
  - Change the setting back afterward.
- **S23a, Untick Stocks** (10 s).
  - *Setup:* untick, Save, then show the rotation skipping Stocks.
  - *Why:* shows the orb is personal, with only the pages you want.
  - Re-tick Stocks afterward.
- **S24, Update page → "Updating nn%"** (8 s).
  - *Setup:* send the same build over Wi-Fi again just for this shot.
  - *Why:* "no cable after the first time" needs proof. The fallback line
    reassures anyone afraid of bricking it.
- **S28, Uploading the boot picture** (6 s, optional).
  - *Setup:* phone only, picking an image and saving. **Shot on the laptop
    instead** (2026-10-25, take-20261025-160407): file dialog, then the
    page column cropped full frame through Upload to "✓ Saved (16 KB)".
  - *Why:* an insert under S27's narration if the beat runs long.

### Computer screen (setup E)

Laptop screen recordings at 1080p or better. Hide the dock, bookmarks and any
notifications.

- **S22, Staged USB flash** (6 s, LAPTOP + CAM).
  - *Setup:* re-run the same USB flash on camera, with the progress bar on
    the laptop and the orb beside it. Settings survive it, but the orb
    restarts, so do it before blocks 2 and 3.
  - *Why:* the one step a beginner can't skip. Seeing it is a single command
    with a progress bar takes the fear out.
- **S19, KiCad: the v1.1 board, 2D then 3D** (5 s).
  - *Setup:* `hardware/hot-info-orbs/` is no longer on main (main carries the
    XIAO boards); open it from the demo-typed worktree. The displays and the
    SuperMini use `${KICAD_USER_3DMODEL_DIR}`, which is unset on the laptop,
    so without it the 3D view is a bare board. Launch with
    `open -a "/Applications/KiCad/PCB Editor.app" --env KICAD_USER_3DMODEL_DIR="$PWD/lib/3d" "$PWD/hot_info_orbs.kicad_pcb"`
    from that folder (found 2026-10-25). Live: hold the 2D view, then
    Option-3 into the 3D viewer. **The orbit is rendered, not filmed**
    (Scott, 2026-10-25: a hand orbit in the viewer is too finicky):
    `S19-kicad-orbit-viewer.mp4`, which starts on the viewer's exact frame
    (same window, same view, fitted) so it cuts straight on after the
    take's `screen.mov`.
  - *Why:* for the advanced crowd. The design files are real and open, and
    they could change them.
- **S21, GitHub README scrolling** (11 s).
  - *Setup:* `github.com/scottdube/hot-info-orbs`, signed out or in a private
    window so no account details show. **Made 2026-10-18 as an even pan, not
    a screen recording:** a full-page capture of the signed-out page, panned
    at 10 px per frame, 60 fps, with a 1 s hold at the top and bottom
    (`S21-github-readme-smooth-10px.mp4` in the asset folder). A sped-up
    screen recording was tried first and juddered (the capture drops
    frames), and a speed ramp that held on each heading looked jerky.
  - *Why:* where everyone goes next. Show it before the closing card gives
    the URL.

### Stills and artwork (gathered, see Assets)

- **S05, Brett Turner's Info Orbs** (6 s).
  - *Setup:* Brett's own photo of his orbs (brett.tech product page), with a
    slow push in the edit and a credit label. The repo screenshot it replaced
    (2026-10-19) read as dull.
  - *Why:* honest credit. The idea is his, and saying so up front earns trust
    with people who know the original.
- **S06, HackerBoxes #0129 Orbital** (5 s).
  - *Setup:* product-page screenshot, credit on screen.
  - *Why:* many viewers met the orbs through this kit, so it connects their
    version to ours.
- **S07, Donald Champion's built HackerBox kit** (8 s, stills).
  - *Setup:* his four photos (clock, stocks, weather, nixie clock), a slow
    push-in or a quick cut through them, credit "Photo: Donald Champion" on
    screen. The weather photo shows his town on one orb; that's fine to
    show (Scott, 2026-10-18: the board itself names the town). Files and notes: the asset folder's `MANIFEST.md`.
  - *Why:* the "members built that kit" line lands harder with the real
    object on screen.
- **S20a, Thermal image of the module, 43.6 °C** (4 s).
  - *Setup:* `docs/assets/thermal/module-2026-10-17.jpg`. The alternative,
    `board-2026-10-17.jpg`, shows the module and displays in one frame.
  - *Why:* the original kit's known flaw was heat (82 °C reported). A
    measured number answers it, which the experienced crowd will look for.
- **S20b, Thermal image of the display backs, 37.9 °C** (3 s).
  - *Setup:* `docs/assets/thermal/display-back-2026-10-17.jpg`.
  - *Why:* shows where the heat went: spread over five displays, each warm,
    none hot.
- **S29, XIAO board** (6 s).
  - *Setup:* the KiCad 3D render, labeled "In manufacturing" (confirm on the
    day). If boards have arrived, film a bare one instead.
  - *Why:* tells viewers it's an active project, and gives a reason to
    follow the repo.
- **S30, Winged Speedster enclosure concept** (6 s).
  - *Setup:* `docs/assets/enclosure-concept-3-winged-speedster.png` on main,
    labeled "Concept".
  - *Why:* the case is what non-electronics members want. The label keeps the
    promise honest.

### Cards (made in the edit)

Use high-contrast text, and never use color alone to carry meaning.

- **S02, Title: "HOT Info Orbs" + Hands-On Tech logo over S01** (4 s).
  - *Why:* names it and the club in the first five seconds.
- **S31, GitHub URL + feature-sheet URL** (4 s).
  - *Why:* the action to take. It's held long enough to read.
- **S32, Credits** (5 s): Brett Turner — Info Orbs · HackerBoxes — #0129
  Orbital · the members who built the first kit ·
  HackerBox build photos — Donald Champion.
  - *Why:* names everyone whose work the video stands on.
- **S33, End card: logo over the S01-position footage** (3 s).
  - *Why:* a clean ending the club can reuse.

---

## Script-order index

| # | Seg | Type | Shot | Length |
|---|---|---|---|---|
| S01 | 1 | CAM A | Wide, cycling | 8 s |
| S02 | 1 | CARD | Title | 4 s |
| S03 | 2 | CAM A | Push-in | 8 s |
| S04 | 2 | CAM C | Board overhead | 8 s |
| S05 | 3 | STILL | Brett Turner page | 6 s |
| S06 | 3 | STILL | HackerBoxes #0129 | 5 s |
| S07 | 3 | Stills | Donald Champion's HackerBox kit (photos) | 8 s |
| S08 | 4 | CAM A | Clock | 5 s |
| S09 | 4 | CAM B | Nixie face | 5 s |
| S10 | 4 | CAM A | Weather | 5 s |
| S11 | 4 | CAM A | Tempest | 5 s |
| S12 | 4 | CAM B | Sun/moon close-up | 5 s |
| S13 | 4 | CAM A | Stocks | 5 s |
| S14 | 4 | CAM B | Hand stepping pages | 6 s |
| S15 | 4 | CAM A | MQTT | 5 s |
| S15a | 4 | CAM A | Web Data | 4 s |
| S16 | 5 | ANIM | XIAO lifts out, plugs back in | 10 s |
| S17 | 5 | CAM C | Displays + buttons pan | 6 s |
| S18 | 5 | CAM C | Bare v1.1 board | 4 s |
| S19 | 5 | LAPTOP | KiCad v1.1 (take + orbit) | 8 s |
| S20 | 5 | ANIM | USB in → boot | 6 s |
| S20a | 5 | STILL | Thermal, module | 4 s |
| S20b | 5 | STILL | Thermal, display backs | 3 s |
| S21 | 5 | PAN | GitHub README (full-page pan) | 11 s |
| S22 | 6 | LAPTOP + CAM | USB flash | 6 s |
| S23 | 6 | LAPTOP + CAM | Settings → restart | 12 s |
| S23a | 6 | PHONE + CAM | Untick Stocks | 10 s |
| S24 | 6 | PHONE + CAM | Updating nn% | 8 s |
| S25 | 6 | CAM B | Dimming | 4 s |
| S26 | 6 | CAM B | Screens off → wake | 5 s |
| S27 | 6 | CAM B | Boot screen | 4 s |
| S28 | 6 | LAPTOP | Boot picture upload | 6 s |
| S29 | 7 | STILL | XIAO render | 6 s |
| S30 | 7 | STILL | Enclosure concept | 6 s |
| S31 | 8 | CARD | URLs | 4 s |
| S32 | 8 | CARD | Credits | 5 s |
| S33 | 8 | CAM + CARD | End card | 3 s |

The listed lengths add up to 3:40 (re-added 2026-10-18; the earlier 3:18 was
a miscount, and S21 went from 4 to 11 s; S16 went from 5 to 10 s on
2026-10-22 to cover its 9 s line), and the narration runs about 3:30. Every
shot is cut to the narration, so roll at least twice the listed length.

---

## Before the shoot

1. The orb firmware went back on over USB on 2026-10-17, off camera. For S22,
   re-run the same flash on camera. The filmed orb is the 4 MB SuperMini
   prototype, so build from the demo-typed worktree (b63e1b7), never main,
   whose `esp32-s3-devkitc-1` env is the XIAO pin map. `esptool flash_id`
   must say 4MB first (checked 2026-10-25):
   `cd ~/code/hot-info-orbs-demo-typed && PLATFORMIO_BUILD_FLAGS='-DDEMO_FIXED_TZ=-4 -DARDUINO_USB_CDC_ON_BOOT=1' pio run -e esp32-s3-devkitc-1 -t upload`
   (`DEMO_FIXED_TZ=-5` after Nov 1).
2. On `/settings`, every page is ticked, Web Data and MQTT included (both
   ticked 2026-10-17 to preview ahead of the shoot). Set the boot picture to
   the HOT hand.
   - Web Data shows the sample with ✔.
   - MQTT reads ✔ Connected. Five rows are filled:

     | Topic | Label | Unit |
     |---|---|---|
     | `orbs/outside` | Outside | F |
     | `orbs/humidity` | Humidity | % |
     | `orbs/pool` | Pool | F |
     | `orbs/power` | Power | W |
     | `orbs/today` | Today | kWh |

   - A Home Assistant automation publishes all five topics every 30 s,
     retained, rounded to show cleanly. The unit is "F", not "°F": the orb
     font has no degree sign, the same reason it draws a hyphen, not a dash.
3. Check a Tempest fetch shows ✔ for station 1 before filming S11.
4. Weekday, market hours for S13.
5. Phone: Do Not Disturb on, full battery, screen recording on.
6. Laptop: dock hidden, notifications off, browser signed out or private for
   S21.

## Assets

Everything that isn't camera work, gathered into
`~/Movies/hot-info-orbs-intro/assets/` with a `MANIFEST.md` (source, date and
credit line for each file). The folder is kept out of the repo: it holds
third-party screenshots, and the repo is public.

| Shot | Asset | Status |
|---|---|---|
| S05 | Brett Turner's photo of his orbs, from his product page | gathered 2026-10-19 |
| S06 | HackerBoxes #0129 product page and Instructables page, screenshots | to collect |
| S16 | XIAO plug in/out, Fusion: `S16-xiao-plug-dark.mp4` + `-transparent.mov`, `sources/fusion_plug.py` | rendered 2026-10-22, in the cut (replaced the placeholder, held to the VO line) |
| S20 | USB plug-in + boot, Fusion: `S20-usb-plug-dark.mp4` + `-transparent.mov`, `sources/fusion_usb.py` + `fusion_usb_screens.py` | rendered 2026-10-25 |
| S19 | v1.1 board 3D orbit, kicad-cli: `S19-kicad-orbit-viewer.mp4`, `sources/kicad_orbit.py` | rendered 2026-10-25, matched to the live viewer; the 2D half is the live screen take |
| S29 | XIAO board 3D render; only the STEP and PCB exist (`hardware/hot-info-orbs-xiao/`) | to render |
| S20a, S20b | Thermal images, in color, 2026-10-17 at 75 °F room: white and red hottest, blue coolest | on hand, `docs/assets/thermal/` |
| S02, S33 | HOT logos: `docs/assets/hot-smcomp-logo.png`, `images/hot-logo.jpg` | on hand |
| S30 | Enclosure concept: `docs/assets/enclosure-concept-3-winged-speedster.png` (main) | on hand |
| S02, S31, S32, S33 | Draft cards, transparent and dark, with sources to re-render | to draft |

**Needs Scott:** all camera footage, the phone and laptop screen recordings,
a bare v1.1 board (S18), and a bare XIAO
board if one arrives (S29).

## Camera notes

- Round TFTs glare. Light them from the side, not the front, and check each
  frame for a window or lamp in the glass.
- Expose for the screens and let the desk fall a little dark; auto exposure
  will blow out the digits.
- Keep color settings fixed between setups A and B so the orbs match when
  cut together.
