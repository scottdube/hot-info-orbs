# Custom pages: Web Data and MQTT from the settings page

Date: 2026-10-17 · Status: approved design, awaiting spec review · Branch:
`custom-pages` (off `main`)

## Why

Members want to drop pages they don't care about and put something of their
own in the rotation. One member's example: no interest in stocks, wants
something else there. Turning built-in pages on and off already ships
(`hidden` key, the Show checkboxes). What's missing is a page whose content the
member chooses, set up **without rebuilding the firmware**.

The firmware already has two such pages from upstream, both switched on only at
build time:

- **Web Data** (`WebDataWidget`) fetches a JSON URL that describes what to draw
  on each orb: label, value text, colors, and shapes (arc, circle, line,
  rectangle, triangle, image, character). URL in `secrets.h`
  (`WEB_DATA_WIDGET_URL`).
- **MQTT** (`MQTTWidget`) subscribes to a broker and shows a value per orb,
  laid out by a JSON message on a setup topic. Broker, port, setup topic in
  `config.h`; user and password in `secrets.h`.

Member uses this design serves:

| Use | Page |
|---|---|
| Home Assistant values (temperatures, door states, power) | MQTT |
| A fixed message or picture from a static file (GitHub Pages, a gist) | Web Data over HTTPS |
| A local events schedule turned into JSON by a script elsewhere | Web Data (the script is out of scope) |

## Decisions

- **Both pages are compiled into every build** and stay off until configured
  on `/settings` (approach A). Measured cost with both enabled: flash 85.0% →
  87.0% of a 1,966,080-byte slot (probe build of `main` 454f340, 2026-10-17),
  under the 92% stop line. The production XIAO board has at least twice the
  flash. Rejected: keeping the build-time switch (still needs one rebuild,
  which is the step this removes) and one combined Custom page with a source
  choice (the two draw differently; merging them buys nothing yet).
- **MQTT content is set per orb on the settings page** (topic, label, unit).
  The setup-topic layout JSON stays as an advanced override. Rejected: settings
  page holds only broker details and members publish layout JSON from Home
  Assistant (too much for a club build).
- **Built on `main`**, because the only orb to test on is a v1.1 SuperMini.
  Merged into `xiao-esp32s3-plus` after it lands on `main`.

## 1. Settings page and storage

A **Custom pages** section on `/settings`, below Weather.

### Web Data block

| Field | Key | Rule | Default |
|---|---|---|---|
| Page name | `wdname` | 1–16 chars | "Web Data" |
| URL | `wdurl` | blank, or `http://` / `https://`, ≤ 256 chars | `WEB_DATA_WIDGET_URL` if defined, else blank |

- Blank URL = no page (not added to the rotation, not on the Show list).
  Saving already restarts the orb, so pages are registered at boot from the
  stored values; nothing is added or removed at run time.
- **The stored URL is never shown in full.** The field shows it with the query
  string replaced: `https://host/path?•••`. A URL without a query string shows
  as stored. Submitting the field unchanged (equal to the masked string) keeps
  the stored URL. Any other value replaces it; blank clears it.
- Status line: ✔ / ✖ and the last fetch result (HTTP code, `bad JSON`, `too
  big`, `timeout`), as the Tempest rows do.

### MQTT block

| Field | Key | Rule | Default |
|---|---|---|---|
| Page name | `mqname` | 1–16 chars | "MQTT" |
| Broker host | `mqhost` | blank, or hostname / IPv4, ≤ 64 chars | `MQTT_WIDGET_HOST` if defined |
| Port | `mqport` | 1–65535 | `MQTT_WIDGET_PORT` or 1883 |
| Username | `mquser` | ≤ 64 chars | `MQTT_WIDGET_USER` or blank |
| Password | `mqpass` | ≤ 64 chars, write-only | `MQTT_WIDGET_PASS` or blank |
| Orb 1–5 topic | `mqt1`…`mqt5` | ≤ 128 chars, no `#`/`+` wildcards | blank |
| Orb 1–5 label | `mql1`…`mql5` | ≤ 16 chars | blank |
| Orb 1–5 unit | `mqu1`…`mqu5` | ≤ 8 chars | blank |
| Layout topic (advanced) | `mqsetup` | ≤ 128 chars | `MQTT_SETUP_TOPIC` or blank |

- Blank host = no page.
- **Password is never sent to the browser.** The field is empty; blank on
  save keeps the stored one; a "clear password" checkbox removes it. The
  page shows "password set" / "no password".
