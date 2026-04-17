#include <Arduino.h>
#include <esp_pm.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
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

#define CDG_TASK_STACK 4096
#define CDG_TASK_PRIORITY 2

CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES);
CompositeColorOutput composite(CompositeColorOutput::NTSC);

Font<CompositeGraphics> font(6, 8, font6x8::pixels);

Player player;

TaskHandle_t cdgTaskHandle = NULL;
QueueHandle_t cdgCommandQueue;

void cdgAudioTask(void* param);

static unsigned long demoFrameCount = 0;

unsigned long lastFrameTime = 0;
const int FRAME_RATE = 10;

const char* scrollText = "  >> ESPOKE! KARAOKE PLAYER <<  PRESENTA: DEMO MODE  >> PRENSA PLAY PARA COMENZAR <<  ";
const int SCROLL_TEXT_LEN = 84;
int scrollX = 0;

void drawRasterBar(int y, int height, int hue, unsigned long frame) {
    graphics.setHue(hue);
    graphics.fillRect(0, y, CompositeColorOutput::XRES, height, 40);
}

void drawScrollText(unsigned long frame) {
    int textWidth = SCROLL_TEXT_LEN * 6;
    scrollX -= 1;
    if (scrollX < -textWidth) {
        scrollX = CompositeColorOutput::XRES;
    }
    
    int baseY = 100;
    int amplitude = 20;
    
    for (int i = 0; i < SCROLL_TEXT_LEN; i++) {
        char c = scrollText[i];
        if (c < 32 || c >= 128) continue;
        
        int charX = scrollX + i * 6;
        if (charX < -6 || charX > CompositeColorOutput::XRES + 6) continue;
        
        float waveY = sin((charX + frame * 2) * 0.03f) * amplitude;
        int charY = baseY + (int)waveY;
        
        int hue = (charX / 20 + frame / 10) % 16;
        graphics.setHue(hue);
        font.drawChar(graphics, charX, charY, c, 50, -1);
        
        graphics.setHue((hue + 8) % 16);
        font.drawChar(graphics, charX + 1, charY + 1, c, 30, -1);
    }
}

void drawSideDecorations(unsigned long frame) {
    for (int y = 0; y < CompositeColorOutput::YRES; y += 16) {
        int hue = (y / 16 + frame / 5) % 16;
        graphics.setHue(hue);
        graphics.fillRect(0, y, 4, 8, 40);
        graphics.fillRect(CompositeColorOutput::XRES - 4, y, 4, 8, 40);
    }
}

void renderDemo(unsigned long frameCount) {
    drawRasterBar(20, 30, (frameCount / 20) % 16, frameCount);
    drawRasterBar(190, 30, (frameCount / 20 + 8) % 16, frameCount);
    
    drawSideDecorations(frameCount);
    
    drawScrollText(frameCount);
    
    graphics.setHue(0);
    graphics.setTextColor(50);
    graphics.setCursor(10, 5);
    graphics.print("ESpoke!");
    
    graphics.setTextColor(40);
    graphics.setCursor(260, 5);
    graphics.print("DEMO");
    
    graphics.setCursor(10, 220);
    graphics.print("PLAY:start NEXT/PREV");
    
    if (player.getTotalFiles() > 0) {
        graphics.setCursor(200, 220);
        graphics.print("Files:");
        graphics.print(player.getTotalFiles());
    }
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
        
        xTaskCreatePinnedToCore(
            cdgAudioTask,
            "CDG/Audio",
            CDG_TASK_STACK,
            NULL,
            CDG_TASK_PRIORITY,
            &cdgTaskHandle,
            0
        );
        Serial.println("CDG/Audio task created on Core 0");
        
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
            if (srcX < 0) srcX += CDG_WIDTH;
            int srcY = (y + state.scrollOffsetY) % CDG_HEIGHT;
            if (srcY < 0) srcY += CDG_HEIGHT;
            uint8_t colorIndex = state.pixels[srcX + srcY * CDG_WIDTH];
            
            if (colorIndex != state.transparentColor) {
                uint8_t color;
                if (colorIndex >= 16) {
                    color = state.colorTable[16 + (colorIndex & 0x0F)];
                } else {
                    color = state.colorTable[colorIndex & 0x0F];
                }
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

void cdgAudioTask(void* param) {
    Serial.println("CDG/Audio task started on Core 0");
    
    const int COMMANDS_PER_FRAME = 75;
    
    while(true) {
        if (player.isPlaying()) {
            player.update();
            player.getCDGParser().getNextCommands(COMMANDS_PER_FRAME);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void loop() {
    updateButtons();
    
    unsigned long now = millis();
    if (now - lastFrameTime > (1000 / FRAME_RATE)) {
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