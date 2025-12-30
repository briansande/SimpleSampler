# Phase 2: Auto-Spawning

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Automatically spawn grains at regular intervals to create continuous granular texture.

> **NOTE:** Spawning timer variables (`timeSinceLastGrain_`, `spawnRate_`) were already added in Phase 1.2. This phase focuses on implementing auto-spawning logic.

---

## Step 2.1: Implement Auto-Spawning in processAudio()

**File:** `SampleLibrary.cpp`

Update `processAudio()` to add auto-spawning logic:

```cpp
// Process audio for all active samples and grains
void SampleLibrary::processAudio(float** out, size_t size) {
    // Clear output buffers to zero
    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
    
    // Auto-spawning: Only spawn if granular mode is enabled
    if (granularModeEnabled_) {
        // Update spawning timer
        double blockDuration = (double)size / Config::samplerate;
        timeSinceLastGrain_ += blockDuration;
        
        // Calculate spawn interval from spawn rate
        double interval = 1.0 / spawnRate_;
        
        // Check if it's time to spawn a new grain
        if (timeSinceLastGrain_ >= interval) {
            // Use granularSampleIndex_ (sampleIndex -1 in spawnGrain will use this)
            spawnGrain(-1, 0.5, 0.1, 1.0);  // Middle of sample, 100ms, normal pitch
            timeSinceLastGrain_ = 0.0;
        }
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
            if (grains_[i].ticker.finished_) {
                grains_[i].finished = true;
            }
        }
    }
}
```

**What this does:**
- Only spawns grains when `granularModeEnabled_` is true
- Tracks time elapsed since last grain spawn
- Spawns a new grain every `1.0 / spawnRate_` seconds
- Resets timer after spawning
- Uses `granularSampleIndex_` for sample selection (via sampleIndex -1)

---

## Step 2.2: Add Method to Control Spawn Rate

**File:** `SampleLibrary.h`

Add to `public` section:

```cpp
public:
    // ... existing methods ...
    
    // Set spawn rate (grains per second)
    // Higher values = denser texture, more grains
    // Lower values = sparser texture, fewer grains
    void setSpawnRate(double rate);
    
    // Get current spawn rate
    double getSpawnRate() const { return spawnRate_; }
```

**File:** `SampleLibrary.cpp`

Add implementation:

```cpp
void SampleLibrary::setSpawnRate(double rate) {
    // Clamp to reasonable range (1 to 100 grains per second)
    if (rate < 1.0) rate = 1.0;
    if (rate > 100.0) rate = 100.0;
    spawnRate_ = rate;
}
```

---

## Success Criteria

- [ ] Continuous stream of grains playing automatically when granular mode is enabled
- [ ] Changing `spawnRate` changes the density of the texture
- [ ] No grain spawning occurs when granular mode is disabled
- [ ] `activeGrainCount_` fluctuates as grains spawn and finish
