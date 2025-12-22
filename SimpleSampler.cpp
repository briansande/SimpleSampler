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

const uint32_t DISPLAY_FPS = 10;                        //  FPS -- later converted to time in ms




int main(void)
{
    hw.Init();
    
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



    char strbuff[128];
    while(1)
    {
        uint32_t now = System::GetNow();
        r = p_knob1.Process();
        g = p_knob2.Process();

        hw.led1.Set(r, g, b);

        hw.UpdateLeds();

        if(now - lastUpdateTime >= 1000/(DISPLAY_FPS))
        {
            
            sprintf(strbuff, "R:%d G:%d B:%d", (int)(r*255), (int)(g*255), (int)(b*255));
            display.Fill(false);
            display.SetCursor(0, 0);
            display.WriteString(strbuff, Font_11x18, false);
            
            sprintf(strbuff, "B:%d", (int)lastUpdateTime);
            display.SetCursor(0, 32);
            display.WriteString(strbuff, Font_11x18, false);
            
            display.Update();
            lastUpdateTime = now;
        }
    }
}
