"""
Generate splash.cdg for ESPoke embedded splash screen.
CDG: 300x216 pixels, 16 colors, tiles of 6x12, 300 packets/sec.
Duration: 3 seconds = 900 packets.

Design:
  - Dark blue background
  - Color gradient bar at top
  - "ESPoke!" title in large (2-tile-high) centered
  - "Karaoke Player" subtitle
  - Equalizer-style bars
  - Bottom color strip
"""
import struct, math

PACKETS_PER_SEC = 300
DURATION_SEC = 3
TOTAL_PACKETS = PACKETS_PER_SEC * DURATION_SEC
CDG_COLS = 50  # 300/6
CDG_ROWS = 18  # 216/12

def make_packet(instruction, data):
    pkt = bytearray(24)
    pkt[0] = 0x09
    pkt[1] = instruction & 0x3F
    for i in range(min(len(data), 16)):
        pkt[4 + i] = data[i] & 0x3F
    return bytes(pkt)

def nop_packet():
    return bytes(24)  # byte 0 = 0, not CDG subchannel

def memory_preset(color):
    return make_packet(1, [color & 0x0F, 0])

def load_color_table_low(colors):
    data = []
    for r, g, b in colors:
        data.append(((r & 0x0F) << 2) | ((g >> 2) & 0x03))
        data.append(((g & 0x03) << 4) | (b & 0x0F))
    return make_packet(30, data)

def load_color_table_high(colors):
    data = []
    for r, g, b in colors:
        data.append(((r & 0x0F) << 2) | ((g >> 2) & 0x03))
        data.append(((g & 0x03) << 4) | (b & 0x0F))
    return make_packet(31, data)

def tile_block(row, col, color0, color1, pixel_rows):
    data = [color0 & 0x0F, color1 & 0x0F, row & 0x1F, col & 0x3F]
    for p in pixel_rows:
        data.append(p & 0x3F)
    while len(data) < 16:
        data.append(0)
    return make_packet(6, data)

# 6-wide pixel font (each row is 6 bits packed)
FONT = {
    'E': [0x3F,0x20,0x20,0x3E,0x20,0x20,0x3F,0x00,0x00,0x00,0x00,0x00],
    'S': [0x1F,0x20,0x20,0x1E,0x01,0x01,0x3E,0x00,0x00,0x00,0x00,0x00],
    'P': [0x3E,0x21,0x21,0x3E,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00],
    'o': [0x00,0x00,0x00,0x1E,0x21,0x21,0x1E,0x00,0x00,0x00,0x00,0x00],
    'k': [0x20,0x20,0x22,0x24,0x38,0x24,0x22,0x00,0x00,0x00,0x00,0x00],
    'e': [0x00,0x00,0x00,0x1E,0x21,0x3F,0x20,0x1E,0x00,0x00,0x00,0x00],
    '!': [0x04,0x04,0x04,0x04,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00],
    'K': [0x21,0x22,0x24,0x38,0x24,0x22,0x21,0x00,0x00,0x00,0x00,0x00],
    'a': [0x00,0x00,0x00,0x1E,0x01,0x1F,0x21,0x1F,0x00,0x00,0x00,0x00],
    'r': [0x00,0x00,0x00,0x2E,0x31,0x20,0x20,0x20,0x00,0x00,0x00,0x00],
    'O': [0x1E,0x21,0x21,0x21,0x21,0x21,0x1E,0x00,0x00,0x00,0x00,0x00],
    'l': [0x0C,0x04,0x04,0x04,0x04,0x04,0x0E,0x00,0x00,0x00,0x00,0x00],
    'y': [0x00,0x00,0x00,0x21,0x21,0x1F,0x01,0x1E,0x00,0x00,0x00,0x00],
    ' ': [0x00]*12,
    'p': [0x00,0x00,0x00,0x3E,0x21,0x21,0x3E,0x20,0x20,0x00,0x00,0x00],
}

# Big letters: each char is 2 tiles wide (12px) x 2 tiles tall (24px)
# Stored as [top_left, top_right, bottom_left, bottom_right] tile pixel rows
BIG_FONT = {
    'E': {
        'tl': [0x3F,0x3F,0x30,0x30,0x3F,0x3F,0x30,0x30,0x30,0x30,0x3F,0x3F],
        'tr': [0x3C,0x3C,0x00,0x00,0x3C,0x3C,0x00,0x00,0x00,0x00,0x3C,0x3C],
    },
    'S': {
        'tl': [0x1F,0x3F,0x30,0x30,0x1F,0x3F,0x01,0x01,0x01,0x01,0x3F,0x1F],
        'tr': [0x3C,0x3C,0x00,0x00,0x3C,0x3C,0x00,0x00,0x00,0x00,0x3C,0x3C],
    },
    'P': {
        'tl': [0x3F,0x3F,0x30,0x30,0x3F,0x3F,0x30,0x30,0x30,0x30,0x30,0x30],
        'tr': [0x3C,0x3C,0x0C,0x0C,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00],
    },
    'o': {
        'tl': [0x00,0x00,0x00,0x0F,0x1F,0x30,0x30,0x30,0x1F,0x0F,0x00,0x00],
        'tr': [0x00,0x00,0x00,0x3C,0x3C,0x0C,0x0C,0x0C,0x3C,0x3C,0x00,0x00],
    },
    'k': {
        'tl': [0x30,0x30,0x30,0x33,0x36,0x3C,0x36,0x33,0x30,0x30,0x00,0x00],
        'tr': [0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x00,0x00],
    },
    'e': {
        'tl': [0x00,0x00,0x00,0x0F,0x1F,0x30,0x3F,0x30,0x1F,0x0F,0x00,0x00],
        'tr': [0x00,0x00,0x00,0x3C,0x3C,0x0C,0x3C,0x00,0x00,0x3C,0x00,0x00],
    },
    '!': {
        'tl': [0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x0C,0x0C,0x00,0x00],
        'tr': [0x00]*12,
    },
}

