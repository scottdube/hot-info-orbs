# VO read 2 — 2026-10-19

Second straight read of the narration, on the new **Hollyland Lark M2** (noise cancellation
on), audio only. Recorded through native AVFoundation, not ffmpeg: ffmpeg's capture crackled
this mic. Script as of commit 5485753, the same lines as the storyboard.
Recording: `~/Movies/hot-info-orbs-intro/vo/vo-read2-2026-10-19.wav` (5:12, 24-bit, peak
−8.9 dBFS, **0 clicks**; not in this repo). Timed the same way as read 1
([vo-practice-2026-10-18.md](vo-practice-2026-10-18.md)): whisper word times, each line
matched to its **last** take, first word to last word.

## Result

- **Pace 170 words a minute** (read 1: 174). Still well over the 150 the storyboard assumes.
- **11 of 33 lines run past their shot**, by 28 s in all (read 1: 13 lines, 50 s).
  Picture is 3:37; holding each shot until its line ends gives **4:06** (read 1: 4:29).
  That is just past the outline's 4:00 ceiling.
- **Biggest overruns:** S20a +6.0 s (the regulator line, down from +12.6 now that it's
  shorter), S03 +4.9 s, S09 +3.0 s, S15 +2.8 s, S16 +2.7 s, S13 +2.3 s, S30 +2.2 s.
- **S15 has a 6.2 s pause** after "yourself." The raw measure is 14.0 s; the line itself
  takes 7.8 s, and that figure is used above. Cut the pause in the edit.
- Not decided here: lengthen those shots, split them, or trim the lines. That is Scott's call.

## Words to check by ear

Whisper can mishear, so listen before changing anything:

- **S29:** heard "A new board **and** an even smaller chip". The script says "**for** an even
  smaller chip". "And" turns one product into two.
- **S20a / S20b:** heard "the display runs" and "the display carries". The script says
  "displays" (plural, all five).
- **S30:** reworded: "In the case… This one's still in design." The script says "And a case…
  That one is still a design." Fine for a voice-over if it sounds right.
- **S15:** "line values" for "live values". **S23:** "tone" for "town". Probably whisper.
- Fixed since read 1: S18 now says JLCPCB correctly, and S15 "tinkerers" became "coders"
  in the script.

## Line by line

| Shot | Shot length | Spoken | Over by | Read 1 | Said differently |
|---|---|---|---|---|---|
| S02 | 4 s | 2.0 s | ✓ | 4.0 s | — |
| S03 | 8 s | 12.9 s | ⚠ +4.9 s | 11.1 s | wi fi → wifi |
| S04 | 8 s | 5.7 s | ✓ | 7.4 s | — |
| S05 | 6 s | 6.9 s | ⚠ +0.9 s | 6.5 s | — |
| S06 | 5 s | 5.1 s | ✓ | 5.1 s | — |
| S07 | 8 s | 8.4 s | ✓ | 9.3 s | ∅ → it |
| S08 | 5 s | 2.4 s | ✓ | 2.7 s | all → ∅ |
| S09 | 5 s | 8.0 s | ⚠ +3.0 s | 9.5 s | in → ∅; twenty four hour → 24 hours |
| S10 | 5 s | 5.0 s | ✓ | 5.5 s | — |
| S11 | 5 s | 5.7 s | ⚠ +0.7 s | 9.0 s | — |
| S12 | 5 s | 4.4 s | ✓ | 5.3 s | — |
| S13 | 5 s | 7.3 s | ⚠ +2.3 s | 8.0 s | with → ∅ |
| S14 | 6 s | 4.5 s | ✓ | 4.1 s | step → steps |
| S15 | 5 s | 7.8 s* | ⚠ +2.8 s | 9.6 s | live → line |
| S15a | 4 s | 4.4 s | ✓ | 7.8 s | — |
| S16 | 5 s | 7.7 s | ⚠ +2.7 s | 7.9 s | — |
| S17 | 6 s | 6.9 s | ⚠ +0.9 s | 7.6 s | — |
| S18 | 9 s | 6.8 s | ✓ | 6.3 s | — |
| S20 | 6 s | 3.6 s | ✓ | 3.6 s | — |
| S20a | 4 s | 10.0 s | ⚠ +6.0 s | 16.6 s | displays run → display runs; ∅ → that; ∅ → got |
| S20b | 3 s | 4.3 s | ⚠ +1.3 s | 4.8 s | displays carry → display carries; and → ∅ |
| S21 | 11 s | 4.7 s | ✓ | 4.0 s | — |
| S22 | 6 s | 4.1 s | ✓ | 3.9 s | — |
| S23 | 12 s | 12.1 s | ✓ | 10.5 s | town → tone |
| S23a | 10 s | 9.9 s | ✓ | 8.5 s | — |
| S24 | 8 s | 5.9 s | ✓ | 5.0 s | — |
| S25 | 4 s | 4.4 s | ✓ | 3.7 s | — |
| S26 | 5 s | 1.0 s | ✓ | 1.3 s | — |
| S27 | 4 s | 2.5 s | ✓ | 2.8 s | — |
| S29 | 8 s | 7.9 s | ✓ | 6.6 s | for → and; the → ∅ |
| S30 | 6 s | 8.2 s | ⚠ +2.2 s | 9.4 s | and a → ∅; and → in; that one is → this one's; a → in |
| S31 | 4 s | 2.2 s | ✓ | 2.0 s | — |
| S32 | 5 s | 3.9 s | ✓ | 8.6 s (older, longer line) | — |

