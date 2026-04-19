"""
Generate a test CDG+MP3 pair for ESPoke.
Creates: test.cdg + test.mp3 (5 seconds)

CDG format: 24-byte packets at 300 packets/sec
  byte 0: command (0x09 for CDG subchannel)
  byte 1: instruction (masked to 6 bits)
  bytes 2-3: padding
  bytes 4-19: data (16 bytes, each masked to 6 bits)
  bytes 20-23: padding
"""
import struct, subprocess, os

PACKETS_PER_SEC = 300
DURATION_SEC = 5
TOTAL_PACKETS = PACKETS_PER_SEC * DURATION_SEC

def make_packet(instruction, data):
    """Build a 24-byte CDG packet."""
    pkt = bytearray(24)
    pkt[0] = 0x09  # CDG subchannel
    pkt[1] = instruction & 0x3F
    for i in range(min(len(data), 16)):
        pkt[4 + i] = data[i] & 0x3F
    return bytes(pkt)

def memory_preset(color):
    """Cmd 1: fill screen with color."""
    return make_packet(1, [color & 0x0F, 0])

def load_color_table_low(colors):
    """Cmd 30: set colors 0-7. Each color = 2 bytes [RRRRGG, GGBBBB]."""
    data = []
    for r, g, b in colors:
        data.append(((r & 0x0F) << 2) | ((g >> 2) & 0x03))
        data.append(((g & 0x03) << 4) | (b & 0x0F))
    return make_packet(30, data)

def load_color_table_high(colors):
    """Cmd 31: set colors 8-15."""
    data = []
    for r, g, b in colors:
        data.append(((r & 0x0F) << 2) | ((g >> 2) & 0x03))
        data.append(((g & 0x03) << 4) | (b & 0x0F))
    return make_packet(31, data)

def tile_block(row, col, color0, color1, pixels_rows):
    """Cmd 6: draw a 6x12 tile at (col*6, row*12)."""
    data = [color0 & 0x0F, color1 & 0x0F, row & 0x1F, col & 0x3F]
    for p in pixels_rows:
        data.append(p & 0x3F)
    while len(data) < 16:
        data.append(0)
    return make_packet(6, data)

# Simple 6x12 font for uppercase letters (6 bits wide)
FONT = {
    'H': [0x21,0x21,0x21,0x3F,0x21,0x21,0x21,0x21,0x00,0x00,0x00,0x00],
    'E': [0x3F,0x20,0x20,0x3E,0x20,0x20,0x3F,0x00,0x00,0x00,0x00,0x00],
    'L': [0x20,0x20,0x20,0x20,0x20,0x20,0x3F,0x00,0x00,0x00,0x00,0x00],
    'O': [0x1E,0x21,0x21,0x21,0x21,0x21,0x1E,0x00,0x00,0x00,0x00,0x00],
    ' ': [0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00],
    'S': [0x1E,0x21,0x20,0x1E,0x01,0x21,0x1E,0x00,0x00,0x00,0x00,0x00],
    'P': [0x3E,0x21,0x21,0x3E,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00],
    'K': [0x21,0x22,0x24,0x38,0x24,0x22,0x21,0x00,0x00,0x00,0x00,0x00],
    '!': [0x04,0x04,0x04,0x04,0x04,0x00,0x04,0x00,0x00,0x00,0x00,0x00],
}

def generate_cdg(filename):
    packets = []

    # Color palette: low table (0-7)
    colors_low = [
        (0,0,0),    # 0: black
        (15,15,15), # 1: white
        (15,0,0),   # 2: red
        (0,15,0),   # 3: green
        (0,0,15),   # 4: blue
        (15,15,0),  # 5: yellow
        (0,15,15),  # 6: cyan
        (15,0,15),  # 7: magenta
    ]
    colors_high = [
        (8,8,8),    # 8: gray
        (8,0,0),    # 9: dark red
        (0,8,0),    # 10: dark green
        (0,0,8),    # 11: dark blue
        (8,8,0),    # 12: dark yellow
        (0,8,8),    # 13: dark cyan
        (8,0,8),    # 14: dark magenta
        (4,4,4),    # 15: dark gray
    ]

    packets.append(load_color_table_low(colors_low))
    packets.append(load_color_table_high(colors_high))

    # Clear screen to blue (color 4)
    packets.append(memory_preset(4))

    # Draw "HELLO" at row 5 (y=60), starting col 10
    text = "HELLO"
    for i, ch in enumerate(text):
        if ch in FONT:
            packets.append(tile_block(5, 10 + i, 4, 1, FONT[ch]))

    # Draw "ESPOKE!" at row 8 (y=96), starting col 8
    text2 = "ESPOKE!"
    for i, ch in enumerate(text2):
        if ch in FONT:
            packets.append(tile_block(8, 8 + i, 4, 5, FONT[ch]))

    # Draw color bars at row 14
    for i in range(8):
        pixels = [0x3F]*12  # full block
        packets.append(tile_block(14, 10 + i, 0, i, pixels))

    # Pad remaining time with empty packets
    while len(packets) < TOTAL_PACKETS:
        packets.append(make_packet(0, [0]*16))  # no-op (not 0x09 subchannel)

    with open(filename, 'wb') as f:
        for pkt in packets[:TOTAL_PACKETS]:
            f.write(pkt)

    print(f"Created {filename} ({TOTAL_PACKETS} packets, {DURATION_SEC}s)")

def generate_mp3(filename):
    """Generate a 5-second 440Hz tone as MP3 using ffmpeg."""
    cmd = [
        'ffmpeg', '-y', '-f', 'lavfi',
        '-i', f'sine=frequency=440:duration={DURATION_SEC}',
        '-b:a', '128k', filename
    ]
    subprocess.run(cmd, capture_output=True)
    print(f"Created {filename}")

if __name__ == '__main__':
    generate_cdg('test.cdg')
    generate_mp3('test.mp3')
    print("\nCopy test.cdg and test.mp3 to your SD card root.")
