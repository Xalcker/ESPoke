#ifndef CDG_PARSER_H
#define CDG_PARSER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <freertos/semphr.h>

#define CDG_WIDTH 300
#define CDG_HEIGHT 216
#define CDG_DISPLAY_WIDTH 288
#define CDG_DISPLAY_HEIGHT 192

#define CDG_PACKET_SIZE 24
#define CDG_DATA_SIZE 16

enum CDGCommand {
    CDG_MEMORY_PRESET = 1,
    CDG_BORDER_PRESET = 2,
    CDG_TILE_BLOCK = 6,
    CDG_SCROLL_PRESET = 20,
    CDG_SCROLL_COPY = 24,
    CDG_DEFINE_TRANSPARENT = 28,
    CDG_LOAD_STATIC_COLOR_TABLE = 30,
    CDG_TILE_BLOCK_XOR = 38,
    CDG_LOAD_STATIC_DATA = 31
};

class CDGParser {
public:
    CDGParser();
    bool init(File cdgFile);
    bool getNextCommand();
    bool getNextCommands(int maxCommands);
    uint8_t* getColorTable() { return state.colorTable; }
    uint8_t getTransparentColor() { return state.transparentColor; }

    void lock();
    void unlock();

    struct CDGState {
        uint8_t pixels[CDG_WIDTH * CDG_HEIGHT];
        uint8_t colorTable[16 * 2];
        uint8_t transparentColor;
        int scrollOffsetX;
        int scrollOffsetY;
        int scrollHDirection;
        int scrollVDirection;
    };

    CDGState& getState() { return state; }

private:
    File cdgFile;
    CDGState state;
    SemaphoreHandle_t mutex;

    // Heap-allocated scroll buffer to avoid 64KB stack allocation
    uint8_t* scrollBuffer;

    void executeCommand(uint8_t instruction, uint8_t* data);
    void memoryPreset(uint8_t color, uint8_t repeat);
    void borderPreset(uint8_t color);
    void tileBlock(uint8_t* data, bool xorMode);
    void scroll(uint8_t* data, bool copy);
    void defineTransparent(uint8_t color);
    void loadColorTable(uint8_t* data, int tableOffset);
    void loadStaticData(uint8_t* data);

    void setPixel(int x, int y, uint8_t color);
    uint8_t getPixel(int x, int y);
};

#endif
