# Task List: Integrate DaisyCloudSeed WAV Player into SimpleSampler

This is a detailed task list for integrating DaisyCloudSeed WAV player implementation (polyphonic playback with granular synthesis foundation) into your [`SimpleSampler.cpp`](../SimpleSampler.cpp:1).

---

## Overview

**Goal:** Replace single-voice Daisy `WavPlayer` with polyphonic `b3ReadWavFile` system

**Source:** DaisyCloudSeed/patch/SamplePlayer/  
**Target:** SimpleSampler (DaisyPod-based)

---

## Phase 1: Copy Core Audio Files

### Task 1.1: Copy b3ReadWav Files
- [ ] Copy [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:1) to project root
- [ ] Copy [`b3ReadWavFile.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.cpp:1) to project root
- [ ] Copy [`b3SwapUtils.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3SwapUtils.h:1) to project root

**Verification:** All three files compile without errors

### Task 1.2: Add Files to Makefile
- [ ] Add `b3ReadWavFile.o` to `OBJS` list in Makefile
- [ ] Verify Makefile includes all required Daisy libraries

**Reference:** See current [`Makefile`](../Makefile:1) for pattern

---

## Phase 2: Add SDRAM Pool

### Task 2.1: Add SDRAM Pool Variables
- [ ] Add `#include "daisy_core.h"` at top of [`SimpleSampler.cpp`](../SimpleSampler.cpp:1)
- [ ] Add global pool definition:
```cpp
#define CUSTOM_POOL_SIZE (48*1024*1024)  // 48MB
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;
```

**Location:** After existing includes (around line 11)

### Task 2.2: Implement Pool Allocator
- [ ] Add custom pool allocation function:
```cpp
void* custom_pool_allocate(size_t size) {
    if (pool_index + size >= CUSTOM_POOL_SIZE) return 0;
    void* ptr = &custom_pool[pool_index];
    pool_index += size;
    return ptr;
}
```

**Location:** Before `main()` function (around line 267)

---

## Phase 3: Replace WavPlayer with Polyphonic System

### Task 3.1: Remove Old WavPlayer
- [ ] Remove or comment out: `#include "daisy_seed.h"` (line 3)
- [ ] Remove or comment out: `WavPlayer sampler;` (line 38)

### Task 3.2: Add Polyphonic Audio Variables
- [ ] Add to global variables (around line 23):
```cpp
// Polyphonic WAV playback system
const int kMaxFiles = 64;
b3ReadWavFile m_wavReaders[kMaxFiles];
b3WavTicker m_tickers[kMaxFiles];
daisy::WavFileInfo m_fileInfo[kMaxFiles];
int m_fileSizes[kMaxFiles];
MemoryDataSource m_dataSources[kMaxFiles];
int m_loadedCount = 0;      // Number of loaded WAV files
int m_activeVoices = 0;      // Number of playing voices
```

### Task 3.3: Add Playback Control Variables
- [ ] Add knob control variables:
```cpp
float m_playbackSpeed = 1.0f;
float m_masterVolume = 0.7f;
bool m_isPlaying = false;
```

---

## Phase 4: Implement WAV File Loading

### Task 4.1: Add WAV Loading Function
- [ ] Implement `LoadWavFile(const char* filename, int index)`:
```cpp
int LoadWavFile(const char* filename, int index) {
    FIL file;
    if (f_open(&file, filename, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        return -1;  // Failed to open
    }
    
    m_fileSizes[index] = f_size(&file);
    char* buffer = (char*) custom_pool_allocate(m_fileSizes[index]);
    
    if (buffer) {
        UINT bytesRead;
        if (f_read(&file, buffer, m_fileSizes[index], &bytesRead) == FR_OK) {
            m_dataSources[index] = MemoryDataSource(buffer, m_fileSizes[index]);
            if (m_wavReaders[index].getWavInfo(m_dataSources[index])) {
                m_tickers[index] = m_wavReaders[index].createWavTicker(48000.0f);
                m_tickers[index].finished_ = true;  // Start inactive
                f_close(&file);
                return 0;  // Success
            }
        }
    }
    f_close(&file);
    return -2;  // Failed to parse
}
```

**Location:** Before `main()` function

### Task 4.2: Modify CacheFileList to Load WAVs
- [ ] In `CacheFileList()`, after detecting WAV file:
```cpp
if (strstr(fno.fname, ".wav") || strstr(fno.fname, ".WAV")) {
    // Load WAV file into memory
    if (m_loadedCount < kMaxFiles) {
        char fullPath[256];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, fno.fname);
        if (LoadWavFile(fullPath, m_loadedCount) >= 0) {
            strncpy(m_fileInfo[m_loadedCount].name, fno.fname, 31);
            m_fileInfo[m_loadedCount].name[31] = '\0';
            m_loadedCount++;
        }
    }
}
```

