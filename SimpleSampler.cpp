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


SdmmcHandler   sd;
FatFSInterface fsi;
FIL            SDFile;
DIR            dir;
FILINFO        fno;



const uint32_t DISPLAY_FPS = 10;                        //  FPS -- later converted to time in ms


void DisplayFilesOnScreen() {
    char strbuff[128];
    int fileCount = 0;
    int lineCount = 0;
    
    // Clear display
    display.Fill(false);
    display.SetCursor(0, 0);
    display.WriteString("SD Files:", Font_11x18, true);
    
    // Open root directory
    if(f_opendir(&dir, "/") != FR_OK) {
        display.SetCursor(0, 16);
        display.WriteString("SD Error!", Font_11x18, true);
        display.Update();
        return;
    }
    
    // Read directory entries
    while(1) {
        FRESULT res = f_readdir(&dir, &fno);
        
        // Break on error or end of directory
        if(res != FR_OK || fno.fname[0] == 0) break;
        
        // Skip directories and hidden files
        if(fno.fattrib & (AM_HID | AM_DIR)) continue;
        
        // Display filename (limited space on screen)
        if(lineCount < 3) {  // Show max 3 files at once
            display.SetCursor(0, 16 + (lineCount * 16));
            
            // Truncate filename if too long
            char displayName[20];
            strncpy(displayName, fno.fname, 19);
            displayName[19] = '\0';
            
            sprintf(strbuff, "%s", displayName);
            display.WriteString(strbuff, Font_11x18, true);
            lineCount++;
        }
        
        fileCount++;
    }
    
    f_closedir(&dir);
    
    // Show total count
    display.SetCursor(0, 48);
    sprintf(strbuff, "Total: %d files", fileCount);
    display.WriteString(strbuff, Font_11x18, true);
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






    char strbuff[128];
    while(1)
    {
        
        hw.encoder.Debounce();
        inc += hw.encoder.Increment();

        uint32_t now = System::GetNow();
        r = p_knob1.Process();
        g = p_knob2.Process();

        hw.led1.Set(r, g, b);

        hw.UpdateLeds();

        if(now - lastUpdateTime >= 1000/(DISPLAY_FPS))
        {
            // Display Testing Code
            sprintf(strbuff, "R:%d G:%d B:%d", (int)(r*255), (int)(g*255), (int)(b*255));
            display.Fill(false);
            display.SetCursor(0, 0);
            display.WriteString(strbuff, Font_11x18, false);
            
            sprintf(strbuff, "B:%d", (int)inc);
            display.SetCursor(0, 32);
            display.WriteString(strbuff, Font_11x18, false);
            // DisplayFilesOnScreen();

            display.Update();
            lastUpdateTime = now;
        }
    }
}

