#include "Config.h"
#include "SampleLibrary.h"


extern FIL SDFile;

// External declaration for custom memory allocator defined in SimpleSampler.cpp
extern void* custom_pool_allocate(size_t size);



SampleLibrary::SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem, DisplayManager& display)
    : sampleCount_(0),
      activeGrainCount_(0),
      granularModeEnabled_(false),
      granularSampleIndex_(0),
      timeSinceLastGrain_(0.0f),
      spawnRate_(30.0f),  // 30 grains per second default
      sdHandler_(sdHandler),
      fileSystem_(fileSystem),
      display_(display)
{
    // Initialize all samples as not loaded
    for (int i = 0; i < Constants::SampleLibrary::MAX_SAMPLES; i++) {
        samples_[i].loaded = false;
        samples_[i].audioDataLoaded = false;
        sampleSpeeds_[i] = 1.0f;
    }
    
    // Initialize all grains as finished (inactive)
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        grains_[i].ticker.finished_ = true;
        grains_[i].sampleIndex = -1;
        grains_[i].envelopePhase = 0.0f;
    }
}

bool SampleLibrary::init() {
    // Show initialization message
    // display_.showMessage("Initializing Library...", 200);
    
    // Scan directory and load all WAV files
    return scanAndLoadFiles();
}


bool SampleLibrary::scanAndLoadFiles()
{
    // display_.showMessage("Opening dir...", 200);
    
    // Open the root directory
    DIR dir;
    if (f_opendir(&dir, "/") != FR_OK) {
        display_.showMessage("Dir open failed!", 200);
        return false;
    }
    
    // display_.showMessage("Scanning files...", 200);
    
    // Scan for WAV files
    FILINFO fno;
    int fileCount = 0;
    
    // Read entries in a loop
    while (f_readdir(&dir, &fno) == FR_OK) {
        if (fno.fname[0] == 0) break;  // No more files
        
        // Check if filename contains .wav or .WAV
        if (strstr(fno.fname, ".wav") != nullptr || strstr(fno.fname, ".WAV") != nullptr) {
            // display_.showMessagef("Found WAV: %s", 200, fno.fname);
            if (fileCount < Constants::SampleLibrary::MAX_SAMPLES) {
                if (loadWavFile(fno.fname, fileCount)) {
                    fileCount++;
                }
            }
        }
    }
    
    // Close directory
    f_closedir(&dir);
    
    // display_.showMessage("Files scanned", 300);
    char msg[64];
    snprintf(msg, sizeof(msg), "WAV Files: %d", fileCount);
    display_.showMessage(msg, 200);
    
    // Store the number of loaded samples
    sampleCount_ = fileCount;
    
    return true;
}


bool SampleLibrary::loadWavFile(const char* filename, int index)
{
    // Open the file using the global SDFile
    if (f_open(&SDFile, filename, (FA_OPEN_EXISTING | FA_READ)) != FR_OK) {
        display_.showMessagef("Open failed!", 200);
        return false;
    }
    
    int size = f_size(&SDFile);
    display_.showMessagef("Size: %d bytes", 200, size);
    
    // Allocate memory from custom pool
    char* memoryBuffer = (char*) custom_pool_allocate(size);
    
    if (!memoryBuffer) {
        display_.showMessagef("Alloc failed!", 200);
        f_close(&SDFile);
        return false;
    }
    
    display_.showMessagef("Alloc OK, reading...", 200);
    
    UINT bytesRead;
    if (f_read(&SDFile, memoryBuffer, size, &bytesRead) != FR_OK || bytesRead != size) {
        display_.showMessagef("Read failed!", 200);
        f_close(&SDFile);
        return false;
    }
    
    display_.showMessagef("Read OK, parsing...", 200);
    
    samples_[index].dataSource = MemoryDataSource(memoryBuffer, size);
    samples_[index].reader.getWavInfo(samples_[index].dataSource);
    
    // Copy filename to SampleInfo
    strncpy(samples_[index].name, filename, sizeof(samples_[index].name) - 1);
    samples_[index].name[sizeof(samples_[index].name) - 1] = '\0';
    
    // Copy WAV metadata from reader to SampleInfo
    samples_[index].numFrames = samples_[index].reader.getNumFrames();
    samples_[index].channels = samples_[index].reader.getChannels();
    samples_[index].sampleRate = (int)samples_[index].reader.getFileDataRate();
    samples_[index].bitsPerSample = samples_[index].reader.getBitsPerSample();
    
    // Mark sample as loaded
    samples_[index].loaded = true;
    samples_[index].audioDataLoaded = true;
    
    display_.showMessagef("Parsed OK, creating ticker...", 200);
    wavTickers_[index] = samples_[index].reader.createWavTicker(Config::samplerate);
    display_.showMessagef("Ticker created!", 200);
    wavTickers_[index].finished_ = true;
    
    display_.showMessagef("Loaded: %s", 200, filename);
    
    f_close(&SDFile);
    return true;
}

// Get a sample by index
SampleInfo* SampleLibrary::getSample(int index) {
    // Check bounds
    if (index < 0 || index >= sampleCount_) {
        return nullptr;
    }
    return &samples_[index];
}

