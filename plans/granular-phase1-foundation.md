# Phase 1: Foundation

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Create the basic data structures and functions to spawn and play multiple grains from a single sample.

---

## Step 1.1: Add Grain Data Structure

**File:** `SampleLibrary.h`

Add a new struct after `SampleInfo`:

```cpp
// Structure to hold state of a single grain in granular synthesis
struct Grain {
    b3WavTicker ticker;          // Playback position tracker
    int sampleIndex;              // Which sample file this grain uses
    double startTime;             // When grain started (in frames)
    double endTime;               // When grain should stop (in frames)
    double speed;                 // Playback speed (pitch multiplier)
    bool finished;                // Is this grain done playing?
    float envelopePhase;          // Progress through grain (0.0 to 1.0)
};
```

**What this does:**
- `ticker`: Tracks current playback position (same as your existing wavTickers)
- `sampleIndex`: Which sample file this grain reads from
- `startTime`: Where the grain started (in frames)
- `endTime`: When the grain should stop (in frames)
- `speed`: Playback speed/pitch (1.0 = normal)
- `finished`: Flag indicating if grain is done
- `envelopePhase`: Used for fade in/out (0.0 = start, 1.0 = end)

---

## Step 1.2: Add Grain Pool and Granular Mode State to SampleLibrary Class

**File:** `SampleLibrary.h`

Add to the `private` section of `SampleLibrary` class:

```cpp
private:
    // Granular synthesis state
    Grain grains_[Constants::SampleLibrary::MAX_GRAINS];  // Pool of grain objects
    int activeGrainCount_;                                 // Number of currently active grains
    bool granularModeEnabled_;                              // Is granular synthesis active?
    int granularSampleIndex_;                                // Which sample to use for granular
    
    // Spawning timer (auto-spawning)
    double timeSinceLastGrain_;  // Time elapsed since last grain spawned
    double spawnRate_;           // Grains per second (replaces spawnInterval_)
    
    // ... existing members ...
```

**What this does:**
- `grains_[]`: Array of grain objects (the "grain pool") using constant from Constants.h
- `activeGrainCount_`: Tracks how many grains are currently playing
- `granularModeEnabled_`: Flag to enable/disable granular synthesis
- `granularSampleIndex_`: Which sample to use for granular playback
- `timeSinceLastGrain_`: Tracks time since last grain spawn
- `spawnRate_`: Grains per second (redundant `spawnInterval_` removed)

---

## Step 1.3: Initialize Grain Pool and Granular State in Constructor

**File:** `SampleLibrary.cpp`

Update the constructor:

```cpp
SampleLibrary::SampleLibrary(daisy::SdmmcHandler& sdHandler, FatFSInterface& fileSystem, DisplayManager& display)
    : sampleCount_(0),
      activeGrainCount_(0),
      granularModeEnabled_(false),
      granularSampleIndex_(0),
      timeSinceLastGrain_(0.0),
      spawnRate_(30.0),  // 30 grains per second default
      sdHandler_(sdHandler),
      fileSystem_(fileSystem),
      display_(display)
{
    // Initialize all samples as not loaded
    for (int i = 0; i < Constants::SampleLibrary::MAX_SAMPLES; i++) {
        samples_[i].loaded = false;
        samples_[i].audioDataLoaded = false;
        sampleSpeeds_[i] = 1.0f;
    }
    
    // Initialize all grains as finished (inactive)
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        grains_[i].finished = true;
        grains_[i].sampleIndex = -1;
        grains_[i].envelopePhase = 0.0f;
    }
}
```

**What this does:**
- Marks all grains as `finished = true` (initially inactive)
- Sets `sampleIndex = -1` (no sample assigned)
- Initializes `envelopePhase` to 0
- Initializes granular mode as disabled by default
- Sets default granular sample index to 0
- Sets default spawn rate to 30 grains/second

---

## Step 1.4: Add Granular Mode and spawnGrain() Method Declarations

**File:** `SampleLibrary.h`

Add to the `public` section:

```cpp
public:
    // ... existing methods ...
    
    // ========== Granular Synthesis Methods ==========
    
    // Enable or disable granular synthesis mode
    void setGranularMode(bool enabled);
    bool isGranularModeEnabled() const { return granularModeEnabled_; }
    
    // Set which sample to use for granular synthesis
    bool setGranularSampleIndex(int index);
    int getGranularSampleIndex() const { return granularSampleIndex_; }
    
    // Spawn a new grain from a sample
    // sampleIndex: which sample to use (or -1 to use granularSampleIndex_)
    // startPosition: where in sample to start (0.0 to 1.0)
    // duration: how long grain lasts (in seconds)
    // speed: playback speed/pitch (1.0 = normal)
    // Returns true if grain was spawned successfully
    bool spawnGrain(int sampleIndex, double startPosition, double duration, double speed);
    
    // Get number of currently active grains
    int getActiveGrainCount() const { return activeGrainCount_; }
```

---

## Step 1.5: Implement spawnGrain() Method

**File:** `SampleLibrary.cpp`

Add this function after `setSampleSpeed()`:

