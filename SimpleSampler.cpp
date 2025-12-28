#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include <string>

#include "Config.h"
#include "DisplayManager.h"
#include "SampleLibrary.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

// Define the static Config::samplerate variable
namespace Config {
    int samplerate;
}


using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;
const uint32_t DISPLAY_FPS = 3;                        //  FPS for OLED screen

DaisyPod      hw;
MyOledDisplay display;
Parameter p_knob1, p_knob2;


static int32_t  inc;

// File browsing state
const int MAX_FILES = 100;
const int FILES_PER_SCREEN = 4;  // Show 4 files at once (fits in 64px height)
char fileList[MAX_FILES][32];    // Cache filenames
bool isDirectory[MAX_FILES];     // Track which entries are directories
int totalFiles = 0;
int selectedFile = 0;             // Currently selected file index
int windowStart = 0;              // First file shown in current window
char currentPath[256];            // Current directory path (e.g., "/" or "/folder")

SdmmcHandler   sdcard;
FatFSInterface fsi;
FIL            SDFile;
DIR            dir;
FILINFO        fno;

// Our custom sampler components
// Line 50: Use raw storage (not yet constructed)
static char display_storage[sizeof(DisplayManager)];
static DisplayManager& display_ = *reinterpret_cast<DisplayManager*>(display_storage);

static SampleLibrary* library = nullptr;

// Memory pool for SDRAM
#define CUSTOM_POOL_SIZE (48*1024*1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;

// Custom memory allocator
void* custom_pool_allocate(size_t size) {
    if (pool_index + size >= CUSTOM_POOL_SIZE) {
        return nullptr;
    }
    void* ptr = &custom_pool[pool_index];
    pool_index += size;
    return ptr;
}



// Forward declarations
void CacheFileList();

// Helper function to get current folder name from path
const char* GetCurrentFolderName() {
    static char folderName[32];
    
    // If at root, return "Root"
    if(strcmp(currentPath, "/") == 0 || strlen(currentPath) == 0) {

        strcpy(folderName, "Root");
        return folderName;
    }
    
    // Find the last '/' in the path to get the folder name
    int len = strlen(currentPath);
    int lastSlash = 0;
    for(int i = len - 1; i >= 0; i--) {
        if(currentPath[i] == '/') {
            lastSlash = i;
            break;
        }
    }
    
    // Copy the folder name (everything after the last '/')
    strncpy(folderName, currentPath + lastSlash + 1, 31);
    folderName[31] = '\0';
    return folderName;
}

// Helper function to enter a directory
void EnterDirectory(const char* dirName) {
    if(strcmp(dirName, "..") == 0) {
        // Go up to parent directory
        // Find the last '/' and truncate there
        int len = strlen(currentPath);
        for(int i = len - 1; i >= 0; i--) {
            if(currentPath[i] == '/') {
                if(i == 0) {
                    // At root, keep "/"
                    currentPath[1] = '\0';
                } else {
                    currentPath[i] = '\0';
                }
                break;
            }
        }
    } else {
        // Enter subdirectory
        // Append directory name to current path
        if(strcmp(currentPath, "/") == 0) {
            // At root, don't add extra slash
            sprintf(currentPath, "/%s", dirName);
        } else {
            // Not at root, add separator
            // Use temporary buffer to avoid overlapping source/destination
            char newPath[256];
            // Check if path would overflow before copying
            size_t currentLen = strlen(currentPath);
            size_t dirLen = strlen(dirName);
            if(currentLen + dirLen + 2 < 256) {
                snprintf(newPath, 256, "%s/%s", currentPath, dirName);
                strcpy(currentPath, newPath);
            }
        }
    }

    // Reset selection and refresh file list
    selectedFile = 0;
    windowStart = 0;
    CacheFileList();
}

