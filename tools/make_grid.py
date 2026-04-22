"""Generate transparent grid overlay for CDG tile design (6x12 tiles)."""
from PIL import Image, ImageDraw

img = Image.new('RGBA', (300, 216), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)

grid_color = (255, 255, 255, 60)
for x in range(0, 300, 6):
    draw.line([(x, 0), (x, 215)], fill=grid_color)
for y in range(0, 216, 12):
    draw.line([(0, y), (299, y)], fill=grid_color)

img.save('tools/grid_overlay.png')
print("Created tools/grid_overlay.png (300x216, transparent with white grid)")
