#include "SampleLibrary.h"

// External memory allocator from main.cpp
extern char custom_pool[];
extern size_t pool_index;
extern const size_t CUSTOM_POOL_SIZE;

// Helper function to allocate from SDRAM pool
extern void* custom_pool_allocate(size_t size);

SampleLibrary::SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem)
    : sdHandler_(sdHandler),
      fileSystem_(fileSystem),
      sampleCount_(0)
{
    // Initialize all samples as not loaded
    for (int i = 0; i < MAX_SAMPLES; i++) {
        samples_[i].loaded = false;
    }
}

bool SampleLibrary::init() {
    // Mount the SD card file system
    FATFS& fs = fileSystem_.GetSDFileSystem();
    if (f_mount(&fs, "/", 1) != FR_OK) {
        return false;  // Failed to mount
    }
    
    // Open the root directory
    DIR dir;
    if (f_opendir(&dir, "/") != FR_OK) {
        return false;  // Failed to open directory
    }
    
    // Scan for WAV files
    FILINFO fno;
    FRESULT result;
    
    do {
        result = f_readdir(&dir, &fno);
        
        // Exit on error or end of directory
        if (result != FR_OK || fno.fname[0] == 0) {
            break;
        }
        
        // Skip directories and hidden files
        if (fno.fattrib & (AM_HID | AM_DIR)) {
            continue;
        }
        
        // Check if it's a .wav file
        const char* filename = fno.fname;
        if (strstr(filename, ".wav") == nullptr && 
            strstr(filename, ".WAV") == nullptr) {
            continue;  // Not a WAV file
        }
        
        // Try to load this sample
        if (sampleCount_ < MAX_SAMPLES) {
            if (loadSample(filename, sampleCount_)) {
                sampleCount_++;
            }
        } else {
            break;  // Reached maximum samples
        }
        
    } while (result == FR_OK);
    
    // Close directory
    f_closedir(&dir);
    
    return true;
}

bool SampleLibrary::loadSample(const char* filename, int index) {
    // Open the file
    FIL file;
    if (f_open(&file, filename, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        return false;  // Failed to open
    }
    
    // Get file size
    UINT fileSize = f_size(&file);
    
    // Allocate memory from SDRAM pool
    char* buffer = (char*)custom_pool_allocate(fileSize);
    if (buffer == nullptr) {
        f_close(&file);
        return false;  // Out of memory
    }
    
    // Read the entire file into memory
    UINT bytesRead;
    if (f_read(&file, buffer, fileSize, &bytesRead) != FR_OK) {
        f_close(&file);
        return false;  // Failed to read
    }
    
    // Close the file (we have it in memory now)
    f_close(&file);
    
    // Store sample info
    strncpy(samples_[index].name, filename, 31);
    samples_[index].name[31] = '\0';  // Ensure null-terminated
    
    // Create MemoryDataSource pointing to our buffer
    samples_[index].data = MemoryDataSource(buffer, fileSize);
    
    // Parse WAV header using b3ReadWavFile
    if (!samples_[index].reader.getWavInfo(samples_[index].data)) {
        return false;  // Failed to parse WAV
    }
    
    // Extract metadata from the reader
    samples_[index].numFrames = samples_[index].reader.getNumFrames();
    samples_[index].sampleRate = (int)samples_[index].reader.getFileDataRate();
    samples_[index].loaded = true;
    
    // Determine channels and bits per sample from reader
    // (b3ReadWavFile stores these internally, we'd need to add getters)
    // For now, assume stereo 16-bit (most common)
    samples_[index].channels = 2;
    samples_[index].bitsPerSample = 16;
    
    return true;
}

SampleInfo* SampleLibrary::getSample(int index) {
    if (index >= 0 && index < sampleCount_) {
        return &samples_[index];
    }
    return nullptr;
}

int SampleLibrary::findSample(const char* name) {
    for (int i = 0; i < sampleCount_; i++) {
        if (strcmp(samples_[i].name, name) == 0) {
            return i;  // Found it!
        }
    }
    return -1;  // Not found
}
