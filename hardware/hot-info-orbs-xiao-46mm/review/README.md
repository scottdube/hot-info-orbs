# Review package: rev 2.5 run 3 (2026-10-25)

What went to Yang for sign-off before the JLCPCB order. Exported from the
same board and schematic as `fab/hot_info_orbs_xiao_46mm_2.5_run3_gerbers.zip`
(commit eee08bc).

- `hot_info_orbs_xiao_46mm_2.5_schematic.pdf`: the schematic, one page.
- `hot_info_orbs_xiao_46mm_2.5_board.pdf`: four pages, all at 1:1 on A4, so a
  print can be held against a display module:
  1. front assembly: silk, pad openings and outline, black and white;
  2. back assembly, the same, mirrored so it reads as seen from the back;
  3. front copper;
  4. back copper, mirrored.

  The assembly pages come first because the ground pour floods each copper
  page solid, which hides the silk and pads a reviewer is checking. That is
  why a single copper+silk page per face was rejected.
- `..._render_top.png`, `..._render_bottom.png`: copies of `fab/46mm_*.png`.

The PDFs are not regenerated in place: a later revision gets its own files.
