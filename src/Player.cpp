#include "Player.h"

Player::Player() : totalFiles(0), currentIndex(0), volume(128), playing(false), songFinished(false), playStartTime(0), pauseStartTime(0) {
    currentFileName[0] = '\0';
}

bool Player::init(SPIClass &spi, uint8_t csPin) {
    if (!SD.begin(csPin, spi, SD_SPI_FREQUENCY_HZ)) {
        Serial.println("SD Card initialization failed!");
        return false;
    }
    Serial.println("SD Card initialized.");

    audioPlayer.init();
    return true;
}

bool Player::scanDirectory(const char* path) {
    File root = SD.open(path);
    if (!root) {
        Serial.println("Failed to open directory");
        return false;
    }

    File file = root.openNextFile();
    while (file && totalFiles < MAX_FILES) {
        if (file.isDirectory()) {
            scanDirectory(file.path());
        } else {
            String name = file.name();
            if (name.endsWith(".cdg") || name.endsWith(".CDG")) {
                // Store full path for nested files
                String fullPath = file.path();
                fullPath.toCharArray(fileList[totalFiles], 64);
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
    if (playing) return;

    playing = true;
    songFinished = false;

    if (audioPlayer.isFileLoaded()) {
        // Resuming from a pause: the file/decoder are still in place
        // (see pause() below), so just restart audio output instead of
        // reloading the song from the beginning. Shift playStartTime
        // forward by the time spent paused so CDG sync (which is driven
        // by wall-clock elapsed time) doesn't try to fast-forward through
        // the packets it "missed" while paused.
        playStartTime += millis() - pauseStartTime;
        audioPlayer.resume();
    } else {
        playStartTime = millis();
        loadSong(currentIndex);
    }
}

void Player::pause() {
    if (playing) {
        pauseStartTime = millis();
    }
    playing = false;
    audioPlayer.pause();
    // Don't call closeSong() here — it destroys the decoder and closes files,
    // making resume impossible. Just pause audio playback; play() detects
    // the still-loaded file and resumes instead of reloading.
}

void Player::selectSong(int index) {
    if (index < 0 || index >= totalFiles) return;
    closeSong();
    playing = false;
    currentIndex = index;
}

void Player::releaseCurrentSong() {
    closeSong();
}

void Player::next() {
    closeSong();
    currentIndex = (currentIndex + 1) % totalFiles;
    // Always load the next song so it's ready; if playing it starts immediately
    loadSong(currentIndex);
}

void Player::previous() {
    closeSong();
    currentIndex = (currentIndex - 1 + totalFiles) % totalFiles;
    loadSong(currentIndex);
}

void Player::setVolume(uint8_t vol) {
    volume = vol;
    audioPlayer.setVolume(vol);
}

// update() only handles audio now — CDG processing is done solely
// by cdgAudioTask calling getNextCommands() to avoid double-processing.
void Player::update() {
    if (!playing) return;
    audioPlayer.update();
    if (!audioPlayer.isPlaying()) {
        songFinished = true;
        playing = false;
    }
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
    if (mp3Name.length() == 0) {
        // No matching MP3 for this CDG — there's nothing to play. Don't try
        // to "decode" the .cdg file itself as audio (it would read as
        // garbage with no real-time pacing and finish almost instantly).
        // Mark it finished so the caller's finished-song handling skips to
        // the next track, same as if a song had played through normally.
        Serial.println("No matching MP3 found, skipping song");
        closeSong();
        playing = false;
        songFinished = true;
        return;
    }

    if (audioPlayer.loadFile(mp3Name.c_str())) {
        if (playing) {
            audioPlayer.play();
        }
    } else {
        // MP3 existed a moment ago but failed to open — same treatment.
        closeSong();
        playing = false;
        songFinished = true;
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

    // No companion MP3 — return empty to signal "no audio", instead of
    // falling back to the .cdg file itself (which isn't audio at all).
    return String();
}

const char* Player::getFileName(int index) {
    if (index < 0 || index >= totalFiles) return nullptr;
    return fileList[index];
}

unsigned long Player::getPlayElapsedMs() {
    if (!playing) return 0;
    return millis() - playStartTime;
}
