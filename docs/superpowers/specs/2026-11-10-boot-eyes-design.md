# Boot eyes: orbs 2 and 4 wake up while WiFi connects

Date: 2026-11-10 · Status: approved design, awaiting spec review · Branch:
`feat/boot-eyes` (off `main` at bd3839e)

## Why

At the 2026-11-10 club breakfast the idea came up of giving the orbs eyes at
start-up. The row of five round displays already reads as a face once two of
them show eyes. Boot also has a natural dead spot to fill: the orb sits on
static splash screens while WiFi connects (typically 3–5 s).

The feature: orbs 2 and 4 show a pair of eyes that act like they are **just
waking up**. They stay groggy for as long as WiFi takes, then come fully awake
when WiFi connects, and the clock takes over.

## What was tried first (spike, 2026-11-10)

A throwaway demo ran on the XIAO ESP32-S3 test rig. It was TFT_eSPI's
`examples/Generic/Animated_Eyes_2`, a port of Adafruit's "Uncanny Eyes" by
Phil Burgess (MIT license), adapted to this board. Afterward the rig was
restored from a full flash dump. Measured and decided there:

| Finding | Consequence for this design |
|---|---|
| **DMA on gave all-black screens** on this board (`USE_HSPI_PORT`, ESP32-S3), and with DMA off the frame rate barely moved (24 vs 23 eye-frames/s). | DMA stays off. The renderer uses plain `pushPixels`. |
| The stock eye is 128×128 px. Pixel-doubled with a 4 px crop per side (120 → 240) it fills the round panel. That gives ~11–12 fps per eye; the limit is the 27 MHz SPI bus, ~34 ms per full 240×240 frame. | Use the 2× renderer. That frame rate was judged good. |
| The example draws lids black. With the eye half open, the black around it looked "incomplete". | Lids get a skin color, a lash line, and lash strands (values below). |
| Lid colors tried: Skin, Tan, Robot, then three blends between Skin and Robot. | **Taupe** chosen. |
| Lash strand length 30 (lid-map units) read as short and 60 as too long. | **45**. |
| The demo build was 451 KB in all, eye graphics (`defaultEye.h`) included. | Rough cost ~200 KB of flash. Measure it at implementation time. |

## What the user sees

Boot with the setting on (the default):

| Orb | Today | With boot eyes |
|---|---|---|
| 1 | Welcome, flash % | unchanged |
| 2 | "HOT Info Orbs / by HOT-Team / version" | **left eye** |
| 3 | start-up picture | unchanged |
| 4 | "Connecting" | **right eye** |
| 5 | blank (WiFi setup instructions if needed) | "HOT Info Orbs / by HOT-Team / version / Connecting" |

1. The eyes start shut and run the **groggy** phase: they crack open a little,
   heavy-blink shut, open halfway and droop, and repeat. The gaze sits slightly
   low. This lasts as long as WiFi takes, with no cap.
2. When WiFi connects, the **finale** plays (about 2 s): look to one side, then
   the other, a quick blink, then wide open.
3. Then the usual post-connect screen shows the IP address on orb 5. Orb 4
   keeps the awake eye: the "Success" screen is skipped when the eyes ran.
   After that the clock takes over as today.

If WiFi fails and the WiFi setup portal starts, the eyes stop at once. Orbs 2
and 4 are cleared, orb 2 gets its version text back, and the portal screens
appear on orbs 4 and 5 exactly as today.

With the setting off, or on a build without the feature, boot is identical to
today's, byte for byte in what is drawn.

## Look

- **Eye:** `defaultEye` (hazel, human-ish) from the example. Upper-lid tracking
  is kept: the lid follows the pupil. Autonomous random motion and auto-blink
  are not used, since the script owns all motion during boot.
- **Scale:** 2× pixel doubling, crop 4 source px per side, window (0,0,240,240).
- **Lids** (RGB): skin `170,143,129`, shadow `128,106,95`, lash `30,22,20`.
  - A lid pixel's *margin* is how far past the closing threshold its lid-map
    value is. A small margin means it is near the lid edge.
  - margin < 6: lash line.
  - margin < 6 + 14: shadow.
  - otherwise: skin.
