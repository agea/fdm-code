#!/usr/bin/env python3
from pathlib import Path
from urllib.request import Request, urlopen
url = "https://raw.githubusercontent.com/google/fonts/main/ofl/teko/Teko%5Bwght%5D.ttf"
out = Path(__file__).resolve().parent / "Fonts" / "Teko-Variable.ttf"
req = Request(url, headers={"User-Agent": "Respawn-and-Rage-Font-Installer/1.0"})
with urlopen(req, timeout=30) as response:
    data = response.read()
if len(data) < 10000 or data[:4] not in (b"\x00\x01\x00\x00", b"OTTO"):
    raise RuntimeError("Il file ricevuto non sembra essere un font TTF/OTF valido")
out.parent.mkdir(parents=True, exist_ok=True)
out.write_bytes(data)
print(f"Font salvato: {out} ({len(data)} byte)")