- [ ] Update `totalFiles` to only count directories (WAVs loaded separately)

### Task 4.3: Add WAV Index Helper
- [ ] Implement `FindWavIndex(const char* filename)`:
```cpp
int FindWavIndex(const char* filename) {
    for (int i = 0; i < m_loadedCount; i++) {
        if (strcmp(filename, m_fileInfo[i].name) == 0) {
            return i;  // Found
        }
    }
    return -1;  // Not found
}
```

---

## Phase 5: Implement Polyphonic Audio Callback

### Task 5.1: Replace AudioCallback
- [ ] Replace entire `AudioCallback()` function:
```cpp
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                    AudioHandle::InterleavingOutputBuffer out,
                    size_t size)
{
    hw.ProcessDigitalControls();
    
    // Process knobs
    m_playbackSpeed = (p_knob1.Process() - 0.5f) * 2.0f;  // -1.0 to 1.0
    m_masterVolume = p_knob2.Process();
    
    // Clear output buffers
    for (size_t i = 0; i < size; i += 2) {
        out[i] = out[i+1] = 0;
    }
    
    // Process all active voices
    m_activeVoices = 0;
    for (int i = 0; i < m_loadedCount; i++) {
        if (!m_tickers[i].finished_ && m_activeVoices < 20) {
            m_activeVoices++;
            float voiceVolume = m_masterVolume / (float)m_activeVoices;
            
            if (!m_tickers[i].finished_) {
                m_wavReaders[i].tick(&m_tickers[i], m_dataSources[i], 
                                        m_playbackSpeed, voiceVolume, 
                                        size, &out[0], &out[1]);
            }
        }
    }
    
    // Apply safety clip
    for (size_t i = 0; i < size; i += 2) {
        if (out[i] > 1.0f) out[i] = 1.0f;
        if (out[i] < -1.0f) out[i] = -1.0f;
        if (out[i+1] > 1.0f) out[i+1] = 1.0f;
        if (out[i+1] < -1.0f) out[i+1] = -1.0f;
    }
}
```

**Key Changes:**
- Polyphonic voice processing (up to 20 voices)
- Volume scaling prevents clipping
- Safety clip for output protection

---

## Phase 6: Implement Playback Triggering

### Task 6.1: Add Playback Trigger in Main Loop
- [ ] In `main()` loop, modify encoder press handler (around line 339):
```cpp
// Replace current handler
if (hw.encoder.RisingEdge() && totalFiles > 0) {
    if (isDirectory[selectedFile]) {
        EnterDirectory(fileList[selectedFile]);
    } else {
        // It's a WAV file - trigger playback
        int wavIndex = FindWavIndex(fileList[selectedFile]);
        if (wavIndex >= 0) {
            if (!m_tickers[wavIndex].finished_) {
                // Stop if currently playing
                m_tickers[wavIndex].finished_ = true;
                m_isPlaying = false;
            } else {
                // Start playback from beginning
                m_tickers[wavIndex].time_ = 0;
                m_tickers[wavIndex].finished_ = false;
                m_isPlaying = true;
            }
        }
    }
}
```

---

## Phase 7: Update Display for Polyphony

### Task 7.1: Update DisplayFilesOnScreen
- [ ] Add voice count display to footer:
```cpp
// Modify footer display (around line 258)
char footer[32];
int voices = m_isPlaying ? m_activeVoices : 0;
snprintf(footer, sizeof(footer), "%d/%d v:%d", 
            selectedFile + 1, totalFiles, voices);
display.WriteString(footer, Font_6x8, true);
```

### Task 7.2: Add Speed Display (Optional)
- [ ] Add playback speed indicator:
```cpp
// After footer display
char speedInfo[16];
snprintf(speedInfo, sizeof(speedInfo), "spd:%.2f", m_playbackSpeed);
display.SetCursor(0, 55);
display.WriteString(speedInfo, Font_6x8, true);
```

---

## Phase 8: Testing and Debugging

### Task 8.1: Compile
- [ ] Run `make clean`
- [ ] Run `make`
- [ ] Fix any compilation errors
- [ ] Verify no linker errors

### Task 8.2: Load to Daisy
- [ ] Run `make program`
- [ ] Verify successful upload
- [ ] Check OLED display shows file browser

### Task 8.3: Test Basic Playback
- [ ] Navigate to a WAV file
- [ ] Press encoder button - verify playback starts
- [ ] Observe voice count on display
- [ ] Test playback speed with knob1

### Task 8.4: Test Polyphony
- [ ] Play one file
- [ ] Enter a different directory, select second file
- [ ] Play second file while first is still playing
- [ ] Verify both files play simultaneously
- [ ] Check voice count shows 2