// Find sample by name (returns index, or -1 if not found)
int SampleLibrary::findSample(const char* name) {
    for (int i = 0; i < sampleCount_; i++) {
        if (strcmp(samples_[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Load audio data for a sample (lazy loading)
// Call this before playing a sample if audioDataLoaded is false
bool SampleLibrary::ensureSampleLoaded(int index) {
    // Check bounds
    if (index < 0 || index >= sampleCount_) {
        return false;
    }
    
    // If already loaded, return true
    if (samples_[index].audioDataLoaded) {
        return true;
    }
    
    // Note: In the current implementation, audio data is loaded during init()
    // This method is provided for future lazy loading support
    // For now, return the current state
    return samples_[index].audioDataLoaded;
}

void SampleLibrary::processAudio(float** out, size_t size) {
    // Clear output buffers to zero
    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
    
    // Process regular sample playback (existing functionality)
    for (int i = 0; i < sampleCount_; i++) {
        if (!wavTickers_[i].finished_) {
            samples_[i].reader.tick(
                &wavTickers_[i],
                samples_[i].dataSource,
                sampleSpeeds_[i],
                1.0,
                size,
                out[0],
                out[1]
            );
        }
    }
    
    // First pass: count active grains
    activeGrainCount_ = 0;
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        if (!grains_[i].ticker.finished_) {
            activeGrainCount_++;
        }
    }
    
    // Calculate uniform volume for all grains
    float grainVolume = (activeGrainCount_ > 0) ? (1.0f / activeGrainCount_) : 0.0f;
    
    // Second pass: process all active grains
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        if (!grains_[i].ticker.finished_) {
            int sampleIndex = grains_[i].sampleIndex;
            double speed = grains_[i].ticker.speed_;
            
            samples_[sampleIndex].reader.tick(
                &grains_[i].ticker,
                samples_[sampleIndex].dataSource,
                speed,
                grainVolume,
                size,
                out[0],
                out[1]
            );
        }
    }
}

// Trigger a sample to start playing
bool SampleLibrary::triggerSample(int index) {
    // Validate index bounds
    if (index < 0 || index >= sampleCount_) {
        return false;
    }
    
    // Reset the ticker to the start position
    wavTickers_[index].time_ = wavTickers_[index].starttime_;
    wavTickers_[index].finished_ = false;
    
    return true;
}

// Stop a currently playing sample
bool SampleLibrary::stopSample(int index) {
    // Validate index bounds
    if (index < 0 || index >= sampleCount_) {
        return false;
    }
    
    // Mark the sample as finished (stopped)
    wavTickers_[index].finished_ = true;
    
    return true;
}

// Set the playback speed for a sample
void SampleLibrary::setSampleSpeed(int index, float speed) {
    // Validate index bounds
    if (index >= 0 && index < sampleCount_) {
        sampleSpeeds_[index] = speed;
    }
}

// Spawn a new grain from a sample
bool SampleLibrary::spawnGrain(int sampleIndex, float startPosition, float duration, float speed) {
    // Use granularSampleIndex_ if sampleIndex is -1
    int actualSampleIndex = sampleIndex;
    if (sampleIndex < 0) {
        actualSampleIndex = granularSampleIndex_;
    }
    
    // Validate sample index
    if (actualSampleIndex < 0 || actualSampleIndex >= sampleCount_) {
        return false;
    }
    
    // Validate sample is loaded
    if (!samples_[actualSampleIndex].audioDataLoaded) {
        return false;
    }
    
    // Find an available grain slot
    int availableSlot = -1;
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        if (grains_[i].ticker.finished_) {
            availableSlot = i;
            break;
        }
    }
    
    // No available slot
    if (availableSlot < 0) {
        return false;
    }
    
    // Get sample info
    SampleInfo& sample = samples_[actualSampleIndex];
    double totalFrames = static_cast<double>(sample.numFrames);
    double sampleRate = static_cast<double>(sample.sampleRate);
    
    // Create a new ticker from the sample
    grains_[availableSlot].ticker = sample.reader.createWavTicker(Config::samplerate);
    
    // Calculate start position (convert 0.0-1.0 to frame number)
    double startFrame = startPosition * totalFrames;
    
    // Clamp to valid range
    if (startFrame < 0.0) startFrame = 0.0;
    if (startFrame >= totalFrames) startFrame = totalFrames - 1.0;
    
    // Set grain position
    grains_[availableSlot].ticker.time_ = startFrame;
    grains_[availableSlot].ticker.starttime_ = startFrame;
    
    // Calculate end position
    double durationFrames = sampleRate * duration;
    double endFrame = startFrame + durationFrames;
    
    // Clamp to end of file
    if (endFrame > totalFrames) {
        endFrame = totalFrames;
    }
    
    grains_[availableSlot].ticker.endtime_ = endFrame;
    
    // Set grain properties
    grains_[availableSlot].ticker.speed_ = speed;
    grains_[availableSlot].sampleIndex = actualSampleIndex;
    grains_[availableSlot].envelopePhase = 0.0f;
    
    // Mark grain as active
    grains_[availableSlot].ticker.finished_ = false;
    
    return true;
}

void SampleLibrary::setGranularMode(bool enabled) {
    granularModeEnabled_ = enabled;
    
    // Clear all grains when disabling
    if (!enabled) {
        for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
            grains_[i].ticker.finished_ = true;
        }
        activeGrainCount_ = 0;
    }
}

bool SampleLibrary::setGranularSampleIndex(int index) {
    if (index < 0 || index >= sampleCount_) {
        return false;
    }
    
    if (!samples_[index].audioDataLoaded) {
        return false;
    }
    
    granularSampleIndex_ = index;
    return true;
}

