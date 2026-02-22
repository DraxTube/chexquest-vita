#!/usr/bin/env python3
"""Generate valid PNG assets for PS Vita LiveArea"""
import struct, zlib, os

def create_vita_png(width, height, r, g, b):
    """Create a proper PNG that PS Vita accepts"""
    
    def write_chunk(chunk_type, data):
        chunk = chunk_type + data
        crc = zlib.crc32(chunk) & 0xFFFFFFFF
        return struct.pack('>I', len(data)) + chunk + struct.pack('>I', crc)
    
    # PNG signature
    png = b'\x89PNG\r\n\x1a\n'
    
    # IHDR: width, height, bit depth 8, color type 2 (RGB), compression 0, filter 0, interlace 0
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    png += write_chunk(b'IHDR', ihdr_data)
    
    # IDAT: image data
    raw_data = bytearray()
    for y in range(height):
        raw_data.append(0)  # filter: none
        for x in range(width):
            raw_data.append(r)
            raw_data.append(g)
            raw_data.append(b)
    
    # Compress with zlib (not raw deflate)
    compressed = zlib.compress(bytes(raw_data), 9)
    png += write_chunk(b'IDAT', compressed)
    
    # IEND
    png += write_chunk(b'IEND', b'')
    
    return png

os.makedirs('sce_sys/livearea/contents', exist_ok=True)

# icon0.png - MUST be exactly 128x128
icon = create_vita_png(128, 128, 44, 100, 180)
with open('sce_sys/icon0.png', 'wb') as f:
    f.write(icon)
print(f"icon0.png: {len(icon)} bytes")

# bg.png - MUST be exactly 840x500
bg = create_vita_png(840, 500, 20, 20, 50)
with open('sce_sys/livearea/contents/bg.png', 'wb') as f:
    f.write(bg)
print(f"bg.png: {len(bg)} bytes")

# startup.png - MUST be exactly 280x158
startup = create_vita_png(280, 158, 44, 100, 180)
with open('sce_sys/livearea/contents/startup.png', 'wb') as f:
    f.write(startup)
print(f"startup.png: {len(startup)} bytes")

# Verify files
for path in ['sce_sys/icon0.png', 
             'sce_sys/livearea/contents/bg.png',
             'sce_sys/livearea/contents/startup.png']:
    size = os.path.getsize(path)
    with open(path, 'rb') as f:
        header = f.read(8)
    is_png = header == b'\x89PNG\r\n\x1a\n'
    print(f"  {path}: {size} bytes, valid PNG: {is_png}")

print("Done!")
