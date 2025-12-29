# Task 1: Create Constants.h File - Implementation Plan

## Overview
Extract all magic numbers and hardcoded values into a centralized constants header file to improve code maintainability.

## Files Summary

| File | Action | Lines Affected |
|------|--------|----------------|
| `Constants.h` | CREATE | New file |
| `SimpleSampler.cpp` | MODIFY | 27, 55, 56, 61, 170, 204 |
| `DisplayManager.cpp` | MODIFY | 33-37, 52, 57, 66, 68, 73, 85 |
| `Menus.h` | MODIFY | 94-96 |
| `Sequencer.h` | MODIFY | 9-12, 29, 44, 62, 69, 85 |
| `SampleLibrary.h` | MODIFY | 15, 34, 35, 36 |

---

## Step 1: Create Constants.h File

**File**: `Constants.h` (new file in project root)

**Content**:
```cpp
#pragma once

namespace Constants {
    // Display Constants
    namespace Display {
        constexpr uint8_t WIDTH = 128;
        constexpr uint8_t HEIGHT = 64;
        constexpr uint8_t CHAR_WIDTH = 7;
        constexpr uint8_t LINE_HEIGHT = 10;
        constexpr uint8_t MAX_CHARS_PER_LINE = WIDTH / CHAR_WIDTH;  // ~18 chars
        constexpr uint32_t FPS = 3;
    }

    // UI Constants
    namespace UI {
        constexpr uint32_t HOLD_DETECT_MS = 500;
        constexpr float MIN_BPM = 60.0f;
        constexpr float MAX_BPM = 180.0f;
        constexpr float BPM_RANGE = MAX_BPM - MIN_BPM;
    }

    // Memory Constants
    namespace Memory {
        constexpr size_t CUSTOM_POOL_SIZE = 48 * 1024 * 1024;  // 48MB
    }

    // Sequencer Constants
    namespace Sequencer {
        constexpr int NUM_STEPS = 16;
        constexpr int NUM_TRACKS = 3;
        constexpr int MIN_BPM = 60;
        constexpr int MAX_BPM = 180;
    }

    // Sample Library Constants
    namespace SampleLibrary {
        constexpr int MAX_SAMPLES = 64;
    }
}
```

---

## Step 2: Modify SimpleSampler.cpp

**Add Include Statement**:
- Add `#include "Constants.h"` after line 15 (after `#include "daisysp.h"`)

**Line 27** - Remove DISPLAY_FPS constant:
```cpp
// BEFORE:
const uint32_t DISPLAY_FPS = 3;                        //  FPS for OLED screen

// AFTER:
// (Remove this line - DISPLAY_FPS is now in Constants.h)
```

**Line 55** - Remove CUSTOM_POOL_SIZE macro:
```cpp
// BEFORE:
#define CUSTOM_POOL_SIZE (48*1024*1024)

// AFTER:
// (Remove this line - CUSTOM_POOL_SIZE is now in Constants.h)
```

**Line 56** - Update array declaration:
```cpp
// BEFORE:
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];

// AFTER:
DSY_SDRAM_BSS char custom_pool[Constants::Memory::CUSTOM_POOL_SIZE];
```

**Line 61** - Update bounds check:
```cpp
// BEFORE:
if (pool_index + size >= CUSTOM_POOL_SIZE) {

// AFTER:
if (pool_index + size >= Constants::Memory::CUSTOM_POOL_SIZE) {
```

**Line 170** - Update BPM mapping:
```cpp
// BEFORE:
float bpm = 60.0f + (knob1_value * 120.0f);  // Map 0.0-1.0 to 60-180 BPM

// AFTER:
float bpm = Constants::UI::MIN_BPM + (knob1_value * Constants::UI::BPM_RANGE);  // Map 0.0-1.0 to 60-180 BPM
```

**Line 204** - Update hold detection:
```cpp
// BEFORE:
if(pressDuration >= 500) {

// AFTER:
if(pressDuration >= Constants::UI::HOLD_DETECT_MS) {
```