### Task 8.5: Test Edge Cases
- [ ] Test with very short WAV files (< 1 second)
- [ ] Test with long WAV files (> 30 seconds)
- [ ] Test reverse playback (knob1 at minimum)
- [ ] Test maximum polyphony (trigger 20 files)
- [ ] Verify no clipping at high voice counts

---

## Phase 9: Optional Enhancements

### Task 9.1: Add Stop All Function
- [ ] Add function to stop all voices:
```cpp
void StopAllVoices() {
    for (int i = 0; i < m_loadedCount; i++) {
        m_tickers[i].finished_ = true;
    }
    m_activeVoices = 0;
    m_isPlaying = false;
}
```

### Task 9.2: Add LED Feedback
- [ ] Use hw.led1 to indicate active voices:
```cpp
hw.led1.Set(m_activeVoices > 0, m_masterVolume, 0);
```

### Task 9.3: Add Loop Mode
- [ ] Add loop toggle (hold encoder press)
- [ ] Modify tickers to auto-restart on finish:
```cpp
// In AudioCallback, after tick()
if (m_tickers[i].finished_ && m_tickers[i].shouldLoop) {
    m_tickers[i].time_ = 0;
    m_tickers[i].finished_ = false;
}
```

---

## File Structure After Integration

```
SimpleSampler/
├── SimpleSampler.cpp       # Modified with polyphonic system
├── SimpleSampler.h         # (Optional) Header file
├── Makefile               # Updated with b3ReadWavFile.o
├── b3ReadWavFile.h        # New: Copied from DaisyCloudSeed
├── b3ReadWavFile.cpp      # New: Copied from DaisyCloudSeed
├── b3SwapUtils.h         # New: Copied from DaisyCloudSeed
└── build/
    ├── b3ReadWavFile.o   # New object file
    └── SimpleSampler.elf
```

---

## Control Mapping After Integration

| Control | Function | Range |
|----------|-----------|-------|
| Knob 1 | Playback Speed | -1.0 (reverse) to +1.0 (fast) |
| Knob 2 | Master Volume | 0.0 to 1.0 |
| Encoder Up/Down | File Selection | Navigate files |
| Encoder Press | Play/Stop Selected | Toggle playback |
| LED 1 | Activity Indicator | On when voices active |

---

## Troubleshooting

### Problem: No sound on playback
- [ ] Verify WAV file is properly loaded (check `LoadWavFile` return value)
- [ ] Check SDRAM pool has space (48MB should be sufficient)
- [ ] Verify `m_tickers[index].finished_` is set to `false` on trigger
- [ ] Check `m_activeVoices` increments in audio callback

### Problem: Distorted audio
- [ ] Verify volume scaling: `m_masterVolume / (float)m_activeVoices`
- [ ] Check safety clip is applied
- [ ] Reduce master volume if multiple voices
- [ ] Test with fewer active voices

### Problem: Compilation errors
- [ ] Verify `#include "b3ReadWavFile.h"` is present
- [ ] Check `daisy_core.h` is included (for `DSY_SDRAM_BSS`)
- [ ] Verify Makefile includes all required Daisy libraries
- [ ] Check for duplicate symbol errors (may need `extern` declarations)

### Problem: SDRAM not working
- [ ] Verify DaisyPod has SDRAM (check hardware specs)
- [ ] If no SDRAM, reduce pool size for external SRAM
- [ ] Check linker script includes SDRAM section

---

## Notes for Granular Synthesis (Future)

This polyphonic system provides foundation for granular synthesis. Future enhancements:

1. **Add ADSR envelopes** - Copy [`b3ADSR.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ADSR.h:1) / [`b3ADSR.cpp`](../../DaisyCloudSeed/patch/SamplePlayer/b3ADSR.cpp:1)
2. **Add grain spawning** - Implement `SpawnGrain()` like in GranularSynth
3. **Add triangular envelope** - Use `env_volume2()` from [`b3ReadWavFile.h`](../../DaisyCloudSeed/patch/SamplePlayer/b3ReadWavFile.h:162)
4. **Add randomization** - Random position, duration, pitch per grain

---

## Estimated Time

| Phase | Tasks | Estimated Time |
|--------|---------|---------------|
| Phase 1: Copy Core Files | 3 tasks | 15 minutes |
| Phase 2: Add SDRAM Pool | 2 tasks | 20 minutes |
| Phase 3: Replace Audio System | 3 tasks | 30 minutes |
| Phase 4: Implement WAV Loading | 3 tasks | 45 minutes |
| Phase 5: Audio Callback | 1 task | 30 minutes |
| Phase 6: Playback Trigger | 1 task | 20 minutes |
| Phase 7: Update Display | 2 tasks | 20 minutes |
| Phase 8: Testing | 5 tasks | 60 minutes |
| **Total** | **20 tasks** | **~4 hours** |
