#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "CDGParser.h"
#include "AudioPlayer.h"

#define MAX_FILES 100

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
