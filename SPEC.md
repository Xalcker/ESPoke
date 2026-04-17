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
  - Memory Preset (0)
  - Border Preset (1)
  - Tile Block (2)
  - Tile Block XOR (6)
  - Scroll (20, 24)
  - Define Transparent Color (28)
  - Load Static Color Table (30)
  - Load Static Data (38)

### Audio Output
- **Method**: I2S or DAC for audio playback
- **Format**: MP3/OGG from SD card

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
│   └── Player.h           # Player header
├── include/
│   └── README
├── lib/
│   └── README
├── test/
│   └── README
├── platformio.ini         # PlatformIO configuration
└── SPEC.md               # This specification
```

## Dependencies

- ESP32CompositeColorVideo (installed via PlatformIO library manager)
- SD (built-in ESP32 SD library)
- SPI (built-in ESP32 SPI library)

## Acceptance Criteria

1. ✓ ESP32 boots and initializes composite video output
2. ✓ Can read CDG files from SD card
3. ✓ CDG graphics display correctly on composite video
4. ✓ Audio playback works with video synchronization
5. ✓ Buttons control playback (play/pause/next/prev)
6. ✓ Project compiles without errors
