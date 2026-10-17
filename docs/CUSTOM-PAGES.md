# Custom pages

## What it is

Two pages of your own that you set up on the orb's settings page
(`http://<orb>/settings`, the **Custom pages** section):

- **Web Data** draws a JSON file from any `http://` or `https://` address.
- **MQTT** shows one live value per orb from an MQTT broker, for example
  Home Assistant.

A page appears in the rotation once its address (Web Data) or broker (MQTT) is
set, and not before. Saving restarts the orb. You can untick a custom page
under **Rotation → Show** like any other page. The tick box uses the page's
name, so renaming a page shows it again if it was unticked.

No rebuild is needed. `WEB_DATA_WIDGET_URL` (in `secrets.h`) and the `MQTT_*`
defines (in `config.h` and `secrets.h`) are only defaults for an orb that has
never saved settings. `WEB_DATA_STOCK_WIDGET_URL` is not: it still adds a
separate "WebData" page at build time, which is not set up on the settings
page (untick it under Rotation → Show to hide it).

## Web Data format

The address must return JSON with two top-level keys:

| Key | Meaning |
|---|---|
| `interval` | Refresh time in milliseconds. At least `10000` (10 s); anything lower is raised to 10 s. `60000` (60 s) if left out. |
| `displays` | Up to five objects, the left orb first. Extra objects are ignored. |

(A bare array of display objects, with no `interval`, is also accepted.)

Each display object may have:

| Key | Meaning |
|---|---|
| `label` | Small text near the top of the orb. |
| `data` | The main text. It wraps at 10 characters per line, so keep each word short. Instead of text it can be an array of drawing elements (text, character, line, rectangle, triangle, circle, arc, image), as upstream's Web Data page. |
| `color` | Color of `data`. |
| `labelColor` | Color of `label`. It does not follow `color`: set it separately or the label is white. |
| `background` | Background color of the orb. |
| `fullDraw` | `true` to clear and repaint the whole orb on every refresh. Default `false`. Set it to `true` if the text changes between refreshes, or a shorter new value can leave pieces of the old one behind. |

Anything left out uses white text on black.

**Colors** are names, not hex codes. Capital letters and spaces do not matter.
The names the orb accepts:

`black`, `navy`, `darkgreen`, `darkcyan`, `maroon`, `purple`, `olive`,
`lightgray` (also `gray`, `lightgrey`, `grey`), `darkgray` (also `darkgrey`),
`blue`, `green`, `cyan`, `red`, `magenta`, `yellow`, `white`, `orange`,
`greenyellow`, `pink`, `brown`, `gold`, `silver`, `skyblue`, `violet`.
An unknown name draws black.

**Limits.** The reply must be 16 KB or smaller, it is given 5 seconds to
connect and 5 seconds per read, and https addresses are not certificate-checked.

**When a fetch fails** the orbs keep the last good data on screen. Before any
good data has arrived, the center orb shows the page name and the reason (for
example `timeout`, `can't connect`, `HTTP 404`, `too big`, `bad JSON`).
`timeout` means nothing answered within 5 seconds (a host that is off, or a
reply that stalled); `can't connect` means the host refused at once or the
name did not resolve. The settings page shows the same reason next to a
failure mark. After a failure the orb waits at least a minute before trying
again, so a silent host does not slow the buttons.

**Keys in the address.** Only the part after `?` is hidden on the settings
page, so put API keys there. A key in the path, or `user:pass@` in front of
the host, shows in full.

### Sample

This is `docs/custom-pages/sample.json`:

```json
{
  "interval": 300000,
  "displays": [
    { "label": "HOT", "data": "Info Orbs", "color": "white", "background": "navy" },
    { "label": "This is", "data": "a custom", "color": "yellow" },
    { "label": "page from", "data": "a JSON", "color": "cyan" },
    { "label": "file you", "data": "host", "color": "green" },
    { "label": "Edit me", "data": "anywhere", "color": "orange" }
  ]
}
```

Its address: `https://scottdube.github.io/hot-info-orbs/custom-pages/sample.json`.

## MQTT

Fields on the settings page:

