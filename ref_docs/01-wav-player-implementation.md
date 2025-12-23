# WAV Player Implementation Documentation

## Overview

This document provides an in-depth analysis of the WAV file playback implementation from the DaisyCloudSeed project. The implementation is located in the `SamplePlayer` directory and consists of several key components:

- [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:1) - Header file defining the WAV reading interface
- [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:1) - Implementation of WAV file parsing and playback
- [`sample_player_app.h`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.h:1) - Application-level header
- [`sample_player_app.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.cpp:1) - Application-level implementation
- [`sample_player_main.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_main.cpp:1) - Main entry point

---

## Architecture

### Component Hierarchy

```
┌─────────────────────────────────────────────────────────────┐
│                     sample_player_main.cpp                   │
│  (Entry point, DaisyPatch initialization, audio callback)    │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                    SamplePlayerApp                         │
│  (Application logic, file loading, UI, playback control)   │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                    b3ReadWavFile                          │
│  (WAV parsing, audio data interpolation, playback engine)   │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                   b3DataSource                             │
│  (Abstract data source interface for file/memory access)     │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Data Structures

### b3DataSource (Abstract Base)

Located in [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:13), this abstract class provides a unified interface for reading WAV data from different sources:

```cpp
struct b3DataSource
{
    virtual long  ftell() = 0;
    virtual size_t fread(void* _Buffer, size_t _ElementSize, size_t _ElementCount) = 0;
    virtual int fseek(long _Offset, int _Origin) = 0;
};
```

**Purpose**: Allows the same WAV parsing code to work with both file-based and memory-based data sources.

#### MemoryDataSource

Located in [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:67), this implementation reads WAV data from a memory buffer:

```cpp
struct MemoryDataSource : public b3DataSource
{
    const char* m_data;        // Pointer to raw WAV data
    int m_numBytes;            // Total size of data
    int m_currentAddress;       // Current read position
};
```

**Key Features**:
- Stores entire WAV file in RAM (SDRAM on Daisy)
- Provides fast random access to audio data
- Critical for real-time granular synthesis

**Why Memory-Based?** The Daisy platform loads entire WAV files into SDRAM (48MB pool) to avoid SD card latency during audio processing.

---

### b3WavTicker (Playback State)

Located in [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:146), this structure maintains the playback state for a single voice:

```cpp
struct b3WavTicker
{
    std::vector<double> lastFrame_;   // Last audio frame (for interpolation)
    bool finished_;                   // Playback completion flag
    double time_;                     // Current playback position (in frames)
    double starttime_;                // Loop/start point (in frames)
    double endtime_;                 // End point (in frames)
    double rate_;                     // Playback rate (file_sr / playback_sr)
    double speed_;                   // Speed multiplier (1.0 = normal)
    int wavindex;                    // Index of WAV file being played
    
