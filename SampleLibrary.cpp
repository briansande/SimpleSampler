#include "Config.h"
#include "SampleLibrary.h"


// DEBUG: Variables for granular mode debugging
static int debugGrainSpawnCount = 0;
static int debugGrainSpawnFailures = 0;

extern FIL SDFile;

// External declaration for custom memory allocator defined in SimpleSampler.cpp
extern void* custom_pool_allocate(size_t size);



SampleLibrary::SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem, DisplayManager& display)
    : sampleCount_(0),
      activeGrainCount_(0),
      granularModeEnabled_(false),
      granularSampleIndex_(0),
      timeSinceLastGrain_(0.0f),
      spawnRate_(30.0f),  // 30 grains per second default (legacy)
      granularSpawnRate_(30.0f),  // 30 grains per second default
      granularDuration_(0.1f),     // 0.1 seconds default
      granularSpeed_(1.0f),        // Normal speed default
      granularPosition_(0.5f),      // Middle of sample default
      gateOpen_(false),              // Gate starts closed
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

    // Scan directory and load all WAV files
    return scanAndLoadFiles();
}


bool SampleLibrary::scanAndLoadFiles()
{

    // Open the root directory
    DIR dir;
    if (f_opendir(&dir, "/") != FR_OK) {
        display_.showMessage("Dir open failed!", 200);
        return false;
    }

    
    // Scan for WAV files
    FILINFO fno;
    int fileCount = 0;
    
    // Read entries in a loop
    while (f_readdir(&dir, &fno) == FR_OK) {
        if (fno.fname[0] == 0) break;  // No more files
        
        // Check if filename contains .wav or .WAV
        if (strstr(fno.fname, ".wav") != nullptr || strstr(fno.fname, ".WAV") != nullptr) {
            if (fileCount < Constants::SampleLibrary::MAX_SAMPLES) {
                if (loadWavFile(fno.fname, fileCount)) {
                    fileCount++;
                }
            }
        }
    }
    
    // Close directory
    f_closedir(&dir);
    
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
    
    // Allocate memory from custom pool
    char* memoryBuffer = (char*) custom_pool_allocate(size);
    
    if (!memoryBuffer) {
        display_.showMessagef("Alloc failed!", 200);
        f_close(&SDFile);
        return false;
    }
    
    
    UINT bytesRead;
    if (f_read(&SDFile, memoryBuffer, size, &bytesRead) != FR_OK || bytesRead != size) {
        display_.showMessagef("Read failed!", 200);
        f_close(&SDFile);
        return false;
    }
    
    
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
    
    wavTickers_[index] = samples_[index].reader.createWavTicker(Config::samplerate);
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
    
    // Auto-spawning: Only spawn if granular mode is enabled AND gate is open
    if (granularModeEnabled_ && gateOpen_) {
        // Calculate the duration of this audio block in seconds
        float blockDuration = (float)size / Config::samplerate;
        
        // Update the time elapsed since last grain spawn
        timeSinceLastGrain_ += blockDuration;
        
        // Calculate spawn interval from spawn rate (grains per second)
        float spawnInterval = 1.0f / granularSpawnRate_;
        
        // Check if it's time to spawn a new grain
        while (timeSinceLastGrain_ >= spawnInterval) {
            // Spawn grain using configurable parameters
            // sampleIndex=-1 (use granularSampleIndex_), startPosition=granularPosition_, duration=granularDuration_, speed=granularSpeed_
            spawnGrain(-1, granularPosition_, granularDuration_, granularSpeed_);
            
            // Reset timer (subtract interval to handle multiple spawns per block)
            timeSinceLastGrain_ -= spawnInterval;
            
            // Clamp to 0 to prevent negative values
            if (timeSinceLastGrain_ < 0.0f) {
                timeSinceLastGrain_ = 0.0f;
            }
        }
    } else if (!gateOpen_) {
        // Reset timer when gate is closed to prevent burst of grains on open
        timeSinceLastGrain_ = 0.0f;
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
        debugGrainSpawnFailures++;
        return false;
    }
    
    // Validate sample is loaded
    if (!samples_[actualSampleIndex].audioDataLoaded) {
        debugGrainSpawnFailures++;
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
        debugGrainSpawnFailures++;
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
    
    // DEBUG: Track successful spawns
    debugGrainSpawnCount++;
    
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
        debugGrainSpawnCount = 0;
        debugGrainSpawnFailures = 0;
    }
}

bool SampleLibrary::setGranularSampleIndex(int index) {

    if (index < 0 || index >= sampleCount_) {
        display_.showMessagef("Invalid index!*%d", 300, index);
        return false;
    }
    
    if (!samples_[index].audioDataLoaded) {
        display_.showMessagef("Sample not*loaded!*%d", 300, index);
        return false;
    }
    
    granularSampleIndex_ = index;
    display_.showMessagef("granularSampleIndex_*%d", 300, index);
    return true;
}

// ========== Debug Getter Methods ==========

int SampleLibrary::getDebugGrainSpawnCount() const {
    return debugGrainSpawnCount;
}

int SampleLibrary::getDebugGrainSpawnFailures() const {
    return debugGrainSpawnFailures;
}

// ========== Gate Control Methods ==========

void SampleLibrary::setGateOpen(bool open) {
    gateOpen_ = open;
    
    // Reset timer when closing gate to prevent burst on next open
    if (!open) {
        timeSinceLastGrain_ = 0.0f;
    }
}

// ========== Granular Parameter Control Methods ==========

void SampleLibrary::setGranularSpawnRate(float rate) {
    // Clamp to valid range: 1.0 - 100.0 grains per second
    if (rate < 1.0f) rate = 1.0f;
    if (rate > 100.0f) rate = 100.0f;
    granularSpawnRate_ = rate;
}

void SampleLibrary::setGranularDuration(float duration) {
    // Clamp to valid range: 0.01 - 1.0 seconds
    if (duration < 0.01f) duration = 0.01f;
    if (duration > 1.0f) duration = 1.0f;
    granularDuration_ = duration;
}

void SampleLibrary::setGranularSpeed(float speed) {
    // Clamp to valid range: 0.1 - 4.0x multiplier
    if (speed < 0.1f) speed = 0.1f;
    if (speed > 4.0f) speed = 4.0f;
    granularSpeed_ = speed;
}

void SampleLibrary::setGranularPosition(float position) {
    // Clamp to valid range: 0.0 - 1.0 normalized
    if (position < 0.0f) position = 0.0f;
    if (position > 1.0f) position = 1.0f;
    granularPosition_ = position;
}

