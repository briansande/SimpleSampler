#pragma once

#include "b3ReadWavFile.h"
#include "daisy_core.h"
#include "daisy_seed.h"
#include "DisplayManager.h"

#include <string>
#include "Constants.h"

using namespace daisy;
using namespace std;

// Structure to hold information about a loaded sample
struct SampleInfo {
    char name[32];              // Filename
    int numFrames;              // Number of sample frames
    int channels;               // 1 = mono, 2 = stereo
    int sampleRate;             // Sample rate (e.g., 48000)
    int bitsPerSample;          // 8, 16, 24, or 32
    MemoryDataSource dataSource;       // Pointer to audio data in SDRAM
    b3ReadWavFile reader;      // WAV file reader/parser
    bool loaded;               // Is metadata loaded?
    bool audioDataLoaded;       // Is full audio data loaded in RAM?
};

// Structure representing a single grain in granular synthesis
// A grain is a short segment of audio that plays with an envelope
struct Grain {
    b3WavTicker ticker;          // Playback position tracker
    int sampleIndex;              // Which sample file this grain uses
    float envelopePhase;          // Progress through grain (0.0 to 1.0)
};


class SampleLibrary {
private:
    SampleInfo samples_[Constants::SampleLibrary::MAX_SAMPLES];  // Array of loaded samples
    b3WavTicker wavTickers_[Constants::SampleLibrary::MAX_SAMPLES]; // Array of tickers for playback -- currently only one per sample
    float sampleSpeeds_[Constants::SampleLibrary::MAX_SAMPLES];   // Per-sample playback speed (default 1.0 = normal speed)

    int sampleCount_;                 // How many samples are loaded
    
    // Granular synthesis state
    Grain grains_[Constants::SampleLibrary::MAX_GRAINS];  // Pool of grain objects
    int activeGrainCount_;                                 // Number of currently active grains
    bool granularModeEnabled_;                              // Is granular synthesis active?
    int granularSampleIndex_;                               // Which sample to use for granular
    
    // Spawning timer (auto-spawning)
    float timeSinceLastGrain_;  // Time elapsed since last grain spawned
    float spawnRate_;           // Grains per second (legacy, use granularSpawnRate_)
    
    // Configurable granular parameters
    float granularSpawnRate_;    // Grains per second (1.0 - 100.0)
    float granularDuration_;     // Grain duration in seconds (0.01 - 1.0)
    float granularSpeed_;       // Playback speed multiplier (0.1 - 4.0)
    float granularPosition_;     // Start position within sample (0.0 - 1.0)
    
    // Randomness parameters (additive variation per grain)
    float granularSpawnRateRandom_;    // Spawn rate randomness (0 - 50 grains/sec)
    float granularDurationRandom_;     // Duration randomness (0 - 0.5 seconds)
    float granularSpeedRandom_;        // Speed randomness (0 - 2.0x)
    float granularPositionRandom_;     // Position randomness (0 - 0.5)
    
    // Gate control for manual grain spawning
    bool gateOpen_;             // Gate is open when Button1 is held
    
    // External references (from main.cpp)
    SdmmcHandler& sdHandler_;
    FatFSInterface& fileSystem_;
    DisplayManager& display_;         // Display manager for showing messages
    
    // Helper function to load only metadata (WAV header) from a file
    bool loadSampleInfo(const char* filename, int index);
    
    // Helper function to load full audio data for a sample (lazy loading)
    bool loadSampleData(int index);
    
    // Helper: Scan directory and load all WAV files
    bool scanAndLoadFiles();
    
