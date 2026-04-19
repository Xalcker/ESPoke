"""
Convert a PNG image to an embedded CDG splash for ESPoke.

Usage:
    python tools/png_to_splash.py <image.png>

Input:
    - PNG image, any size (will be resized to 300x216)
    - Any color mode (will be quantized to 16 colors)

Output:
    - tools/splash.cdg     (raw CDG file for preview)
    - src/splash_cdg.h     (C header with PROGMEM array)

The image is quantized to 16 colors. For best results:
    - Use 300x216 pixels
    - Use max 16 flat colors (no gradients)
    - Or let Pillow dither for you (Floyd-Steinberg)

CDG tiles are 6x12 pixels. Each tile can only use 2 colors
(foreground + background), so the script picks the best pair
per tile from the 16-color palette.
"""
import sys
from PIL import Image

CDG_W, CDG_H = 300, 216
TILE_W, TILE_H = 6, 12
TILES_X = CDG_W // TILE_W   # 50
TILES_Y = CDG_H // TILE_H   # 18
PACKETS_PER_SEC = 300
DURATION_SEC = 3
TOTAL_PACKETS = PACKETS_PER_SEC * DURATION_SEC

def make_packet(instruction, data):
    pkt = bytearray(24)
    pkt[0] = 0x09
    pkt[1] = instruction & 0x3F
    for i in range(min(len(data), 16)):
        pkt[4 + i] = data[i] & 0x3F
    return bytes(pkt)

def nop_packet():
    return bytes(24)

def rgb_to_cdg_color(r, g, b):
    """Convert 8-bit RGB to CDG 4-bit per channel."""
    return (r >> 4, g >> 4, b >> 4)

def load_color_table(colors, high=False):
    """Build color table packet. colors = list of (r4,g4,b4) tuples."""
    data = []
    for r, g, b in colors:
        data.append(((r & 0x0F) << 2) | ((g >> 2) & 0x03))
        data.append(((g & 0x03) << 4) | (b & 0x0F))
    while len(data) < 16:
        data.append(0)
    return make_packet(31 if high else 30, data)

def tile_block_packet(row, col, color0, color1, pixel_rows):
    data = [color0 & 0x0F, color1 & 0x0F, row & 0x1F, col & 0x3F]
    for p in pixel_rows:
        data.append(p & 0x3F)
    while len(data) < 16:
        data.append(0)
    return make_packet(6, data)

def find_best_pair(tile_pixels, palette_size):
    """Find the 2 most common color indices in a tile."""
    from collections import Counter
    counts = Counter(tile_pixels)
    most_common = counts.most_common(2)
    c0 = most_common[0][0]
    c1 = most_common[1][0] if len(most_common) > 1 else c0
    return c0, c1

def image_to_cdg(img_path):
    img = Image.open(img_path).convert('RGB')

    # Resize to CDG resolution
    if img.size != (CDG_W, CDG_H):
        img = img.resize((CDG_W, CDG_H), Image.LANCZOS)
        print(f"Resized to {CDG_W}x{CDG_H}")

    # Quantize to 16 colors
    img_q = img.quantize(colors=16, method=Image.Quantize.MAXCOVERAGE, dither=Image.Dither.NONE)
    palette_raw = img_q.getpalette()[:48]  # 16 colors * 3 channels
    palette = []
    for i in range(16):
        r, g, b = palette_raw[i*3], palette_raw[i*3+1], palette_raw[i*3+2]
        palette.append(rgb_to_cdg_color(r, g, b))

    pixels = list(img_q.getdata())  # flat list of palette indices

    print(f"Palette ({len(palette)} colors):")
    for i, (r, g, b) in enumerate(palette):
        print(f"  {i:2d}: R={r:2d} G={g:2d} B={b:2d}")

    # Build CDG packets
    packets = []

    # Color tables
    packets.append(load_color_table(palette[:8], high=False))
    packets.append(load_color_table(palette[8:] if len(palette) > 8 else [(0,0,0)]*8, high=True))

    # Memory preset (clear to color 0)
    packets.append(make_packet(1, [0, 0]))

    # Generate tile blocks
    for ty in range(TILES_Y):
        for tx in range(TILES_X):
            # Extract tile pixels
            tile_pixels = []
            for dy in range(TILE_H):
                for dx in range(TILE_W):
                    px = tx * TILE_W + dx
                    py = ty * TILE_H + dy
                    tile_pixels.append(pixels[py * CDG_W + px])

            # Find best 2-color pair for this tile
            c0, c1 = find_best_pair(tile_pixels, len(palette))

            # Encode rows: bit=0 → c0, bit=1 → c1
            pixel_rows = []
            for dy in range(TILE_H):
                row_bits = 0
                for dx in range(TILE_W):
                    idx = tile_pixels[dy * TILE_W + dx]
                    # Map to closest of c0 or c1
                    bit = 0 if idx == c0 else 1
                    row_bits = (row_bits << 1) | bit
                pixel_rows.append(row_bits & 0x3F)

            # Skip all-background tiles
            if c0 == c1 == 0 and all(r == 0 for r in pixel_rows):
                continue

            packets.append(tile_block_packet(ty, tx, c0, c1, pixel_rows))

    # Pad to fill duration
    while len(packets) < TOTAL_PACKETS:
        packets.append(nop_packet())

    cdg_data = b''.join(packets[:TOTAL_PACKETS])
    print(f"Generated {len(packets)} packets ({len(cdg_data)} bytes)")
    return cdg_data

def cdg_to_header(cdg_data, header_path):
    with open(header_path, 'w') as f:
        f.write('#ifndef SPLASH_CDG_H\n#define SPLASH_CDG_H\n\n')
        f.write('#include <pgmspace.h>\n\n')
        f.write(f'#define SPLASH_CDG_SIZE {len(cdg_data)}\n')
        f.write(f'#define SPLASH_CDG_PACKETS {len(cdg_data) // 24}\n\n')
        f.write('const uint8_t splash_cdg[] PROGMEM = {\n')
        for i in range(0, len(cdg_data), 16):
            chunk = cdg_data[i:i+16]
            hex_vals = ', '.join(f'0x{b:02X}' for b in chunk)
            f.write(f'    {hex_vals},\n')
        f.write('};\n\n#endif\n')
    print(f"Created {header_path}")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python tools/png_to_splash.py <image.png>")
        print("\nCreates tools/splash.cdg and src/splash_cdg.h")
        print("Image will be resized to 300x216 and quantized to 16 colors.")
        sys.exit(1)

    cdg_data = image_to_cdg(sys.argv[1])

    with open('tools/splash.cdg', 'wb') as f:
        f.write(cdg_data)
    print("Created tools/splash.cdg")

    cdg_to_header(cdg_data, 'src/splash_cdg.h')
    print("\nDone! Run 'pio run --target upload' to flash.")