---

## Step 3: Modify DisplayManager.cpp

**Add Include Statement**:
- Add `#include "Constants.h"` after line 2 (after `#include <cstdarg>`)

**Lines 33-37** - Remove local display constants:
```cpp
// BEFORE (lines 33-37):
    const uint8_t CHAR_WIDTH = 7;              // Width of each character in Font_7x10
    const uint8_t LINE_HEIGHT = 10;            // Height of each line (font height)
    const uint8_t SCREEN_WIDTH = 128;          // OLED display width in pixels
    const uint8_t SCREEN_HEIGHT = 64;          // OLED display height in pixels
    const uint8_t MAX_CHARS_PER_LINE = SCREEN_WIDTH / CHAR_WIDTH;  // ~18 chars per line

// AFTER:
    // (Remove these 5 lines - constants are now in Constants.h)
```

**Line 52** - Update LINE_HEIGHT usage:
```cpp
// BEFORE:
    cursorY += LINE_HEIGHT;

// AFTER:
    cursorY += Constants::Display::LINE_HEIGHT;
```

**Line 57** - Update SCREEN_HEIGHT usage:
```cpp
// BEFORE:
    if (cursorY >= SCREEN_HEIGHT) {

// AFTER:
    if (cursorY >= Constants::Display::HEIGHT) {
```

**Line 66** - Update MAX_CHARS_PER_LINE usage:
```cpp
// BEFORE:
    if (charsOnLine >= MAX_CHARS_PER_LINE) {

// AFTER:
    if (charsOnLine >= Constants::Display::MAX_CHARS_PER_LINE) {
```

**Line 68** - Update LINE_HEIGHT usage:
```cpp
// BEFORE:
    cursorY += LINE_HEIGHT;

// AFTER:
    cursorY += Constants::Display::LINE_HEIGHT;
```

**Line 73** - Update SCREEN_HEIGHT usage:
```cpp
// BEFORE:
    if (cursorY >= SCREEN_HEIGHT) {

// AFTER:
    if (cursorY >= Constants::Display::HEIGHT) {
```

**Line 85** - Update CHAR_WIDTH usage:
```cpp
// BEFORE:
    cursorX += CHAR_WIDTH;

// AFTER:
    cursorX += Constants::Display::CHAR_WIDTH;
```

---

## Step 4: Modify Menus.h

**Add Include Statement**:
- Add `#include "Constants.h"` after line 7 (after `#include "SampleLibrary.h"`)

**Lines 94-96** - Update SampleSelectMenu constants:
```cpp
// BEFORE (lines 94-96):
    static const int ITEMS_PER_SCREEN = 4;  // Samples shown at once
    static const int CHAR_WIDTH = 7;        // Width of each character in Font_7x10
    static const int MAX_CHARS_PER_LINE = 17; // ~120px available for text

// AFTER:
    static const int ITEMS_PER_SCREEN = 4;  // Samples shown at once (kept as is - not in Constants.h)
    // CHAR_WIDTH and MAX_CHARS_PER_LINE are now in Constants::Display
```

**Note**: The `ITEMS_PER_SCREEN` constant is not in the required Constants.h structure, so it remains in Menus.h.

---

## Step 5: Modify Sequencer.h

**Add Include Statement**:
- Add `#include "Constants.h"` after line 6 (after `#include "daisy_core.h"`)

**Lines 9-12** - Remove #define macros:
```cpp
// BEFORE (lines 9-12):
#define NUM_STEPS 16
#define NUM_TRACKS 3
#define MIN_BPM 60
#define MAX_BPM 180

// AFTER:
// (Remove these 4 lines - constants are now in Constants.h)
```

**Line 29** - Update NUM_STEPS usage in Track struct:
```cpp
// BEFORE:
    bool steps[NUM_STEPS];

// AFTER:
    bool steps[Constants::Sequencer::NUM_STEPS];
```

