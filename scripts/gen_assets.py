#!/usr/bin/env python3
"""Generate PS Vita–compliant sce_sys PNG assets.

Output images are strictly RGB (colour-type 2, 8-bit), with zero
metadata chunks — exactly what the Vita firmware expects.
"""
import struct, zlib, os, io


def create_png_rgb(width, height, r, g, b):
    """Return bytes of a minimal RGB PNG (no alpha, no metadata)."""
    out = io.BytesIO()
    out.write(b'\x89PNG\r\n\x1a\n')

    def write_chunk(name, data):
        out.write(struct.pack('>I', len(data)))
        out.write(name)
        out.write(data)
        crc = zlib.crc32(name + data) & 0xFFFFFFFF
        out.write(struct.pack('>I', crc))

    # IHDR — colour-type 2 = RGB (no alpha)
    ihdr = struct.pack('>II', width, height) + struct.pack('BBBBB', 8, 2, 0, 0, 0)
    write_chunk(b'IHDR', ihdr)

    # IDAT — unfiltered scanlines (filter byte 0 per row)
    raw = bytearray()
    row = bytearray()
    row.append(0)          # filter byte
    for _ in range(width):
        row.append(r)
        row.append(g)
        row.append(b)

    for _ in range(height):
        raw.extend(row)

    compressed = zlib.compress(bytes(raw), 9)
    write_chunk(b'IDAT', compressed)
    write_chunk(b'IEND', b'')

    return out.getvalue()


# ── Generate into sce_sys/ ──────────────────────────────────────────
os.makedirs('sce_sys/livearea/contents', exist_ok=True)

# icon0.png  – 128 × 128 RGB
data = create_png_rgb(128, 128, 44, 100, 180)
with open('sce_sys/icon0.png', 'wb') as f:
    f.write(data)
print("icon0.png: %d bytes" % len(data))

# bg.png – 840 × 500 RGB
data = create_png_rgb(840, 500, 20, 20, 50)
with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(data)
print("bg.png: %d bytes" % len(data))

# startup.png – 280 × 158 RGB
data = create_png_rgb(280, 158, 44, 100, 180)
with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(data)
print("startup.png: %d bytes" % len(data))

# ── Validate ────────────────────────────────────────────────────────
png_sig = b'\x89PNG\r\n\x1a\n'
for p in ['sce_sys/icon0.png',
          'sce_sys/livearea/contents/bg.png',
          'sce_sys/livearea/contents/startup.png']:
    sz = os.path.getsize(p)
    with open(p, 'rb') as f:
        hdr = f.read(8)
        valid = hdr == png_sig
        # Read IHDR to confirm colour type
        f.read(4)   # length
        f.read(4)   # 'IHDR'
        ihdr_data = f.read(13)
        colour_type = ihdr_data[9] if len(ihdr_data) >= 10 else -1
    ct_name = {0: 'Grey', 2: 'RGB', 3: 'Palette', 4: 'Grey+A', 6: 'RGBA'}.get(colour_type, '?')
    print("  %s: %d bytes, valid PNG: %s, colour: %s" % (p, sz, valid, ct_name))

print("Done!")
