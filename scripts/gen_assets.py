#!/usr/bin/env python3
import struct, zlib, os

def create_png(w, h, r, g, b):
    def chunk(t, d):
        c = t + d
        return struct.pack('>I', len(d)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    raw = b''
    for _ in range(h):
        raw += b'\x00' + bytes([r, g, b]) * w
    idat = chunk(b'IDAT', zlib.compress(raw))
    iend = chunk(b'IEND', b'')
    return sig + ihdr + idat + iend

os.makedirs('sce_sys/livearea/contents', exist_ok=True)
with open('sce_sys/icon0.png', 'wb') as f:
    f.write(create_png(128, 128, 74, 134, 200))
with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(create_png(840, 500, 30, 30, 60))
with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(create_png(280, 158, 74, 134, 200))
print("Assets generated OK")
