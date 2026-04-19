#include <Arduino.h>
#include <esp_pm.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <CompositeGraphics.h>
#include <CompositeColorOutput.h>
#include <SPI.h>
#include <SD.h>
#include <Font.h>
#include "font6x8.h"

#include "Player.h"
#include "splash_cdg.h"

// --- Pin definitions ---
#define BTN_PLAY   0
#define BTN_NEXT   2
#define BTN_PREV   4
#define BTN_VOL_UP 15
#define BTN_VOL_DN 13

#define SD_CLK  14
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS    5

#define CDG_TASK_STACK    8192
#define CDG_TASK_PRIORITY 2

// --- App states ---
enum AppState {
    STATE_SPLASH,
    STATE_BROWSER,
    STATE_PLAYING,
    STATE_SCREENSAVER
};

AppState appState = STATE_SPLASH;
unsigned long splashStartTime = 0;
const unsigned long SPLASH_DURATION = 3000; // 3 seconds

unsigned long lastActivityTime = 0;
const unsigned long SCREENSAVER_TIMEOUT = 300000; // 5 minutes

int browserSelection = 0;   // currently highlighted file in browser
int browserScrollTop = 0;   // first visible file index
const int BROWSER_VISIBLE_LINES = 18; // max files visible on screen

// --- Globals ---
CompositeGraphics graphics(CompositeColorOutput::XRES, CompositeColorOutput::YRES);
CompositeColorOutput composite(CompositeColorOutput::NTSC);
Font<CompositeGraphics> font(6, 8, font6x8::pixels);

Player player;
TaskHandle_t cdgTaskHandle = NULL;

unsigned long lastFrameTime = 0;
const int FRAME_RATE = 15;
unsigned long frameCount = 0;

// --- Forward declarations ---
void cdgAudioTask(void* param);
bool debounceButton(int pin);

// ============================================================
// Splash screen
// ============================================================
void renderSplash() {
    CDGParser::CDGState& st = player.getCDGParser().getState();

    int offsetX = (CompositeColorOutput::XRES - CDG_WIDTH) / 2;
    int offsetY = (CompositeColorOutput::YRES - CDG_HEIGHT) / 2;

    for (int y = 0; y < CDG_HEIGHT; y++) {
        for (int x = 0; x < CDG_WIDTH; x++) {
            uint8_t colorIndex = st.pixels[x + y * CDG_WIDTH];
            if (colorIndex != st.transparentColor) {
                uint8_t color = st.colorTable[colorIndex & 0x0F];
                uint8_t hue = (color >> 4) & 0x0F;
                uint8_t brightness = color & 0x0F;
                graphics.setHue(hue);
                graphics.dot(offsetX + x, offsetY + y, brightness * 4);
            }
        }
    }
}

void updateSplash() {
    if (millis() - splashStartTime >= SPLASH_DURATION) {
        appState = STATE_BROWSER;
        browserSelection = 0;
        browserScrollTop = 0;
    }
}

// ============================================================
// File browser
// ============================================================
void renderBrowser() {
    int total = player.getTotalFiles();
    const int TOP_MARGIN = 20;

    // Header bar
    graphics.setHue(6);
    graphics.fillRect(0, TOP_MARGIN, CompositeColorOutput::XRES, 14, 30);
    graphics.setHue(0);
    graphics.setTextColor(55);
    graphics.setCursor(4, TOP_MARGIN + 3);
    graphics.print("ESPoke! - Select a song");

    if (total == 0) {
        graphics.setHue(0);
        graphics.setTextColor(40);
        graphics.setCursor(55, 110);
        graphics.print("No CDG files found on SD");
    } else {

    // Ensure selection is in visible range
    if (browserSelection < browserScrollTop) {
        browserScrollTop = browserSelection;
    }
    if (browserSelection >= browserScrollTop + BROWSER_VISIBLE_LINES) {
        browserScrollTop = browserSelection - BROWSER_VISIBLE_LINES + 1;
    }

    // File list
    int yStart = TOP_MARGIN + 18;
    for (int i = 0; i < BROWSER_VISIBLE_LINES && (browserScrollTop + i) < total; i++) {
        int fileIdx = browserScrollTop + i;
        int y = yStart + i * 10;

        if (fileIdx == browserSelection) {
            // Highlight bar
            graphics.setHue(2);
            graphics.fillRect(2, y - 1, CompositeColorOutput::XRES - 4, 10, 25);
            graphics.setHue(0);
            graphics.setTextColor(55);
        } else {
            graphics.setHue(0);
            graphics.setTextColor(35);
        }

        graphics.setCursor(8, y);

        // Show file name without extension
        const char* name = player.getFileName(fileIdx);
        if (name) {
            // Skip path prefix, show only filename without extension
            const char* slash = strrchr(name, '/');
            const char* display = slash ? slash + 1 : name;
            int len = strlen(display);
            int dotPos = -1;
            for (int j = len - 1; j >= 0; j--) {
                if (display[j] == '.') { dotPos = j; break; }
            }
            int printLen = (dotPos > 0) ? dotPos : len;
            if (printLen > 50) printLen = 50;
            for (int j = 0; j < printLen; j++) {
                char c = display[j];
                // Replace underscores with spaces for readability
                if (c == '_') c = ' ';
                graphics.print(c);
            }
        }
    }

    // Scroll indicators
    if (browserScrollTop > 0) {
        graphics.setHue(0);
        graphics.setTextColor(40);
        graphics.setCursor(CompositeColorOutput::XRES - 12, yStart);
        graphics.print("^");
    }
    if (browserScrollTop + BROWSER_VISIBLE_LINES < total) {
        graphics.setHue(0);
        graphics.setTextColor(40);
        graphics.setCursor(CompositeColorOutput::XRES - 12, yStart + (BROWSER_VISIBLE_LINES - 1) * 10);
        graphics.print("v");
    }
    }

    // Footer
    graphics.setHue(0);
    graphics.setTextColor(25);
    graphics.setCursor(4, 240 - TOP_MARGIN);
    graphics.print("PLAY:ok  NEXT/PREV:browse");

    // File count
    graphics.setCursor(250, 240 - TOP_MARGIN);
    graphics.print(browserSelection + 1);
    graphics.print("/");
    graphics.print(total);
}