    // Envelope functions
    double env_volume();              // Linear decay envelope
    double env_volume2();            // Triangular envelope (for granular)
};
```

**Key Design Pattern**: Separating playback state (`b3WavTicker`) from WAV data (`b3ReadWavFile`) enables multiple simultaneous voices from the same sample.

---

### b3ReadWavFile (WAV Parser and Playback Engine)

Located in [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:171), this class handles WAV file parsing and audio playback:

```cpp
class b3ReadWavFile
{
    bool byteswap_;                  // Endianness correction flag
    bool wavFile_;                  // Valid WAV file flag
    unsigned long m_numFrames;       // Total number of frames
    unsigned long dataType_;          // Audio format (SINT16, FLOAT32, etc.)
    double fileDataRate_;            // File sample rate
    unsigned long dataOffset_;        // Offset to audio data in file
    unsigned int channels_;          // Number of channels (1=mono, 2=stereo)
    bool m_machineIsLittleEndian;    // Platform endianness
};
```

---

## WAV File Parsing

### getWavInfo() Method

Located in [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:241), this method parses the WAV file header:

```cpp
bool b3ReadWavFile::getWavInfo(b3DataSource& dataSource)
```

**Parsing Steps**:

1. **Validate RIFF/WAVE Header** (lines 249-251):
   ```cpp
   if (!strncmp(header, "RIFF", 4) &&
       !strncmp(&header[8], "WAVE", 4))
       res = true;
   ```

2. **Locate "fmt " Chunk** (lines 259-271):
   - Skips non-format chunks until "fmt " is found
   - Handles chunk skipping with proper seek operations

3. **Parse Format Information** (lines 274-313):
   - Reads format tag (1 = PCM, 3 = FLOAT, 0xFFFE = EXTENSIBLE)
   - Supports WAVE_FORMAT_EXTENSIBLE format
   - Validates supported formats

4. **Extract Channel Count** (lines 316-323):
   ```cpp
   channels_ = (unsigned int)temp;
   ```

5. **Extract Sample Rate** (lines 326-333):
   ```cpp
   fileDataRate_ = (double)srate;
   ```

6. **Determine Data Type** (lines 336-367):
   - Reads bits per sample
   - Maps to internal data type constants:
     - `B3_SINT8` (8-bit signed)
     - `B3_SINT16` (16-bit signed)
     - `B3_SINT24` (24-bit signed)
     - `B3_SINT32` (32-bit signed)
     - `B3_FLOAT32` (32-bit float)
     - `B3_FLOAT64` (64-bit float)

7. **Locate "data" Chunk** (lines 377-390):
   - Skips any chunks between "fmt " and "data"

8. **Calculate Frame Count** (lines 393-401):
   ```cpp
   m_numFrames = bytes / temp / channels_;  // sample frames
   m_numFrames *= 8;                        // sample frames
   ```

9. **Store Data Offset** (line 403):
   ```cpp
   dataOffset_ = dataSource.ftell();
   ```

---

## Audio Playback

### createWavTicker() Method

Located in [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:228), creates a new playback voice:

```cpp
b3WavTicker b3ReadWavFile::createWavTicker(double sampleRate)
{
    b3WavTicker ticker;
    ticker.lastFrame_.resize(this->channels_);
    ticker.time_ = 0;
    ticker.starttime_ = 0.;
    ticker.endtime_ = (double)(this->m_numFrames - 1.0);
    ticker.finished_ = false;
    ticker.rate_ = fileDataRate_ / sampleRate;  // Rate conversion ratio
    ticker.speed_ = 1.;
    return ticker;
}
```

**Key Points**:
- Calculates rate conversion ratio for sample rate differences
- Sets up full-file playback by default
- Initializes last frame buffer for interpolation

---

### tick() Method

Located in [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:201), processes one audio buffer:

```cpp
void b3ReadWavFile::tick(b3WavTicker *ticker, b3DataSource& dataSource, 
                          double speed, double volume, int size, 
                          float* out0, float* out1)
```

**Flow**:

1. **Check if finished** (lines 203-204):
   ```cpp
   if (ticker->finished_) 
       return;
   ```

2. **Check bounds** (lines 205-209):
   ```cpp
   if (ticker->time_ < ticker->starttime_ || ticker->time_ > ticker->endtime_)
   {
       ticker->finished_ = true;
       return;
   }
   ```

3. **Process each sample** (lines 211-220):
   - Calls `interpolate()` for each output sample
   - Advances playback position: `ticker->time_ += ticker->rate_ * speed`
   - Checks for completion after each sample

---

### interpolate() Method

Located in [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:48), performs linear interpolation:

```cpp
void b3ReadWavFile::interpolate(b3WavTicker *ticker, b3DataSource& dataSource, 
                                double speed, double volume, int size, 
                                float* out0, float* out1, int oIndex) const
```

**Algorithm**:

1. **Extract integer and fractional parts** (lines 50-51):
   ```cpp
   int iIndex = (int)ticker->time_;                        // Integer part
   double alpha = ticker->time_ - (double)iIndex;          // Fractional part
   ```

2. **Seek to data position** (varies by format):
   - For 16-bit: `dataSource.fseek(dataOffset_ + (iIndex * 2), B3_SEEK_SET)`
   - For 32-bit float: `dataSource.fseek(dataOffset_ + (iIndex * 4), B3_SEEK_SET)`

3. **Read two consecutive samples** (varies by format):
   ```cpp
   signed short int buf[4];
   dataSource.fread(buf, 2*channels_ * 2, 1);
   ```

4. **Convert to float and apply gain** (example for SINT16):
   ```cpp
   double gain = 1.0 / 32768.0;
   double tmp0 = buf[0]*gain;
   double tmp1 = buf[1]*gain;
   ```

5. **Apply linear interpolation** (lines 170-172):
   ```cpp
   if (alpha > 0.0)
   {
       tmp0 += (alpha * (buf[channels_]*gain - tmp0));
       tmp1 += (alpha * (buf[channels_+1]*gain - tmp1));
   }
   ```

6. **Output to audio buffer** (lines 174-175):
   ```cpp
   out0[oIndex] += tmp0 * volume;
   out1[oIndex] += (channels_ > 1) ? tmp1 * volume : tmp0 * volume;
   ```

**Supported Formats**:

| Format | Lines | Gain Factor | Notes |
|--------|-------|-------------|-------|
| SINT8 | 73-91 | 1.0/128.0 | Unsigned, offset by 128 |
| SINT16 | 158-176 | 1.0/32768.0 | Most common |
| SINT24 | 93-135 | 1.0/2147483648.0 | Packed 24-bit in 32-bit container |
| SINT32 | 137-156 | 1.0/2147483648.0 | 32-bit signed |
| FLOAT32 | 55-71 | 1.0 | Direct float values |
| FLOAT64 | 179-196 | 1.0 | Double precision |

---

## Application-Level Implementation

### SamplePlayerApp Class

Located in [`sample_player_app.h`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.h:16), the main application class:

```cpp
class SamplePlayerApp : public DaisySynthApp
{
    daisy::DaisyPatch& m_patch;
    daisy::SdmmcHandler& m_sd_handler;
    std::string m_title;
    float m_ctrlVal[4];
    float m_prevCtrlVal[4];
    b3ReadWavFile m_wavFileReaders[kMaxFiles];  // WAV parsers
    b3WavTicker m_wavTickers[kMaxFiles];        // Playback voices
    daisy::WavFileInfo m_file_info_[kMaxFiles];
    int m_file_sizes[kMaxFiles];
    MemoryDataSource m_dataSources[kMaxFiles];
    int m_selected_file_index;
    int m_risingEdge;
    int m_active_voices;
    int m_file_cnt_;
    bool m_gateOut;
    float m_playback_speed;
    std::string m_sd_debug_msg;
};
```

---

### File Loading (loadWavFiles)

Located in [`sample_player_app.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.cpp:76), loads WAV files from SD card:

