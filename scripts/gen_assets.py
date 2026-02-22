#!/usr/bin/env python3
import struct, zlib, os, io

def create_png(width, height, r, g, b, a=255):
    out = io.BytesIO()
    out.write(b'\x89PNG\r\n\x1a\n')
    
    def write_chunk(name, data):
        out.write(struct.pack('>I', len(data)))
        out.write(name)
        out.write(data)
        crc = zlib.crc32(name + data) & 0xFFFFFFFF
        out.write(struct.pack('>I', crc))
    
    ihdr = struct.pack('>II', width, height) + struct.pack('BBBBB', 8, 6, 0, 0, 0)
    write_chunk(b'IHDR', ihdr)
    
    raw = bytearray()
    row = bytearray()
    row.append(0)
    for x in range(width):
        row.append(r)
        row.append(g)
        row.append(b)
        row.append(a)
    
    for y in range(height):
        raw.extend(row)
    
    compressed = zlib.compress(bytes(raw), 9)
    write_chunk(b'IDAT', compressed)
    write_chunk(b'IEND', b'')
    
    return out.getvalue()

os.makedirs('sce_sys/livearea/contents', exist_ok=True)

data = create_png(128, 128, 44, 100, 180)
with open('sce_sys/icon0.png', 'wb') as f:
    f.write(data)
print("icon0.png: %d bytes" % len(data))

data = create_png(840, 500, 20, 20, 50)
with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(data)
print("bg.png: %d bytes" % len(data))

data = create_png(280, 158, 44, 100, 180)
with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(data)
print("startup.png: %d bytes" % len(data))

png_sig = b'\x89PNG\r\n\x1a\n'
for p in ['sce_sys/icon0.png', 'sce_sys/livearea/contents/bg.png', 'sce_sys/livearea/contents/startup.png']:
    sz = os.path.getsize(p)
    with open(p, 'rb') as f:
        valid = f.read(8) == png_sig
    print("  %s: %d bytes, valid PNG: %s" % (p, sz, valid))

print("Done!")
