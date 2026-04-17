#include "AudioOutput.h"

AudioOutput::AudioOutput() : volume(128), playing(false), audioRead(0), audioWrite(0) {
}

void AudioOutput::init() {
    ledcSetup(0, 20000000, 7);
    ledcAttachPin(AUDIO_PIN, 0);
    ledcWrite(0, 0);
    Serial.println("Audio output initialized on pin 18");
}

void AudioOutput::setVolume(uint8_t vol) {
    volume = vol;
}

void AudioOutput::writeSample(uint8_t sample) {
    uint32_t writePos = (audioWrite + 1) & (sizeof(audioBuffer) - 1);
    if (writePos != audioRead) {
        audioBuffer[audioWrite] = sample;
        audioWrite = writePos;
    }
}

uint8_t AudioOutput::readSample() {
    if (audioRead != audioWrite) {
        uint8_t sample = audioBuffer[audioRead];
        audioRead = (audioRead + 1) & (sizeof(audioBuffer) - 1);
        return sample;
    }
    return 0x20;
}

void AudioOutput::play() {
    playing = true;
}

void AudioOutput::stop() {
    playing = false;
    ledcWrite(0, 0);
}
