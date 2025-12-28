#include "Config.h"
#include "SampleLibrary.h"


extern FIL SDFile;

// External declaration for custom memory allocator defined in SimpleSampler.cpp
extern void* custom_pool_allocate(size_t size);



SampleLibrary::SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem, DisplayManager& display)
    : sampleCount_(0),
      sdHandler_(sdHandler),
      fileSystem_(fileSystem),
      display_(display)
{
    // Initialize all samples as not loaded
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples_[i].loaded = false;
        samples_[i].audioDataLoaded = false;
        sampleSpeeds_[i] = 1.0f;  // Default to normal playback speed
    }
}

bool SampleLibrary::init() {
    // Show initialization message
    display_.showMessage("Initializing Library...", 200);
    
    // Filesystem is already mounted by SimpleSampler
    // (passed in via constructor reference)
    
    display_.showMessage("Opening dir...", 200);
    
    // Open the root directory
    DIR dir;
    if (f_opendir(&dir, "/") != FR_OK) {
        display_.showMessage("Dir open failed!", 200);
        return false;  // Failed to open directory
    }
    
    display_.showMessage("Scanning files...", 200);
    
    // Scan for WAV files
    FILINFO fno;
    FRESULT result;
    int fileCount = 0;



    // Read entries in a loop
    while (f_readdir(&dir, &fno) == FR_OK) {

        if (fno.fname[0] == 0) break;  // No more files
        
        // check if filename contains .wav or .WAV
        if (strstr(fno.fname, ".wav") != nullptr || strstr(fno.fname, ".WAV") != nullptr) {
            // Found a WAV file
            display_.showMessagef("Found WAV: %s", 200, fno.fname);
            if (fileCount < MAX_SAMPLES) {
                // Load sample info
                if(f_open(&SDFile, fno.fname, (FA_OPEN_EXISTING | FA_READ)) == FR_OK){
                    //print file name
                    int size = f_size(&SDFile);
                    display_.showMessagef("Size: %d bytes", 200, size);

                    // Allocate memory from custom pool, return pointer to buffer
                    char* memoryBuffer = 0;
                    memoryBuffer = (char*) custom_pool_allocate(size);

                    // If memory allocation succeeded, read file into buffer
                    if(memoryBuffer){
                        display_.showMessagef("Alloc OK, reading...", 200);
                        UINT bytesRead;
                        if(f_read(&SDFile, memoryBuffer, size, &bytesRead) == FR_OK && bytesRead == size){
                            display_.showMessagef("Read OK, parsing...", 200);
                            samples_[fileCount].dataSource = MemoryDataSource(memoryBuffer, size);
                            samples_[fileCount].reader.getWavInfo(samples_[fileCount].dataSource);
                            
                            // Copy filename to SampleInfo
                            strncpy(samples_[fileCount].name, fno.fname, 31);
                            samples_[fileCount].name[31] = '\0';
                            
                            // Copy WAV metadata from reader to SampleInfo
                            samples_[fileCount].numFrames = samples_[fileCount].reader.getNumFrames();
                            samples_[fileCount].channels = samples_[fileCount].reader.getChannels();
                            samples_[fileCount].sampleRate = (int)samples_[fileCount].reader.getFileDataRate();
                            samples_[fileCount].bitsPerSample = samples_[fileCount].reader.getBitsPerSample();
                            
                            display_.showMessagef("Parsed OK, creating ticker...", 200);
                            wavTickers[fileCount] = samples_[fileCount].reader.createWavTicker(Config::samplerate);
                            display_.showMessagef("Ticker created!", 200);
                            wavTickers[fileCount].finished_ = true; // Mark as finished initially
                            
                            display_.showMessagef("Loaded: %s", 200, fno.fname);

                            fileCount++;
                        
                        } else {
                            display_.showMessagef("Read failed!", 200);
                        }

                    } else {
                        display_.showMessagef("Alloc failed!", 200);
                    }
                    f_close(&SDFile);
                    
                } else {
                    display_.showMessagef("Open failed!", 200);
                }



            }
        }

    }

    // Close directory
    f_closedir(&dir);

    display_.showMessage("Files scanned", 300);
    // Print total files found
    char msg[64];
    snprintf(msg, sizeof(msg), "WAV Files: %d", fileCount);
    display_.showMessage(msg, 200);

    // Store the number of loaded samples
    sampleCount_ = fileCount;

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

// Process audio for all active samples
void SampleLibrary::processAudio(float** out, size_t size) {
    // Clear output buffers to zero
    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
    
    // Process each sample that is actively playing
    for (int i = 0; i < sampleCount_; i++) {
        if (!wavTickers[i].finished_) {
            // Call the reader's tick method to generate audio
            // Use per-sample speed from sampleSpeeds_ array (controlled by knobs)
            // Fixed volume: 1.0 (full volume)
            samples_[i].reader.tick(
                &wavTickers[i],
                samples_[i].dataSource,
                sampleSpeeds_[i],  // speed (controlled by knobs)
                1.0,  // volume
                size,
                out[0],
                out[1]
            );
            
            // The tick() method automatically sets finished_ to true
            // when the sample reaches the end, so no additional handling needed
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
    wavTickers[index].time_ = wavTickers[index].starttime_;
    wavTickers[index].finished_ = false;
    
    return true;
}

// Stop a currently playing sample
bool SampleLibrary::stopSample(int index) {
    // Validate index bounds
    if (index < 0 || index >= sampleCount_) {
        return false;
    }
    
    // Mark the sample as finished (stopped)
    wavTickers[index].finished_ = true;
    
    return true;
}

// Set the playback speed for a sample
void SampleLibrary::setSampleSpeed(int index, float speed) {
    // Validate index bounds
    if (index >= 0 && index < MAX_SAMPLES) {
        sampleSpeeds_[index] = speed;
    }
}