```cpp
void SamplePlayerApp::loadWavFiles()
```

**Process**:

1. **Mount SD Card** (lines 88-90):
   ```cpp
   FATFS& fs = fsi.GetSDFileSystem();
   f_mount(&fs, "/", 1);
   ```

2. **Open Root Directory** (lines 93):
   ```cpp
   if(f_opendir(&dir, "/") == FR_OK)
   ```

3. **Scan for WAV Files** (lines 96-151):
   - Uses `f_readdir()` to iterate through files
   - Skips hidden files and directories
   - Checks for `.wav` or `.WAV` extension (lines 109)

4. **Load Each WAV File** (lines 115-143):
   ```cpp
   if(f_open(&SDFile, m_file_info_[m_file_cnt_].name, (FA_OPEN_EXISTING | FA_READ)) == FR_OK)
   {
       m_file_sizes[m_file_cnt_] = f_size(&SDFile);
       memoryBuffer = (char*) custom_pool_allocate(size);
       if (memoryBuffer)
       {
           f_read(&SDFile, (void *)memoryBuffer, size, &bytesread);
           m_dataSources[m_file_cnt_] = MemoryDataSource(memoryBuffer, memorySize);
           m_wavFileReaders[m_file_cnt_].getWavInfo(m_dataSources[m_file_cnt_]);
           m_wavFileReaders[m_file_cnt_].resize();
           m_wavTickers[m_file_cnt_] = m_wavFileReaders[m_file_cnt_].createWavTicker(samplerate);
           m_wavTickers[m_file_cnt_].finished_ = true;  // Start inactive
       }
   }
   ```

**Memory Allocation**: Uses a custom 48MB SDRAM pool defined in [`sample_player_main.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_main.cpp:38):
```cpp
#define CUSTOM_POOL_SIZE (48*1024*1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
```

---

### Audio Processing (AudioTickCallback)

Located in [`sample_player_app.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.cpp:159), processes audio in real-time:

```cpp
void SamplePlayerApp::AudioTickCallback(float ctrlVal[4], const float *const*in, 
                                      float **out, size_t size)
```

**Key Operations**:

1. **Handle Encoder/Gate Input** (lines 171-174):
   ```cpp
   if (m_patch.encoder.RisingEdge() || m_patch.gate_input[DaisyPatch::GateInput::GATE_IN_1].Trig())
   {
       m_risingEdge = true;
   }
   ```

2. **File Selection** (lines 179-195):
   - Encoder increment/decrement changes selected file
   - Bounds checking prevents out-of-range access

3. **Clear Output Buffers** (lines 217-223):
   ```cpp
   for (size_t i = 0; i < size; i++)
   {
       out[0][i] = 0.;
       out[1][i] = 0.;
       out[2][i] = 0.;
       out[3][i] = 0.;
   }
   ```

4. **Count Active Voices** (lines 224-234):
   ```cpp
   for (int g=0;g<m_file_cnt_;g++)
   {
       if (!m_wavTickers[g].finished_ && m_active_voices < 20)
       {
           m_active_voices++;
       }
   }
   ```

