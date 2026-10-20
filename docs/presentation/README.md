# Club presentation: HOT Info Orbs intro

`HOT-Info-Orbs-Small-Computers-2026-10-27.pptx` is the 12-slide deck for the
Hands-On Tech Small Computers meeting on Tuesday 2026-10-27 (Rohan Rec
Center, 6:30 pm). The `.pdf` beside it is a LibreOffice render for anyone
without PowerPoint. Speaker notes on every slide carry the narration, adapted
from `../video/intro-script.md`: the deck and the intro video use the same
assets, the video being for members who miss the meeting.

Built on Yang's club template ("HOT template & intro slides.pptx", his
2026-05-30 email), **SmComp Dark Theme** master, as his README slide asks:
copy, delete the preface slides, add slides from the right theme group, never
touch the logos by hand. The logo top right comes from the layout.

## Decisions (Scott, 2026-10-20)

- **No cost figures anywhere**, slides or notes.
- **Board is rev 2.4 and the slide says "design locked"**, not "in
  manufacturing" or "ordering next". The renders are from the 46 mm board in
  git (`hardware/hot-info-orbs-xiao-46mm`, rev 2.3 run 3 geometry on
  `board-46mm-pitch`); re-render from the 2.4 file when it lands:
  `kicad-cli pcb render --side top --rotate "-28,0,0" --zoom 1.9` (front) and
  `--side top --zoom 1.9` (plan), background transparent, then crop to the
  alpha bbox.
- **No embedded video.** The orb itself is on the table for the live demo.
- Public deck: no station IDs, MQTT broker, Wi-Fi details or tokens. The
  settings-page panels are cropped to Rotation/Clock, Dim/Screens-off and
  Start-up picture for that reason; the page header (access point MAC) and the
  Tempest/MQTT sections are deliberately left out.

## Rebuilding

```
python build_deck.py assets/base-smcomp-dark-12-empty.pptx HOT-Info-Orbs-Small-Computers-2026-10-27.pptx
```

needs python-pptx and Pillow. `assets/base-smcomp-dark-12-empty.pptx` is
Yang's template with the seven original slides removed and twelve empty slides
added (Title Slide, 10 × Title Only, Title Slide). `assets/` holds every
image the script places: frame grabs from the 2026-10-19/20 camera takes
(`~/Movies/hot-info-orbs-intro/camera`, cropped to the orb band; ffmpeg
grabs carry no GPS metadata), the thermal images from `../assets/thermal`,
the KiCad renders, the settings-page panels (headless Chrome at 640 px, 2×),
the provenance photos (credited on the slide) and a QR code for the GitHub
repo.
