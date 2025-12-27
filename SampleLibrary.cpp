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
    }
}

bool SampleLibrary::init() {
    // Show initialization message
    display_.showMessage("Initializing Library...", 1000);
    
    // Filesystem is already mounted by SimpleSampler
    // (passed in via constructor reference)
    
    display_.showMessage("Opening dir...", 1000);
    
    // Open the root directory
    DIR dir;
    if (f_opendir(&dir, "/") != FR_OK) {
        display_.showMessage("Dir open failed!", 1000);
        return false;  // Failed to open directory
    }
    
    display_.showMessage("Scanning files...", 1000);
    
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
            display_.showMessagef("Found WAV: %s", 1000, fno.fname);
            if (fileCount < MAX_SAMPLES) {
                // Load sample info
                if(f_open(&SDFile, fno.fname, (FA_OPEN_EXISTING | FA_READ)) == FR_OK){
                    //print file name
                    int size = f_size(&SDFile);
                    display_.showMessagef("Size: %d bytes", 1000, size);

                    // Allocate memory from custom pool, return pointer to buffer
                    char* memoryBuffer = 0;
                    memoryBuffer = (char*) custom_pool_allocate(size);

                    // If memory allocation succeeded, read file into buffer
                    if(memoryBuffer){
                        display_.showMessagef("Alloc OK, reading...", 1000);
                        UINT bytesRead;
                        if(f_read(&SDFile, memoryBuffer, size, &bytesRead) == FR_OK && bytesRead == size){
                            display_.showMessagef("Read OK, parsing...", 1000);
                            samples_[fileCount].dataSource = MemoryDataSource(memoryBuffer, size);
                            samples_[fileCount].reader.getWavInfo(samples_[fileCount].dataSource);
                            display_.showMessagef("Parsed OK, creating ticker...", 1000);
                            wavTickers[fileCount] = samples_[fileCount].reader.createWavTicker(Config::samplerate);
                            display_.showMessagef("Ticker created!", 1000);
                            wavTickers[fileCount].finished_ = true; // Mark as finished initially
                            
                            display_.showMessagef("Loaded: %s", 1000, fno.fname);

                            fileCount++;
                        
                        } else {
                            display_.showMessagef("Read failed!", 1000);
                        }

                    } else {
                        display_.showMessagef("Alloc failed!", 1000);
                    }
                    
                } else {
                    display_.showMessagef("Open failed!", 1000);
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
    display_.showMessage(msg, 1000);

    return true;
}

