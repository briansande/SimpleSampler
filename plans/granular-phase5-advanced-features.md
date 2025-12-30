# Phase 5: Advanced Features

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Add advanced features like envelope control, freeze mode, and presets.

---

## Step 5.1: Add Envelope Control

> **NOTE:** The `b3WavTicker` struct already has built-in envelope methods:
> - `env_volume()`: Linear fade out
> - `env_volume2()`: Triangular envelope (fade in then out)
> 
> This step adds envelope shape selection to leverage these built-in methods.

**File:** `SampleLibrary.h`

Add to `private` section:

```cpp
private:
    // ... existing members ...
    
    // Envelope shape (0 = none, 1 = triangular, 2 = linear fade out)
    int envelopeShape_;
```

**File:** `SampleLibrary.cpp`

Update constructor to initialize:

```cpp
envelopeShape_ = 1;  // Default to triangular (best for granular)
```

Add setter/getter to `public` section:

```cpp
// Set envelope shape (0 = none, 1 = triangular, 2 = linear fade out)
void setEnvelopeShape(int shape);
int getEnvelopeShape() const { return envelopeShape_; }
```

Implementation:

```cpp
void SampleLibrary::setEnvelopeShape(int shape) {
    if (shape < 0) shape = 0;
    if (shape > 2) shape = 2;
    envelopeShape_ = shape;
}
```

**Note:** The `b3WavTicker` envelope is applied internally by the `tick()` method. To use different envelopes, you would need to modify the `b3ReadWavFile` implementation or apply envelope multiplication externally. For now, the default triangular envelope (`env_volume2()`) provides good results for granular synthesis.

---

## Step 5.2: Add Freeze Mode

**File:** `SampleLibrary.h`

Add to `private` and `public` sections:

```cpp
private:
    // ... existing members ...
    
    bool freezeMode_;  // When true, grains don't expire

public:
    // ... existing methods ...
    
    // Enable/disable freeze mode (infinite sustain)
    void setFreezeMode(bool enabled);
    bool getFreezeMode() const { return freezeMode_; }
```

**File:** `SampleLibrary.cpp`

Add implementation:

```cpp
void SampleLibrary::setFreezeMode(bool enabled) {
    freezeMode_ = enabled;
    
    if (enabled) {
        // Set all active grains to infinite duration
        for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
            if (!grains_[i].finished) {
                // Set endtime to a very large value
                grains_[i].ticker.endtime_ = 999999999.0;
            }
        }
    }
}
```

Update `processAudio()` spawning logic to not spawn new grains in freeze mode:

```cpp
// In processAudio(), inside auto-spawning block:
if (timeSinceLastGrain_ >= interval) {
    // Don't spawn new grains in freeze mode
    if (!freezeMode_) {
        spawnGrain(-1, grainPosition_, grainDuration_, grainPitch_);
        timeSinceLastGrain_ = 0.0;
    }
}
```

---

## Step 5.3: Add Preset System

**File:** `SampleLibrary.h`

Add struct and methods:

```cpp
// Structure to hold a granular preset
struct GranularPreset {
    double grainPosition;
    double grainDuration;
    double spawnRate;
    double grainPitch;
    double positionRandomness;
    double durationRandomness;
    double pitchRandomness;
    int envelopeShape;
};

private:
    // ... existing members ...
    
    GranularPreset presets_[8];  // 8 preset slots
    int currentPreset_;

public:
    // ... existing methods ...
    
    // Save current parameters to a preset slot
    bool savePreset(int slot);
    
    // Load parameters from a preset slot
    bool loadPreset(int slot);
    
    // Get current preset index
    int getCurrentPreset() const { return currentPreset_; }
```

**File:** `SampleLibrary.cpp`

Add implementations:

```cpp
bool SampleLibrary::savePreset(int slot) {
    if (slot < 0 || slot >= 8) return false;
    
    presets_[slot].grainPosition = grainPosition_;
    presets_[slot].grainDuration = grainDuration_;
    presets_[slot].spawnRate = spawnRate_;
    presets_[slot].grainPitch = grainPitch_;
    presets_[slot].positionRandomness = positionRandomness_;
    presets_[slot].durationRandomness = durationRandomness_;
    presets_[slot].pitchRandomness = pitchRandomness_;
    presets_[slot].envelopeShape = envelopeShape_;
    
    currentPreset_ = slot;
    return true;
}

bool SampleLibrary::loadPreset(int slot) {
    if (slot < 0 || slot >= 8) return false;
    
    grainPosition_ = presets_[slot].grainPosition;
    grainDuration_ = presets_[slot].grainDuration;
    spawnRate_ = presets_[slot].spawnRate;
    grainPitch_ = presets_[slot].grainPitch;
    positionRandomness_ = presets_[slot].positionRandomness;
    durationRandomness_ = presets_[slot].durationRandomness;
    pitchRandomness_ = presets_[slot].pitchRandomness;
    envelopeShape_ = presets_[slot].envelopeShape;
    
    currentPreset_ = slot;
    return true;
}
```

---

## Step 5.4: Add Multi-Sample Granular Mode (Optional)

**File:** `SampleLibrary.h`

Add members and methods:

```cpp
private:
    // ... existing members ...
    
    bool multiSampleMode_;              // Spawn from multiple samples
    int activeSampleIndices_[4];       // Up to 4 samples to use
    int activeSampleCount_;            // How many samples are active

public:
    // ... existing methods ...
    
    // Enable/disable multi-sample mode
    void setMultiSampleMode(bool enabled);
    bool getMultiSampleMode() const { return multiSampleMode_; }
    
    // Add a sample to the active list
    bool addActiveSample(int index);
    
    // Clear the active sample list
    void clearActiveSamples();
```

**File:** `SampleLibrary.cpp`

Initialize in constructor:

```cpp
multiSampleMode_ = false;
activeSampleCount_ = 0;
for (int i = 0; i < 4; i++) {
    activeSampleIndices_[i] = -1;
}
```

Add implementations:

```cpp
void SampleLibrary::setMultiSampleMode(bool enabled) {
    multiSampleMode_ = enabled;
    if (!enabled) {
        activeSampleCount_ = 0;
    }
}

bool SampleLibrary::addActiveSample(int index) {
    if (index < 0 || index >= sampleCount_) return false;
    if (!samples_[index].audioDataLoaded) return false;
    if (activeSampleCount_ >= 4) return false;
    
    // Check if already in list
    for (int i = 0; i < activeSampleCount_; i++) {
        if (activeSampleIndices_[i] == index) return true;
    }
    
    activeSampleIndices_[activeSampleCount_++] = index;
    return true;
}

void SampleLibrary::clearActiveSamples() {
    activeSampleCount_ = 0;
}
```

Update `spawnGrain()`:

```cpp
bool SampleLibrary::spawnGrain(int sampleIndex, double startPosition, double duration, double speed) {
    // Use granularSampleIndex_ if sampleIndex is -1
    int actualSampleIndex = sampleIndex;
    if (sampleIndex < 0) {
        actualSampleIndex = granularSampleIndex_;
        
        // If multi-sample mode is enabled, pick random sample
        if (multiSampleMode_ && activeSampleCount_ > 0) {
            int randomIndex = rand() % activeSampleCount_;
            actualSampleIndex = activeSampleIndices_[randomIndex];
        }
    }
    
    // ... rest of function using actualSampleIndex
}
```

---

## Success Criteria

- [ ] Envelope shape can be selected (triangular is default and recommended)
- [ ] Freeze mode allows infinite sustain of grains
- [ ] Presets can be saved and loaded
- [ ] Optional: Multi-sample mode allows grains from multiple samples
