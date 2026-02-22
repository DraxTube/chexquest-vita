#!/usr/bin/env python3
import struct, zlib, os, io

def create_png(width, height, r, g, b, a=255):
    """Create a valid RGBA PNG file that PS Vita accepts"""
    
    out = io.BytesIO()
    
    # PNG Signature
    out.write(b'\x89PNG\r\n\x1a\n')
    
    def write_chunk(name, data):
        out.write(struct.pack('>I', len(data)))
        out.write(name)
        out.write(data)
        crc = zlib.crc32(name + data) & 0xFFFFFFFF
        out.write(struct.pack('>I', crc))
    
    # IHDR chunk - 13 bytes
    # Width(4) Height(4) BitDepth(1) ColorType(1) Compression(1) Filter(1) Interlace(1)
    ihdr = struct.pack('>II', width, height) + struct.pack('BBBBB', 8, 6, 0, 0, 0)
    write_chunk(b'IHDR', ihdr)
    
    # Build raw pixel data (RGBA, filter byte per row)
    raw = bytearray()
    row = bytearray()
    row.append(0)  # filter: none
    for x in range(width):
        row.append(r)
        row.append(g)
        row.append(b)
        row.append(a)
    
    for y in range(height):
        raw.extend(row)
    
    # IDAT chunk - compressed pixel data
    compressed = zlib.compress(bytes(raw), 9)
    write_chunk(b'IDAT', compressed)
    
    # IEND chunk
    write_chunk(b'IEND', b'')
    
    return out.getvalue()

os.makedirs('sce_sys/livearea/contents', exist_ok=True)

# icon0.png - 128x128 RGBA
data = create_png(128, 128, 44, 100, 180)
with open('sce_sys/icon0.png', 'wb') as f:
    f.write(data)
print(f"icon0.png: {len(data)} bytes (128x128)")

# bg.png - 840x500 RGBA
data = create_png(840, 500, 20, 20, 50)
with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(data)
print(f"bg.png: {len(data)} bytes (840x500)")

# startup.png - 280x158 RGBA
data = create_png(280, 158, 44, 100, 180)
with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(data)
print(f"startup.png: {len(data)} bytes (280x158)")

# Quick verify
for p in ['sce_sys/icon0.png', 'sce_sys/livearea/contents/bg.png', 'sce_sys/livearea/contents/startup.png']:
    with open(p, 'rb') as f:
        h = f.read(8)
        f.seek(16)
        w = struct.unpack('>I', f.read(4))[0]
        h2 = struct.unpack('>I', f.read(4))[0]
    sz = os.path.getsize(p)
    print(f"  CHECK {p}: {sz} bytes, PNG valid: {h == b'\x89PNG\r\n\x1a\n'}, {w}x{h2}")

print("Done!")