**Line 44** - Update NUM_STEPS usage in Track::init():
```cpp
// BEFORE:
        for (int i = 0; i < NUM_STEPS; i++) {

// AFTER:
        for (int i = 0; i < Constants::Sequencer::NUM_STEPS; i++) {
```

**Line 62** - Update comment (optional but recommended):
```cpp
// BEFORE:
    int bpm;                     // Current tempo (60-180)

// AFTER:
    int bpm;                     // Current tempo (MIN_BPM - MAX_BPM from Constants::Sequencer)
```

**Line 69** - Update NUM_TRACKS usage in SequencerState:
```cpp
// BEFORE:
    Track tracks[NUM_TRACKS];

// AFTER:
    Track tracks[Constants::Sequencer::NUM_TRACKS];
```

**Line 85** - Update NUM_TRACKS usage in SequencerState::init():
```cpp
// BEFORE:
        for (int i = 0; i < NUM_TRACKS; i++) {

// AFTER:
        for (int i = 0; i < Constants::Sequencer::NUM_TRACKS; i++) {
```

---

## Step 6: Modify SampleLibrary.h

**Add Include Statement**:
- Add `#include "Constants.h"` after line 9 (after `#include <string>`)

**Line 15** - Remove MAX_SAMPLES macro:
```cpp
// BEFORE:
#define MAX_SAMPLES 64

// AFTER:
// (Remove this line - constant is now in Constants.h)
```

**Line 34** - Update MAX_SAMPLES usage:
```cpp
// BEFORE:
    SampleInfo samples_[MAX_SAMPLES];  // Array of loaded samples

// AFTER:
    SampleInfo samples_[Constants::SampleLibrary::MAX_SAMPLES];  // Array of loaded samples
```

**Line 35** - Update MAX_SAMPLES usage:
```cpp
// BEFORE:
    b3WavTicker wavTickers[MAX_SAMPLES]; // Array of tickers for playback

// AFTER:
    b3WavTicker wavTickers[Constants::SampleLibrary::MAX_SAMPLES]; // Array of tickers for playback
```

**Line 36** - Update MAX_SAMPLES usage:
```cpp
// BEFORE:
    float sampleSpeeds_[MAX_SAMPLES];   // Per-sample playback speed

// AFTER:
    float sampleSpeeds_[Constants::SampleLibrary::MAX_SAMPLES];   // Per-sample playback speed
```

---

## Important Notes

### Discrepancy in MAX_CHARS_PER_LINE
- In the current code, `Menus.h` line 96 defines `MAX_CHARS_PER_LINE = 17` (accounting for the `>` selection indicator)
- The new `Constants::Display::MAX_CHARS_PER_LINE` is calculated as `128 / 7 = 18` (full width)
- **Task requirement explicitly states to replace the 17 with the constant value from Constants.h**
- This may cause text overflow in menus with long sample names - this is expected per the task specification

### Constants Not Included
The following constants were found in the code but are NOT in the required Constants.h structure:
- `ITEMS_PER_SCREEN = 4` (Menus.h line 94) - Kept as local constant
- `STEPS_PER_ROW = 8` (Menus.h line 137) - Kept as local constant
- `MAX_STACK_DEPTH = 8` (UIManager.h line 133) - Kept as local constant
- `SCROLL_DELAY_MS = 200` (UIManager.h line 50) - Kept as local constant

### Compilation Verification
After completing all modifications, verify compilation with:
```bash
make clean && make
```

---

## Verification Checklist

After implementation, verify:
- [ ] Constants.h file is created with all required namespaces
- [ ] SimpleSampler.cpp includes Constants.h and all replacements are correct
- [ ] DisplayManager.cpp includes Constants.h and all replacements are correct
- [ ] Menus.h includes Constants.h and MAX_CHARS_PER_LINE is replaced
- [ ] Sequencer.h includes Constants.h and all #define macros are removed
- [ ] SampleLibrary.h includes Constants.h and MAX_SAMPLES macro is removed
- [ ] Code compiles without errors
- [ ] No new warnings are introduced
