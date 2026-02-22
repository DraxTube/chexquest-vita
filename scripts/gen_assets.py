#!/usr/bin/env python3
import struct, zlib, os

def create_rgba_png(width, height, r, g, b):
    """PNG with RGBA color type 6 - required by PS Vita"""
    
    def chunk(ctype, data):
        c = ctype + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF)
    
    sig = b'\x89PNG\r\n\x1a\n'
    
    # Color type 6 = RGBA
    ihdr = chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
    
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter none
        for x in range(width):
            raw.append(r)
            raw.append(g)
            raw.append(b)
            raw.append(255)  # alpha
    
    idat = chunk(b'IDAT', zlib.compress(bytes(raw), 9))
    iend = chunk(b'IEND', b'')
    
    return sig + ihdr + idat + iend

os.makedirs('sce_sys/livearea/contents', exist_ok=True)

with open('sce_sys/icon0.png', 'wb') as f:
    f.write(create_rgba_png(128, 128, 44, 100, 180))

with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(create_rgba_png(840, 500, 20, 20, 50))

with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(create_rgba_png(280, 158, 44, 100, 180))

print("Assets OK (RGBA format)")
