# Summary and Comparison: WAV Player vs Granular Synth

## Overview

This document provides a high-level summary and comparison of the WAV playback and granular synthesis implementations from the DaisyCloudSeed project. Both implementations share a common foundation but serve different musical purposes.

---

## Quick Reference

| Document | Description |
|----------|-------------|
| [01-wav-player-implementation.md](01-wav-player-implementation.md) | Detailed WAV file parsing and playback documentation |
| [02-granular-synth-implementation.md](02-granular-synth-implementation.md) | Detailed granular synthesis engine documentation |

---

## Shared Infrastructure

### Common Components

Both implementations rely on these shared components:

| Component | File | Purpose |
|-----------|------|---------|
| `b3DataSource` | [`b3ReadWavFile.h:13`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:13) | Abstract data source interface |
| `MemoryDataSource` | [`b3ReadWavFile.h:67`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:67) | Memory-based WAV data access |
| `b3ReadWavFile` | [`b3ReadWavFile.h:171`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:171) | WAV parsing and playback engine |
| `b3WavTicker` | [`b3ReadWavFile.h:146`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:146) | Playback state structure |
| `DaisySynthApp` | [`daisy_synth_app.h:7`](../../DaisyCloudSeed/patch/daisy_synth_app.h:7) | Base application interface |

### Memory Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     48MB SDRAM Pool                        │
│  (custom_pool in granular_synth_main.cpp/sample_player_main) │
├─────────────────────────────────────────────────────────────┤
│  WAV File 1  │  WAV File 2  │  ...  │  WAV File N        │
│  (Full file) │  (Full file)  │       │  (Full file)       │
└─────────────────────────────────────────────────────────────┘
```

**Key Point**: Entire WAV files are loaded into SDRAM at startup for zero-latency access during audio processing.

---

## Feature Comparison

### Core Characteristics

| Feature | SamplePlayer | GranularSynth |
|---------|--------------|---------------|
| **Primary Use Case** | Playback of full samples | Textural/time-stretched sounds |
| **Voice Architecture** | Polyphonic sampler | Grain cloud generator |
| **Max Voices** | 20 | 17 |
| **Trigger Method** | Manual (encoder/gate) | Automatic (interval-based) |
| **Envelope** | None | Triangular (fade in/out) |
| **Position Control** | Sequential (start to end) | Randomized around base |
| **Duration Control** | Full file length | Configurable grain size |
| **Pitch Control** | Global per voice | Per grain + randomization |

### Control Mapping

| Control | SamplePlayer | GranularSynth (Mode 0) | GranularSynth (Mode 1) |
|---------|--------------|-------------------------|-------------------------|
| CTRL0 | Playback Speed (-1 to 1) | Spawn Position (0-1) | Spawn Position (0-1) |
| CTRL1 | Volume (0-1) | Position Random Range | Position Random Range |
| CTRL2 | Volume (0-1) | Spawn Interval | Spawn Interval |
| CTRL3 | Volume (0-1) | Grain Duration | Grain Pitch |
| Encoder | File Select | File Select | Toggle Mode |
| Encoder Press | Play/Stop | Toggle Mode | Toggle Mode |

---

## Architecture Comparison

### SamplePlayer Flow

```
┌─────────────┐
│  User Input │ (Encoder press / Gate)
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────────────────┐
│  Toggle selected voice:                            │
│  - If playing: stop                               │
│  - If stopped: start from beginning                 │
└──────┬─────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────┐
│  AudioTickCallback (per buffer):                   │
│  - Process all active voices                       │
│  - Each voice plays full file at its speed         │
│  - Non-selected voices loop automatically          │
└──────────────────────────────────────────────────────┘
```

### GranularSynth Flow

```
┌──────────────────────────────────────────────────────┐
│  AudioTickCallback (per buffer):                   │
│  - Increment spawn timer                           │
│  - If timer > interval: SpawnGrain()              │
│  - Process all active grains                       │
│  - Each grain plays for its duration with envelope │
└──────┬─────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────┐
│  SpawnGrain():                                   │
│  - Find available grain slot                      │
│  - Calculate random spawn position                │
│  - Calculate random grain duration                │
│  - Calculate random grain pitch                  │
│  - Activate grain                                │
└──────────────────────────────────────────────────────┘
```

---

## Key Differences Explained

### 1. Voice Management

**SamplePlayer**:
- Each voice corresponds to one loaded WAV file
- Voice plays entire file from start to end
- Non-selected voices loop automatically
- Voices can be triggered independently

**GranularSynth**:
- All voices come from the same selected WAV file
- Each voice is a short "grain" (typically 10-200ms)
- Grains are spawned automatically at intervals
- All grains share the same source but have different parameters

### 2. Envelope Application

**SamplePlayer**:
```cpp
// No envelope - full amplitude
float volume = ctrlVal[1] / float(m_active_voices);
m_wavFileReaders[g].tick(&m_wavTickers[g], ..., volume, ...);
```

**GranularSynth**:
```cpp
// Triangular envelope per grain
double volume_gain = 1. / 10;
double volume = volume_gain * m_grains[g].env_volume2();
m_wavFileReaders[wavindex].tick(&m_grains[g], ..., volume, ...);
```

**Why?** Granular synthesis requires smooth fade-in/out to prevent clicks when grains overlap.

### 3. Position Control

**SamplePlayer**:
- Position advances sequentially: `ticker->time_ += ticker->rate_ * speed`
- Can reverse with negative speed
- Loops at end: `time_ = starttime_`

**GranularSynth**:
- Each grain starts at random position around base
- Position calculated at spawn time: `spawn_position + random_offset`
- Grains don't loop - they play to end and finish

### 4. Triggering

**SamplePlayer**:
```cpp
// Manual triggering only
if (m_patch.encoder.RisingEdge() || gate_input.Trig())
{
    // Toggle playback of selected file
}
```

**GranularSynth**:
```cpp
// Automatic spawning based on timer
m_timeSinceLastGrain += 0.001;
if (m_timeSinceLastGrain > spawn_interval)
{
    SpawnGrain();
    m_timeSinceLastGrain = 0;
}
```

---

## Use Case Recommendations

### Choose SamplePlayer When You Need:

1. **Traditional Sample Playback**: Drum hits, one-shots, loops
2. **Multiple Independent Sounds**: Playing different samples simultaneously
3. **Manual Triggering**: MIDI note triggering, button presses
4. **Pitch Shifting**: Changing playback speed without granular texture
5. **Simple Implementation**: Straightforward playback with minimal parameters

### Choose GranularSynth When You Need:

1. **Time Stretching**: Slowing down sounds without changing pitch
2. **Textural Pads**: Ambient, evolving soundscapes
3. **Sound Design**: Creating new sounds from existing samples
4. **Freeze Effects**: Stopping time on a particular sound
5. **Granular Effects**: Clouds, stutter, glitch effects

---

## Implementation Patterns

### Reusable Patterns

Both implementations demonstrate these useful patterns:

#### 1. Data/State Separation
```cpp
// Immutable data (shared)
b3ReadWavFile wavReader;  // Contains WAV data

