#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "CDGParser.h"
#include "AudioPlayer.h"

#define MAX_FILES 100

// SD.begin()'s 3rd argument is the SPI clock frequency (Hz), not max_files —
// this used to be passed as literal "2" (2 Hz!), making every SD read take
// ages. The SD card sits on its own dedicated VSPI bus (pins 14/19/23/5),
// separate from the I2S+DMA/APLL path driving composite video (DAC1/GPIO25)
// and the LEDC+hardware-timer path driving audio (GPIO18), so raising this
// doesn't compete with either. Kept at the SD library's own conservative
// default rather than pushing to the ~40MHz max, since this is typically a
// breadboard/jumper-wire setup where signal integrity matters more than a
// few extra MHz.
#define SD_SPI_FREQUENCY_HZ 4000000UL

class Player {
public:
    Player();
    
    bool init(SPIClass &spi, uint8_t csPin);
    bool scanDirectory(const char* path);
    void play();
    void pause();
    void next();
    void previous();
    void selectSong(int index);
    void releaseCurrentSong(); // close the current file handles without changing the selection
    void setVolume(uint8_t vol);
    uint8_t getVolume() { return volume; }
    bool isPlaying() { return playing; }
    bool isSongFinished() { return songFinished; }
    unsigned long getPlayElapsedMs();
    
    void update();
    
    CDGParser& getCDGParser() { return cdgParser; }
    int getCurrentFileIndex() { return currentIndex; }
    int getTotalFiles() { return totalFiles; }
    const char* getCurrentFileName() { return currentFileName; }
    const char* getFileName(int index);
    
private:
    File cdgFile;
    AudioPlayer audioPlayer;
    CDGParser cdgParser;
    
    char fileList[MAX_FILES][64];
    int totalFiles;
    int currentIndex;
    uint8_t volume;
    bool playing;
    bool songFinished;
    unsigned long playStartTime;
    unsigned long pauseStartTime;
    char currentFileName[64];
    
    void loadSong(int index);
    void closeSong();
    String getMP3Filename(const char* cdgFilename);
};

#endif