void updateBrowser() {
    int total = player.getTotalFiles();
    if (total == 0) return;

    if (debounceButton(BTN_NEXT)) {
        browserSelection = (browserSelection + 1) % total;
        lastActivityTime = millis();
    }

    if (debounceButton(BTN_PREV)) {
        browserSelection = (browserSelection - 1 + total) % total;
        lastActivityTime = millis();
    }

    if (debounceButton(BTN_PLAY)) {
        player.selectSong(browserSelection);
        player.play();
        appState = STATE_PLAYING;
        lastActivityTime = millis();
    }
}

// ============================================================
// CDG playback
// ============================================================
void renderCDG() {
    player.getCDGParser().lock();
    CDGParser::CDGState& state = player.getCDGParser().getState();

    int offsetX = (CompositeColorOutput::XRES - CDG_WIDTH) / 2;
    int offsetY = (CompositeColorOutput::YRES - CDG_HEIGHT) / 2;

    for (int y = 0; y < CDG_HEIGHT; y++) {
        for (int x = 0; x < CDG_WIDTH; x++) {
            int srcX = (x + state.scrollOffsetX) % CDG_WIDTH;
            if (srcX < 0) srcX += CDG_WIDTH;
            int srcY = (y + state.scrollOffsetY) % CDG_HEIGHT;
            if (srcY < 0) srcY += CDG_HEIGHT;
            uint8_t colorIndex = state.pixels[srcX + srcY * CDG_WIDTH];

            if (colorIndex != state.transparentColor) {
                uint8_t color = state.colorTable[colorIndex & 0x0F];
                uint8_t hue = (color >> 4) & 0x0F;
                uint8_t brightness = color & 0x0F;
                graphics.setHue(hue);
                graphics.dot(offsetX + x, offsetY + y, brightness * 4);
            }
        }
    }
    player.getCDGParser().unlock();
}

void renderPlayingUI() {
    graphics.setHue(0);
    graphics.setTextColor(30);
    graphics.setCursor(4, 3);
    graphics.print(player.getCurrentFileName());

    graphics.setCursor(280, 3);
    graphics.print("Vol:");
    graphics.print(player.getVolume() / 25);
}

void updatePlaying() {
    if (debounceButton(BTN_PLAY)) {
        player.pause();
        appState = STATE_BROWSER;
        lastActivityTime = millis();
        return;
    }

    if (debounceButton(BTN_NEXT)) {
        int total = player.getTotalFiles();
        browserSelection = (browserSelection + 1) % total;
        player.selectSong(browserSelection);
        player.play();
        lastActivityTime = millis();
    }

    if (debounceButton(BTN_PREV)) {
        int total = player.getTotalFiles();
        browserSelection = (browserSelection - 1 + total) % total;
        player.selectSong(browserSelection);
        player.play();
        lastActivityTime = millis();
    }

    if (debounceButton(BTN_VOL_UP)) {
        uint8_t vol = player.getVolume();
        if (vol <= 239) player.setVolume(vol + 16);
        else player.setVolume(255);
        lastActivityTime = millis();
    }

    if (debounceButton(BTN_VOL_DN)) {
        uint8_t vol = player.getVolume();
        if (vol >= 16) player.setVolume(vol - 16);
        else player.setVolume(0);
        lastActivityTime = millis();
    }
}

