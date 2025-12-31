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

// DEBUG: Debug state tracking
static uint32_t debugLastDisplayUpdate = 0;
static const uint32_t DEBUG_DISPLAY_INTERVAL_MS = 500;  // Update debug display every 0.5 seconds
static bool debugEnabled = false;  // Set to true to enable debug output

// Sequencer components (pointers, initialized in main)
static Sequencer* sequencer = nullptr;
static Metronome* metronome = nullptr;
static UIManager* uiManager = nullptr;

// Granular test mode state
static AppMode previousMode = MODE_MAIN_MENU;

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


// Granular test mode: Spawn 5 grains at different positions
void spawnTestGrains() {
    library->setGranularSampleIndex(2);  // Use sample 1 for granular test
    library->setGranularMode(true);       // Enable granular mode
    
    // Spawn 5 grains at evenly spaced positions across the sample
    for (int i = 0; i < 5; i++) {
        float position = i / 5.0f;  // 0.0, 0.2, 0.4, 0.6, 0.8
        library->spawnGrain(-1, position, 0.02f, 1.0f);  // 20ms duration, normal pitch
    }
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
    
    // DEBUG: Log current mode
    static uint32_t debugCounter = 0;
    debugCounter++;
    if (debugCounter % 48000 == 0) {  // Log every ~1 second (assuming 48kHz)
        // We'll use the display for debugging since we can't use printf in audio callback
        // The display update will be handled in the main loop
    }
    
    // Process audio based on current mode
    if (uiManager->getCurrentMode() == MODE_SEQUENCER) {
        // Process sequencer audio (includes sample playback and metronome triggering)
        sequencer->processAudio(out, size);
        
        // Process metronome (adds metronome sound to output)
        metronome->process(out, size);
    } else if (uiManager->getCurrentMode() == MODE_GRANULAR) {
        // Process granular synthesis audio
        library->processAudio(out, size);
    }
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


// DEBUG: Display current mode and sequencer state
void debugDisplayState()
{
    if (!debugEnabled) return;
    
    uint32_t now = System::GetNow();
    if (now - debugLastDisplayUpdate < DEBUG_DISPLAY_INTERVAL_MS) {
        return;
    }
    debugLastDisplayUpdate = now;
    
    // Get current state
    AppMode mode = uiManager->getCurrentMode();
    bool isRunning = sequencer->isRunning();
    
    // Clear and display debug info
    display_.clear();
    display_.setCursor(0, 0);
    display_.writeString("DEBUG INFO:", Font_7x10);
    
    // Display mode
    display_.setCursor(0, 12);
    const char* modeStr = (mode == MODE_MAIN_MENU) ? "MAIN MENU" :
                          (mode == MODE_GRANULAR) ? "GRANULAR" :
                          (mode == MODE_SEQUENCER) ? "SEQUENCER" : "UNKNOWN";
    display_.writeString("Mode: ", Font_7x10);
    display_.setCursor(42, 12);
    display_.writeString(modeStr, Font_7x10);
    
    // Display mode-specific debug info
    if (mode == MODE_GRANULAR) {
        // Granular mode debug info
        char line[32];
        
        // Display grain spawn count
        display_.setCursor(0, 24);
        display_.writeString("Spawned:", Font_7x10);
        display_.setCursor(56, 24);
        snprintf(line, sizeof(line), "%d", library->getDebugGrainSpawnCount());
        display_.writeString(line, Font_7x10);
        
        // Display grain spawn failures
        display_.setCursor(0, 36);
        display_.writeString("Failed:", Font_7x10);
        display_.setCursor(56, 36);
        snprintf(line, sizeof(line), "%d", library->getDebugGrainSpawnFailures());
        display_.writeString(line, Font_7x10);
        
        // Display active grain count
        display_.setCursor(0, 48);
        display_.writeString("Active:", Font_7x10);
        display_.setCursor(56, 48);
        snprintf(line, sizeof(line), "%d", library->getActiveGrainCount());
        display_.writeString(line, Font_7x10);
    } else {
        // Sequencer mode debug info
        // Display sequencer running state
        display_.setCursor(0, 24);
        display_.writeString("Seq Run: ", Font_7x10);
        display_.setCursor(56, 24);
        display_.writeString(isRunning ? "YES" : "NO", Font_7x10);
        
        // Display audio processing state
        display_.setCursor(0, 36);
        display_.writeString("Audio: ", Font_7x10);
        display_.setCursor(56, 36);
        display_.writeString((mode == MODE_SEQUENCER) ? "ENABLED" : "DISABLED", Font_7x10);
    }
    
    display_.update();
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


    /* SD Card Additions */
    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
        
    // Initialize library
    library = new SampleLibrary(sdcard, fsi, display_);
    if (!library->init()) {
        display_.showMessage("SD Card Error!", 2000);
        while(1);  // Halt
    }

    // === Initialize Sequencer Components ===
    sequencer = new Sequencer(library, Config::samplerate);
    sequencer->init();
    metronome = new Metronome();
    metronome->init(static_cast<float>(Config::samplerate));
    uiManager = new UIManager(&display_, sequencer, library);
    uiManager->init();
    
    // Set default BPM (but don't start sequencer - user enters from main menu)
    sequencer->setBpm(120.0f);
    sequencer->setRunning(false);
    
    display_.showMessage("Ready!", 400);

    // Start the audio callback
    hw.StartAudio(AudioCallback);

    while(1)
    {
        uint32_t now = System::GetNow();
        hw.ProcessDigitalControls();

        // === Granular Test Mode Logic ===
        AppMode currentMode = uiManager->getCurrentMode();
        
        // Detect mode changes
        if (currentMode != previousMode) {
            // Entering granular mode
            if (currentMode == MODE_GRANULAR && previousMode != MODE_GRANULAR) {
                ;
                //spawnTestGrains();  // Spawn 10 grains
            }
            // Exiting granular mode
            else if (previousMode == MODE_GRANULAR && currentMode != MODE_GRANULAR) {
                library->setGranularMode(false);  // Clear all grains
            }
            previousMode = currentMode;
        }
        
        // Note: Auto-spawning is now handled in SampleLibrary::processAudio()
        // This legacy code has been removed to avoid conflicts

        // === Knob and Button Handling (only in sequencer mode) ===
        if (uiManager->getCurrentMode() == MODE_SEQUENCER) {
            // === Knob 1: BPM Control (60-180) ===
            float knob1_value = p_knob1.Process();
            float bpm = Constants::UI::MIN_BPM + (knob1_value * Constants::UI::BPM_RANGE);  // Map 0.0-1.0 to 60-180 BPM
            sequencer->setBpm(bpm);
            
            // === Knob 2: Metronome Volume (0.0-1.0) ===
            float knob2_value = p_knob2.Process();
            metronome->setVolume(knob2_value);
        } else {
            // Process knobs anyway to prevent stale values
            p_knob1.Process();
            p_knob2.Process();
        }
        
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
            // Open the gate in granular mode when Button1 is pressed
            if (uiManager->getCurrentMode() == MODE_GRANULAR) {
                library->setGateOpen(true);
            }
        }
        if(hw.button1.FallingEdge()) {
            // Close the gate in granular mode when Button1 is released
            if (uiManager->getCurrentMode() == MODE_GRANULAR) {
                library->setGateOpen(false);
            }
        }
        if(hw.button2.RisingEdge()) {
            uiManager->handleButton2Press();
        }
        
        // === Update UI ===
        uiManager->update();
        
        // === DEBUG: Display debug state ===
        debugDisplayState();
        
        // === LED Feedback (only in sequencer mode) ===
        if (uiManager->getCurrentMode() == MODE_SEQUENCER) {
            updateSequencerLED(hw, sequencer);

            // LED2 shows metronome volume level
            float knob2_value = p_knob2.Process();
            hw.led2.Set(knob2_value, 0.0f, 0.0f);
        } else {
            // In main menu or granular mode, set LEDs to indicate mode
            if (uiManager->getCurrentMode() == MODE_MAIN_MENU) {
                hw.led1.Set(0.0f, 0.5f, 0.0f);  // Green for main menu
                hw.led2.Set(0.0f, 0.5f, 0.0f);
            } else if (uiManager->getCurrentMode() == MODE_GRANULAR) {
                // Show gate state with LED color in granular mode
                bool gateOpen = library->isGateOpen();
                if (gateOpen) {
                    hw.led1.Set(1.0f, 1.0f, 1.0f);  // White when gate is open
                    hw.led2.Set(1.0f, 1.0f, 1.0f);
                } else {
                    hw.led1.Set(0.5f, 0.0f, 0.5f);  // Purple when gate is closed
                    hw.led2.Set(0.5f, 0.0f, 0.5f);
                }
            }
        }
        
        hw.UpdateLeds();
    }
}