5. **Process Each Active Voice** (lines 236-262):
   ```cpp
   for (int g=0;g<m_file_cnt_;g++)
   {
       if (!m_wavTickers[g].finished_ && m_active_voices < 20)
       {
           float volume = ctrlVal[1]/float(m_active_voices);
           m_wavFileReaders[g].tick(&m_wavTickers[g], m_dataSources[g], 
                                    m_playback_speed, volume, size, 
                                    out[0], out[1]);
           
           // Handle voice completion
           if (m_wavTickers[g].finished_)
           {
               if (g == m_selected_file_index)
               {
                   m_gateOut = true;  // Trigger gate output
               } else
               {
                   // Loop non-selected voices
                   if (m_wavTickers[g].time_<0)
                       m_wavTickers[g].time_ = m_wavTickers[g].endtime_;
                   else
                       m_wavTickers[g].time_ = m_wavTickers[g].starttime_;
                   m_wavTickers[g].finished_ = false;
               }
           }
       }
   }
   ```

**Voice Limiting**: Maximum 20 active voices to prevent CPU overload.

**Volume Scaling**: Volume is divided by active voice count to prevent clipping.

---

### Playback Control (MainLoopCallback)

Located in [`sample_player_app.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/sample_player_app.cpp:327), handles user input:

```cpp
void SamplePlayerApp::MainLoopCallback()
```

**Encoder Press Behavior** (lines 329-344):
```cpp
if (m_risingEdge)
{
    if (!m_wavTickers[m_selected_file_index].finished_)
    {
        m_wavTickers[m_selected_file_index].finished_ = true;  // Stop
    } else
    {
        // Start from beginning
        if (m_wavTickers[m_selected_file_index].time_<0)
            m_wavTickers[m_selected_file_index].time_ = m_wavTickers[m_selected_file_index].endtime_;
        else
            m_wavTickers[m_selected_file_index].time_ = m_wavTickers[m_selected_file_index].starttime_;
        m_wavTickers[m_selected_file_index].finished_ = false;
    }
    m_risingEdge = false;
}
```

---

## Control Mapping

| Control | Parameter | Range | Description |
|---------|-----------|-------|-------------|
| CTRL0 | Playback Speed | -1.0 to 1.0 | Negative = reverse, 1.0 = normal |
| CTRL1 | Volume | 0.0 to 1.0 | Master volume |
| Encoder | File Selection | - | Browse through loaded WAV files |
| Encoder Press | Play/Stop | - | Toggle playback of selected file |
| GATE IN 1 | Trigger | - | Same as encoder press |
| GATE OUT | End Trigger | - | Pulses when selected file ends |

---

## Key Design Patterns

### 1. Separation of Data and State

The `b3ReadWavFile` class contains immutable WAV data, while `b3WavTicker` contains mutable playback state. This allows:

- Multiple simultaneous voices from the same sample
- Efficient memory usage (single copy of audio data)
- Independent control of each voice (position, speed, envelope)

### 2. Abstract Data Source

The `b3DataSource` interface enables the same parsing code to work with:
- File-based access (for desktop testing)
- Memory-based access (for embedded real-time performance)

### 3. Linear Interpolation

All playback uses linear interpolation between samples:
- Prevents aliasing at slow speeds
- Provides smooth pitch shifting
- Relatively low CPU cost

### 4. Pool Allocation

Custom SDRAM pool allocation:
- Avoids heap fragmentation
- Provides deterministic memory behavior
- Enables loading of large sample libraries

---

## Performance Considerations

### Memory Usage

- **Per WAV File**: File size bytes in SDRAM
- **Per Voice**: Minimal (just `b3WavTicker` structure)
- **Total Pool**: 48MB SDRAM

### CPU Usage

- **Per Sample**: Interpolation + memory access
- **Per Voice**: Linear with active voice count
- **Max Voices**: 20 (hard limit)

### Latency

- **File Loading**: One-time at startup
- **Playback**: Minimal (memory-based access)
- **No SD Card Access**: During audio processing

---

## Limitations

1. **Fixed 48MB Pool**: All samples must fit in SDRAM
2. **Max 64 Files**: Hard limit on file count
3. **Max 20 Voices**: CPU limitation
4. **Linear Interpolation Only**: No higher-quality resampling
5. **No Streaming**: Entire files loaded at startup

---

## Integration Notes

### For Your Own Implementation

1. **Adapt Data Source**: If you want streaming, implement a new `b3DataSource` subclass that reads from SD card on demand.

2. **Modify Voice Limit**: Adjust the `20` voice limit based on your CPU budget.

3. **Custom Envelopes**: The `env_volume()` and `env_volume2()` methods can be modified for different envelope shapes.

4. **Add Effects**: The output buffers can be processed with effects before output.

5. **MIDI Integration**: Replace encoder/gate logic with MIDI note triggering.

---

## References

- **Original Code**: [DaisyCloudSeed on GitHub](https://github.com/erwincoumans/DaisyCloudSeed)
- **STK Toolkit**: The WAV parsing code is based on the STK (Synthesis ToolKit) library
- **Daisy Documentation**: https://electro-smith.github.io/libDaisy/
