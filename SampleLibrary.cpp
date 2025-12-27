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
        samples_[i].audioDataLoaded = false;
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
            if (loadSampleInfo(filename, sampleCount_)) {
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

bool SampleLibrary::loadSampleInfo(const char* filename, int index) {
    // Open the file
    FIL file;
    if (f_open(&file, filename, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        return false;  // Failed to open
    }
    
    // Read only the WAV header (first 44 bytes)
    // WAV header structure: RIFF(4) + filesize(4) + WAVE(4) + fmt(4) +
    //                       fmt_size(4) + audio_fmt(2) + channels(2) +
    //                       sample_rate(4) + byte_rate(4) + block_align(2) +
    //                       bits_per_sample(2) + data(4) + data_size(4) = 44 bytes
    const int WAV_HEADER_SIZE = 44;
    char headerBuffer[WAV_HEADER_SIZE];
    
    UINT bytesRead;
    if (f_read(&file, headerBuffer, WAV_HEADER_SIZE, &bytesRead) != FR_OK || bytesRead != WAV_HEADER_SIZE) {
        f_close(&file);
        return false;  // Failed to read header
    }
    
    // Close the file - we only needed the header
    f_close(&file);
    
    // Store filename
    strncpy(samples_[index].name, filename, 31);
    samples_[index].name[31] = '\0';  // Ensure null-terminated
    
    // Create MemoryDataSource pointing to header buffer (temporary)
    samples_[index].data = MemoryDataSource(headerBuffer, WAV_HEADER_SIZE);
    
    // Parse WAV header using b3ReadWavFile
    if (!samples_[index].reader.getWavInfo(samples_[index].data)) {
        return false;  // Failed to parse WAV header
    }
    
    // Extract metadata from the reader
    samples_[index].numFrames = samples_[index].reader.getNumFrames();
    samples_[index].sampleRate = (int)samples_[index].reader.getFileDataRate();
    samples_[index].loaded = true;           // Metadata is loaded
    samples_[index].audioDataLoaded = false;  // Audio data NOT loaded yet
    
    // Determine channels and bits per sample from reader
    samples_[index].channels = 2;
    samples_[index].bitsPerSample = 16;
    
    return true;
}

bool SampleLibrary::loadSampleData(int index) {
    // Check if already loaded
    if (samples_[index].audioDataLoaded) {
        return true;  // Already loaded
    }
    
    // Open the file
    FIL file;
    if (f_open(&file, samples_[index].name, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
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
    
    // Close the file
    f_close(&file);
    
    // Update MemoryDataSource to point to full file buffer
    samples_[index].data = MemoryDataSource(buffer, fileSize);
    
    // Re-parse WAV header with the full buffer
    if (!samples_[index].reader.getWavInfo(samples_[index].data)) {
        return false;  // Failed to parse WAV
    }
    
    // Mark audio data as loaded
    samples_[index].audioDataLoaded = true;
    
    return true;
}

SampleInfo* SampleLibrary::getSample(int index) {
    if (index >= 0 && index < sampleCount_) {
        return &samples_[index];
    }
    return nullptr;
}

bool SampleLibrary::ensureSampleLoaded(int index) {
    if (index < 0 || index >= sampleCount_) {
        return false;  // Invalid index
    }
    return loadSampleData(index);
}

int SampleLibrary::findSample(const char* name) {
    for (int i = 0; i < sampleCount_; i++) {
        if (strcmp(samples_[i].name, name) == 0) {
            return i;  // Found it!
        }
    }
    return -1;  // Not found
}
