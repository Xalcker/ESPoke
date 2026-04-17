#include "AudioPlayer.h"

AudioPlayer::AudioPlayer() : volume(128), playing(false), beepEndTime(0), beepFrequency(523) {
}

bool AudioPlayer::init() {
    ledcSetup(0, 20000000, 8);
    ledcAttachPin(18, 0);
    ledcWrite(0, 0);
    Serial.println("Audio initialized on GPIO 18 (LEDC)");
    return true;
}

bool AudioPlayer::loadFile(const char* filename) {
    Serial.print("Audio file selected: ");
    Serial.println(filename);
    return true;
}

void AudioPlayer::play() {
    playing = true;
    beepEndTime = millis() + 300;
    beepFrequency = 523;
    
    ledcSetup(0, beepFrequency * 2, 8);
    ledcAttachPin(18, 0);
    ledcWrite(0, 128);
    
    Serial.println("Playing beep");
}

void AudioPlayer::pause() {
    playing = false;
    ledcWrite(0, 0);
}

void AudioPlayer::stop() {
    playing = false;
    ledcWrite(0, 0);
}

void AudioPlayer::setVolume(uint8_t vol) {
    volume = vol;
}

void AudioPlayer::update() {
    if (beepEndTime > 0 && millis() >= beepEndTime) {
        ledcWrite(0, 0);
        beepEndTime = 0;
    }
}