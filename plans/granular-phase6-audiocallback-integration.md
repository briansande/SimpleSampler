# Phase 6: AudioCallback Integration

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Integrate granular audio processing into the existing AudioCallback.

---

## Step 6.1: Update AudioCallback to Handle MODE_GRANULAR

**File:** `SimpleSampler.cpp`

Update the `AudioCallback` function:

```cpp
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
    // Note: MODE_MAIN_MENU produces no audio
}
```

**What this does:**
- Routes audio processing based on current application mode
- `MODE_SEQUENCER`: Processes sequencer and metronome (existing)
- `MODE_GRANULAR`: Processes granular synthesis (new)
- `MODE_MAIN_MENU`: No audio output

---

## Step 6.2: Add Granular Mode Handling to LED Feedback

**File:** `SimpleSampler.cpp`

Update the LED feedback function:

```cpp
void updateLED(DaisyPod& hw) {
    if (uiManager->getCurrentMode() == MODE_SEQUENCER) {
        // Sequencer mode LED feedback
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
        
        // LED2 shows metronome volume level
        float knob2_value = p_knob2.Process();
        hw.led2.Set(knob2_value, 0.0f, 0.0f);
    } else if (uiManager->getCurrentMode() == MODE_GRANULAR) {
        // Granular mode LED feedback
        // Purple color for granular mode
        hw.led1.Set(0.5f, 0.0f, 0.5f);
        hw.led2.Set(0.5f, 0.0f, 0.5f);
    } else if (uiManager->getCurrentMode() == MODE_MAIN_MENU) {
        // Main menu LED feedback
        // Green color for main menu
        hw.led1.Set(0.0f, 0.5f, 0.0f);
        hw.led2.Set(0.0f, 0.5f, 0.0f);
    }
}
```

**What this does:**
- Provides visual feedback for granular mode (purple LEDs)
- Differentiates granular mode from sequencer and main menu
- LED1 and LED2 both show purple in granular mode

---

## Success Criteria

- [ ] Granular audio is processed when in `MODE_GRANULAR`
- [ ] Sequencer audio still works when in `MODE_SEQUENCER`
- [ ] LED feedback shows correct mode
- [ ] No audio glitches when switching between modes
