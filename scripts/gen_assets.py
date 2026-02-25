#!/usr/bin/env python3
"""Convert assets/ PNGs to 8-bit indexed (palette) PNGs for PS Vita sce_sys.

PS Vita firmware (error 0x8010113D) requires ALL sce_sys images to be
8-bit indexed PNG — not 24-bit RGB, not 32-bit RGBA.
"""
from PIL import Image, PngImagePlugin
import os

SPECS = [
    ("assets/icon0.png",   "sce_sys/icon0.png",                      (128, 128)),
    ("assets/bg.png",      "sce_sys/livearea/contents/bg.png",       (840, 500)),
    ("assets/startup.png", "sce_sys/livearea/contents/startup.png",  (280, 158)),
]

os.makedirs("sce_sys/livearea/contents", exist_ok=True)

for src, dst, size in SPECS:
    if not os.path.isfile(src):
        print("SKIP (not found): " + src)
        continue

    img = Image.open(src)
    # Convert to RGB first (strip alpha)
    img = img.convert("RGB")
    # Resize to Vita-required dimensions
    img = img.resize(size, Image.LANCZOS)
    # Quantize to 256-colour palette = 8-bit indexed PNG
    img = img.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    # Save with empty PngInfo (no metadata chunks)
    pnginfo = PngImagePlugin.PngInfo()
    img.save(dst, "PNG", optimize=True, pnginfo=pnginfo)

    # Verify
    sz = os.path.getsize(dst)
    v = Image.open(dst)
    print("%s: %dx%d mode=%s  %d bytes" % (dst, v.size[0], v.size[1], v.mode, sz))

print("Done!")
