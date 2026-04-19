"""Analyze Atari palette hues to build a proper RGB→hue mapping."""

# Atari palette RGB values (from video_out.h), taking mid-brightness (index 8) per hue
# Each hue row has 16 brightness levels, we take ~index 8 for representative color
atari_hues_rgb = [
    (0x68, 0x68, 0x68),  # 0: gray
    (0x68, 0x7A, 0x00),  # 1: yellow-green / olive
    (0x80, 0x6C, 0x12),  # 2: orange-brown
    (0x92, 0x5D, 0x2F),  # 3: orange
    (0x9C, 0x50, 0x58),  # 4: red-pink
    (0x9B, 0x49, 0x84),  # 5: magenta-pink
    (0x8E, 0x48, 0xAA),  # 6: purple
    (0x7A, 0x4E, 0xC2),  # 7: blue-purple
    (0x61, 0x5A, 0xC6),  # 8: blue
    (0x4A, 0x68, 0xB6),  # 9: blue-cyan
    (0x39, 0x77, 0x96),  # 10: cyan-blue
    (0x33, 0x82, 0x6B),  # 11: teal
    (0x37, 0x88, 0x40),  # 12: green
    (0x47, 0x87, 0x1C),  # 13: green-yellow
    (0x5D, 0x7F, 0x00),  # 14: yellow-green
    (0x76, 0x72, 0x0B),  # 15: yellow
]

print("Atari Hue Map (mid-brightness):")
print(f"{'Hue':>3} {'R':>3} {'G':>3} {'B':>3}  Color")
names = ['Gray','Yellow-Green','Orange-Brown','Orange','Red-Pink','Magenta',
         'Purple','Blue-Purple','Blue','Blue-Cyan','Cyan-Blue','Teal',
         'Green','Green-Yellow','Yellow-Green2','Yellow']
for i, (r,g,b) in enumerate(atari_hues_rgb):
    print(f"  {i:2d} {r:3d} {g:3d} {b:3d}  {names[i]}")

print("\nBest hue for common CDG colors:")
# Find closest hue for typical CDG colors
test_colors = [
    ("Red",     15, 0, 0),
    ("Green",   0, 15, 0),
    ("Blue",    0, 0, 15),
    ("Yellow",  15, 15, 0),
    ("Cyan",    0, 15, 15),
    ("Magenta", 15, 0, 15),
    ("Orange",  15, 8, 0),
    ("White",   15, 15, 15),
    ("Pink",    15, 8, 8),
]

for name, r4, g4, b4 in test_colors:
    r8, g8, b8 = r4*17, g4*17, b4*17
    best_hue = 0
    best_dist = 999999
    for h, (hr, hg, hb) in enumerate(atari_hues_rgb):
        dist = (r8-hr)**2 + (g8-hg)**2 + (b8-hb)**2
        if dist < best_dist:
            best_dist = dist
            best_hue = h
    print(f"  {name:10s} ({r4:2d},{g4:2d},{b4:2d}) -> hue {best_hue:2d} ({names[best_hue]})")