// Function to cache filenames and directories from SD card
void CacheFileList() {
    totalFiles = 0;

    if(f_opendir(&dir, currentPath) != FR_OK) return;

    // First, read all files and directories
    while(totalFiles < MAX_FILES - 1) {  // Reserve space for ".." entry
        FRESULT res = f_readdir(&dir, &fno);
        if(res != FR_OK || fno.fname[0] == 0) break;
        if(fno.fattrib & AM_HID) continue;  // Skip hidden files, but NOT directories

        strncpy(fileList[totalFiles], fno.fname, 31);
        fileList[totalFiles][31] = '\0';
        isDirectory[totalFiles] = (fno.fattrib & AM_DIR) ? true : false;
        totalFiles++;
    }

    f_closedir(&dir);

    // Add ".." (parent directory) entry at the bottom
    // Only add if we're not at root (root is "/" or empty)
    if(strlen(currentPath) > 1) {
        strncpy(fileList[totalFiles], "..", 31);
        fileList[totalFiles][31] = '\0';
        isDirectory[totalFiles] = true;  // ".." is a virtual directory
        totalFiles++;
    }
}



void DisplayFilesOnScreen() {
    display.Fill(false);

    // === HEADER (font is 10px tall) ===
    // Draw inverted header bar: white background rectangle + black text
    // Rectangle spans full width (0-127), height of font (0-9)
    display.DrawRect(0, 0, 127, 9, true, true);  // true, true = white, filled
    display.SetCursor(0, 0);
    display.WriteString(GetCurrentFolderName(), Font_7x10, false);  // false = black text

    // === WINDOW SCROLLING LOGIC ===
    // The "window" is which 4 files are currently visible on screen.
    // As selectedFile moves, we need to slide the window to keep it in view.

    // If we scrolled past the bottom of the window, move window down
    if(selectedFile >= windowStart + FILES_PER_SCREEN) {
        windowStart = selectedFile - FILES_PER_SCREEN + 1;
    }
    // If we scrolled past the top of the window, move window up
    if(selectedFile < windowStart) {
        windowStart = selectedFile;
    }

    // === BOUNDARY CHECKING ===
    // Keep selectedFile within valid range (0 to totalFiles-1)
    // This is important if totalFiles is 0 or after wrapping around
    if(selectedFile >= totalFiles) selectedFile = totalFiles - 1;
    if(selectedFile < 0) selectedFile = 0;

    // === DISPLAY FILES IN CURRENT WINDOW ===
    // Font_7x10 is 10px tall, so we position at 10, 20, 30, 40
    // This leaves room for footer at bottom
    for(int i = 0; i < FILES_PER_SCREEN && (windowStart + i) < totalFiles; i++) {
        int fileIndex = windowStart + i;  // Actual file index in our cached list
        display.SetCursor(0, 10 + (i * 10));  // Y position: 10, 20, 30, 40

        // === SELECTION INDICATOR ===
        // Prepend ">" for selected file, space for others
        // Add "/" suffix to directories (except ".." which is already clear)
        char strbuff[35];
        if(fileIndex == selectedFile) {
            if(isDirectory[fileIndex] && strcmp(fileList[fileIndex], "..") != 0) {
                sprintf(strbuff, ">%s/", fileList[fileIndex]);
            } else {
                sprintf(strbuff, ">%s", fileList[fileIndex]);
            }
        } else {
            if(isDirectory[fileIndex] && strcmp(fileList[fileIndex], "..") != 0) {
                sprintf(strbuff, " %s/", fileList[fileIndex]);
            } else {
                sprintf(strbuff, " %s", fileList[fileIndex]);
            }
        }

        // === TRUNCATE FOR DISPLAY ===
        // Font_7x10 is 7px wide, 128px / 7px ≈ 18 characters
        char displayName[22];
        strncpy(displayName, strbuff, 21);
        displayName[21] = '\0';  // Ensure null termination

        display.WriteString(displayName, Font_7x10, true);
    }

    // === FOOTER WITH POSITION INFO ===
    // Display current file position (e.g. "3/10") at bottom right
    display.SetCursor(105, 55);
    char footer[12];  // Increased from 8 to accommodate "999/999" + null terminator
    snprintf(footer, sizeof(footer), "%d/%d", selectedFile + 1, totalFiles);
    display.WriteString(footer, Font_6x8, true);


}



// Helper function to display a message with a delay
void DisplayMessage(const char* message, uint32_t delayMs) {
    display.Fill(false);                    // Clear the screen
    display.SetCursor(0, 0);                 // Position cursor at top-left
    display.WriteString((char*)message, Font_7x10, true);  // Write the message
    display.Update();                       // Update the display
    hw.DelayMs(delayMs);                    // Wait for specified time
}



