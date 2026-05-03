# ESP32 Karaoke Player (ESPoke)

Karaoke player for ESP32 with composite NTSC video output.

## Features

- **Video**: Composite NTSC output (336x240) using the ESP32CompositeColorVideo library
- **Formats**: Supports CDG (Compact Disc Graphics) - MP3+G files
- **Audio**: MP3 playback from SD card
- **Controls**: Buttons for Play/Pause, Next, Previous, Volume +/-
- **CDG/Audio Sync**: Time-based synchronization at 300 packets/sec
- **Auto-advance**: Automatically plays next song; returns to browser after last
- **Recursive Scan**: Finds CDG files in subdirectories (organize by genre, language, etc.)
- **File Browser**: Scrollable song list, shows filenames without path or extension
- **Splash Screen**: Animated intro with gradient, equalizer bars, and progress bar
- **Screensaver**: Activates after 5 minutes of inactivity

## Required Hardware

### Board
- ESP32 (any variant with DAC)

### Full Wiring Diagram

```
                    ┌─────────────────┐
                    │     ESP32       │
                    │                 │
                    │  ┌───────────┐  │
                    │  │   GPIO    │  │
                    │  │    25     │──┼────────────────────┐
                    │  │   DAC1    │  │                    │
                    │  │   GND     │──┼────────────────────┤
                    │  │    0      │  │                    │
                    │  │    2      │  │                    │
                    │  │    4      │  │                    │
                    │  │   13      │  │                    │
                    │  │   15      │  │                    │
                    │  │   18      │──┼────────────────────┤
                    │  │   19      │  │                    │
                    │  │   23      │  │                    │
                    │  │   14      │  │                    │
                    │  │    5      │  │                    │
                    │  └───────────┘  │                    │
                    └─────────────────┘                    │
                                                           │
         ┌─────────────────────────────────────────────────┤
         │                    RCA JACK                     │
         │              (Composite Video)                  │
         │                                                 │
         │    ┌──────────────┐                             │
         │    │   1 Center   │◄── GPIO 25 (DAC1)           │
         │    │              │                             │
         │    │  2 Outer     │◄── GND                      │
         │    └──────────────┘                             │
         └─────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────────┐
         │              AUDIO OUTPUT (GPIO 18)             │
         │                                                 │
         │    GPIO 18 ──[ 1kΩ ]──┬───► Speaker             │
         │                       │                         │
         │                     [ 10nF ]                    │
         │                       │                         │
         │                      GND                        │
         └─────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────────┐
         │                    SD CARD                      │
         │              (SPI Reader)                       │
         │                                                 │
         │    ESP32      SD Reader                         │
         │    GPIO 23 ──► MOSI                             │
         │    GPIO 19 ◄── MISO                             │
         │    GPIO 14 ──► CLK                              │
         │    GPIO 5  ──► CS                               │
         │    GND      ──► GND                             │
         │    3.3V     ──► VCC (if 3.3V supported)         │
         └─────────────────────────────────────────────────┘

         ┌─────────────────────────────────────────────────┐
         │                   BUTTONS                       │
         │            (All with internal Pull-Up)          │
         │                                                 │
         │    ESP32      Function        Button            │
         │    GPIO  0 ──► Play/Pause ◄───► [SW1]           │
         │    GPIO  2 ──► Next       ◄───► [SW2]           │
         │    GPIO  4 ──► Previous   ◄───► [SW3]           │
         │    GPIO 13 ──► Vol-       ◄───► [SW4]           │
         │    GPIO 15 ──► Vol+       ◄───► [SW5]           │
         │    GND      ──► GND (common)                    │
         │                                                 │
         │    [SW1]                    ▲                   │
         │      │                     │                    │
         │      └─────────[ GND ]─────┘                    │
         │                                                 │
         │    (Buttons connect GPIO to GND)                │
         └─────────────────────────────────────────────────┘
```

### Connection Summary

| ESP32 GPIO | Function | Physical Connection |
|------------|----------|---------------------|
| 25 (DAC1) | Video | RCA center (video) |
| GND | Ground | RCA outer (ground) |
| 18 | Audio Out | RC filter → Speaker |
| 23 | SD MOSI | SD reader MOSI |
| 19 | SD MISO | SD reader MISO |
| 14 | SD CLK | SD reader CLK |
| 5 | SD CS | SD reader CS |
| 0 | Play/Pause Button | Button → GND |
| 2 | Next Button | Button → GND |
| 4 | Previous Button | Button → GND |
| 13 | Vol- Button | Button → GND |
| 15 | Vol+ Button | Button → GND |

## Installation