- **Lash strands**, on the upper lid only:
  - They run for lid margins 6 ≤ m < 6 + 45.
  - Spacing 6 source px.
  - Outward lean 5/16 px per margin unit per 64 px from center, so they fan
    toward the corners.
  - Thickness 1.5 source px for the first half of the length, 0.875 after.
  - Formula as in the spike: `sx = x*16 − (ox*k*5)/64`; lash where
    `sx mod 96 < th`.
- **Orientation:** both panels use the rotation ScreenManager already set. The
  left and right eye textures are mirrored as in the example (`e ? 1 : −1` lid
  step), so each caruncle (inner corner) faces orb 3. Checked on the rig with
  `invert` off. With `invert` on it is unchecked; see Testing.

## Wake script

Times are in ms. "Open" runs from 0 (lids shut) to 256 (lids free, normal
tracking). Gaze runs from 0 to 1023, with 512 = center; low y = looking down.
Values in between are interpolated with smoothstep.

**Groggy intro** (once, from `start`):

| t | 0 | 1500 | 2600 | 3200 | 3500 | 3900 |
|---|---|---|---|---|---|---|
| open | 0 | 0 | 70 | 60 | 0 | 0 |
| gaze | (512,380) | | | | | (512,400) |

**Groggy loop** (period 3000, repeats from 3900 until connected):

| loop t | 0 | 1000 | 1700 | 2100 | 2700 | 3000 |
|---|---|---|---|---|---|---|
| open | 0 | 110 | 90 | 150 | 40 | 0 |
| gaze | (512,400) | | (512,420) | | | (512,400) |

**Finale** (starts on `connected`, but never before t = 2600, so the first
crack-open always shows):

| finale t | 0 | 300 | 700 | 1000 | 1300 | 1500 | 1600 | 1800 | 2000 |
|---|---|---|---|---|---|---|---|---|---|
| open | (current) | 170 | 170 | 170 | 170 | 170 | 0 | 256 | 256 |
| gaze | (current) | (512,512) | (180,520) | (180,520) | (850,520) | (850,520) | (850,520) | (512,512) | (512,512) |

The first 300 ms of the finale eases from whatever pose the groggy phase was
in, so nothing jumps. At finale t = 2000 the script reports `done`.

Added boot time: about 2 s (the finale, ease included) when WiFi takes 2.6 s or
more. When WiFi connects sooner, the finale waits for t = 2600, so the eyes end
at about 4.6 s from eye start whatever the WiFi time.

## Architecture

New folder `firmware/src/core/booteyes/`:

| Unit | Does | Depends on |
|---|---|---|
| `WakeScript.{h,cpp}` | Pure logic. `begin(nowMs)`, `connected(nowMs)`, `EyePose at(nowMs)`, where `EyePose = {uint16_t open; int16_t gazeX, gazeY; bool done;}`. Holds the tables above. | nothing (native-testable) |
| `EyeRenderer.{h,cpp}` | Draws one eye for a pose: sclera, iris and polar maps, lid thresholds from tracking plus `open`, the Taupe lids and lashes, 2× scaling, `pushPixels` per doubled line. | TFT_eSPI, eye data |
| `eyeData.h` | `defaultEye.h` from the example: sclera, iris, polar, upper and lower lid maps. Adafruit MIT notice kept at the top. | — |
| `BootEyes.{h,cpp}` | `start(ScreenManager&)` creates a FreeRTOS task pinned to core 1 at priority 1 and returns false if it cannot. The task loops: `pose = script.at(millis())`, select screen index 1 (orb 2) and draw eye 0, select index 3 (orb 4) and draw eye 1, `vTaskDelay(1)`. It exits on `done` or when a stop flag is set; it checks the flag between doubled lines, never mid-transaction. `finish(bool connected)`: if connected, calls `script.connected()` and waits for the task to end (timeout 6 s, then sets the stop flag and waits for the exit). If not connected, sets the stop flag, waits for the exit, and clears both eye orbs. | WakeScript, EyeRenderer, ScreenManager |

**No two threads ever draw at once.** The eye task is the only thing drawing
between `start` and `finish`: every splash screen is drawn before `start`, and
nothing in `begin` or `connect` draws while the eyes run. That makes locking
unnecessary; TFT_eSPI and ScreenManager stay single-threaded.
`WifiWidget::setup()` splits into three calls:

- `begin(bool eyesRunning)`: configures WiFiManager and keeps the existing
  right-button WiFi reset at power-on. It draws "Connecting" on orb 4 only when
  `eyesRunning` is false; otherwise it draws nothing.
