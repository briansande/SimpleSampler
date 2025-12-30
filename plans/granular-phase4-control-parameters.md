# Phase 4: Control Parameters

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Map knobs/controls to granular parameters for real-time manipulation.

---

## Step 4.1: Add Control Parameter Variables

**File:** `SampleLibrary.h`

Add to `private` section:

```cpp
private:
    // ... existing members ...
    
    // Granular parameters (controlled by knobs/encoders)
    double grainPosition_;        // Where to spawn grains (0.0 to 1.0)
    double grainDuration_;        // How long grains last (in seconds)
    double grainPitch_;           // Pitch multiplier (0.5 to 2.0)
    
    // Note: spawnRate_ and randomness parameters were added in previous phases
```

---

## Step 4.2: Initialize Control Parameters

**File:** `SampleLibrary.cpp`

Update constructor:

```cpp
SampleLibrary::SampleLibrary(...) : ... {
    // ... existing initialization ...
    
    // Initialize granular parameters with defaults
    grainPosition_ = 0.5;        // Middle of sample
    grainDuration_ = 0.1;        // 100ms grains
    grainPitch_ = 1.0;            // Normal pitch
    
    // Note: spawnRate_ and randomness parameters already initialized in Phase 1/3
}
```

---

## Step 4.3: Update Auto-Spawning with Parameters

**File:** `SampleLibrary.cpp`

Update `processAudio()` spawning logic to use controlled parameters:

```cpp
// In processAudio(), inside the granular mode block:
if (granularModeEnabled_) {
    // Update spawning timer
    double blockDuration = (double)size / Config::samplerate;
    timeSinceLastGrain_ += blockDuration;
    
    // Calculate spawn interval from spawn rate
    double interval = 1.0 / spawnRate_;
    
    if (timeSinceLastGrain_ >= interval) {
        // Spawn grain with controlled parameters
        spawnGrain(
            -1,               // Use granularSampleIndex_
            grainPosition_,     // position
            grainDuration_,     // duration
            grainPitch_         // pitch
        );
        timeSinceLastGrain_ = 0.0;
    }
}
```

---

## Step 4.4: Add Getter/Setter Methods

**File:** `SampleLibrary.h`

Add to `public` section:

```cpp
public:
    // ... existing methods ...
    
    // ========== Granular Parameter Control ==========
    
    // Set grain spawn position (0.0 to 1.0)
    void setGrainPosition(double value);
    double getGrainPosition() const { return grainPosition_; }
    
    // Set grain duration (in seconds, 0.005 to 2.0)
    void setGrainDuration(double value);
    double getGrainDuration() const { return grainDuration_; }
    
    // Set grain pitch (0.5 to 2.0, 1.0 = normal)
    void setGrainPitch(double value);
    double getGrainPitch() const { return grainPitch_; }
```

**File:** `SampleLibrary.cpp`

Add implementations:

```cpp
void SampleLibrary::setGrainPosition(double value) {
    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    grainPosition_ = value;
}

void SampleLibrary::setGrainDuration(double value) {
    if (value < 0.005) value = 0.005;  // Minimum 5ms
    if (value > 2.0) value = 2.0;
    grainDuration_ = value;
}

void SampleLibrary::setGrainPitch(double value) {
    if (value < 0.5) value = 0.5;
    if (value > 2.0) value = 2.0;
    grainPitch_ = value;
}
```

---

## Success Criteria

- [ ] Granular parameters can be set via methods
- [ ] Parameters are used in auto-spawning logic
- [ ] Changes to parameters are reflected immediately in spawned grains
