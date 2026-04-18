#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <driver/ledc.h>
#include "MP3DecoderHelix.h"

// Audio output pin
#define AUDIO_PIN 18

// LEDC config
#define AUDIO_LEDC_CHANNEL 0
#define AUDIO_LEDC_FREQ    40000   // PWM carrier frequency (well above audible)
#define AUDIO_LEDC_BITS    8       // 8-bit resolution

// MP3 read buffer
#define MP3_READ_BUF_SIZE  1600

// PCM ring buffer — holds decoded samples for the timer ISR
#define PCM_RING_BUF_SIZE  4096

class AudioPlayer {
public:
    AudioPlayer();
    bool init();
    bool loadFile(const char* filename);
    void play();
    void pause();
    void stop();
    bool isPlaying() { return playing; }
    void setVolume(uint8_t vol);
    uint8_t getVolume() { return volume; }
    void update();  // call from task loop to feed the decoder

    // Called by timer ISR — writes next sample to LEDC
    static void IRAM_ATTR onTimer();

private:
    uint8_t volume;
    volatile bool playing;
    bool fileLoaded;
    File audioFile;

    // MP3 decoder
    libhelix::MP3DecoderHelix* decoder;
    uint8_t mp3ReadBuf[MP3_READ_BUF_SIZE];

    // PCM ring buffer (mono 8-bit unsigned, ready for LEDC)
    static volatile uint8_t pcmRing[PCM_RING_BUF_SIZE];
    static volatile int pcmWritePos;
    static volatile int pcmReadPos;
    static volatile int pcmCount;     // samples available
    static volatile uint8_t currentVolume;

    // Sample rate from decoded MP3
    int sampleRate;
    int channels;
    bool timerStarted;

    hw_timer_t* audioTimer;

    void startTimer(int rate);
    void stopTimer();

    // Callback from libhelix when a frame is decoded
    static void pcmCallback(MP3FrameInfo &info, short *pcm, size_t len, void* ref);
};

#endif