\* 14.0 s measured, less the 6.2 s pause after "yourself."
"✓" allows 0.5 s over, the same as the storyboard's ⚠ rule.
Hyphen and number spellings ("wifi", "handson", "24") are whisper's, not the reader's.

## Pickup: S25–S32, re-read the same day

Scott heard a couple of stumbles in S25–S32 and re-read them: `vo/vo-read2-pickup-S25-S32.wav`
(67 s, same Lark M2, **0 clicks**, peak −9.5 dBFS). Last take of each line:

| Shot | Shot length | Spoken | Over by | Read 2 | Notes |
|---|---|---|---|---|---|
| S25 | 4 s | 3.4 s | ✓ | 4.4 s | clean |
| S26 | 5 s | 1.3 s | ✓ | 1.0 s | clean |
| S27 | 4 s | 2.1 s | ✓ | 2.5 s | clean |
| S29 | 8 s | ~6 s* | ✓ | 7.9 s | now says "**for** an even smaller chip", as scripted; "the buttons" → "buttons" |
| S30 | 6 s | 6.9 s | ⚠ +0.9 s | 8.2 s | same rewording as read 2: "In the case… in finned end caps. It's all 3D printed. That one is still in design." |
| S31 | 4 s | ~2.6 s* | ✓ | 2.2 s | — |
| S32 | 5 s | 4.2 s | ✓ | 3.9 s | third take is word-perfect; the first two said "who first built it" |

\* Raw measure includes a pause the edit will cut: S29 11.1 s with ~5 s after "chip.",
S31 5.1 s with ~2.5 s after "at". Use the pickup for S25–S32 unless Scott's ear says otherwise.

## Retakes cut, one file for the edit

Every abandoned attempt is cut with `fc-retakes`, which keeps the last complete reading and cuts at
measured speech onsets. The original recordings are untouched; the projects are in `vo/edit/`.

- **Read 2:** four retakes cut (S03 opening, S10, S16, S22), 312.1 → 283.5 s.
- **Pickup:** two false starts of S32 cut, 67.0 → 57.0 s.
- **Joined:** read 2 through S24, then the pickup from S25, cut in the middle of the 0.9 s
  pause between them with a 30 ms crossfade. Result: **`vo/vo-read2-final-2026-10-19.wav`,
  4:42**. Checked by transcript: 33 of 33 lines found, each line once, **0 clicks**.
  (Whisper wrote "Any button wakes it" twice; the audio has one ~1 s phrase between two
  pauses there, and the second "button" has zero length. That's whisper's error, not the audio's.)
- Not cut: the pauses inside S15 (~6 s), S29 (~5 s) and S31 (~2.5 s). Those are a job for
  `fc-cut-silences` or the edit.