def generate_splash():
    packets = []

    # Palette
    colors_low = [
        (0, 0, 3),    # 0: dark blue (background)
        (15,15,15),    # 1: white
        (15, 4, 0),    # 2: orange
        (0, 12, 15),   # 3: cyan
        (15, 0, 4),    # 4: red-pink
        (15, 15, 0),   # 5: yellow
        (0, 15, 4),    # 6: green
        (8, 4, 15),    # 7: purple
    ]
    colors_high = [
        (2, 2, 6),     # 8: medium blue
        (15, 8, 0),    # 9: dark orange
        (4, 4, 10),    # 10: lighter blue
        (0, 8, 12),    # 11: teal
        (12, 12, 12),  # 12: light gray
        (6, 6, 6),     # 13: dark gray
        (10, 0, 10),   # 14: magenta
        (0, 0, 8),     # 15: medium dark blue
    ]

    packets.append(load_color_table_low(colors_low))
    packets.append(load_color_table_high(colors_high))
    packets.append(memory_preset(0))

    # Top gradient bar (row 1, y=12)
    bar_colors = [4, 2, 9, 5, 6, 3, 7, 14, 4, 2, 9, 5, 6, 3, 7, 14]
    for i in range(16):
        col = 9 + i * 2
        full = [0x3F]*12
        packets.append(tile_block(1, col, 0, bar_colors[i % len(bar_colors)], full))
        packets.append(tile_block(1, col+1, 0, bar_colors[i % len(bar_colors)], full))

    # "ESPoke!" big title using simple tiles — row 4-5 (y=48)
    title = "ESPoke!"
    start_col = 11
    for i, ch in enumerate(title):
        if ch in BIG_FONT:
            c = 5 if i < 3 else 3  # yellow for ESP, cyan for oke!
            packets.append(tile_block(4, start_col + i*4, 0, c, BIG_FONT[ch]['tl']))
            packets.append(tile_block(4, start_col + i*4 + 1, 0, c, BIG_FONT[ch]['tr']))

    # "Karaoke Player" subtitle — row 8 (y=96)
    subtitle = "Karaoke Player"
    sub_start = (CDG_COLS - len(subtitle)) // 2
    for i, ch in enumerate(subtitle):
        if ch in FONT:
            packets.append(tile_block(8, sub_start + i, 0, 12, FONT[ch]))

    # Equalizer bars (row 11-14, y=132-168)
    bar_heights = [3, 5, 7, 9, 10, 9, 7, 5, 8, 10, 6, 4, 7, 9, 8, 6]
    eq_colors = [4, 2, 5, 6, 3, 7, 4, 2, 5, 6, 3, 7, 4, 2, 5, 6]
    for i, h in enumerate(bar_heights):
        col = 9 + i * 2
        # Each unit = partial fill of a tile
        rows_filled = min(h, 12)
        pixels = [0x00] * (12 - rows_filled) + [0x1E] * rows_filled
        packets.append(tile_block(12, col, 0, eq_colors[i], pixels))
        if h > 6:
            pixels2 = [0x00] * (24 - h) + [0x1E] * (h - 12) if h > 12 else [0x00]*12
            packets.append(tile_block(11, col, 0, eq_colors[i], pixels2))

    # Bottom gradient bar (row 16, y=192)
    for i in range(16):
        col = 9 + i * 2
        full = [0x3F]*12
        c = bar_colors[(i + 4) % len(bar_colors)]
        packets.append(tile_block(16, col, 0, c, full))
        packets.append(tile_block(16, col+1, 0, c, full))

    # Pad with nops
    while len(packets) < TOTAL_PACKETS:
        packets.append(nop_packet())

    data = b''.join(packets[:TOTAL_PACKETS])

    with open('tools/splash.cdg', 'wb') as f:
        f.write(data)
    print(f"Created tools/splash.cdg ({len(data)} bytes, {TOTAL_PACKETS} packets)")
    return data

def cdg_to_header(cdg_data, header_path):
    """Convert raw CDG bytes to a C header with PROGMEM array."""
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
    print(f"Created {header_path} ({len(cdg_data)} bytes)")

if __name__ == '__main__':
    data = generate_splash()
    cdg_to_header(data, 'src/splash_cdg.h')
