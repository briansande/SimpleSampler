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
    int granularSampleIndex_;                                // Which sample to use for granular
    
    // Spawning timer (auto-spawning)
    float timeSinceLastGrain_;  // Time elapsed since last grain spawned
    float spawnRate_;           // Grains per second
    
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
};
