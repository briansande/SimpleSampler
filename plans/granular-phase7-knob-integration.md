# Phase 7: Knob Control Integration

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Map Daisy Pod knobs to granular parameters for real-time control.

---

## Step 7.1: Add Granular Mode Knob Handling

**File:** `SimpleSampler.cpp`

Update main loop knob handling:

```cpp
// === Knob and Button Handling ===
if (uiManager->getCurrentMode() == MODE_SEQUENCER) {
    // === Knob 1: BPM Control (60-180) ===
    float knob1_value = p_knob1.Process();
    float bpm = Constants::UI::MIN_BPM + (knob1_value * Constants::UI::BPM_RANGE);
    sequencer->setBpm(bpm);
    
    // === Knob 2: Metronome Volume (0.0-1.0) ===
    float knob2_value = p_knob2.Process();
    metronome->setVolume(knob2_value);
} else if (uiManager->getCurrentMode() == MODE_GRANULAR) {
    // === Granular Mode Knob Handling ===
    float knob1_value = p_knob1.Process();
    float knob2_value = p_knob2.Process();
    
    // Map knobs to granular parameters
    // Knob 1: Grain Position (0.0 to 1.0)
    library->setGrainPosition(knob1_value);
    
    // Knob 2: Spawn Rate (1.0 to 100.0 grains/sec)
    float spawnRate = 1.0 + (knob2_value * 99.0);
    library->setSpawnRate(spawnRate);
} else {
    // Process knobs anyway to prevent stale values
    p_knob1.Process();
    p_knob2.Process();
}
```

**What this does:**
- Knob 1 controls grain position (where in sample to spawn grains)
- Knob 2 controls spawn rate (how many grains per second)
- Knobs are only active when in granular mode

---

## Step 7.2: Add Encoder Control for Additional Parameters

**File:** `SimpleSampler.cpp`

You can use the encoder to cycle through additional parameters. This is optional but provides more control.

Add encoder handling in granular mode:

```cpp
// In main loop, after encoder increment/decrement handling
if (uiManager->getCurrentMode() == MODE_GRANULAR) {
    // Optional: Use encoder to cycle through parameters
    // This requires tracking a "selected parameter" state
    // For now, knobs provide primary control
}
```

---

## Alternative: Full Knob Mapping (4 Knobs)

If you want more comprehensive knob control, you could map all parameters:

```cpp
// === Granular Mode Knob Handling (Full Mapping) ===
float knob1_value = p_knob1.Process();
float knob2_value = p_knob2.Process();

// Knob 1: Grain Position (0.0 to 1.0)
library->setGrainPosition(knob1_value);

// Knob 2: Spawn Rate (1.0 to 100.0 grains/sec)
float spawnRate = 1.0 + (knob2_value * 99.0);
library->setSpawnRate(spawnRate);

// For more parameters, consider:
// - Using encoder for grain duration
// - Using buttons for randomness toggles
// - Creating a parameter selection system
```

---

## Success Criteria

- [ ] Knobs control granular parameters in real-time
- [ ] Changes are audible immediately
- [ ] Knobs only affect granular mode, not sequencer
- [ ] LED feedback shows granular mode is active
