# Phase 3: Randomness

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Add random variation to grain parameters for more organic, less mechanical sound.

---

## Step 3.1: Add Randomness Parameters

**File:** `SampleLibrary.h`

Add to `private` section:

```cpp
private:
    // ... existing members ...
    
    // Randomness amounts (0.0 = no randomness, 1.0 = full randomness)
    double positionRandomness_;     // Random variation in start position
    double durationRandomness_;     // Random variation in grain duration
    double pitchRandomness_;        // Random variation in pitch
```

---

## Step 3.2: Initialize Randomness Parameters

**File:** `SampleLibrary.cpp`

Update constructor:

```cpp
SampleLibrary::SampleLibrary(...) : ... {
    // ... existing initialization ...
    
    // Initialize randomness
    positionRandomness_ = 0.1;   // 10% variation in position
    durationRandomness_ = 0.25;  // 25% variation in duration
    pitchRandomness_ = 0.0;      // No pitch variation initially
}
```

---

## Step 3.3: Update spawnGrain() with Randomness

**File:** `SampleLibrary.cpp`

Modify `spawnGrain()` to add randomness:

```cpp
bool SampleLibrary::spawnGrain(int sampleIndex, double startPosition, double duration, double speed) {
    // Use granularSampleIndex_ if sampleIndex is -1
    int actualSampleIndex = sampleIndex;
    if (sampleIndex < 0) {
        actualSampleIndex = granularSampleIndex_;
    }
    
    // Validate sample index
    if (actualSampleIndex < 0 || actualSampleIndex >= sampleCount_) {
        return false;
    }
    
    // Validate sample is loaded
    if (!samples_[actualSampleIndex].audioDataLoaded) {
        return false;
    }
    
    // Find an available grain slot
    int availableSlot = -1;
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        if (grains_[i].finished) {
            availableSlot = i;
            break;
        }
    }
    
    // No available slot
    if (availableSlot < 0) {
        return false;
    }
    
    // Get sample info
    SampleInfo& sample = samples_[actualSampleIndex];
    double totalFrames = sample.numFrames;
    double sampleRate = sample.sampleRate;
    
    // Create a new ticker from the sample
    grains_[availableSlot].ticker = sample.reader.createWavTicker(Config::samplerate);
    
    // Add randomness to position
    double randomValue = (double)rand() / RAND_MAX;  // 0.0 to 1.0
    double position = startPosition + (positionRandomness_ * randomValue * 0.5 - positionRandomness_ * 0.25);
    
    // Clamp to valid range
    if (position < 0.0) position = 0.0;
    if (position > 1.0) position = 1.0;
    
    // Calculate start frame
    double startFrame = position * totalFrames;
    
    // Set grain position
    grains_[availableSlot].ticker.time_ = startFrame;
    grains_[availableSlot].ticker.starttime_ = startFrame;
    
    // Add randomness to duration
    randomValue = (double)rand() / RAND_MAX;
    double actualDuration = duration * (1.0 + durationRandomness_ * (randomValue * 2.0 - 1.0));
    
    // Clamp duration to minimum 5ms
    if (actualDuration < 0.005) actualDuration = 0.005;
    
    // Calculate end frame
    double durationFrames = sampleRate * actualDuration;
    double endFrame = startFrame + durationFrames;
    
    // Clamp to end of file
    if (endFrame > totalFrames) {
        endFrame = totalFrames;
    }
    
    grains_[availableSlot].ticker.endtime_ = endFrame;
    
    // Add randomness to pitch
    randomValue = (double)rand() / RAND_MAX;
    double actualSpeed = speed * (1.0 + pitchRandomness_ * (randomValue * 2.0 - 1.0));
    
    // Clamp speed to reasonable range
    if (actualSpeed < 0.1) actualSpeed = 0.1;
    if (actualSpeed > 4.0) actualSpeed = 4.0;
    
    // Set grain properties
    grains_[availableSlot].startTime = startFrame;
    grains_[availableSlot].endTime = endFrame;
    grains_[availableSlot].speed = actualSpeed;
    grains_[availableSlot].sampleIndex = actualSampleIndex;
    grains_[availableSlot].envelopePhase = 0.0f;
    
    // Mark grain as active
    grains_[availableSlot].finished = false;
    
    return true;
}
```

**What this does:**
- Adds random variation to each parameter
- Position: varies around `startPosition` by `positionRandomness_` (centered)
- Duration: varies around `duration` by `durationRandomness_` (±50% of randomness)
- Pitch: varies around `speed` by `pitchRandomness_` (±50% of randomness)
- Uses symmetric randomization (random * 2.0 - 1.0) for centered variation

---

## Step 3.4: Add Methods to Control Randomness

**File:** `SampleLibrary.h`

Add to `public` section:

```cpp
public:
    // ... existing methods ...
    
    // Set randomness amounts (0.0 to 1.0)
    void setPositionRandomness(double value);
    void setDurationRandomness(double value);
    void setPitchRandomness(double value);
    
    // Get current randomness values
    double getPositionRandomness() const { return positionRandomness_; }
    double getDurationRandomness() const { return durationRandomness_; }
    double getPitchRandomness() const { return pitchRandomness_; }
```

**File:** `SampleLibrary.cpp`

Add implementations:

```cpp
void SampleLibrary::setPositionRandomness(double value) {
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    positionRandomness_ = value;
}

void SampleLibrary::setDurationRandomness(double value) {
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    durationRandomness_ = value;
}

void SampleLibrary::setPitchRandomness(double value) {
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    pitchRandomness_ = value;
}
```

---

## Success Criteria

- [ ] Sound is more organic and less mechanical
- [ ] Increasing randomness makes sound more chaotic
- [ ] Randomness can be controlled in real-time via setter methods