// ============================================================
// Screensaver (static noise)
// ============================================================
void renderScreensaver() {
    // Draw sparse random dots instead of full-screen pixel-by-pixel
    // to avoid freezing the system (~76K dot() calls per frame is too slow)
    int xres = CompositeColorOutput::XRES;
    int yres = CompositeColorOutput::YRES;

    for (int i = 0; i < 200; i++) {
        int x = random(xres);
        int y = random(yres);
        int hue = random(16);
        uint8_t brightness = random(60);
        graphics.setHue(hue);
        graphics.dot(x, y, brightness);
    }

    // "ESPoke" text floating
    int textX = (frameCount * 2) % (xres - 60);
    int textY = 100 + sin(frameCount * 0.05) * 40;
    graphics.setHue((frameCount / 8) % 16);
    graphics.setTextColor(40);
    graphics.setCursor(textX, textY);
    graphics.print("ESPoke!");
}

bool checkScreensaverWake() {
    return debounceButton(BTN_PLAY) || debounceButton(BTN_NEXT) ||
           debounceButton(BTN_PREV) || debounceButton(BTN_VOL_UP) ||
           debounceButton(BTN_VOL_DN);
}

// ============================================================
// Utilities
// ============================================================
// Non-blocking debounce: returns true once per press (on release)
static uint32_t btnLastPress[40] = {0};  // indexed by pin number
static bool btnWasPressed[40] = {false};
const uint32_t DEBOUNCE_MS = 50;

bool debounceButton(int pin) {
    if (pin >= 40) return false;
    bool pressed = (digitalRead(pin) == LOW);
    uint32_t now = millis();

    if (pressed && !btnWasPressed[pin] && (now - btnLastPress[pin] > DEBOUNCE_MS)) {
        btnWasPressed[pin] = true;
        btnLastPress[pin] = now;
        return true;
    }
    if (!pressed) {
        btnWasPressed[pin] = false;
    }
    return false;
}

// ============================================================
// FreeRTOS task for CDG/Audio processing (Core 0)
// ============================================================
void cdgAudioTask(void* param) {
    Serial.println("CDG/Audio task started on Core 0");

    while (true) {
        if (appState == STATE_PLAYING) {
            if (player.isPlaying()) {
                player.update();
                player.getCDGParser().syncToTime(player.getPlayElapsedMs());
            } else if (player.isSongFinished()) {
                int total = player.getTotalFiles();
                if (browserSelection + 1 < total) {
                    browserSelection++;
                    player.selectSong(browserSelection);
                    player.play();
                } else {
                    browserSelection = 0;
                    appState = STATE_BROWSER;
                }
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ============================================================
// Setup
// ============================================================
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

    // Init splash from embedded CDG (reuse player's parser)
    player.getCDGParser().initFromBuffer(splash_cdg, SPLASH_CDG_SIZE);
    player.getCDGParser().getNextCommands(SPLASH_CDG_PACKETS);

    // Start with splash
    appState = STATE_SPLASH;
    splashStartTime = millis();
    lastActivityTime = millis();

    Serial.println("Initializing player...");

    static SPIClass sdSPI(VSPI);  // static: must outlive setup() for SD reads
    sdSPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

    if (player.init(sdSPI, SD_CS)) {
        player.scanDirectory("/");
        Serial.print("Files found: ");
        Serial.println(player.getTotalFiles());
    }

    // Create CDG/Audio task on Core 0
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
}

// ============================================================
// Main loop (Core 1) — render + input
// ============================================================
void loop() {
    unsigned long now = millis();
    if (now - lastFrameTime < (1000 / FRAME_RATE)) return;
    lastFrameTime = now;
    frameCount++;

    if (appState == STATE_BROWSER || appState == STATE_PLAYING) {
        if (now - lastActivityTime >= SCREENSAVER_TIMEOUT) {
            appState = STATE_SCREENSAVER;
        }
    }

    switch (appState) {
        case STATE_SPLASH:  updateSplash();  break;
        case STATE_BROWSER: updateBrowser(); break;
        case STATE_PLAYING: updatePlaying(); break;
        case STATE_SCREENSAVER: {
            if (checkScreensaverWake()) appState = STATE_BROWSER;
            break;
        }
    }

    graphics.setHue(0);
    graphics.begin(0);

    switch (appState) {
        case STATE_SPLASH:  renderSplash();  break;
        case STATE_BROWSER: renderBrowser(); break;
        case STATE_PLAYING:
            renderCDG();
            renderPlayingUI();
            break;
        case STATE_SCREENSAVER: renderScreensaver(); break;
    }

    graphics.end();
    composite.sendFrameHalfResolution(&graphics.frame);
}