- A blank topic leaves that orb blank.
- Status line: ✔ connected / ✖ with the reason (`can't reach broker`,
  `login refused`), and the age of the newest value.

### Storage

- New keys in NVS namespace `orbs` (all ≤ 15 chars, the NVS key limit). Same
  pattern as the keys added after schema 1: no schema bump; an absent key
  falls back to its default.
- `config.h` / `secrets.h` defines become **defaults only**, so a build that
  already sets `WEB_DATA_WIDGET_URL` or the MQTT defines keeps working with no
  settings saved.
- Every field is validated before save, with ✔ / ✖ / ⚠ beside it like the
  rest of the page. Validation lives in `SettingsValidation.h` (host-testable).
- The Show list uses the page names. `hidden` stores names, so renaming a page
  un-hides it. That is acceptable and documented beside the field.

## 2. Page behavior

### Web Data

- Registered in the rotation only when `wdurl` is set.
- **Fetches only while in the rotation:** once at start-up (`updateAll` already
  skips hidden pages), then on its timer while it is on screen.
- **Refresh:** the JSON's `interval` (ms, as upstream) is honored, **clamped to
  ≥ 10 s**. With no `interval`, **60 s**. (Upstream default is 1 s.)
- **HTTPS:** same client path as the weather pages. No certificate check
  (out of scope).
- **Limits:** 5 s connect/read timeout. The reply is parsed from the stream,
  not read into a `String`, and refused above **16 KB** (✖ `too big`).
- **On failure:** keep drawing the last good data. With no good data yet, the
  center orb shows the page name and the reason.
- **Format:** unchanged upstream format (`interval`, `displays[]` with label,
  data, colors, `elements[]`). Documented in `docs/CUSTOM-PAGES.md` with a
  sample message page served from this repo's GitHub Pages.

### MQTT

- Registered in the rotation only when `mqhost` is set.
- **Never blocks.** Upstream `reconnect()` loops `while (!connected)` forever:
  with a mistyped broker the orb would freeze with no buttons and no settings
  page to fix it on. Replaced with **one short attempt (socket timeout 3 s),
  then at most one retry every 30 s**. The retry timing is a pure function (like `RoamPolicy.h`) with
  host tests. While disconnected the page shows ✖ and the orb keeps running.
- **Stays connected in the background** while the page is in the rotation:
  `mqttClient.loop()` runs from the main loop, not only when the page is on
  screen, so values are current when it comes up.
- **Layout from the rows:** label at the top, the value large in the middle,
  the unit after it; the value's font steps down until it fits the orb. No
  value yet = "–".
- A value is the topic's payload as text. Home Assistant MQTT Statestream or an
  automation publishing a state both produce this.
- If a JSON message arrives on the layout topic, its layout (upstream format)
  replaces the rows until restart, as upstream does today.

## 3. Testing, docs, scope

### Host tests (`pio test -e native`, run in CI)

- URL masking, and unchanged-masked-value keeps the stored URL.
- Each new field's validation (lengths, URL scheme, host, port range, topic
  wildcards).
- Refresh clamp: missing → 60 s, 2 s → 10 s, 300 s → 300 s.
- MQTT retry policy: first attempt immediate, then no attempt within 30 s of
  the last.

### On the orb (each result recorded with what was seen)

1. Sample message page from GitHub Pages over HTTPS draws on all five orbs.
2. Unreachable URL: ✖ on the page and on `/settings`; buttons and settings page
   still respond.
3. A large web page as the URL: refused as `too big`, no restart.
4. MQTT against the home broker on the orb's own network shows live Home
   Assistant values. Needs an orb user added to the broker's ACL (owner's
   step).
5. Wrong broker host: ✖ shown, buttons and settings page still respond (the
   hang fix).
6. Flash percentage of the final build, recorded.

### Docs

- New `docs/CUSTOM-PAGES.md`: the Web Data JSON format with a sample, the
  sample page's URL, and a Home Assistant recipe (MQTT Statestream, or an
  automation that publishes).
- `docs/features.html` and `docs/REQUIREMENTS.md` updated.
- `config.h.template` / `secrets.h.template` comments: the defines are now
  defaults, settable on `/settings`.

### Out of scope

- The script that turns a local events schedule into Web Data JSON (needs a
  data source; its own project).
- More than one page of each kind.
- HTTPS certificate checking.
- MQTT over TLS.
