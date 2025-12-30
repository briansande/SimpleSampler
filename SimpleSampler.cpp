#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include <string>

#include "Config.h"
#include "DisplayManager.h"
#include "SampleLibrary.h"
#include "Sequencer.h"
#include "Metronome.h"
#include "UIManager.h"
#include "Menus.h"
#include "daisysp.h"
#include "Constants.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

// Define the static Config::samplerate variable
namespace Config {
    int samplerate;
}

using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;

DaisyPod      hw;
MyOledDisplay display;
Parameter p_knob1, p_knob2;

// SD Card and filesystem
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

// Sequencer components (pointers, initialized in main)
static Sequencer* sequencer = nullptr;
static Metronome* metronome = nullptr;
static UIManager* uiManager = nullptr;

// Memory pool for SDRAM
DSY_SDRAM_BSS char custom_pool[Constants::Memory::CUSTOM_POOL_SIZE];
size_t pool_index = 0;

// Custom memory allocator
void* custom_pool_allocate(size_t size) {
    if (pool_index + size >= Constants::Memory::CUSTOM_POOL_SIZE) {
        return nullptr;
    }
    void* ptr = &custom_pool[pool_index];
    pool_index += size;
    return ptr;
}


// Audio Callback
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                                size)
{
    // Clear output buffers
    for(size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
    
    // Process sequencer audio (includes sample playback and metronome triggering)
    sequencer->processAudio(out, size);
    
    // Process metronome (adds metronome sound to output)
    metronome->process(out, size);
}

void updateSequencerLED(DaisyPod& hw, Sequencer* sequencer)
{
    if(sequencer->isRunning()) {
        // Flash LED on each step
        int currentStep = sequencer->getCurrentStep();
        if(currentStep % 4 == 0) {
            // Bright on beat
            hw.led1.Set(1.0f, 1.0f, 1.0f);
        } else {
            // Dim on off-beat
            hw.led1.Set(0.2f, 0.2f, 0.2f);
        }
    } else {
        // Red when stopped
        hw.led1.Set(0.5f, 0.0f, 0.0f);
    }
}


int main(void)
{
    hw.Init();
    // Set the sample rate now that hw is initialized
    Config::samplerate = hw.AudioSampleRate();

    /** Configure then initialize the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.pin_config.dc = hw.seed.GetPin(9);
    disp_cfg.driver_config.transport_config.pin_config.reset = hw.seed.GetPin(22);
    display.Init(disp_cfg);

    // Initialize DisplayManager - wraps the display for easy access
    new (&display_) DisplayManager(display, hw);

    display_.showMessage("Initializing...", 100);

    // Initialize the knobs
    float r = 0, g = 0, b = 0;
    p_knob1.Init(hw.knob1, 0, 1, Parameter::LINEAR);
    p_knob2.Init(hw.knob2, 0, 1, Parameter::LINEAR);

    hw.StartAdc();

    display_.showMessage("Sampler Started", 1000);
    display_.showMessage("Loading Samples...", 1000);

    /* SD Card Additions */
    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
        
    display_.showMessage("Starting Library Initialization", 1000);
    // Initialize library
    library = new SampleLibrary(sdcard, fsi, display_);
    display_.showMessage("Library created, calling init...", 1000);
    if (!library->init()) {
        display_.showMessage("SD Card Error!", 0);
        while(1);  // Halt
    }
    display_.showMessage("Library Initialized", 1000);

    // === Initialize Sequencer Components ===
    display_.showMessage("Initializing Sequencer...", 1000);
    sequencer = new Sequencer(library, Config::samplerate);
    sequencer->init();
    display_.showMessage("Initializing Metronome...", 1000);
    metronome = new Metronome();
    metronome->init(static_cast<float>(Config::samplerate));
    display_.showMessage("Initializing UI...", 1000);
    uiManager = new UIManager(&display_, sequencer, library);
    uiManager->init();
    
    // Set default BPM and start sequencer
    sequencer->setBpm(120.0f);
    sequencer->setRunning(true);
    
    display_.showMessage("Sequencer Ready!", 1000);

    // Start the audio callback
    hw.StartAudio(AudioCallback);

    while(1)
    {
        uint32_t now = System::GetNow();
        hw.ProcessDigitalControls();

        // === Knob 1: BPM Control (60-180) ===
        float knob1_value = p_knob1.Process();
        float bpm = Constants::UI::MIN_BPM + (knob1_value * Constants::UI::BPM_RANGE);  // Map 0.0-1.0 to 60-180 BPM
        sequencer->setBpm(bpm);
        
        // === Knob 2: Metronome Volume (0.0-1.0) ===
        float knob2_value = p_knob2.Process();
        metronome->setVolume(knob2_value);
        
        // === Encoder Handling ===
        int32_t enc_incr = hw.encoder.Increment();
        if(enc_incr > 0) {
            uiManager->handleEncoderIncrement();
        } else if(enc_incr < 0) {
            uiManager->handleEncoderDecrement();
        }
        
        // Encoder click detection (rising edge = button pressed)
        if(hw.encoder.RisingEdge()) {
            // Record press time for hold detection
            uiManager->getState().encoderPressed = true;
            uiManager->getState().encoderPressTime = System::GetNow();
            uiManager->getState().encoderHeld = false;
            uiManager->handleEncoderClick();
        }
        
        // Encoder release detection (falling edge = button released)
        if(hw.encoder.FallingEdge()) {
            // Reset hold state
            uiManager->getState().encoderPressed = false;
            uiManager->getState().encoderHeld = false;
        }
        
        // Non-blocking hold detection (check if pressed for more than 500ms)
        if(uiManager->getState().encoderPressed && !uiManager->getState().encoderHeld) {
            uint32_t pressDuration = System::GetNow() - uiManager->getState().encoderPressTime;
            if(pressDuration >= Constants::UI::HOLD_DETECT_MS) {
                // Hold detected - call handleEncoderHold() once
                uiManager->getState().encoderHeld = true;
                uiManager->handleEncoderHold();
            }
        }
        
        // === Button Handling ===
        if(hw.button1.RisingEdge()) {
            uiManager->handleButton1Press();
        }
        if(hw.button2.RisingEdge()) {
            uiManager->handleButton2Press();
        }
        
        // === Update UI ===
        uiManager->update();
        
        // === LED Feedback ===
        updateSequencerLED(hw, sequencer);

        // LED2 shows metronome volume level
        hw.led2.Set(knob2_value, 0.0f, 0.0f);
        
        hw.UpdateLeds();
    }
}
