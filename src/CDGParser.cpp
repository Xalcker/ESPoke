#include "CDGParser.h"

CDGParser::CDGParser() : transparentColor(0xFF) {
    memset(state.pixels, 0, sizeof(state.pixels));
    memset(state.colorTable, 0, sizeof(state.colorTable));
    state.transparentColor = 0xFF;
    state.scrollOffsetX = 0;
    state.scrollOffsetY = 0;
    state.scrollHDirection = 0;
    state.scrollVDirection = 0;
    memset(colorTable, 0, sizeof(colorTable));
}

bool CDGParser::init(File file) {
    cdgFile = file;
    if (!cdgFile) {
        return false;
    }
    
    memset(state.pixels, 0, sizeof(state.pixels));
    memset(state.colorTable, 0, sizeof(state.colorTable));
    state.transparentColor = 0xFF;
    state.scrollOffsetX = 0;
    state.scrollOffsetY = 0;
    
    return true;
}

bool CDGParser::getNextCommand() {
    if (!cdgFile.available()) {
        return false;
    }
    
    uint8_t packet[CDG_PACKET_SIZE];
    if (cdgFile.read(packet, CDG_PACKET_SIZE) != CDG_PACKET_SIZE) {
        return false;
    }
    
    if ((packet[0] & 0x3F) != 0x09) {
        return false;
    }
    
    uint16_t data = ((packet[1] & 0x3F) << 8) | packet[2];
    uint8_t instruction = data & CDG_INSTRUCTION_MASK;
    uint16_t commandData = (data >> 6) & CDG_INSTRUCTION_DATA_MASK;
    
    executeCommand(instruction, commandData);
    return true;
}

bool CDGParser::getNextCommands(int maxCommands) {
    int count = 0;
    while (count < maxCommands && getNextCommand()) {
        count++;
    }
    return count > 0;
}

void CDGParser::executeCommand(uint8_t instruction, uint16_t data) {
    uint8_t rawData[4];
    rawData[0] = data & 0xFF;
    rawData[1] = (data >> 8) & 0xFF;
    rawData[2] = (data >> 4) & 0xFF;
    rawData[3] = data & 0xFF;
    
    switch (instruction) {
        case CDG_MEMORY_PRESET:
            memoryPreset((data >> 1) & 0x0F, (data >> 1) & 0x0F);
            break;
            
        case CDG_BORDER_PRESET:
            borderPreset((data >> 1) & 0x0F);
            break;
            
        case CDG_TILE_BLOCK:
            tileBlock(rawData, false);
            break;
            
        case CDG_TILE_BLOCK_XOR:
            tileBlock(rawData, true);
            break;
            
        case CDG_SCROLL_PRESET:
            scroll((data >> 2) & 0x3F, (data >> 8) & 0x3F, false);
            break;
            
        case CDG_SCROLL_COPY:
            scroll((data >> 2) & 0x3F, (data >> 8) & 0x3F, true);
            break;
            
        case CDG_DEFINE_TRANSPARENT:
            defineTransparent(data & 0x0F);
            break;
            
        case CDG_LOAD_STATIC_COLOR_TABLE:
            loadColorTable(rawData);
            break;
            
        case CDG_LOAD_STATIC_DATA:
            loadStaticData(rawData);
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
        for (int i = 0; i < 6; i++) {
            int idx = j * 6 + i;
            uint8_t color = (data[4 + idx] & 0x30) >> 4;
            color |= (data[4 + idx] & 0x03) << 2;
            
            if (color == 0) {
                color = color0;
            } else if (color == 1) {
                color = color1;
            } else {
                continue;
            }
            
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

void CDGParser::scroll(int hScroll, int vScroll, bool copy) {
    int hCmd = (hScroll >> 4) & 0x03;
    int hOffset = hScroll & 0x3F;
    int vCmd = (vScroll >> 4) & 0x03;
    int vOffset = vScroll & 0x3F;
    
    uint8_t fillColor = 0;
    if (!copy) {
        fillColor = state.colorTable[0] & 0x0F;
    }
    
    int newX = state.scrollOffsetX;
    int newY = state.scrollOffsetY;
    
    switch (vCmd) {
        case 0: newY = vOffset; break;
        case 1: newY += vOffset; break;
        case 2: newY -= vOffset; break;
    }
    
    switch (hCmd) {
        case 0: newX = hOffset; break;
        case 1: newX += hOffset; break;
        case 2: newX -= hOffset; break;
    }
    
    while (newX < 0) newX += CDG_WIDTH;
    while (newX >= CDG_WIDTH) newX -= CDG_WIDTH;
    while (newY < 0) newY += CDG_HEIGHT;
    while (newY >= CDG_HEIGHT) newY -= CDG_HEIGHT;
    
    if (newX != state.scrollOffsetX || newY != state.scrollVDirection) {
        uint8_t tempPixels[CDG_WIDTH * CDG_HEIGHT];
        memcpy(tempPixels, state.pixels, sizeof(tempPixels));
        
        int shiftX = (newX - state.scrollOffsetX);
        int shiftY = (newY - state.scrollOffsetY);
        
        if (shiftX < 0) shiftX += CDG_WIDTH;
        if (shiftY < 0) shiftY += CDG_HEIGHT;
        
        for (int y = 0; y < CDG_HEIGHT; y++) {
            for (int x = 0; x < CDG_WIDTH; x++) {
                int srcX = (x - shiftX + CDG_WIDTH) % CDG_WIDTH;
                int srcY = (y - shiftY + CDG_HEIGHT) % CDG_HEIGHT;
                state.pixels[x + y * CDG_WIDTH] = tempPixels[srcX + srcY * CDG_WIDTH];
            }
        }
    }
    
    state.scrollOffsetX = newX;
    state.scrollOffsetY = newY;
}

void CDGParser::defineTransparent(uint8_t color) {
    state.transparentColor = color;
}

void CDGParser::loadColorTable(uint8_t* data) {
    int index = ((data[0] & 0x0F) << 4);
    for (int i = 0; i < 16; i++) {
        state.colorTable[index + i] = ((data[2 * i + 1] & 0x0F) << 4) | (data[2 * i + 2] & 0x0F);
    }
}

void CDGParser::loadStaticData(uint8_t* data) {
    int index = ((data[0] & 0x3F) << 4) | ((data[1] & 0xF0) >> 4);
    for (int i = 0; i < 12; i++) {
        uint8_t b = data[2 + i];
        state.colorTable[16 + index + i] = b;
    }
}

void CDGParser::setPixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= CDG_WIDTH || y < 0 || y >= CDG_HEIGHT) return;
    
    int offset = x + y * CDG_WIDTH;
    state.pixels[offset] = color;
}

uint8_t CDGParser::getPixel(int x, int y) {
    if (x < 0 || x >= CDG_WIDTH || y < 0 || y >= CDG_HEIGHT) return 0;
    
    int offset = x + y * CDG_WIDTH;
    return state.pixels[offset];
}
