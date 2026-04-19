#include "CDGParser.h"

CDGParser::CDGParser() : scrollBuffer(nullptr), packetsProcessed(0) {
    mutex = xSemaphoreCreateMutex();
    memset(state.pixels, 0, sizeof(state.pixels));
    memset(state.colorTable, 0, sizeof(state.colorTable));
    state.transparentColor = 0xFF;
    state.scrollOffsetX = 0;
    state.scrollOffsetY = 0;
    state.scrollHDirection = 0;
    state.scrollVDirection = 0;
}

CDGParser::~CDGParser() {
    if (scrollBuffer) {
        free(scrollBuffer);
        scrollBuffer = nullptr;
    }
    if (mutex) {
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

bool CDGParser::init(File file) {
    cdgFile = file;
    if (!cdgFile) {
        return false;
    }

    lock();
    memset(state.pixels, 0, sizeof(state.pixels));
    memset(state.colorTable, 0, sizeof(state.colorTable));
    state.transparentColor = 0xFF;
    state.scrollOffsetX = 0;
    state.scrollOffsetY = 0;
    packetsProcessed = 0;

    // Allocate scroll buffer on heap once (64,800 bytes)
    if (!scrollBuffer) {
        scrollBuffer = (uint8_t*)malloc(CDG_WIDTH * CDG_HEIGHT);
    }
    unlock();

    return true;
}

void CDGParser::lock() {
    xSemaphoreTake(mutex, portMAX_DELAY);
}

void CDGParser::unlock() {
    xSemaphoreGive(mutex);
}

bool CDGParser::getNextCommand() {
    if (!cdgFile.available()) {
        return false;
    }

    uint8_t packet[CDG_PACKET_SIZE];
    if (cdgFile.read(packet, CDG_PACKET_SIZE) != CDG_PACKET_SIZE) {
        return false;
    }

    // Only process CD+G packets (command byte masked = 0x09)
    if ((packet[0] & 0x3F) != 0x09) {
        return true; // not a CDG packet, skip
    }

    uint8_t instruction = packet[1] & 0x3F;

    // Extract the 16-byte data payload, each byte masked to 6 bits
    uint8_t data[CDG_DATA_SIZE];
    for (int i = 0; i < CDG_DATA_SIZE; i++) {
        data[i] = packet[4 + i] & 0x3F;
    }

    lock();
    executeCommand(instruction, data);
    unlock();
    packetsProcessed++;
    return true;
}

bool CDGParser::getNextCommands(int maxCommands) {
    int count = 0;
    while (count < maxCommands && getNextCommand()) {
        count++;
    }
    return count > 0;
}

// Advance CDG stream to match elapsed time (300 packets/sec)
void CDGParser::syncToTime(unsigned long elapsedMs) {
    unsigned long targetPacket = (elapsedMs * 300UL) / 1000UL;
    int catchUp = (int)(targetPacket - packetsProcessed);
    if (catchUp <= 0) return;
    // Cap to avoid blocking too long in one call
    if (catchUp > 300) catchUp = 300;
    getNextCommands(catchUp);
}

void CDGParser::executeCommand(uint8_t instruction, uint8_t* data) {
    switch (instruction) {
        case CDG_MEMORY_PRESET:
            memoryPreset(data[0] & 0x0F, data[1] & 0x0F);
            break;

        case CDG_BORDER_PRESET:
            borderPreset(data[0] & 0x0F);
            break;

        case CDG_TILE_BLOCK:
            tileBlock(data, false);
            break;

        case CDG_TILE_BLOCK_XOR:
            tileBlock(data, true);
            break;

        case CDG_SCROLL_PRESET:
            scroll(data, false);
            break;

        case CDG_SCROLL_COPY:
            scroll(data, true);
            break;

        case CDG_DEFINE_TRANSPARENT:
            defineTransparent(data[0] & 0x0F);
            break;

        case CDG_LOAD_COLOR_TABLE_LOW:
            loadColorTable(data, 0);
            break;

        case CDG_LOAD_COLOR_TABLE_HIGH:
            loadColorTable(data, 8);
            break;
    }
}

void CDGParser::memoryPreset(uint8_t color, uint8_t repeat) {
    uint8_t c = color & 0x0F;
    for (int i = 0; i < CDG_WIDTH * CDG_HEIGHT; i++) {
        state.pixels[i] = c;
    }
}

void CDGParser::borderPreset(uint8_t color) {
    uint8_t c = color & 0x0F;
    for (int y = 0; y < CDG_HEIGHT; y++) {
        for (int x = 0; x < CDG_WIDTH; x++) {
            if (x < 6 || x >= CDG_WIDTH - 6 || y < 12 || y >= CDG_HEIGHT - 12) {
                setPixel(x, y, c);
            }
        }
    }
}

void CDGParser::tileBlock(uint8_t* data, bool xorMode) {
    int color0 = data[0] & 0x0F;
    int color1 = data[1] & 0x0F;
    int row = (data[2] & 0x1F) * 12;
    int col = (data[3] & 0x3F) * 6;

    for (int j = 0; j < 12; j++) {
        uint8_t rowBits = data[4 + j] & 0x3F; // 6 pixels packed in 6 bits
        for (int i = 0; i < 6; i++) {
            int bit = (rowBits >> (5 - i)) & 0x01;
            uint8_t color = bit ? color1 : color0;

            int x = col + i;
            int y = row + j;

            if (xorMode) {
                setPixel(x, y, getPixel(x, y) ^ color);
            } else {
                setPixel(x, y, color);
            }
        }
    }
}

// Scroll uses heap-allocated scrollBuffer instead of stack to avoid
// blowing the 4KB FreeRTOS task stack with a 64KB local array.
void CDGParser::scroll(uint8_t* data, bool copy) {
    if (!scrollBuffer) return; // safety check

    uint8_t color = data[0] & 0x0F;
    int hCmd    = (data[1] >> 4) & 0x03;
    int hOffset = data[1] & 0x07;
    int vCmd    = (data[2] >> 4) & 0x03;
    int vOffset = data[2] & 0x0F;

    int shiftX = 0;
    int shiftY = 0;

    switch (hCmd) {
        case 0: break;
        case 1: shiftX = 6; break;
        case 2: shiftX = -6; break;
    }

    switch (vCmd) {
        case 0: break;
        case 1: shiftY = 12; break;
        case 2: shiftY = -12; break;
    }

    state.scrollOffsetX = hOffset;
    state.scrollOffsetY = vOffset;

    if (shiftX == 0 && shiftY == 0) return;

    memcpy(scrollBuffer, state.pixels, CDG_WIDTH * CDG_HEIGHT);

    for (int y = 0; y < CDG_HEIGHT; y++) {
        for (int x = 0; x < CDG_WIDTH; x++) {
            int srcX = x - shiftX;
            int srcY = y - shiftY;

            if (copy) {
                srcX = (srcX + CDG_WIDTH) % CDG_WIDTH;
                srcY = (srcY + CDG_HEIGHT) % CDG_HEIGHT;
                state.pixels[x + y * CDG_WIDTH] = scrollBuffer[srcX + srcY * CDG_WIDTH];
            } else {
                if (srcX >= 0 && srcX < CDG_WIDTH && srcY >= 0 && srcY < CDG_HEIGHT) {
                    state.pixels[x + y * CDG_WIDTH] = scrollBuffer[srcX + srcY * CDG_WIDTH];
                } else {
                    state.pixels[x + y * CDG_WIDTH] = color;
                }
            }
        }
    }
}

void CDGParser::defineTransparent(uint8_t color) {
    state.transparentColor = color;
}

void CDGParser::loadColorTable(uint8_t* data, int tableOffset) {
    for (int i = 0; i < 8; i++) {
        int idx = tableOffset + i;
        if (idx >= 16) break;

        uint8_t highByte = data[i * 2] & 0x3F;
        uint8_t lowByte  = data[i * 2 + 1] & 0x3F;

        // CDG color: [---RRRRGG] [---GGBBBB]
        int r = (highByte >> 2) & 0x0F;
        int g = ((highByte & 0x03) << 2) | ((lowByte >> 4) & 0x03);
        int b = lowByte & 0x0F;

        // Map RGB to hue (0-15) and brightness for composite palette
        int brightness = (r + g + b) / 3;
        int hue = 0;
        if (r + g + b > 0) {
            if (r >= g && r >= b) {
                hue = (g > b) ? 1 : 14;
            } else if (g >= r && g >= b) {
                hue = (r > b) ? 3 : 6;
            } else {
                hue = (r > g) ? 12 : 9;
            }
        }

        state.colorTable[idx] = ((hue & 0x0F) << 4) | (brightness & 0x0F);
    }
}

void CDGParser::setPixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= CDG_WIDTH || y < 0 || y >= CDG_HEIGHT) return;
    state.pixels[x + y * CDG_WIDTH] = color;
}

uint8_t CDGParser::getPixel(int x, int y) {
    if (x < 0 || x >= CDG_WIDTH || y < 0 || y >= CDG_HEIGHT) return 0;
    return state.pixels[x + y * CDG_WIDTH];
}
