#!/usr/bin/env python3
"""Generate placeholder PNG assets for PS Vita LiveArea"""

import struct
import zlib
import os

def create_png(width, height, r, g, b):
    """Create a minimal valid PNG file"""
    def chunk(chunk_type, data):
        c = chunk_type + data
        crc = struct.pack('>I', zlib.crc32(c) & 0xffffffff)
        return struct.pack('>I', len(data)) + c + crc

    signature = b'\x89PNG\r\n\x1a\n'
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    ihdr = chunk(b'IHDR', ihdr_data)

    raw_data = b''
    for y in range(height):
        raw_data += b'\x00'  # filter byte
        raw_data += bytes([r, g, b]) * width

    compressed = zlib.compress(raw_data)
    idat = chunk(b'IDAT', compressed)
    iend = chunk(b'IEND', b'')

    return signature + ihdr + idat + iend

os.makedirs('sce_sys/livearea/contents', exist_ok=True)

# icon0.png - 128x128, blue
with open('sce_sys/icon0.png', 'wb') as f:
    f.write(create_png(128, 128, 74, 134, 200))
print("Created sce_sys/icon0.png")

# bg.png - 840x500, dark blue
with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(create_png(840, 500, 30, 30, 60))
print("Created sce_sys/livearea/contents/bg.png")

# startup.png - 280x158, blue
with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(create_png(280, 158, 74, 134, 200))
print("Created sce_sys/livearea/contents/startup.png")

print("All assets generated successfully!")