- `connect()`: `autoConnect`, returning success.
- `showPortal()`: the existing portal screens on orbs 4 and 5.

`setup()` in `main.cpp` becomes:

```
splash: orb 1 and orb 3 as today
        eyes setting on  -> orb 5 "HOT Info Orbs / by HOT-Team / version / Connecting"
        eyes setting off -> orb 2 version text (today)
eyesRan = setting && BootEyes::start(*sm)
if (setting && !eyesRan) draw orb 2 version text      // fallback
wifiWidget->begin(eyesRan)
bool ok = wifiWidget->connect()
if (eyesRan) BootEyes::finish(ok)
if (!ok) { if (eyesRan) draw orb 2 version text; wifiWidget->showPortal(); }
```

`WifiWidget` also remembers `eyesRan` so that its post-connect `draw()` skips
the "Success" screen on orb 4 and leaves the awake eye there. The IP screen on
orb 5 is unchanged.

**Setting:**

- **NVS:** key `booteyes`, a bool, default `true`, added to `SettingsValues`.
  Absent means the default, as with every other key.
- **Settings page:** a checkbox in the "Start-up picture" fieldset reading
  "Waking eyes on orbs 2 and 4 at start-up". Saving restarts the orb, as all
  settings do, so the next boot shows the change.

**Build gate:** `-D BOOT_EYES` goes in `env:xiao-esp32s3` `build_flags`, which
`ota` and `release` inherit. `env:supermini-proto` gets
`build_unflags = -D BOOT_EYES`: its 1.875 MB slot is already 88.5% full and
~200 KB more would bring it to ~99%. Without `BOOT_EYES`, the checkbox is not
rendered, the key is ignored, and boot is today's.

## Error handling and edge cases

- **WiFi slower than usual (retries):** the groggy loop keeps going. There is
  no timeout of its own; WiFiManager's retries decide.
- **WiFi faster than 2.6 s:** the finale waits for the first crack-open, which
  ends the eyes at ~4.6 s from eye start.
- **Portal (no saved network, or connect failed):** `finish(false)` stops the
  eyes within one frame of both eyes (~90 ms), clears both eye orbs, and the
  portal shows as today.
- **Task fails to start** (out of memory): `start` returns false and serial
  logs it. Orb 2 gets its version text, `begin` draws "Connecting" on orb 4 as
  today, and orb 5 keeps the version text it already shows.
- **Finale overruns:** the longest legitimate wait is a connect at t ≈ 0, which
  waits 2.6 s for the crack-open plus 2 s of finale. `finish` gives up after
  6 s and stops the task, so boot never hangs on the eyes.
- **Watchdog:** the task yields (`vTaskDelay(1)`) between eye frames. WiFi
  runs on core 0 and is not touched.

## Testing

- **Native** (`pio test -e native`): `WakeScript` covers:
  - the intro plays once;
  - the loop repeats with period 3000;
  - `connected` before 2600 starts the finale at 2600;
  - the finale lasts 2000 from its start (ease included) and ends `done` with
    open = 256;
  - the hand-off is continuous: the pose at finale t = 0 equals the groggy
    pose at that same moment (±1 on every field), tested at several points in
    the intro and the loop.
- **Settings:** the `booteyes` key defaults to true when absent and
  round-trips, in the existing settings tests.
- **Builds:** every env through `ci_build.sh`. `supermini-proto` must build
  without the eye code; record the flash % change for `xiao-esp32s3`.
- **On the rig** (OTA, so NVS and the filesystem are kept):
  1. Boot with eyes on: groggy, then finale, then IP on orb 5, then the clock.
  2. Check that the caruncles face orb 3.
  3. Boot with eyes off: compare with today's boot.
  4. Toggle `invert` and check the eyes are upright and still face inward.
- **Not covered by a test:** the portal path needs a board with no saved WiFi.
  It is checked by reading the code unless a spare board can have its WiFi
  wiped.

## Out of scope

- Eyes as a page in the rotation, or eyes after boot.
- Choosing eye style or colors on the settings page. One look ships; the other
  example eyes (cat, dragon, owl…) stay available in source for a later change.
- DMA, or raising the SPI clock above 27 MHz.
- Changing the start-up picture or orb 1.