// Audio Callback
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                                size)
{
    // Process audio for all active samples
    // This will clear output buffers and mix in all playing samples

    library->processAudio(out, size);
}


int main(void)
{

    hw.Init();
    // Set the sample rate now that hw is initialized
    Config::samplerate = hw.AudioSampleRate();
    inc     = 0;

    uint32_t lastUpdateTime = System::GetNow();             // Initialize lastUpdateTime to the current time

    /** Configure then initialize the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.pin_config.dc = hw.seed.GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = hw.seed.GetPin(22);
    display.Init(disp_cfg);

    // Initialize DisplayManager - wraps the display for easy access
    // Line 289: Construct in-place using placement new
    new (&display_) DisplayManager(display, hw);

    display_.showMessage("Initializing...", 100);

    // Initialize the knobs
    float r = 0, g = 0, b = 0;
    p_knob1.Init(hw.knob1, 0, 1, Parameter::LINEAR);
    p_knob2.Init(hw.knob2, 0, 1, Parameter::LINEAR);

    hw.StartAdc();



    // === CACHE FILE LIST ===
    // Read all filenames from SD card into memory once at startup.
    // This is much faster than reading the SD card every frame.
    // We do this AFTER mounting so the filesystem is ready.
    // strcpy(currentPath, "/");  // Start at root directory
    // CacheFileList();


    DisplayMessage("Sampler Started", 1000);
    DisplayMessage("Loading Samples...", 1000);


    /* SD Card Additions */
    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
    
    DisplayMessage("Starting Library Initialization", 1000);
    // Initialize library
    library = new SampleLibrary(sdcard, fsi, display_);
    DisplayMessage("Library created, calling init...", 1000);
    if (!library->init()) {
        display.SetCursor(0, 0);
        display.WriteString((char*)"SD Card Error!", Font_7x10, true);
        display.Update();
        while(1);  // Halt
    }
    DisplayMessage("Library Initialized", 1000);



    // Start the audio callback
    hw.StartAudio(AudioCallback);

    
    while(1)
    {
        uint32_t now = System::GetNow();
        hw.ProcessDigitalControls();

        if(hw.button1.RisingEdge()){
            library->triggerSample(0);  // Trigger first sample
            display_.showMessage("Triggered Sample 0", 0);
        }  
        if(hw.button2.RisingEdge()){
            library->triggerSample(1);  // Trigger first sample
            display_.showMessage("Triggered Sample 1", 0);
        }
            


        // Read knob values and map to speed range (0.1 to 3.0)
        float knob1_value = p_knob1.Process();
        float knob2_value = p_knob2.Process();
        
        // Map 0-1 to 0.1-3.0
        float speed0 = 0.1f + (knob1_value * 2.9f);  // Sample 0 speed
        float speed1 = 0.1f + (knob2_value * 2.9f);  // Sample 1 speed
        
        // Update sample speeds in real-time
        library->setSampleSpeed(0, speed0);
        library->setSampleSpeed(1, speed1);
        
        // Use knob values for LED feedback (optional visual indicator)
        r = knob1_value;
        g = knob2_value;
        hw.led1.Set(r, g, b);

        hw.UpdateLeds();

        

        // // Only update based on DISPLAY_FPS
        // if(now - lastUpdateTime >= 1000/(DISPLAY_FPS))
        // {        


        //     // SampleLibrary.samples_;
        //     // display_.showMessage("Main Loop", 0);

        //     // Loop to show all samples in SampleLibrary
        //     int sampleCount = library->getSampleCount();
        //     display_.showMessagef("Total Samples:\n%d", 1000,
        //         sampleCount);

        //     for(int i = 0; i < sampleCount; i++) {
        //         SampleInfo* sample = library->getSample(i);
        //         display_.showMessagef("Sample %d:\n%s\n%d Hz, %d ch", 1000,
        //             i + 1,
        //             sample->name,
        //             sample->sampleRate,
        //             sample->channels);
        //     }




        //     // display.Update();
        //     lastUpdateTime = now;
        // }




        
    }
}