// Mutable state (per voice)
b3WavTicker ticker;       // Contains playback position, speed, etc.
```

**Benefit**: Multiple voices from same sample with minimal memory overhead.

#### 2. Abstract Data Source
```cpp
struct b3DataSource {
    virtual long ftell() = 0;
    virtual size_t fread(void* buffer, ...) = 0;
    virtual int fseek(long offset, int origin) = 0;
};
```

**Benefit**: Same parsing code works with files or memory buffers.

#### 3. Linear Interpolation
```cpp
int iIndex = (int)ticker->time_;
double alpha = ticker->time_ - (double)iIndex;
double sample = buf[iIndex] + alpha * (buf[iIndex+1] - buf[iIndex]);
```

**Benefit**: Smooth pitch shifting with reasonable CPU cost.

#### 4. Pool Allocation
```cpp
#define CUSTOM_POOL_SIZE (48*1024*1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];

void* custom_pool_allocate(size_t size) {
    void* ptr = &custom_pool[pool_index];
    pool_index += size;
    return ptr;
}
```

**Benefit**: Deterministic memory behavior, no fragmentation.

---

## Performance Comparison

### CPU Usage

| Operation | SamplePlayer | GranularSynth |
|-----------|--------------|---------------|
| Per voice interpolation | Same | Same |
| Envelope calculation | None | 1 multiply-add |
| Spawn logic | None | ~10-20 operations |
| Total per sample | ~10-15 cycles | ~20-30 cycles |

### Memory Usage

| Component | SamplePlayer | GranularSynth |
|-----------|--------------|---------------|
| WAV data | Same (file size) | Same (file size) |
| Per voice | ~64 bytes | ~64 bytes |
| Total (max voices) | ~1.2KB | ~1.1KB |
| SDRAM pool | 48MB | 48MB |

### Latency

| Aspect | SamplePlayer | GranularSynth |
|--------|--------------|---------------|
| File loading | Startup | Startup |
| Playback start | Immediate | Immediate |
| Grain spawn | N/A | ~1ms (next buffer) |

---

## Code Reuse Summary

### Shared Code (~60%)

Both implementations share:
- WAV file parsing (`getWavInfo`)
- Audio interpolation (`interpolate`, `tick`)
- Data source abstraction (`b3DataSource`)
- Memory management (`custom_pool_allocate`)
- File loading from SD card
- Daisy hardware initialization

### Unique Code (~40%)

**SamplePlayer Unique**:
- Manual triggering logic
- Voice selection UI
- Loop behavior for non-selected voices
- Speed-based pitch control

**GranularSynth Unique**:
- Grain spawning logic
- Randomization algorithms
- Triangular envelope
- Mode switching (duration/pitch)
- Spawn interval timing

---

## Integration Guide

### Creating a Hybrid Implementation

You can combine features from both:

```cpp
// Hybrid: Manual grain triggering
if (gate_input.Trig()) {
    SpawnGrain();  // Spawn single grain on trigger
}

