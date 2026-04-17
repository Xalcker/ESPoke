#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <Arduino.h>
#include <driver/ledc.h>

#define AUDIO_PIN 18

class AudioOutput {
public:
    AudioOutput();
    void init();
    void setVolume(uint8_t volume);
    void writeSample(uint8_t sample);
    uint8_t readSample();
    
    void play();
    void stop();
    bool isPlaying() { return playing; }
    
private:
    uint8_t volume;
    bool playing;
    
    uint8_t audioBuffer[1024];
    uint32_t audioRead;
    uint32_t audioWrite;
};

#endif
