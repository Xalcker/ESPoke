#include "Player.h"

Player::Player() : totalFiles(0), currentIndex(0), volume(128), playing(false) {
    currentFileName[0] = '\0';
}

bool Player::init() {
    if (!SD.begin()) {
        Serial.println("SD Card initialization failed!");
        return false;
    }
    Serial.println("SD Card initialized.");
    return true;
}

bool Player::scanDirectory(const char* path) {
    File root = SD.open(path);
    if (!root) {
        Serial.println("Failed to open directory");
        return false;
    }
    
    totalFiles = 0;
    
    File file = root.openNextFile();
    while (file && totalFiles < MAX_FILES) {
        if (!file.isDirectory()) {
            String name = file.name();
            if (name.endsWith(".cdg") || name.endsWith(".CDG")) {
                name.toCharArray(fileList[totalFiles], 64);
                totalFiles++;
            }
        }
        file = root.openNextFile();
    }
    
    root.close();
    Serial.print("Found ");
    Serial.print(totalFiles);
    Serial.println(" CDG files");
    
    return totalFiles > 0;
}

void Player::play() {
    if (totalFiles == 0) return;
    
    if (!playing) {
        playing = true;
        loadSong(currentIndex);
    }
}

void Player::pause() {
    playing = false;
}

void Player::next() {
    closeSong();
    currentIndex = (currentIndex + 1) % totalFiles;
    if (playing) {
        loadSong(currentIndex);
    }
}

void Player::previous() {
    closeSong();
    currentIndex = (currentIndex - 1 + totalFiles) % totalFiles;
    if (playing) {
        loadSong(currentIndex);
    }
}

void Player::setVolume(uint8_t vol) {
    volume = vol;
    audioPlayer.setVolume(vol);
}

void Player::update() {
    if (!playing) return;
    
    if (cdgFile) {
        cdgParser.getNextCommand();
    }
    
    audioPlayer.update();
}

void Player::loadSong(int index) {
    if (index < 0 || index >= totalFiles) return;
    
    closeSong();
    
    strcpy(currentFileName, fileList[index]);
    Serial.print("Loading: ");
    Serial.println(currentFileName);
    
    cdgFile = SD.open(currentFileName);
    if (cdgFile) {
        cdgParser.init(cdgFile);
    }
    
    String mp3Name = getMP3Filename(currentFileName);
    if (audioPlayer.loadFile(mp3Name.c_str())) {
        audioPlayer.play();
    }
}

void Player::closeSong() {
    if (cdgFile) {
        cdgFile.close();
    }
    audioPlayer.stop();
}

String Player::getMP3Filename(const char* cdgFilename) {
    String name(cdgFilename);
    int dotPos = name.lastIndexOf('.');
    if (dotPos > 0) {
        name = name.substring(0, dotPos);
    }
    
    if (SD.exists(name + ".mp3")) {
        return name + ".mp3";
    }
    if (SD.exists(name + ".MP3")) {
        return name + ".MP3";
    }
    
    return String(cdgFilename);
}
