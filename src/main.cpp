#define USE_ATARI_COLORS
#include <Arduino.h>
#include <CompositeGraphics.h>
#include <CompositeVideo.h>
#include <SD.h>
#include <driver/dac.h>

#include "Player.h"

#define PIN_DAC 25

#define BTN_PLAY 0
#define BTN_NEXT 2
#define BTN_PREV 4
#define BTN_VOL_UP 15
#define BTN_VOL_DN 13

CompositeGraphics graphics(336, 240);
CompositeVideo video(graphics);

Player player;

unsigned long lastFrameTime = 0;
const int FRAME_RATE = 10;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(BTN_PLAY, INPUT_PULLUP);
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(BTN_PREV, INPUT_PULLUP);
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DN, INPUT_PULLUP);
    
    video.init(NTSC, PIN_DAC);
    video.start();
    
    graphics.begin(0);
    graphics.setFrameRate(60);
    
    Serial.println("Initializing player...");
    if (player.init()) {
        player.scanDirectory("/");
        if (player.getTotalFiles() > 0) {
            player.play();
        }
    }
    
    graphics.fill(0);
    graphics.setTextSize(2);
    graphics.setCursor(10, 100);
    graphics.println("ESP32 Karaoke");
    graphics.setCursor(10, 130);
    graphics.setTextSize(1);
    graphics.println("Press PLAY to start");
    
    delay(2000);
}

void updateButtons() {
    if (digitalRead(BTN_PLAY) == LOW) {
        delay(50);
        if (digitalRead(BTN_PLAY) == LOW) {
            if (player.isPlaying()) {
                player.pause();
            } else {
                player.play();
            }
            while (digitalRead(BTN_PLAY) == LOW);
        }
    }
    
    if (digitalRead(BTN_NEXT) == LOW) {
        delay(50);
        if (digitalRead(BTN_NEXT) == LOW) {
            player.next();
            while (digitalRead(BTN_NEXT) == LOW);
        }
    }
    
    if (digitalRead(BTN_PREV) == LOW) {
        delay(50);
        if (digitalRead(BTN_PREV) == LOW) {
            player.previous();
            while (digitalRead(BTN_PREV) == LOW);
        }
    }
    
    if (digitalRead(BTN_VOL_UP) == LOW) {
        delay(50);
        if (digitalRead(BTN_VOL_UP) == LOW) {
            uint8_t vol = player.getVolume();
            if (vol < 255) player.setVolume(vol + 16);
            while (digitalRead(BTN_VOL_UP) == LOW);
        }
    }
    
    if (digitalRead(BTN_VOL_DN) == LOW) {
        delay(50);
        if (digitalRead(BTN_VOL_DN) == LOW) {
            uint8_t vol = player.getVolume();
            if (vol > 0) player.setVolume(vol - 16);
            while (digitalRead(BTN_VOL_DN) == LOW);
        }
    }
}

void renderCDG() {
    CDGParser::CDGState& state = player.getCDGParser().getState();
    
    graphics.setColor(0);
    graphics.fillRect(0, 0, 336, 240);
    
    int offsetX = (336 - CDG_DISPLAY_WIDTH) / 2;
    int offsetY = (240 - CDG_DISPLAY_HEIGHT) / 2;
    
    for (int y = 0; y < CDG_DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < CDG_DISPLAY_WIDTH; x++) {
            int srcX = (x + state.scrollOffsetX) % CDG_WIDTH;
            int srcY = (y + state.scrollOffsetY) % CDG_HEIGHT;
            uint8_t colorIndex = state.pixels[srcX + srcY * CDG_WIDTH];
            
            if (colorIndex != state.transparentColor && colorIndex < 16) {
                uint8_t color = state.colorTable[colorIndex & 0x0F];
                uint8_t hue = (color >> 4) & 0x0F;
                uint8_t brightness = color & 0x0F;
                uint16_t atariCol = graphics.Color(brightness * 17, hue);
                graphics.setColor(atariCol);
                graphics.fillRect(offsetX + x, offsetY + y, 1, 1);
            }
        }
    }
}

void renderUI() {
    if (player.getTotalFiles() > 0) {
        graphics.setTextSize(1);
        graphics.setCursor(10, 5);
        graphics.print("Track: ");
        graphics.print(player.getCurrentFileIndex() + 1);
        graphics.print("/");
        graphics.println(player.getTotalFiles());
        
        graphics.setCursor(10, 220);
        graphics.print("Vol: ");
        graphics.print(player.getVolume() / 25);
        graphics.print("%");
    } else {
        graphics.setCursor(10, 100);
        graphics.setTextSize(2);
        graphics.println("No CDG files");
        graphics.setTextSize(1);
        graphics.setCursor(10, 130);
        graphics.println("Place .cdg files on SD card");
    }
}

void loop() {
    updateButtons();
    
    unsigned long now = millis();
    if (now - lastFrameTime > (1000 / FRAME_RATE)) {
        player.update();
        lastFrameTime = now;
    }
    
    if (player.isPlaying()) {
        renderCDG();
    } else {
        graphics.fill(0);
        graphics.setColor(7);
        graphics.setTextSize(2);
        graphics.setCursor(10, 100);
        graphics.println("PAUSED");
    }
    
    renderUI();
}