### Requirements
- [PlatformIO](https://platformio.org/) installed
- ESP32 toolchain

### Build
```bash
# Install PlatformIO if you don't have it
pip install platformio

# Build the project
pio run

# Upload to ESP32
pio run --target upload
```

## Karaoke Files

### File structure
```
/karaoke/
   ├── rock/
   │   ├── song1.cdg
   │   └── song1.mp3
   ├── pop/
   │   ├── song2.cdg
   │   └── song2.mp3
   ├── song3.cdg
   └── song3.mp3
```

Files can be in the root or organized in subdirectories (by genre, language, etc.). The browser shows only the filename without path or extension.

### Supported formats
- **CDG + MP3**: .cdg files with .mp3 audio

Video files (.cdg) must have the same name as the corresponding audio file.

## Technical Details

### Composite Video (APLL)

The project uses the ESP32's **Audio Phase Locked Loop (APLL)** to generate precise NTSC color carriers. This technique provides stable color instead of the more common DAC+DDS method.

**Color carrier frequencies:**
| Standard | Target Frequency | APLL Frequency |
|----------|-----------------|----------------|
| NTSC     | 14.318182 MHz   | 14.318180 MHz  |
| PAL      | 17.734476 MHz   | 17.734476 MHz  |

The APLL allows very precise frequency control needed to generate stable NTSC color on most TVs.

### Audio PWM

Audio is generated using the ESP32's **LED PWM** peripheral:
- LEDC Channel: 0
- Frequency: 40 kHz
- Resolution: 8 bits
- Output pin: GPIO 18

This technique produces sufficient audio quality for classic 80s-style sounds.

### Supported CDG Commands

The parser implements the following CDG format commands:
- `Memory Preset` (1): Clear screen with color
- `Border Preset` (2): Set border color
- `Tile Block` (6): Draw 6x12 pixel blocks
- `Tile Block XOR` (38): Draw with XOR operation
- `Scroll Preset` (20): Scroll with fill
- `Scroll Copy` (24): Scroll with copy
- `Define Transparent` (28): Transparent color
- `Load Color Table Low` (30): Color palette (colors 0-7)
- `Load Color Table High` (31): Color palette (colors 8-15)

## Project Documentation

Technical documentation and development guidelines are now consolidated in the [`.kiro/steering/`](file:///.kiro/steering/) directory:

- [**Overview**](file:///.kiro/steering/project_overview.md): High-level goals and acceptance criteria.
- [**Features**](file:///.kiro/steering/features.md): Detailed logic for sync, auto-advance, and scanning.
- [**Architecture**](file:///.kiro/steering/architecture.md): Dual-core task distribution and state machine.
- [**Hardware**](file:///.kiro/steering/hardware-pinout.md): GPIO assignments and wiring diagrams.
- [**CDG Parser**](file:///.kiro/steering/cdg-parser.md): CDG format rules and command values.
- [**Video API**](file:///.kiro/steering/composite-video-api.md): ESP32CompositeColorVideo library usage.
- [**Build**](file:///.kiro/steering/build.md): PlatformIO commands and dependencies.
- [**Code Style**](file:///.kiro/steering/code_style.md): Naming conventions and patterns.

## Project Structure

```
ESPoke/
├── .kiro/steering/        # Technical steering documents
├── platformio.ini         # PlatformIO configuration
├── README.md              # User documentation
└── src/
    ├── main.cpp           # Main program
    ├── CDGParser.h        # CDG parser (header)
    ├── CDGParser.cpp      # CDG parser implementation
    ├── Player.h           # Player (header)
    ├── Player.cpp         # Player logic
    ├── AudioPlayer.h      # Audio PWM output
    ├── AudioPlayer.cpp    # Audio implementation
    └── font6x8.h          # 6x8 bitmap font
```

## Credits and References

### Libraries and Base Projects
- **Video**: [ESP32CompositeColorVideo](https://github.com/marciot/ESP32CompositeColorVideo) by marciot
- **MP3 Audio**: [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) by pschatzmann
- **APLL Technique**: Based on [esp_8_bit](https://github.com/CornN64/esp_8_bit) by rossumur/CornN64
- **CDG Format**: Public CD+Graphics specification
- **Karaoke Reference**: [PyKaraoke](https://github.com/kelvinlawson/pykaraoke)

### How NTSC Video Works

The key principle for generating good quality NTSC color is the precision and stability of the synthesized color carrier. The DAC + DDS method at 13.33 MHz produces a beautiful waveform but very intermittent color if it works at all.

The ESP32 has an excellent tool for creating solid color carriers: the **Audio Phase Locked Loop (APLL)**. This ultra-low-noise fractional-N PLL can produce DAC sample frequencies up to ~20 MHz with very precise frequency control.

### Audio Alternatives

The project uses LED PWM for audio but there are other options:
- **PDM (Pulse Density Modulation)**: Higher quality, more complex
- **I2S**: Requires additional hardware
- **DAC**: Limited when APLL is used for video

## Troubleshooting

### No color on TV
- Check RCA connections
- Try a different TV (some TVs are more tolerant)
- Adjust synchronization

### No files detected
- Verify the SD card is formatted as FAT32
- .cdg files must be in the root or a subfolder
- File names in 8.3 format (no spaces)
- Max 100 files supported across all directories

### Noisy audio
- Check the RC filter
- Reduce volume if there is distortion

## License

MIT License - Free to use and modify.
