#include "daisy_pod.h"
#include <math.h>

using namespace daisy;

DaisyPod  hw;
Parameter p_knob1, p_knob2;

// Global variables for audio detection
volatile float audio_level    = 0.0f;
volatile bool  audio_detected = false;

// Audio callback function
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // Calculate RMS of input signal
    float sum = 0.0f;
    for(size_t i = 0; i < size; i++)
    {
        // Copy input to output for audio pass-through
        out[0][i] = in[0][i]; // Left channel
        out[1][i] = in[1][i]; // Right channel

        // Use left channel (index 0) for audio detection
        sum += in[0][i] * in[0][i];
    }
    audio_level = sqrtf(sum / size);
}

int main(void)
{
    hw.Init();
    float r = 0, g = 0, b = 0;
    p_knob1.Init(hw.knob1, 0, 1, Parameter::LINEAR);
    p_knob2.Init(hw.knob2, 0, 1, Parameter::LINEAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while(1)
    {
        // Get knob values
        float threshold = p_knob1.Process(); // Use knob1 as threshold control
        float sensitivity
            = p_knob2.Process(); // Use knob2 as sensitivity control

        // Detect audio presence based on threshold
        float adjusted_threshold
            = threshold * 0.1f; // Scale threshold to reasonable range

        if(audio_level > adjusted_threshold)
        {
            // Audio detected - light up LEDs
            r              = sensitivity;
            g              = sensitivity * 0.5f;
            b              = sensitivity * 0.8f;
            audio_detected = true;
        }
        else
        {
            // No audio - LEDs off
            r              = 0;
            g              = 0;
            b              = 0;
            audio_detected = false;
        }

        hw.led1.Set(r, g, b);
        hw.led2.Set(r * 0.8f, g * 0.8f, b * 0.8f);

        hw.UpdateLeds();

        // Small delay to prevent overwhelming the system
        System::Delay(1);
    }
}