#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"
#include "dev/oled_ssd130x.h"
#include <string>


using namespace daisy;
using namespace std;


using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;

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


SdmmcHandler   sd;
FatFSInterface fsi;
FIL            SDFile;
DIR            dir;
FILINFO        fno;

// Daisy WavPlayer -- not fully featured but will be OK for testing
WavPlayer      sampler;


const uint32_t DISPLAY_FPS = 10;                        //  FPS for OLED screen

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    int32_t inc;

    // Debounce digital controls
    hw.ProcessDigitalControls();

    // Change file with encoder.
    inc = hw.encoder.Increment();
    if(inc > 0)
    {
        size_t curfile;
        curfile = sampler.GetCurrentFile();
        if(curfile < sampler.GetNumberFiles() - 1)
        {
            sampler.Open(curfile + 1);
        }
    }
    else if(inc < 0)
    {
        size_t curfile;
        curfile = sampler.GetCurrentFile();
        if(curfile > 0)
        {
            sampler.Open(curfile - 1);
        }
    }

    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = out[i + 1] = s162f(sampler.Stream()) * 0.5f;
    }
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



int main(void)
{
    

    hw.Init();
    inc     = 0;

    uint32_t lastUpdateTime = System::GetNow();             // Initialize lastUpdateTime to the current time

    /** Configure then initialize the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.pin_config.dc = hw.seed.GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = hw.seed.GetPin(22);
    display.Init(disp_cfg);


    // Initialize the knobs
    float r = 0, g = 0, b = 0;
    p_knob1.Init(hw.knob1, 0, 1, Parameter::LINEAR);
    p_knob2.Init(hw.knob2, 0, 1, Parameter::LINEAR);

    hw.StartAdc();

    /* SD Card Additions */
    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    // === CACHE FILE LIST ===
    // Read all filenames from SD card into memory once at startup.
    // This is much faster than reading the SD card every frame.
    // We do this AFTER mounting so the filesystem is ready.
    strcpy(currentPath, "/");  // Start at root directory
    CacheFileList();






    while(1)
    {
        
        // === ENCODER FILE NAVIGATION ===
        hw.encoder.Debounce();
        int32_t increment = hw.encoder.Increment();

        // Only process if encoder moved and we have files
        if(increment != 0 && totalFiles > 0) {
            selectedFile += increment;

            // === WRAP-AROUND LOGIC ===
            // This makes browsing feel seamless - going past the last file
            // wraps to the first, and going before the first wraps to the last.
            // It's more intuitive than hitting a "wall" at the ends.
            if(selectedFile >= totalFiles) {
                selectedFile = 0;  // Wrap to beginning
            }
            if(selectedFile < 0) {
                selectedFile = totalFiles - 1;  // Wrap to end
            }
        }

        // === ENCODER CLICK - ENTER DIRECTORY ===
        // Check if encoder was clicked (pressed down)
        if(hw.encoder.RisingEdge() && totalFiles > 0) {
            // Only enter if the selected item is a directory
            if(isDirectory[selectedFile]) {
                EnterDirectory(fileList[selectedFile]);
            }
            // TODO: File handling will be added here later for WAV playback
        }

        uint32_t now = System::GetNow();
        r = p_knob1.Process();
        g = p_knob2.Process();

        hw.led1.Set(r, g, b);

        hw.UpdateLeds();

        if(now - lastUpdateTime >= 1000/(DISPLAY_FPS))
        {
            // Display Testing Code
            // sprintf(strbuff, "R:%d G:%d B:%d", (int)(r*255), (int)(g*255), (int)(b*255));
            // display.Fill(false);
            // display.SetCursor(0, 0);
            // display.WriteString(strbuff, Font_11x18, false);
            
            // sprintf(strbuff, "B:%d", (int)inc);
            // display.SetCursor(0, 32);
            // display.WriteString(strbuff, Font_11x18, false);
            
            
            DisplayFilesOnScreen();

            display.Update();
            lastUpdateTime = now;
        }
    }
}

