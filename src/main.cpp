#include <Arduino.h>
#include <esp_pm.h>
#include <CompositeGraphics.h>
#include <CompositeColorOutput.h>
#include <SD.h>
#include <Font.h>
#include "font6x8.h"

#include "Player.h"

#define BTN_PLAY 0
#define BTN_NEXT 2
#define BTN_PREV 4
#define BTN_VOL_UP 15
#define BTN_VOL_DN 13

CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES);
CompositeColorOutput composite(CompositeColorOutput::NTSC);

Font<CompositeGraphics> font(6, 8, font6x8::pixels);

Player player;

unsigned long lastFrameTime = 0;
const int FRAME_RATE = 10;

struct Ball {
    int x, y;
    int vx, vy;
    int radius;
    int hue;
};

const int NUM_BALLS = 5;
Ball balls[NUM_BALLS];
bool demoInitialized = false;

void initDemo() {
    for (int i = 0; i < NUM_BALLS; i++) {
        balls[i].x = 50 + random(200);
        balls[i].y = 50 + random(100);
        balls[i].vx = random(-3, 4);
        balls[i].vy = random(-3, 4);
        balls[i].radius = random(5, 15);
        balls[i].hue = i * 3;
        if (balls[i].vx == 0) balls[i].vx = 1;
        if (balls[i].vy == 0) balls[i].vy = 1;
    }
    demoInitialized = true;
}

void renderDemo(unsigned long frameCount) {
    if (!demoInitialized) {
        initDemo();
    }
    
    for (int i = 0; i < NUM_BALLS; i++) {
        balls[i].x += balls[i].vx;
        balls[i].y += balls[i].vy;
        
        if (balls[i].x < balls[i].radius || balls[i].x > CompositeColorOutput::XRES - balls[i].radius) {
            balls[i].vx = -balls[i].vx;
        }
        if (balls[i].y < balls[i].radius + 20 || balls[i].y > CompositeColorOutput::YRES - balls[i].radius - 20) {
            balls[i].vy = -balls[i].vy;
        }
        
        int hue = (balls[i].hue + frameCount / 20) % 16;
        graphics.setHue(hue);
        
        for (int r = balls[i].radius; r > 0; r--) {
            graphics.fillRect(balls[i].x - r, balls[i].y - r, r * 2, r * 2, 50);
        }
        
        graphics.setHue((hue + 8) % 16);
        graphics.fillRect(balls[i].x - 2, balls[i].y - 2, 4, 4, 60);
    }
    
    graphics.setHue(0);
    graphics.setTextColor(50);
    graphics.setCursor(10, 5);
    graphics.print("ESP32 Karaoke Player");
    graphics.setCursor(10, 220);
    graphics.print("Press PLAY to start");
    
    if (player.getTotalFiles() > 0) {
        graphics.setCursor(200, 220);
        graphics.print("Files:");
        graphics.print(player.getTotalFiles());
    }
    
    graphics.setCursor(10, 230);
    graphics.setTextColor(30);
    graphics.print("Use buttons: NEXT/PREV");
}

void setup() {
    esp_pm_lock_handle_t powerManagementLock;
    esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "compositeCorePerformanceLock", &powerManagementLock);
    esp_pm_lock_acquire(powerManagementLock);
    
    Serial.begin(115200);
    delay(1000);
    
    pinMode(BTN_PLAY, INPUT_PULLUP);
    pinMode(BTN_NEXT, INPUT_PULLUP);
    pinMode(BTN_PREV, INPUT_PULLUP);
    pinMode(BTN_VOL_UP, INPUT_PULLUP);
    pinMode(BTN_VOL_DN, INPUT_PULLUP);
    
    composite.init();
    graphics.init();
    graphics.setFont(font);
    
    Serial.println("Initializing player...");
    if (player.init()) {
        player.scanDirectory("/");
        if (player.getTotalFiles() > 0) {
            player.play();
        }
    }
    
    delay(500);
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
    
    int offsetX = (CompositeColorOutput::XRES - CDG_DISPLAY_WIDTH) / 2;
    int offsetY = (CompositeColorOutput::YRES - CDG_DISPLAY_HEIGHT) / 2;
    
    for (int y = 0; y < CDG_DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < CDG_DISPLAY_WIDTH; x++) {
            int srcX = (x + state.scrollOffsetX) % CDG_WIDTH;
            int srcY = (y + state.scrollOffsetY) % CDG_HEIGHT;
            uint8_t colorIndex = state.pixels[srcX + srcY * CDG_WIDTH];
            
            if (colorIndex != state.transparentColor && colorIndex < 16) {
                uint8_t color = state.colorTable[colorIndex & 0x0F];
                uint8_t hue = (color >> 4) & 0x0F;
                uint8_t brightness = color & 0x0F;
                graphics.setHue(hue);
                graphics.dot(offsetX + x, offsetY + y, brightness * 4);
            }
        }
    }
}

void renderUI() {
    if (player.isPlaying()) {
        graphics.setHue(0);
        graphics.setTextColor(50);
        graphics.setCursor(260, 5);
        graphics.print("Playing");
        
        graphics.setCursor(260, 15);
        graphics.print("Vol:");
        graphics.print(player.getVolume() / 25);
        graphics.print("%");
    }
}

static unsigned long demoFrameCount = 0;

void loop() {
    updateButtons();
    
    unsigned long now = millis();
    if (now - lastFrameTime > (1000 / FRAME_RATE)) {
        player.update();
        lastFrameTime = now;
        demoFrameCount++;
    }
    
    graphics.setHue(0);
    graphics.begin(0);
    
    if (player.isPlaying() && player.getTotalFiles() > 0) {
        renderCDG();
    } else {
        renderDemo(demoFrameCount);
    }
    
    renderUI();
    graphics.end();
    
    composite.sendFrameHalfResolution(&graphics.frame);
}