```cpp
// Spawn a new grain from a sample
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
    
    // Calculate start position (convert 0.0-1.0 to frame number)
    double startFrame = startPosition * totalFrames;
    
    // Clamp to valid range
    if (startFrame < 0) startFrame = 0;
    if (startFrame >= totalFrames) startFrame = totalFrames - 1;
    
    // Set grain position
    grains_[availableSlot].ticker.time_ = startFrame;
    grains_[availableSlot].ticker.starttime_ = startFrame;
    
    // Calculate end position
    double durationFrames = sampleRate * duration;
    double endFrame = startFrame + durationFrames;
    
    // Clamp to end of file
    if (endFrame > totalFrames) {
        endFrame = totalFrames;
    }
    
    grains_[availableSlot].ticker.endtime_ = endFrame;
    
    // Set grain properties
    grains_[availableSlot].startTime = startFrame;
    grains_[availableSlot].endTime = endFrame;
    grains_[availableSlot].speed = speed;
    grains_[availableSlot].sampleIndex = actualSampleIndex;
    grains_[availableSlot].envelopePhase = 0.0f;
    
    // Mark grain as active
    grains_[availableSlot].finished = false;
    
    return true;
}
```

**What this does:**
1. Uses `granularSampleIndex_` if `sampleIndex` is -1
2. Validates inputs (sample index, loaded state)
3. Finds an empty grain slot in the pool
4. Creates a new ticker from the sample
5. Calculates start position from `startPosition` (0.0-1.0)
6. Calculates end position from `duration` (seconds)
7. Sets grain properties (speed, sample index, etc.)
8. Marks grain as active (`finished = false`)

---

## Step 1.6: Implement Granular Mode Methods

**File:** `SampleLibrary.cpp`

Add these implementations:

```cpp
void SampleLibrary::setGranularMode(bool enabled) {
    granularModeEnabled_ = enabled;
    
    // Clear all grains when disabling
    if (!enabled) {
        for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
            grains_[i].finished = true;
        }
        activeGrainCount_ = 0;
    }
}

bool SampleLibrary::setGranularSampleIndex(int index) {
    if (index < 0 || index >= sampleCount_) {
        return false;
    }
    
    if (!samples_[index].audioDataLoaded) {
        return false;
    }
    
    granularSampleIndex_ = index;
    return true;
}
```

---

## Step 1.7: Update processAudio() to Handle Grains

**File:** `SampleLibrary.cpp`

Replace the existing `processAudio()` function:

```cpp
// Process audio for all active samples and grains
void SampleLibrary::processAudio(float** out, size_t size) {
    // Clear output buffers to zero
    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
    
    // Process regular sample playback (existing functionality)
    for (int i = 0; i < sampleCount_; i++) {
        if (!wavTickers_[i].finished_) {
            samples_[i].reader.tick(
                &wavTickers_[i],
                samples_[i].dataSource,
                sampleSpeeds_[i],
                1.0,
                size,
                out[0],
                out[1]
            );
        }
    }
    
    // Process all active grains
    activeGrainCount_ = 0;
    for (int i = 0; i < Constants::SampleLibrary::MAX_GRAINS; i++) {
        if (!grains_[i].finished) {
            activeGrainCount_++;
            
            int sampleIndex = grains_[i].sampleIndex;
            double speed = grains_[i].speed;
            
            // Volume scaled by active grain count to prevent clipping
            // Use activeGrainCount_ instead of MAX_GRAINS for better dynamics
            float volume = (activeGrainCount_ > 0) ? (1.0f / activeGrainCount_) : 0.0f;
            
            samples_[sampleIndex].reader.tick(
                &grains_[i].ticker,
                samples_[sampleIndex].dataSource,
                speed,
                volume,
                size,
                out[0],
                out[1]
            );
            
            // Check if grain finished after tick
            // b3WavTicker.tick() sets finished_ when endtime_ is reached
            if (grains_[i].ticker.finished_) {
                grains_[i].finished = true;
            }
        }
    }
}
```

**What this does:**
- Clears output buffers
- Processes regular sample playback (preserves existing functionality)
- Processes all active grains
- Scales volume by `1.0 / activeGrainCount_` to prevent clipping when many grains overlap
- Properly manages grain lifecycle by checking `finished_` flag after `tick()`

---

## Step 1.8: Test Basic Grain Spawning

**File:** Your main control code (e.g., `SimpleSampler.cpp`)

Add manual grain spawning for testing:

```cpp
// Test: Spawn 3 grains from sample 0 at different positions
sampleLibrary.setGranularSampleIndex(0);  // Set sample to use
sampleLibrary.setGranularMode(true);       // Enable granular mode
sampleLibrary.spawnGrain(-1, 0.0, 0.1, 1.0);   // Start at 0%, 100ms, normal pitch
sampleLibrary.spawnGrain(-1, 0.5, 0.1, 1.0);   // Start at 50%, 100ms, normal pitch
sampleLibrary.spawnGrain(-1, 0.25, 0.1, 1.0);  // Start at 25%, 100ms, normal pitch
```

**Success Criteria:**
- You hear multiple short snippets from different positions in the sample playing simultaneously
- No clicking or popping sounds (grains fade naturally due to envelope in tick())
- `activeGrainCount_` decreases as grains finish playing

---

## Success Criteria

- [x] Grain data structure is defined
- [x] Grain pool is added to SampleLibrary class
- [x] Granular mode can be enabled/disabled
- [x] Sample for granular playback can be selected
- [x] Grains can be spawned manually
- [x] Grains play back correctly with proper volume scaling
- [x] Grain lifecycle is properly managed (grains are marked as finished when done)
