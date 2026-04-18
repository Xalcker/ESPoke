# Build Guide - ESPoke

Karaoke player for ESP32 with composite video.

## Prerequisites

### Hardware
- ESP32 board (any variant)
- USB cable for programming
- SD card reader
- Components for connections (see README.md)

### Software
- Python 3.x
- PlatformIO Core (CLI)

---

## Installation

### 1. Install Python

**Windows:**
Download from https://www.python.org/downloads/
(check "Add Python to PATH" during installation)

**Linux:**
```bash
sudo apt update
sudo apt install python3 python3-pip
```

**macOS:**
```bash
brew install python3
```

### 2. Install PlatformIO Core

```bash
pip install platformio
```

Or using the official installer:
```bash
curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py | python3
```

### 3. Verify installation

```bash
pio --version
```

Should display something like `PlatformIO Core, version x.x.x`

---

## Build the Project

### Navigate to the project directory

```bash
cd path/to/ESPoke
```

### Build

```bash
pio run
```

This will download the necessary dependencies and build the project.

**First build may take several minutes** (toolchain download, libraries, etc.)

### Build output

If everything goes well, you'll see something like:

```
Parsing XML...
...
Linking .pio/build/esp32dev/firmware.elf
Calculating size .pio/build/esp32dev/firmware.elf
RAM:   [=         ]  10.2% (used 33408 bytes from 327680 bytes)
Flash: [==        ]  18.5% (used 484128 bytes from 1310720 bytes)
```

---

## Upload to ESP32

### 1. Connect the ESP32

Connect the ESP32 to your computer via USB cable.

### 2. Identify the port

**Windows:**
```bash
pio device list
```
Look for something like `COM3`, `COM4`, etc.

**Linux/macOS:**
```bash
pio device list
```
Look for something like `/dev/ttyUSB0`, `/dev/cu.usbserial-xxx`, etc.

### 3. Upload the firmware

```bash
pio run --target upload
```

Or specify the port:
```bash
pio run --target upload --upload-port COM3
```

### 4. Serial monitor (optional)

To view program messages:

```bash
pio device monitor
```

---

## Troubleshooting

### Error: "python not found"

Add Python to PATH or use `python3` instead of `python`.

### Error: "Permission denied" when uploading

**Linux/macOS:**
```bash
sudo usermod -a -G dialout $USER
# Log out and log back in
```

### Error: "Failed to connect"

- Verify the USB cable works
- Press the ESP32 BOOT button while connecting
- Select the correct port

### Build error with libraries

Clean and rebuild:
```bash
pio run --target clean
pio run
```

---

## Customization

### Change pins

Edit `src/main.cpp`:
```cpp
#define BTN_PLAY 0      // Change GPIO
#define BTN_NEXT 2
// etc.

#define PIN_DAC 25      // Video pin
#define AUDIO_PIN 18    // Audio pin
```

### Change frame rate

In `src/main.cpp`:
```cpp
const int FRAME_RATE = 10;  // Frames per second for CDG
```

---

## Useful PlatformIO Commands

| Command | Description |
|---------|-------------|
| `pio run` | Build |
| `pio run --target upload` | Build and upload |
| `pio device list` | List serial ports |
| `pio device monitor` | Open serial monitor |
| `pio run --target clean` | Clean build files |
| `pio lib list` | List installed libraries |
| `pio pkg list` | List project dependencies |

---

## Next Steps

Once the firmware is uploaded:
1. Connect the hardware (see README.md)
2. Place .cdg and .mp3 files on the SD card
3. Power the ESP32 and connect to a TV

For more technical details, see README.md.
