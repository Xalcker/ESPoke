#include "AudioPlayer.h"

// Static ring buffer for ISR
volatile uint8_t AudioPlayer::pcmRing[PCM_RING_BUF_SIZE];
volatile int AudioPlayer::pcmWritePos = 0;
volatile int AudioPlayer::pcmReadPos = 0;
volatile int AudioPlayer::pcmCount = 0;
volatile uint8_t AudioPlayer::currentVolume = 128;

static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Global pointer so the static callback can reach the instance
static AudioPlayer* g_player = nullptr;

// ============================================================
// Timer ISR — runs at sample rate, writes one sample to LEDC
// ============================================================
void IRAM_ATTR AudioPlayer::onTimer() {
    if (pcmCount > 0) {
        uint8_t sample = pcmRing[pcmReadPos];
        uint16_t scaled = ((uint16_t)sample * currentVolume) >> 8;
        ledcWrite(AUDIO_LEDC_CHANNEL, (uint8_t)scaled);

        pcmReadPos = (pcmReadPos + 1) % PCM_RING_BUF_SIZE;
        portENTER_CRITICAL_ISR(&timerMux);
        pcmCount--;
        portEXIT_CRITICAL_ISR(&timerMux);
    } else {
        ledcWrite(AUDIO_LEDC_CHANNEL, 0);
    }
}

// ============================================================
// PCM callback from libhelix decoder (static)
// ============================================================
void AudioPlayer::pcmCallback(MP3FrameInfo &info, short *pcm, size_t len, void* ref) {
    if (!g_player) return;

    // Start the playback timer on first decoded frame
    if (!g_player->timerStarted && info.samprate > 0) {
        g_player->sampleRate = info.samprate;
        g_player->channels = info.nChans;
        g_player->startTimer(info.samprate);
        g_player->timerStarted = true;
        Serial.printf("MP3: %dHz, %dch\n", info.samprate, info.nChans);
    }

    int nChans = info.nChans;
    if (nChans < 1) nChans = 1;

    // Convert 16-bit signed stereo PCM → 8-bit unsigned mono
    // and push into ring buffer
    for (size_t i = 0; i < len; i += nChans) {
        int32_t mixed = 0;
        for (int ch = 0; ch < nChans; ch++) {
            mixed += pcm[i + ch];
        }
        mixed /= nChans;

        // Signed 16-bit → unsigned 8-bit
        uint8_t sample = (uint8_t)((mixed + 32768) >> 8);

        // Spin-wait if buffer is full (keeps decoder in sync with playback)
        int timeout = 0;
        while (pcmCount >= PCM_RING_BUF_SIZE - 1) {
            delayMicroseconds(10);
            if (++timeout > 5000) break; // ~50ms safety
        }

        pcmRing[pcmWritePos] = sample;
        pcmWritePos = (pcmWritePos + 1) % PCM_RING_BUF_SIZE;
        portENTER_CRITICAL_ISR(&timerMux);
        pcmCount++;
        portEXIT_CRITICAL_ISR(&timerMux);
    }
}

// ============================================================
// Constructor / Init
// ============================================================
AudioPlayer::AudioPlayer()
    : volume(128), playing(false), fileLoaded(false),
      decoder(nullptr), sampleRate(44100), channels(2),
      timerStarted(false), audioTimer(nullptr) {
}

bool AudioPlayer::init() {
    ledcSetup(AUDIO_LEDC_CHANNEL, AUDIO_LEDC_FREQ, AUDIO_LEDC_BITS);
    ledcAttachPin(AUDIO_PIN, AUDIO_LEDC_CHANNEL);
    ledcWrite(AUDIO_LEDC_CHANNEL, 0);

    g_player = this;

    Serial.println("Audio: LEDC PWM on GPIO 18");
    return true;
}

// ============================================================
// File management
// ============================================================
bool AudioPlayer::loadFile(const char* filename) {
    stop();

    audioFile = SD.open(filename);
    if (!audioFile) {
        Serial.printf("Audio file not found: %s\n", filename);
        fileLoaded = false;
        return false;
    }

    fileLoaded = true;
    Serial.printf("Audio loaded: %s (%d bytes)\n", filename, audioFile.size());
    return true;
}

// ============================================================
// Playback control
// ============================================================
void AudioPlayer::play() {
    if (!fileLoaded) return;

    // Reset ring buffer
    pcmWritePos = 0;
    pcmReadPos = 0;
    pcmCount = 0;
    timerStarted = false;
    currentVolume = volume;

    // Create fresh decoder
    if (decoder) {
        delete decoder;
    }
    decoder = new libhelix::MP3DecoderHelix(pcmCallback);
    decoder->begin();

    playing = true;
    Serial.println("MP3 playback started");
}

void AudioPlayer::pause() {
    playing = false;
    stopTimer();
    ledcWrite(AUDIO_LEDC_CHANNEL, 0);
}

void AudioPlayer::stop() {
    playing = false;
    stopTimer();
    ledcWrite(AUDIO_LEDC_CHANNEL, 0);

    if (decoder) {
        delete decoder;
        decoder = nullptr;
    }

    if (audioFile) {
        audioFile.close();
    }
    fileLoaded = false;
    timerStarted = false;

    pcmWritePos = 0;
    pcmReadPos = 0;
    pcmCount = 0;
}

void AudioPlayer::setVolume(uint8_t vol) {
    volume = vol;
    currentVolume = vol;
}

// ============================================================
// update() — called from the CDG/Audio task on Core 0.
// Reads MP3 data from SD and feeds it to the decoder.
// The decoder's callback pushes decoded PCM into the ring buffer.
// ============================================================
void AudioPlayer::update() {
    if (!playing || !fileLoaded || !decoder) return;

    // Don't decode if ring buffer is mostly full
    if (pcmCount > PCM_RING_BUF_SIZE * 3 / 4) return;

    int bytesRead = audioFile.read(mp3ReadBuf, MP3_READ_BUF_SIZE);
    if (bytesRead > 0) {
        decoder->write(mp3ReadBuf, bytesRead);
    } else {
        Serial.println("MP3 playback finished");
        stop();
    }
}

// ============================================================
// Hardware timer for sample-rate accurate output
// ============================================================
void AudioPlayer::startTimer(int rate) {
    stopTimer();

    // Timer 1, prescaler 80 → 1µs per tick
    audioTimer = timerBegin(1, 80, true);
    timerAttachInterrupt(audioTimer, &AudioPlayer::onTimer, true);
    timerAlarmWrite(audioTimer, 1000000 / rate, true);
    timerAlarmEnable(audioTimer);

    Serial.printf("Audio timer: %dHz\n", rate);
}

void AudioPlayer::stopTimer() {
    if (audioTimer) {
        timerAlarmDisable(audioTimer);
        timerDetachInterrupt(audioTimer);
        timerEnd(audioTimer);
        audioTimer = nullptr;
    }
}
