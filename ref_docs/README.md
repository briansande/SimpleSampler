# DaisyCloudSeed Reference Documentation

This folder contains comprehensive documentation for the WAV player and granular synthesis implementations from the [DaisyCloudSeed](https://github.com/erwincoumans/DaisyCloudSeed) project.

## Documentation Files

| File | Description |
|------|-------------|
| [`01-wav-player-implementation.md`](01-wav-player-implementation.md) | Detailed analysis of WAV file parsing, audio interpolation, and sample playback implementation |
| [`02-granular-synth-implementation.md`](02-granular-synth-implementation.md) | Detailed analysis of granular synthesis engine, grain spawning, and envelope generation |
| [`03-summary-and-comparison.md`](03-summary-and-comparison.md) | High-level comparison of both implementations, shared infrastructure, and integration guide |

## Quick Start

### For WAV Player Implementation

1. Read [`01-wav-player-implementation.md`](01-wav-player-implementation.md) for:
   - WAV file parsing details
   - Linear interpolation algorithm
   - Memory-based playback architecture
   - Control mappings and parameter ranges

2. Key files to reference:
   - [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:1) - Core WAV reading interface
   - [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:1) - Parsing and playback implementation
   - [`sample_player_app.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.cpp:1) - Application logic

### For Granular Synth Implementation

1. Read [`02-granular-synth-implementation.md`](02-granular-synth-implementation.md) for:
   - Grain spawning algorithms
   - Randomization strategies
   - Envelope generation
   - Granular synthesis techniques

2. Key files to reference:
   - [`granular_synth_app.cpp`](../../DaisyCloudSeed/patch/Granular/granular_synth_app.cpp:1) - Granular engine
   - `SpawnGrain()` method - Grain creation logic
   - `env_volume2()` - Triangular envelope

### For Comparison and Integration

1. Read [`03-summary-and-comparison.md`](03-summary-and-comparison.md) for:
   - Feature comparison table
   - Architecture comparison
   - Shared infrastructure
   - Integration guide for hybrid implementations

## Implementation Overview

### Shared Components

Both implementations share these core components:

```
┌─────────────────────────────────────────────────────────────┐
│                    b3ReadWavFile                          │
│  (WAV parsing, interpolation, playback engine)              │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                   b3DataSource                             │
│  (Abstract data source for file/memory access)             │
└─────────────────────────────────────────────────────────────┘
```

### Memory Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     48MB SDRAM Pool                        │
│  (All WAV files loaded at startup)                         │
├─────────────────────────────────────────────────────────────┤
│  WAV File 1  │  WAV File 2  │  ...  │  WAV File N        │
└─────────────────────────────────────────────────────────────┘
```

## Key Concepts

### WAV File Parsing

The [`b3ReadWavFile`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:171) class handles:
- RIFF/WAVE header validation
- Format chunk parsing
- Data type detection (SINT8, SINT16, SINT24, SINT32, FLOAT32, FLOAT64)
- Sample rate extraction
- Data offset calculation

### Linear Interpolation

All playback uses linear interpolation:
```cpp
int iIndex = (int)ticker->time_;
double alpha = ticker->time_ - (double)iIndex;
double sample = buf[iIndex] + alpha * (buf[iIndex+1] - buf[iIndex]);
```

### Playback State Separation

The [`b3WavTicker`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:146) structure separates playback state from WAV data:
- Enables multiple simultaneous voices from same sample
- Each voice has independent position, speed, and envelope
- Minimal memory overhead per voice

## Use Cases

### SamplePlayer

- **Traditional sample playback**: Drum hits, one-shots, loops
- **Polyphonic sampler**: Multiple independent sounds
- **Pitch shifting**: Variable speed playback
- **Manual triggering**: Gate/MIDI note triggering

### GranularSynth

- **Time stretching**: Slow playback without pitch change
- **Textural pads**: Ambient, evolving soundscapes
- **Sound design**: Creating new sounds from samples
- **Freeze effects**: Stopping time on a sound

## Control Mappings

### SamplePlayer

| Control | Function | Range |
|----------|-----------|-------|
| CTRL0 | Playback Speed | -1.0 to 1.0 (reverse to forward) |
| CTRL1 | Volume | 0.0 to 1.0 |
| Encoder | File Select | - |
| Encoder Press | Play/Stop | - |

### GranularSynth

| Control | Mode 0 | Mode 1 | Range |
|----------|--------|--------|-------|
| CTRL0 | Spawn Position | Spawn Position | 0.0 to 1.0 |
| CTRL1 | Position Random | Position Random | 0.0 to 1.0 |
| CTRL2 | Spawn Interval | Spawn Interval | 0.0 to 1.0 |
| CTRL3 | Grain Duration | Grain Pitch | 0.0 to 1.0 |
| Encoder Press | Toggle Mode | Toggle Mode | - |

## Performance Considerations

### CPU Usage

- **Per voice**: ~10-30 cycles per sample
- **Max voices**: 20 (SamplePlayer), 17 (GranularSynth)
- **Bottleneck**: Interpolation + envelope calculation

### Memory Usage

- **Per WAV file**: File size bytes in SDRAM
- **Per voice**: ~64 bytes
- **Total pool**: 48MB SDRAM

### Latency

- **File loading**: One-time at startup
- **Playback**: Minimal (memory-based access)
- **No SD access**: During audio processing

## Building Your Own Implementation

### Step 1: Choose Base

- **SamplePlayer**: For traditional playback
- **GranularSynth**: For textural sounds

### Step 2: Customize

Modify control mappings, voice limits, and parameters to suit your needs.

### Step 3: Add Features

- Effects (reverb, delay, filter)
- Different envelopes
- Alternative interpolation methods
- Streaming from SD card

### Step 4: Optimize

Profile and adjust based on:
- Available CPU cycles
- Memory constraints
- Desired quality

## Source Code Location

The source code is located at:
```
../../DaisyCloudSeed/patch/
├── SamplePlayer/
│   ├── sample_player_main.cpp
│   ├── sample_player_app.h
│   ├── sample_player_app.cpp
│   ├── b3ReadWavFile.h
│   ├── b3ReadWavFile.cpp
│   └── b3SwapUtils.h
└── Granular/
    ├── granular_synth_main.cpp
    ├── granular_synth_app.h
    └── granular_synth_app.cpp
```

## References

- **DaisyCloudSeed**: https://github.com/erwincoumans/DaisyCloudSeed
- **libDaisy**: https://github.com/electro-smith/libDaisy
- **DaisySP**: https://github.com/electro-smith/DaisySP
- **Daisy Documentation**: https://electro-smith.github.io/libDaisy/

## License

The DaisyCloudSeed project is open source. Refer to the original repository for licensing information.
