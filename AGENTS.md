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
- `arduino-libhelix` - MP3 decoder:
  ```
  lib_deps = https://github.com/pschatzmann/arduino-libhelix.git
  ```

## Architecture

- **Core 0**: FreeRTOS task (`cdgAudioTask`) — CDG parsing, audio decoding, auto-advance
- **Core 1**: Main loop — rendering, input handling, state machine

## App States

`STATE_SPLASH` → `STATE_BROWSER` → `STATE_PLAYING` → (auto-advance or back to browser)
`STATE_SCREENSAVER` activates after 5 min inactivity from browser or playing.

## CDG/Audio Synchronization

- CDG packets run at 300 packets/sec (1 every 3.33ms)
- `CDGParser::syncToTime(elapsedMs)` calculates target packet from elapsed time
- `Player::getPlayElapsedMs()` provides time since playback started
- Max 300 packets catch-up per call to avoid blocking

## Auto-advance

- `Player::isSongFinished()` flag set when AudioPlayer stops
- `cdgAudioTask` checks flag: advances to next song or returns to browser after last
- `browserSelection` tracks current position in file list

## Recursive Directory Scan

- `Player::scanDirectory()` recurses into subdirectories
- Stores full path in `fileList[]` (e.g., `/rock/song.cdg`)
- Browser strips path with `strrchr('/')`, shows filename without extension
- Max 100 files across all directories

## Audio LEDC Configuration

- Channel: 0
- Frequency: 40 kHz PWM carrier
- Bits: 8
- Pin: GPIO 18
- Timer for sample output: 3

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

```
ESP32 GPIO Connections:

  Video (Composite NTSC):
    GPIO 25 (DAC1) ──► RCA Center (video)
    GND            ──► RCA Outer (ground)

  Audio (PWM):
    GPIO 18 ──[1kΩ]──┬──► Speaker
                      │
                    [10nF]
                      │
                     GND

  SD Card (SPI):
    GPIO 23 ──► MOSI
    GPIO 19 ◄── MISO
    GPIO 14 ──► CLK
    GPIO  5 ──► CS

  Buttons (Pull-Up, active LOW):
    GPIO  0 ──► [Play/Pause] ──► GND
    GPIO  2 ──► [Next] ──► GND
    GPIO  4 ──► [Prev] ──► GND
    GPIO 13 ──► [Vol-] ──► GND
    GPIO 15 ──► [Vol+] ──► GND
```

## CDG Parser Rules

- `CDG_LOAD_COLOR_TABLE_HIGH` must be value 31, NOT 38 (38 conflicts with `CDG_TILE_BLOCK_XOR`)
- Remove duplicate case values in switch statements
- Command values in CDGParser.h are correct (30 for Low, 31 for High, 38 for Tile Block XOR)
- State colorTable is 16*2 = 32 bytes (16 for primary + 16 for secondary palette)
- CDG framebuffer is 300x216 pixels with a 16-color palette
- Scroll buffer is heap-allocated (64.8KB) to avoid stack overflow

## Memory Usage

- RAM: 31.7% (104KB / 320KB)
- Flash: 13.1% (413KB / 3MB)
- No PSRAM available on ESP32-D0WD
