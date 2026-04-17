# ESPoke - ESP32 Karaoke Player

## Build & Flash

```powershell
pio run              # Compile
pio run --target upload  # Flash to ESP32
pio device monitor   # View serial output
```

## Key Dependencies

- `ESP32CompositeColorVideo` - loaded from GitHub (not in PlatformIO registry):
  ```
  lib_deps = https://github.com/marciot/ESP32CompositeColorVideo.git
  ```

## Library API Notes

The `ESP32CompositeColorVideo` library uses different method names than expected:

| Use This | Not This |
|----------|----------|
| `print()` | `println()` |
| `setTextColor(n)` | `setTextSize()` |
| `Color(n)` (single arg) | `Color(brightness, hue)` |
| `dot(x, y, colorValue)` | `setColor()` then draw |
| `begin(0)` to clear | `fill()` |
| `CompositeColorOutput::XRES` | hardcoded width |
| `graphics.setHue(h)` before drawing | color passed to draw calls |
| `font6x8.h` for font | no default font |

## Hardware

- Video: GPIO 25 (DAC1) → RCA center pin
- Audio: GPIO 18 → filtro RC (1kΩ + 10nF)
- SD Card: SPI pins (MOSI=23, MISO=19, CLK=18, CS=5)

## CDG Parser Fixes

- `CDG_LOAD_STATIC_DATA` must be value 31, NOT 38 (38 conflicts with `CDG_TILE_BLOCK_XOR`)
- Remove duplicate case values in switch statements