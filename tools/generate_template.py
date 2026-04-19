"""
Generate a CDG design template PNG for Paint.NET / GIMP.
Shows tile grid (6x12) and includes the 16-color palette as swatches.
"""
from PIL import Image, ImageDraw

CDG_W, CDG_H = 300, 216
TILE_W, TILE_H = 6, 12

# Suggested 16-color palette (RGB)
PALETTE = [
    (0, 0, 0),        # 0: Black (background)
    (255, 255, 255),   # 1: White (main text)
    (255, 255, 0),     # 2: Yellow (title)
    (0, 255, 255),     # 3: Cyan (subtitle/accents)
    (255, 0, 0),       # 4: Red (mic, details)
    (0, 255, 0),       # 5: Green (EQ bar)
    (255, 128, 0),     # 6: Orange (EQ bar)
    (255, 0, 255),     # 7: Magenta (EQ bar, notes)
    (0, 0, 80),        # 8: Dark blue (alt bg/border)
    (255, 128, 255),   # 9: Pink (EQ bar)
    (128, 255, 128),   # 10: Light green (EQ bar)
    (80, 180, 255),    # 11: Light blue (neon border)
    (180, 180, 0),     # 12: Dark yellow (title shadow)
    (160, 160, 160),   # 13: Gray (secondary text)
    (128, 0, 0),       # 14: Dark red (detail)
    (128, 0, 128),     # 15: Purple (detail)
]

# Template with extra space for palette swatches
SWATCH_H = 40
TOTAL_H = CDG_H + SWATCH_H + 2

img = Image.new('RGB', (CDG_W, TOTAL_H), (0, 0, 0))
draw = ImageDraw.Draw(img)

# Draw tile grid (subtle dark blue lines)
grid_color = (30, 30, 60)
for x in range(0, CDG_W, TILE_W):
    draw.line([(x, 0), (x, CDG_H - 1)], fill=grid_color)
for y in range(0, CDG_H, TILE_H):
    draw.line([(0, y), (CDG_W - 1, y)], fill=grid_color)

# Draw palette swatches at bottom
swatch_w = CDG_W // 16
y_start = CDG_H + 2
for i, color in enumerate(PALETTE):
    x0 = i * swatch_w
    draw.rectangle([x0, y_start, x0 + swatch_w - 1, y_start + SWATCH_H - 1], fill=color)
    # Number label
    label_color = (0, 0, 0) if sum(color) > 300 else (255, 255, 255)
    draw.text((x0 + 2, y_start + 2), str(i), fill=label_color)

# Save template
img.save('tools/template.png')
print(f"Created tools/template.png ({CDG_W}x{TOTAL_H})")
print(f"  Top {CDG_H}px: design area with {TILE_W}x{TILE_H} tile grid")
print(f"  Bottom {SWATCH_H}px: 16-color palette swatches")
print()
print("Instructions:")
print("  1. Open in Paint.NET")
print("  2. Use eyedropper to pick colors from the palette swatches")
print("  3. Design within the grid lines (each cell = 1 CDG tile)")
print("  4. Each tile can only have 2 colors!")
print("  5. Crop or delete the palette bar before converting")
print("  6. Save as PNG, then: python tools/png_to_splash.py tools/splash.png")

# Also save just the design area (blank) and palette as separate file
design = Image.new('RGB', (CDG_W, CDG_H), (0, 0, 0))
design_draw = ImageDraw.Draw(design)
for x in range(0, CDG_W, TILE_W):
    design_draw.line([(x, 0), (x, CDG_H - 1)], fill=grid_color)
for y in range(0, CDG_H, TILE_H):
    design_draw.line([(0, y), (CDG_W - 1, y)], fill=grid_color)
design.save('tools/template_clean.png')
print(f"\nAlso created tools/template_clean.png ({CDG_W}x{CDG_H}, no palette bar)")
