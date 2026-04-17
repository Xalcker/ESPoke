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
static unsigned long beepEndTime = 0;

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

struct LogoParticle {
    float x, y;
    float vx, vy;
    float size;
    int hue;
    bool active;
};

const int MAX_PARTICLES = 20;
LogoParticle particles[MAX_PARTICLES];

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
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
    demoInitialized = true;
}

void drawRotatingDiamond(int cx, int cy, float size, int hue, unsigned long frame) {
    float angle = frame * 0.02;
    graphics.setHue(hue % 16);
    
    short vertices[4][2];
    for (int i = 0; i < 4; i++) {
        float a = angle + i * 1.5708f;
        vertices[i][0] = cx + cos(a) * size;
        vertices[i][1] = cy + sin(a) * size * 0.6f;
    }
    
    graphics.triangle(vertices[0], vertices[1], vertices[2], 40);
    graphics.triangle(vertices[1], vertices[2], vertices[3], 35);
    
    float innerSize = size * 0.5f;
    short innerVerts[4][2];
    for (int i = 0; i < 4; i++) {
        float a = angle + i * 1.5708f + 0.7854f;
        innerVerts[i][0] = cx + cos(a) * innerSize;
        innerVerts[i][1] = cy + sin(a) * innerSize * 0.6f;
    }
    
    graphics.triangle(innerVerts[0], innerVerts[1], innerVerts[2], 50);
    graphics.triangle(innerVerts[1], innerVerts[2], innerVerts[3], 45);
    
    graphics.setHue((hue + 4) % 16);
    graphics.fillRect(cx - 6, cy - 6, 12, 12, 55);
    graphics.setHue(hue % 16);
    graphics.fillRect(cx - 3, cy - 3, 6, 6, 60);
}

void spawnParticle(int cx, int cy, int hue) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            float angle = random(0, 628) / 100.0f;
            particles[i].x = cx;
            particles[i].y = cy;
            particles[i].vx = cos(angle) * random(1, 4);
            particles[i].vy = sin(angle) * random(1, 4);
            particles[i].size = random(2, 5);
            particles[i].hue = hue;
            particles[i].active = true;
            break;
        }
    }
}

void updateAndDrawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].size *= 0.97f;
            
            if (particles[i].size < 1) {
                particles[i].active = false;
            } else {
                int hue = (particles[i].hue + (int)particles[i].x / 10) % 16;
                graphics.setHue(hue);
                graphics.fillRect((int)particles[i].x, (int)particles[i].y, 
                                  (int)particles[i].size, (int)particles[i].size, 40);
            }
        }
    }
}

void drawAnimatedLogo(int centerX, int centerY, unsigned long frame) {
    static int lastSpawnFrame = 0;
    
    if (frame - lastSpawnFrame > 5) {
        int hue = (frame / 20) % 16;
        spawnParticle(centerX, centerY, hue);
        spawnParticle(centerX, centerY, hue);
        lastSpawnFrame = frame;
    }
    
    int baseHue = (frame / 15) % 16;
    drawRotatingDiamond(centerX, centerY, 40, baseHue, frame);
    drawRotatingDiamond(centerX, centerY, 25, (baseHue + 8) % 16, frame + 100);
    
    updateAndDrawParticles();
}

void drawWaveDecoration(int y, unsigned long frame) {
    graphics.setHue((frame / 40) % 16);
    for (int x = 0; x < CompositeColorOutput::XRES; x += 8) {
        int waveY = y + sin((x + frame * 2) * 0.05f) * 5;
        graphics.dot(x, waveY, 30);
        graphics.dot(x + 1, waveY + 1, 25);
    }
}

void drawMicrophoneIcon(int cx, int cy, unsigned long frame) {
    int hue = (frame / 20) % 16;
    graphics.setHue(hue);
    
    graphics.fillRect(cx - 4, cy - 16, 8, 20, 50);
    graphics.fillRect(cx - 6, cy - 12, 12, 12, 40);
    
    graphics.fillRect(cx - 2, cy + 4, 4, 8, 45);
    graphics.fillRect(cx - 8, cy + 12, 16, 2, 45);
    graphics.fillRect(cx - 6, cy + 14, 12, 6, 50);
    
    graphics.setHue((hue + 8) % 16);
    graphics.fillRect(cx - 3, cy - 10, 6, 8, 60);
}

void drawMusicNotes(int cx, int cy, unsigned long frame) {
    int hue = (frame / 25) % 16;
    graphics.setHue(hue);
    
    float bounce = sin(frame * 0.08f) * 4;
    
    graphics.fillRect(cx - 12, cy - 6 + bounce, 2, 12, 45);
    graphics.fillRect(cx - 12, cy - 14 + bounce, 8, 2, 45);
    graphics.dot(cx - 4, cy - 4 + bounce, 50);
    
    graphics.fillRect(cx + 6, cy + 2 - bounce, 2, 10, 40);
    graphics.fillRect(cx + 6, cy - 6 - bounce, 8, 2, 40);
    graphics.dot(cx + 14, cy - 2 - bounce, 55);
}

void renderDemo(unsigned long frameCount) {
    if (!demoInitialized) {
        initDemo();
    }
    
    int centerX = CompositeColorOutput::XRES / 2;
    int centerY = CompositeColorOutput::YRES / 2 - 20;
    
    drawAnimatedLogo(centerX, centerY, frameCount);
    
    drawMicrophoneIcon(centerX - 55, centerY - 10, frameCount);
    drawMusicNotes(centerX + 55, centerY - 10, frameCount);
    
    graphics.setHue(0);
    graphics.setTextColor(50);
    graphics.setCursor(70, 5);
    graphics.print("ESpoke!");
    
    graphics.setTextColor(35);
    graphics.setCursor(35, 18);
    graphics.print("Karaoke Player");
    
    drawWaveDecoration(185, frameCount);
    drawWaveDecoration(195, frameCount);
    
    graphics.setTextColor(40);
    graphics.setCursor(80, 220);
    graphics.print("Press PLAY to start");
    
    if (player.getTotalFiles() > 0) {
        graphics.setCursor(10, 5);
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
    
    ledcSetup(1, 523, 8);
    ledcAttachPin(AUDIO_PIN, 1);
    ledcWrite(1, 50);
    beepEndTime = millis() + 300;
    
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

void cdgAudioTask(void* param) {
    Serial.println("CDG/Audio task started on Core 0");
    
    while(true) {
        if (player.isPlaying()) {
            player.update();
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
    
    if (beepEndTime && millis() >= beepEndTime) {
        ledcWrite(1, 0);
        beepEndTime = 0;
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