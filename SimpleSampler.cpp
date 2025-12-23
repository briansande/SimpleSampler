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
int totalFiles = 0;
int selectedFile = 0;             // Currently selected file index
int windowStart = 0;              // First file shown in current window


SdmmcHandler   sd;
FatFSInterface fsi;
FIL            SDFile;
DIR            dir;
FILINFO        fno;



const uint32_t DISPLAY_FPS = 10;                        //  FPS -- later converted to time in ms


// Function to cache filenames from SD card
void CacheFileList() {
    totalFiles = 0;
    
    if(f_opendir(&dir, "/") != FR_OK) return;
    
    while(totalFiles < MAX_FILES) {
        FRESULT res = f_readdir(&dir, &fno);
        if(res != FR_OK || fno.fname[0] == 0) break;
        if(fno.fattrib & (AM_HID | AM_DIR)) continue;
        
        strncpy(fileList[totalFiles], fno.fname, 31);
        fileList[totalFiles][31] = '\0';
        totalFiles++;
    }
    
    f_closedir(&dir);
}



void DisplayFilesOnScreen() {
    display.Fill(false);

    // === HEADER (font is 10px tall) ===
    // Draw inverted header bar: white background rectangle + black text
    // Rectangle spans full width (0-127), height of font (0-9)
    display.DrawRect(0, 0, 127, 9, true, true);  // true, true = white, filled
    display.SetCursor(0, 0);
    display.WriteString("SD Files:", Font_7x10, false);  // false = black text

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
        char strbuff[32];
        if(fileIndex == selectedFile) {
            sprintf(strbuff, ">%s", fileList[fileIndex]);
        } else {
            sprintf(strbuff, " %s", fileList[fileIndex]);
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
    char footer[8];
    sprintf(footer, "%d/%d", selectedFile + 1, totalFiles);
    display.WriteString(footer, Font_6x8, true);

    display.Update();
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
    CacheFileList();






    char strbuff[128];
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