- **Page name** (16 characters).
- **Broker host name or IP address.** Blank means no MQTT page.
- **Port** (default 1883).
- **Username**, blank for no login.
- **Password.** Write-only: the page never shows it back. Leave the field blank
  to keep the stored one, or tick **Clear the stored password**.
- **One value per orb: topic, label, unit.** Five rows, orbs left to right.
  A blank topic leaves that orb blank. No `#` or `+` wildcards. The label is up
  to 16 characters and the unit up to 8.
- **Layout topic (advanced).** A JSON layout published on this topic replaces
  the rows until the next restart (see `firmware/src/widgets/mqttwidget/README.md`).

A value is the topic's payload as text. Each orb shows the label on top, the
value in the middle at the largest size that fits and the unit
below. It shows a dash (-) until the first value arrives.

**It never blocks the orb.** The orb makes one connection attempt that gives
up after a few seconds, then at most one more every 30 seconds, while the buttons and
the settings page keep working. The settings page says why it is not connected:
`can't reach broker`, `login refused`, `broker refused the connection`,
`connection lost`, or `not connected`.

## Home Assistant recipe

Either of these gives an orb a topic to read.

**(a) MQTT Statestream** publishes every state change of the entities you list:

```yaml
mqtt_statestream:
  base_topic: homeassistant/statestream
  include:
    entities:
      - sensor.outdoor_temperature
```

The topic for an orb is
`homeassistant/statestream/sensor/outdoor_temperature/state`.

**(b) An automation** publishes whatever you choose to a topic you choose:

```yaml
- alias: "Orb: garage door"
  triggers:
    - trigger: state
      entity_id: cover.garage_door
  actions:
    - action: mqtt.publish
      data:
        topic: orbs/garage
        payload: "{{ states('cover.garage_door') }}"
        retain: true
```

With `retain: true` the broker hands the last value to the orb as soon as it
subscribes, so the page is not blank after a restart. The orb test below
used (b); Statestream's retain behavior was not tested.

## Tested

On-orb tests, run by hand. Each result records what was done and what was seen.

| # | Test | Result | Date |
|---|---|---|---|
| 1 | The sample page from GitHub Pages over https draws on all five orbs | ✔ Fetched over https from the raw GitHub copy, ✔ in 0.5–0.7 s on `/settings`; all five orbs drew the sample's text and colors, checked by eye and photo. One earlier look, before a restart, showed other content on four orbs and the page name on the center one; it did not recur and its cause is not known | 2026-10-17 |
| 2 | An unreachable address shows the failure on the page and on `/settings`; buttons and the settings page still respond | ✔ `http://10.255.255.1/x.json`: ✖ after 5.0 s on `/settings`. The reason read `connection refused` although the try ran the full 5 s; the wording was fixed after this run (a silent host now reads `timeout`, a quick refusal `can't connect`) and the re-run on build 33230ff read `✖ … took 5.0 s — timeout`. `/settings` answered in 0.08–0.39 s throughout a 75 s sample. The center orb showed the page name and `timeout`, checked by eye; from that page left and right stepped to the previous and next pages; the middle button does nothing there, as designed | 2026-10-17 |
| 3 | A large web page as the address is refused as `too big`, with no restart | ✔ A 344 KB Wikipedia page: ✖ `too big` after 1.0 s, refetched while on screen, uptime kept counting (no restart) | 2026-10-17 |
| 4 | MQTT against a home broker shows live Home Assistant values | ✔ Mosquitto add-on on a Home Assistant host, orb with its own broker user. A temperature sensor's current state sent with HA's `mqtt.publish` service to the orb's topic: `/settings` read `✔ Connected. Newest value under a minute ago`, and the value showed on orb 1, checked by eye. Before the login was set: `✖ login refused`, at once. Statestream not tested | 2026-10-17 |
| 5 | A wrong broker host shows the failure; buttons and the settings page still respond | ✔ Broker `10.255.255.1`: ✖ `can't reach broker`, each try took 3.0 s. Sampling `/settings` every second for 60 s: replies in 0.08–0.67 s except one 2.1–2.5 s stall at each retry, about 30 s apart | 2026-10-17 |
| 6 | Flash use of the final build, as a share of the OTA slot (stop line 92%) | ✔ 87.9% (1,728,477 of 1,966,080 bytes) | 2026-10-17 |
