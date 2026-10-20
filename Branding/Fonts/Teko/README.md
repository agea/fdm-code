# Teko for Respawn & Rage

The user-supplied package contains `download_teko.py`, `CREDITS.txt`, the complete
SIL OFL 1.1 license and the original instructions (`SourceInstructions.it.md`).
Its download script was inspected and used unchanged on 20 October 2026.

Source: https://github.com/google/fonts/tree/main/ofl/teko
Specimen: https://fonts.google.com/specimen/Teko

`Fonts/Teko-Variable.ttf` is the original Google Fonts variable font. The game
uses `Fonts/Teko-SemiBold.ttf`, a static weight-600 instance generated with
fonttools 4.67.0 by `make_static.py`. The instance removes variation tables;
letter outlines, character coverage, copyright and license are preserved.

To regenerate, run `python3 download_teko.py`, then `python3 make_static.py`
in an environment with fonttools. No Python or runtime font download is needed
by players. Native Slate uses the static file through its composite font.

Runtime staging includes the static font, `CREDITS.txt` and the complete
`ThirdPartyLicenses/Teko-OFL.txt`. The isolated Mac/Linux build inputs retain
the entire directory. The OFL applies to this font, not the game's code/assets.
