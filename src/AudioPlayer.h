#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <driver/ledc.h>

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
    void update();

private:
    uint8_t volume;
    bool playing;
    unsigned long beepEndTime;
    int beepFrequency;
};

#endif