// Hybrid: Granular time stretching with manual position
grain_spawn_position = ctrlVal[0];  // Manual position control
grain_spawn_position_rand_range = 0;  // No randomization
```

### Adding Features to SamplePlayer

From GranularSynth:
```cpp
// Add envelope to SamplePlayer
double volume = ctrlVal[1] * m_wavTickers[g].env_volume2();
```

### Adding Features to GranularSynth

From SamplePlayer:
```cpp
// Add manual triggering to GranularSynth
if (m_patch.encoder.RisingEdge()) {
    SpawnGrain();  // Spawn grain on encoder press
}
```

---

## Building Your Own Implementation

### Step 1: Choose Base

- Start with **SamplePlayer** for traditional playback
- Start with **GranularSynth** for textural sounds

### Step 2: Customize Parameters

Modify the control mappings to suit your needs:
```cpp
// Example: MIDI velocity controls grain duration
grain_duration = midi_velocity * max_grain_duration;
```

### Step 3: Add Effects

Process output before sending to DAC:
```cpp
// Add reverb to output
for (size_t i = 0; i < size; i++) {
    float wetL, wetR;
    reverb.Process(out[0][i], out[1][i], &wetL, &wetR);
    out[0][i] = out[0][i] * 0.7 + wetL * 0.3;
    out[1][i] = out[1][i] * 0.7 + wetR * 0.3;
}
```

### Step 4: Optimize

Profile and optimize based on your needs:
- Reduce voice count if CPU limited
- Simplify interpolation if quality not critical
- Use fixed-point math if floating point too slow

---

## File Structure

```
DaisyCloudSeed/patch/
├── daisy_synth_app.h          # Base application interface
├── SamplePlayer/
│   ├── sample_player_main.cpp   # Entry point
│   ├── sample_player_app.h      # Application header
│   ├── sample_player_app.cpp    # Application implementation
│   ├── b3ReadWavFile.h        # WAV parsing header
│   ├── b3ReadWavFile.cpp      # WAV parsing implementation
│   └── b3SwapUtils.h         # Byte swapping utilities
└── Granular/
    ├── granular_synth_main.cpp  # Entry point
    ├── granular_synth_app.h     # Application header
    └── granular_synth_app.cpp  # Application implementation
```

---

## Key Takeaways

1. **Shared Foundation**: Both implementations share ~60% of code through the `b3ReadWavFile` class
2. **Different Philosophies**: SamplePlayer for traditional playback, GranularSynth for textural manipulation
3. **Memory-Based Design**: Entire files loaded to SDRAM for zero-latency access
4. **Voice Limiting**: Both limit voices to prevent CPU overload (20 for SamplePlayer, 17 for GranularSynth)
5. **Linear Interpolation**: Both use linear interpolation for pitch shifting
6. **Pool Allocation**: Custom SDRAM pool provides deterministic memory behavior

---

## Further Reading

### Daisy Resources
- [libDaisy Documentation](https://electro-smith.github.io/libDaisy/)
- [DaisySP Documentation](https://electro-smith.github.io/DaisySP/)
- [Daisy Examples](https://github.com/electro-smith/DaisyExamples)

### Granular Synthesis
- *Microsound* by Curtis Roads
- *Granular Synthesis* by Barry Truax
- Mutable Instruments Clouds (open source Eurorack module)

### Audio Programming
- *The Audio Programming Book* by Boulanger and Lazzarini
- *Designing Audio Effect Plugins in C++* by Will Pirkle

---

## Repository Links

- **DaisyCloudSeed**: https://github.com/erwincoumans/DaisyCloudSeed
- **libDaisy**: https://github.com/electro-smith/libDaisy
- **DaisySP**: https://github.com/electro-smith/DaisySP
