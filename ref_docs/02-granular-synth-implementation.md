# Granular Synth Implementation Documentation

## Overview

This document provides an in-depth analysis of the granular synthesis implementation from the DaisyCloudSeed project. The implementation is located in the `Granular` directory and extends the WAV playback infrastructure to create a real-time granular synthesizer.

### Key Files

- [`granular_synth_app.h`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.h:1) - Application-level header
- [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:1) - Granular synthesis engine
- [`granular_synth_main.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_main.cpp:1) - Main entry point

### Shared Components

The granular synth reuses the WAV parsing and playback infrastructure from the SamplePlayer:
- [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:1) - WAV reading interface
- [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:1) - WAV parsing and interpolation

---

## What is Granular Synthesis?

Granular synthesis is a sound synthesis method that operates on tiny segments of sound called **grains**. Each grain is typically 1-100 milliseconds long and is played back with potentially different parameters:

- **Position**: Where in the source sample the grain starts
- **Duration**: How long the grain plays
- **Pitch**: Playback speed of the grain
- **Envelope**: Amplitude shape (fade in/out)

By overlapping many grains with randomized parameters, complex textures and time-stretched effects can be created.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    granular_synth_main.cpp                   │
│              (Entry point, audio callback)                   │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                    GranularSynthApp                        │
│  (Grain spawning, parameter control, voice management)      │
└───────┬───────────────────────────────────┬───────────────┘
        │                                   │
        ▼                                   ▼
┌──────────────────────┐          ┌──────────────────────┐
│   SpawnGrain()      │          │   b3WavTicker[]     │
│  (Creates grains)    │          │  (Active grain pool) │
└──────────────────────┘          └──────────────────────┘
        │                                   │
        └───────────────────┬───────────────┘
                            │
                            ▼
                   ┌──────────────────────┐
                   │    b3ReadWavFile    │
                   │  (Audio data source) │
                   └──────────────────────┘
```

---

## Core Data Structures

### GranularSynthApp Class

Located in [`granular_synth_app.h`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.h:16):

```cpp
class GranularSynthApp : public DaisySynthApp
{
    daisy::DaisyPatch& m_patch;
    daisy::SdmmcHandler& m_sd_handler;
    std::string m_title;
    float m_ctrlVal[4];
    float m_prevCtrlVal[4];
    
    // WAV file infrastructure (shared with SamplePlayer)
    b3ReadWavFile m_wavFileReaders[kMaxFiles];
    b3WavTicker m_grains[kMaxFiles];        // Grain pool (max 64)
    daisy::WavFileInfo m_file_info_[kMaxFiles];
    int m_file_sizes[kMaxFiles];
    MemoryDataSource m_dataSources[kMaxFiles];
    
    // Granular-specific state
    int m_selected_file_index;
    int m_risingEdge;
    int m_active_voices;
    int m_file_cnt_;
    bool m_gateOut;
    float m_playback_speed;
    std::string m_sd_debug_msg;
    int m_mode;                      // 0 = grain size mode, 1 = pitch mode
    double m_timeSinceLastGrain;      // For grain spawning timing
    int m_samplerate;
    
    void SpawnGrain();
    void loadWavFiles();
};
```

**Key Differences from SamplePlayer**:
- Uses `m_grains[]` array instead of `m_wavTickers[]` (same type, different semantics)
- Adds `m_mode` for toggling between grain size and pitch control
- Adds `m_timeSinceLastGrain` for timing grain spawns
- Maximum 17 active grains (vs 20 in SamplePlayer)

---

## Granular Parameters

### Global Grain Parameters

Located in [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:20-29):

```cpp
float grain_spawn_position = 0.f;              // Start position (0-1 normalized)
float grain_spawn_position_rand_range = 0.05f; // Random position variation
float grain_pitch = 1.f;                      // Pitch multiplier (1.0 = original)
float grain_pitch_rand_range = 0.f;             // Random pitch variation
float grain_duration = .6f;                    // Grain duration in seconds
float grain_duration_rand_range_fraction = 0.25f; // Duration randomization
float max_grain_duration = 3.0;              // Maximum grain duration (seconds)
float grain_spawn_interval = 0.03f;           // Time between grains (seconds)
float grain_spawn_interval_rand_fraction = 0.25f; // Interval randomization
double spawn_interval = grain_spawn_interval;   // Current spawn interval
```

---

## Grain Spawning

### SpawnGrain() Method

Located in [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:57), creates a new grain:

```cpp
void GranularSynthApp::SpawnGrain()
```

**Process**:

1. **Find Available Grain Slot** (lines 59-68):
   ```cpp
   int available_index = -1;
   for (int i=0;i<MAX_GRAINS;i++)
   {
       if (m_grains[i].finished_)
       {
           available_index = i;
           break;
       }
   }
   ```

2. **Create New Grain from Selected WAV** (line 71):
   ```cpp
   m_grains[available_index] = m_wavFileReaders[m_selected_file_index]
                               .createWavTicker(m_samplerate);
   ```

3. **Calculate Spawn Position** (lines 72-75):
   ```cpp
   double r = ((double)rand() / (RAND_MAX));
   double spawn_position = grain_spawn_position + grain_spawn_position_rand_range * r;
   double maxNumFrames = double(m_wavFileReaders[m_selected_file_index].getNumFrames());
   m_grains[available_index].time_ = spawn_position * maxNumFrames;
   ```

   **Algorithm**:
   - Generate random value `r` between 0 and 1
   - Add random offset to base position
   - Convert normalized position to frame count

4. **Store Start Time** (line 76):
   ```cpp
   m_grains[available_index].starttime_ = m_grains[available_index].time_;
   ```

5. **Calculate Grain Duration** (lines 77-82):
   ```cpp
   r = ((double)rand() / (RAND_MAX));
   double duration = max_grain_duration * (grain_duration + 
                                          grain_duration_rand_range_fraction * 
                                          grain_duration * r);
   double endTime = m_grains[available_index].time_ + m_samplerate * duration;
   if (endTime > maxNumFrames)
       endTime = maxNumFrames;
   m_grains[available_index].endtime_ = endTime;
   ```

   **Algorithm**:
   - Generate new random value
   - Calculate duration with randomization
   - Convert to frame count
   - Clamp to file end

6. **Associate with WAV File** (line 84):
   ```cpp
   m_grains[available_index].wavindex = m_selected_file_index;
   ```

7. **Calculate Grain Pitch** (lines 85-89):
   ```cpp
   double rnd = ((double)rand() / (RAND_MAX)); 
   double pitch = grain_pitch * (1. + rnd * grain_pitch_rand_range);
   if (pitch < 0.1)
       pitch = 0.1;
   m_grains[available_index].speed_ = pitch;
   ```

8. **Activate Grain** (line 90):
   ```cpp
   m_grains[available_index].finished_ = false;
   ```

---

## Grain Envelope

### Triangular Envelope (env_volume2)

Located in [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:162):

```cpp
double env_volume2()
{
    double frac = (time_ - starttime_) / (endtime_ - starttime_);
    if (frac > 0.5)
        return 1. - frac;
    return frac;
}
```

**Visualization**:

```
Volume
  1.0 |      /\
      |     /  \
      |    /    \
  0.0 |___/      \___
       0%        50%       100%
      start            end
```

**Characteristics**:
- Linear fade-in from 0 to 0.5
- Linear fade-out from 0.5 to 1.0
- Prevents clicks at grain boundaries
- Creates smooth overlapping textures

---

## Audio Processing

### AudioTickCallback Method

Located in [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:207):

```cpp
void GranularSynthApp::AudioTickCallback(float ctrlVal[4], const float *const*in, 
                                        float **out, size_t size)
```

**Process**:

1. **Update Grain Spawn Timer** (lines 214-222):
   ```cpp
   m_timeSinceLastGrain += 0.001;
   if (m_timeSinceLastGrain > spawn_interval)
   {
       SpawnGrain();
       m_timeSinceLastGrain = 0;
       double r = ((double)rand() / (RAND_MAX));
       double grain_spawn_interval_rand_range = 
           grain_spawn_interval_rand_fraction * grain_spawn_interval;
       spawn_interval = grain_spawn_interval + r * grain_spawn_interval_rand_range;
   }
   ```

   **Note**: `0.001` is approximate time per audio tick (1ms at 48kHz with 48 samples)

2. **Handle Encoder/Gate** (lines 225-228):
   ```cpp
   if (m_patch.encoder.RisingEdge() || 
       m_patch.gate_input[DaisyPatch::GateInput::GATE_IN_1].Trig())
   {
       m_risingEdge = true;
   }
   ```

3. **Clear Output Buffers** (lines 271-277):
   ```cpp
   for (size_t i = 0; i < size; i++)
   {
       out[0][i] = 0.f;
       out[1][i] = 0.f;
       out[2][i] = 0;
       out[3][i] = 0;
   }
   ```

4. **Update Granular Parameters from Controls** (lines 279-289):
   ```cpp
   m_active_voices = 0;
   grain_spawn_position = ctrlVal[0];
   grain_spawn_position_rand_range = ctrlVal[1];
   grain_spawn_interval = ctrlVal[2];
   if (m_mode == 1)
   {
       grain_pitch = ctrlVal[3];
   }
   else
   {
       grain_duration = ctrlVal[3];
   }
   ```

5. **Process Active Grains** (lines 291-309):
   ```cpp
   for (int g = 0; g < m_file_cnt_; g++)
   {
       if (!m_grains[g].finished_ && m_active_voices < 20)
       {
           m_active_voices++;
           
           double volume_gain = 1. / 10;
           double volume = volume_gain * m_grains[g].env_volume2();
           
           if (!m_grains[g].finished_)
           {
               double speed = m_grains[g].speed_;
               int wavindex = m_grains[g].wavindex;
               m_wavFileReaders[wavindex].tick(&m_grains[g], m_dataSources[wavindex], 
                                               speed, volume, size, out[0], out[1]);
           }
       }
   }
   ```

   **Key Points**:
   - Volume scaled by `1/10` to prevent clipping with multiple grains
   - Triangular envelope applied per grain
   - Each grain plays at its own pitch/speed

---

## Mode Switching

### Encoder Press Toggles Mode

Located in [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:376-381):

```cpp
void GranularSynthApp::MainLoopCallback()
{
    if (m_risingEdge)
    {
        m_mode = 1 - m_mode;  // Toggle between 0 and 1
        m_risingEdge = false;
    }
    // ...
}
```

**Modes**:
- **Mode 0 (Default)**: CTRL3 controls grain duration
- **Mode 1**: CTRL3 controls grain pitch

---

## Control Mapping

| Control | Mode 0 | Mode 1 | Range | Description |
|---------|--------|--------|-------|-------------|
| CTRL0 | Spawn Position | Spawn Position | 0.0 - 1.0 | Base position for grain starts |
| CTRL1 | Position Random Range | Position Random Range | 0.0 - 1.0 | Random variation in position |
| CTRL2 | Spawn Interval | Spawn Interval | 0.0 - 1.0 | Time between grain spawns |
| CTRL3 | Grain Duration | Grain Pitch | 0.0 - 1.0 | Duration or pitch based on mode |
| Encoder Press | Toggle Mode | Toggle Mode | - | Switch between duration/pitch control |

---

## File Loading

### loadWavFiles Method

Located in [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:123):

The file loading is identical to SamplePlayer, with one key difference:

```cpp
m_grains[m_file_cnt_] = m_wavFileReaders[m_file_cnt_].createWavTicker(m_samplerate);
m_grains[m_file_cnt_].finished_ = true;  // Start all grains inactive
```

Each loaded WAV file gets a grain slot initialized but inactive.

---

## Grain Voice Limit

```cpp
#define MAX_GRAINS 17
//max grain around in range 17-24
```

Located in [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:54).

**Why 17?** This is a practical limit based on CPU performance. The comment suggests the actual limit is around 17-24 grains before CPU overload.

---

## Granular Synthesis Techniques Demonstrated

### 1. Time Stretching

By setting:
- Low spawn interval (CTRL2 near 0)
- Short grain duration (CTRL3 near 0)
- Low position randomization (CTRL1 near 0)

The sample plays back much slower than original while maintaining pitch.

### 2. Pitch Shifting

By setting:
- Mode 1 (encoder press)
- CTRL3 > 1.0 for higher pitch
- CTRL3 < 1.0 for lower pitch
- Low spawn interval for smooth texture

### 3. Texture Generation

By setting:
- High position randomization (CTRL1 high)
- Moderate spawn interval
- Short grain duration

Creates ambient, textural sounds from any sample.

### 4. Cloud Synthesis

By setting:
- High spawn interval (many overlapping grains)
- Variable grain duration
- Random pitch variation

Creates "cloud-like" evolving textures.

---

## Envelope Analysis

### Why Triangular Envelope?

The triangular envelope (`env_volume2`) is ideal for granular synthesis because:

1. **No Clicks**: Fade-in prevents clicks at grain start
2. **Smooth Overlap**: Fade-out allows smooth overlap between grains
3. **Center-Peak**: Maximum amplitude in middle for best signal-to-noise
4. **Computationally Cheap**: Simple linear calculation

### Comparison with Linear Decay (`env_volume`)

```cpp
double env_volume()
{
    double frac = 1. - (time_ - starttime_) / (endtime_ - starttime_);
    return frac;
}
```

This simple decay envelope is used in SamplePlayer but not granular synthesis because:
- No fade-in creates clicks at grain start
- Better for one-shot playback, not overlapping grains

---

## Randomization Strategy

### Position Randomization

```cpp
double spawn_position = grain_spawn_position + grain_spawn_position_rand_range * r;
```

**Effect**: Creates "clouds" of grains around the base position.

### Duration Randomization

```cpp
double duration = max_grain_duration * (grain_duration + 
                                      grain_duration_rand_range_fraction * 
                                      grain_duration * r);
```

**Effect**: Creates varied grain lengths for more natural textures.

### Interval Randomization

```cpp
spawn_interval = grain_spawn_interval + r * grain_spawn_interval_rand_range;
```

**Effect**: Prevents rhythmic patterns, creates more organic textures.

### Pitch Randomization

```cpp
double pitch = grain_pitch * (1. + rnd * grain_pitch_rand_range);
```

**Effect**: Adds detuning for richer, thicker sounds.

---

## Performance Considerations

### CPU Usage per Grain

Each grain requires:
- Envelope calculation (1 multiply-add)
- Interpolation (several memory accesses + math)
- Output mixing (2 adds)

**Approximate**: ~10-20 CPU cycles per sample per grain

### Memory Usage

- **Per Grain**: ~64 bytes (b3WavTicker structure)
- **Total for 17 grains**: ~1KB
- **Audio Data**: Shared with WAV file storage

### Voice Limiting

```cpp
if (!m_grains[g].finished_ && m_active_voices < 20)
```

Even though MAX_GRAINS is 17, the code checks for 20. This provides headroom for edge cases.

---

## Comparison with SamplePlayer

| Feature | SamplePlayer | GranularSynth |
|---------|--------------|---------------|
| Voices | 20 max | 17 max |
| Envelope | None (full amplitude) | Triangular |
| Position | Sequential | Randomized |
| Duration | Full file | Configurable |
| Pitch | Configurable | Configurable + random |
| Spawn | Manual trigger | Automatic interval |
| Mode | Single | Duration/Pitch toggle |

---

## Implementation Tips for Your Own Granular Synth

### 1. Grain Pool Management

Consider using a circular buffer for grain slots:
```cpp
int next_grain_slot = 0;
void SpawnGrain() {
    m_grains[next_grain_slot] = ...;
    next_grain_slot = (next_grain_slot + 1) % MAX_GRAINS;
}
```

This avoids searching for available slots.

### 2. Window Functions

Replace triangular envelope with better windows:
- Hanning: `0.5 * (1 - cos(2*PI*frac))`
- Hamming: `0.54 - 0.46 * cos(2*PI*frac)`
- Gaussian: `exp(-((frac-0.5)^2) / (2*sigma^2))`

### 3. Overlap-Add

For smoother results, use overlap-add:
- Grains overlap by 50%
- Sum contributions at each sample

### 4. Pitch-Synchronous Grains

For pitched material, sync grain length to period:
```cpp
float period = samplerate / frequency;
grain_duration = 4 * period;  // 4 periods per grain
```

### 5. Freeze Mode

Add a mode where position doesn't change:
```cpp
if (freeze_mode) {
    grain_spawn_position = current_position;
    grain_spawn_position_rand_range = 0;
}
```

---

## Common Granular Synthesis Parameters

### Texture Parameters

| Parameter | Texture | Ambient | Rhythmic |
|-----------|----------|---------|----------|
| Spawn Interval | 10-100ms | 100-500ms | Fixed tempo |
| Grain Duration | 10-50ms | 50-200ms | Short |
| Position Random | High | Medium | Low |
| Pitch Random | Medium | Low | None |

### Time Stretching Parameters

| Stretch Factor | Interval | Duration | Position Random |
|----------------|----------|----------|-----------------|
| 2x | 25ms | 50ms | 0 |
| 4x | 50ms | 100ms | 0 |
| 8x | 100ms | 200ms | 0 |

### Pitch Shifting Parameters

| Shift | Pitch | Interval | Duration |
|--------|-------|----------|----------|
| +1 octave | 2.0 | 10ms | 20ms |
| -1 octave | 0.5 | 20ms | 40ms |
| +5th | 1.5 | 10ms | 20ms |

---

## Debugging Tips

### Monitor Active Voices

```cpp
test::sprintf(buf, "a=%d, dur=%f", m_active_voices, grain_duration);
```

Display shows active grain count for performance monitoring.

### Check Grain Completion

Add logging in SpawnGrain to see when grains are spawned:
```cpp
// Debug: log grain spawn
printf("Spawned grain %d at position %f\n", available_index, spawn_position);
```

### Visualize Envelope

Create a test output that shows envelope shape:
```cpp
out[2][i] = m_grains[g].env_volume2();  // Send to output 2
```

---

## References

- **Original Code**: [DaisyCloudSeed on GitHub](https://github.com/erwincoumans/DaisyCloudSeed)
- **Granular Synthesis**: Barry Truax - *Granular Synthesis*
- **Microsound**: Curtis Roads - *Microsound*
- **Clouds Mutable Instruments**: Eurorack module inspiration
- **Daisy Documentation**: https://electro-smith.github.io/libDaisy/
