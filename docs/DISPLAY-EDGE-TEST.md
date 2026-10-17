# Display edge test (enclosure design)

What part of each 1.28" GC9A01 round module is usable, measured with
`pio run -e displaytest -t upload` (`firmware/src/displaytest/main.cpp`).
It answers the enclosure question of whether there is dead black space between
the lit area and the glass edge.

## Results

| Date | Orb | Pattern | Seen | By |
|---|---|---|---|---|
| 2026-10-17 | SuperMini v1.1, MAC …ea:ab:84 | 2 RINGS | **5 of 5 white bands visible**: the outermost (r 118–120 px) shows, so the panel renders out to r=120, the full 240 px | Scott, by eye |
| 2026-10-17 | same | 1 FULL | **a dark band sits outside the white**: the glass extends past the active area | Scott, by eye |
| 2026-10-17 | same | 1 FULL, calipers | **lit circle ≈32.5 mm; dark band ≈1.5 mm wide, even all the way round**, so the glass is ≈35.5 mm | Scott, "roughly" |
| 2026-10-17 | same | 4 BEYOND | **3 of 5 rings visible**: the two drawn past r=120 (r 122–124, 126–128, which can only appear as diagonal arcs) do not show | Scott, by eye |

**There are no pixels under the dark band.** BEYOND drew past r=120 and nothing
showed, so the 1.5 mm band is plain black glass that no firmware change can
light. The enclosure can cover it freely.

**For the enclosure:** a round opening of ~32.5 mm shows every pixel and none of
the dark band. Anything wider shows up to 1.5 mm of black glass per side, out to
~35.5 mm. The band looked even all round, so by eye the active area is centered
in the glass. These are rough caliper readings (Scott's word); measure the
module's own outline before setting a pocket size.

Not yet checked: whether all five displays match the one measured.

## Scale

240 px across ≈32.5 mm measured (the nominal figure is 32.4 mm) gives
≈0.135 mm/px, so one ring band is about 0.27 mm.