    // Helper: Load a single WAV file
    bool loadWavFile(const char* filename, int index);

public:
    // Constructor
    SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem, DisplayManager& display);
    
    // Initialize: Scan SD card and load all WAV files
    bool init();
    
    // Get a sample by index
    SampleInfo* getSample(int index);
    
    // Load audio data for a sample (lazy loading)
    // Call this before playing a sample if audioDataLoaded is false
    bool ensureSampleLoaded(int index);
    
    // Get number of loaded samples
    int getSampleCount() const { return sampleCount_; }
    
    // Find sample by name (returns index, or -1 if not found)
    int findSample(const char* name);
    
    // Process audio for active samples
    // Called from AudioCallback to generate audio output
    void processAudio(float** out, size_t size);
    
    // Trigger a sample to start playing
    // Returns true if sample was triggered successfully
    bool triggerSample(int index);
    
    // Stop a currently playing sample
    // Returns true if sample was stopped successfully
    bool stopSample(int index);
    
    // Set the playback speed for a sample
    // Speed values: 0.1 = 10% speed, 1.0 = normal speed, 3.0 = 300% speed
    void setSampleSpeed(int index, float speed);

    // ========== Granular Synthesis Methods ==========

    // Enable or disable granular synthesis mode
    void setGranularMode(bool enabled);
    bool isGranularModeEnabled() const { return granularModeEnabled_; }

    // Set which sample to use for granular synthesis
    bool setGranularSampleIndex(int index);
    int getGranularSampleIndex() const { return granularSampleIndex_; }

    // Spawn a new grain from a sample
    // sampleIndex: which sample to use (or -1 to use granularSampleIndex_)
    // startPosition: where in sample to start (0.0 to 1.0)
    // duration: how long grain lasts (in seconds)
    // speed: playback speed/pitch (1.0 = normal)
    // Returns true if grain was spawned successfully
    bool spawnGrain(int sampleIndex, float startPosition, float duration, float speed);

    // Get number of currently active grains
    int getActiveGrainCount() const { return activeGrainCount_; }
    
    // ========== Debug Methods ==========
    
    // Get debug grain spawn count
    int getDebugGrainSpawnCount() const;
    
    // Get debug grain spawn failures
    int getDebugGrainSpawnFailures() const;
    
    // Get time since last grain spawn
    float getTimeSinceLastGrain() const { return timeSinceLastGrain_; }
    
    // Get spawn rate (grains per second)
    float getSpawnRate() const { return spawnRate_; }
    
    // ========== Gate Control Methods ==========
    
    // Set gate state (true = open, false = closed)
    // When gate is open, grains spawn at the configured spawn rate
    void setGateOpen(bool open);
    
    // Get current gate state
    bool isGateOpen() const { return gateOpen_; }
    
    // ========== Granular Parameter Control Methods ==========
    
    // Set spawn rate (grains per second, range: 1.0 - 100.0)
    void setGranularSpawnRate(float rate);
    float getGranularSpawnRate() const { return granularSpawnRate_; }
    
    // Set grain duration (seconds, range: 0.01 - 1.0)
    void setGranularDuration(float duration);
    float getGranularDuration() const { return granularDuration_; }
    
    // Set playback speed (multiplier, range: 0.1 - 4.0)
    void setGranularSpeed(float speed);
    float getGranularSpeed() const { return granularSpeed_; }
    
    // Set start position (normalized 0.0 - 1.0)
    void setGranularPosition(float position);
    float getGranularPosition() const { return granularPosition_; }
    
    // ========== Granular Randomness Control Methods ==========
    
    // Set spawn rate randomness (grains per second, range: 0.0 - 50.0)
    void setGranularSpawnRateRandom(float random);
    float getGranularSpawnRateRandom() const { return granularSpawnRateRandom_; }
    
    // Set duration randomness (seconds, range: 0.0 - 0.5)
    void setGranularDurationRandom(float random);
    float getGranularDurationRandom() const { return granularDurationRandom_; }
    
    // Set speed randomness (multiplier, range: 0.0 - 2.0)
    void setGranularSpeedRandom(float random);
    float getGranularSpeedRandom() const { return granularSpeedRandom_; }
    
    // Set position randomness (normalized, range: 0.0 - 0.5)
    void setGranularPositionRandom(float random);
    float getGranularPositionRandom() const { return granularPositionRandom_; }
};
