# ESP32 Karaoke Player Specification

## Project Overview

- **Project Name**: ESP32 CDG Karaoke Player
- **Core Functionality**: A karaoke player running on ESP32 that outputs composite video (NTSC) and plays CDG (Compact Disc Graphics) files with synchronized lyrics display
- **Target Users**: Karaoke enthusiasts, hobbyists, retro gaming fans
- **Hardware**: ESP32 (any variant with DAC), RCA composite output, buttons for control

## Technical Specification

### Video Output
- **Library**: ESP32CompositeColorVideo (marciot/ESP32CompositeColorVideo)
- **Resolution**: 336x240 pixels (NTSC)
- **Color Mode**: Atari 256-color palette
- **Wiring**: 
  - GND → RCA outer barrel
  - DAC1 (A1) → RCA center pin

### CDG Format
- **Format**: Compact Disc Graphics
- **Resolution**: 300x216 pixels (displayed in 288x192 region)
- **Commands Supported**: 
  - Memory Preset (1)
  - Border Preset (2)
  - Tile Block (6)
  - Tile Block XOR (38)
  - Scroll Preset (20)
  - Scroll Copy (24)
  - Define Transparent Color (28)
  - Load Color Table Low (30)
  - Load Color Table High (31) ⚠️ Value is 31, NOT 38 (38 conflicts with Tile Block XOR)

### Audio Output
- **Method**: LED PWM on GPIO 18
- **LEDC Channel**: 0
- **Frequency**: 40 kHz PWM carrier
- **Resolution**: 8 bits
- **Format**: MP3 from SD card (libhelix decoder)
- **Filter**: RC low-pass (1kΩ + 10nF)

### SD Card (SPI)
- **MOSI**: GPIO 23
- **MISO**: GPIO 19
- **CLK**: GPIO 14
- **CS**: GPIO 5
- **DMA Channel**: 2

### Controls
- **Play/Pause Button**: GPIO 0
- **Next Button**: GPIO 2
- **Previous Button**: GPIO 4
- **Volume Up**: GPIO 15
- **Volume Down**: GPIO 13

## Features

1. **CDG Playback**: Parse and display CDG graphics on composite video
2. **Audio Playback**: Play MP3/OGG audio tracks from SD card
3. **File Browser**: Browse and select karaoke files from SD card
4. **Playback Controls**: Play, pause, next, previous, volume control
5. **Lyrics Display**: CDG graphics contain lyrics with timing

## Project Structure

```
/ESPoke/
├── src/
│   ├── main.cpp           # Main application
│   ├── CDGParser.cpp      # CDG file parser
│   ├── CDGParser.h        # CDG parser header
│   ├── Player.cpp         # Karaoke player logic
│   ├── Player.h           # Player header
│   ├── AudioPlayer.cpp    # Audio PWM output
│   ├── AudioPlayer.h      # Audio header
│   └── font6x8.h          # 6x8 font bitmap
├── platformio.ini         # PlatformIO configuration
├── README.md              # User documentation
└── SPEC.md               # This specification
```

## Dependencies

- ESP32CompositeColorVideo (installed via PlatformIO library manager)
- arduino-libhelix (MP3 decoder from GitHub)
- SD (built-in ESP32 SD library)
- SPI (built-in ESP32 SPI library)
- WiFi (built-in ESP32 WiFi library)
- WebServer (built-in ESP32 WebServer library)

## Acceptance Criteria

1. ✓ ESP32 boots and initializes composite video output
2. ✓ Can read CDG files from SD card
3. ✓ CDG graphics display correctly on composite video
4. ✓ Audio playback works with video synchronization
5. ✓ Buttons control playback (play/pause/next/prev)
6. ✓ Project compiles without errors
