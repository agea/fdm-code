#!/usr/bin/env python3
"""Derive local Unreal branding from the user-supplied originals (Pillow)."""
from pathlib import Path
import json

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ORIGINALS = ROOT / "Branding/Originals"


def resized(name, bounds):
    with Image.open(ORIGINALS / name) as source:
        image = source.convert("RGBA")
    image.thumbnail(bounds, Image.Resampling.LANCZOS)
    return image


def save(image, relative, **options):
    path = ROOT / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, **options)
    print(f"{relative}: {image.width}x{image.height}")


def mac_asset_catalog(icon):
    catalog = ROOT / "Build/Mac/Resources/Assets.xcassets"
    appicon = catalog / "AppIcon.appiconset"
    appicon.mkdir(parents=True, exist_ok=True)
    info = {"author": "xcode", "version": 1}
    (catalog / "Contents.json").write_text(json.dumps({"info": info}, indent=2) + "\n")
    images = []
    for size in (16, 32, 128, 256, 512):
        for scale in (1, 2):
            name = f"icon_{size}x{size}" + ("@2x" if scale == 2 else "") + ".png"
            icon.resize((size * scale, size * scale), Image.Resampling.LANCZOS).save(appicon / name, optimize=True)
            images.append({"filename": name, "idiom": "mac", "scale": f"{scale}x", "size": f"{size}x{size}"})
    (appicon / "Contents.json").write_text(json.dumps({"images": images, "info": info}, indent=2) + "\n")


if __name__ == "__main__":
    save(resized("logo.png", (1024, 1024)), "Branding/logo.png", optimize=True)
    save(resized("splash.png", (2560, 1441)).convert("RGB"),
         "Branding/background.jpg", quality=92, optimize=True)
    icon = resized("icon.png", (1024, 1024))
    save(icon, "Branding/icon.png", optimize=True)
    save(icon, "Build/Mac/Application.icns")
    mac_asset_catalog(icon)
    save(icon, "Build/Windows/Application.ico",
         sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    splash = resized("splash.png", (1600, 900)).convert("RGB")
    save(splash, "Content/Splash/Splash.png", optimize=True)
    save(splash, "Content/Splash/EdSplash.png", optimize=True)
    save(resized("icon.png", (256, 256)), "Content/Splash/Icon.png", optimize=True)
