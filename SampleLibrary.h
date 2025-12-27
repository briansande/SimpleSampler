#ifndef SAMPLE_LIBRARY_H
#define SAMPLE_LIBRARY_H

#include "b3ReadWavFile.h"
#include "daisy_core.h"
#include "daisy_seed.h"
#include <string>

using namespace daisy;
using namespace std;

// Maximum number of samples we can load at once
#define MAX_SAMPLES 64

// Structure to hold information about a loaded sample
struct SampleInfo {
    char name[32];              // Filename
    int numFrames;              // Number of sample frames
    int channels;               // 1 = mono, 2 = stereo
    int sampleRate;             // Sample rate (e.g., 48000)
    int bitsPerSample;          // 8, 16, 24, or 32
    MemoryDataSource data;       // Pointer to audio data in SDRAM
    b3ReadWavFile reader;      // WAV file reader/parser
    bool loaded;               // Is metadata loaded?
    bool audioDataLoaded;       // Is full audio data loaded in RAM?
};

class SampleLibrary {
private:
    SampleInfo samples_[MAX_SAMPLES];  // Array of loaded samples
    int sampleCount_;                 // How many samples are loaded
    
    // External references (from main.cpp)
    SdmmcHandler& sdHandler_;
    FatFSInterface& fileSystem_;
    
    // Helper function to load only metadata (WAV header) from a file
    bool loadSampleInfo(const char* filename, int index);
    
    // Helper function to load full audio data for a sample (lazy loading)
    bool loadSampleData(int index);

public:
    // Constructor
    SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem);
    
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
};

#endif // SAMPLE_LIBRARY_H
