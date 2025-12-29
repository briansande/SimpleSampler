# Code Refactoring Task List

This document contains a detailed list of code cleanliness improvements for the SimpleSampler project. Each task is designed to improve code flow, minimize redundancy, and enhance code organization without affecting any features.

**IMPORTANT**: Mark each task as complete by changing `[ ]` to `[x]` when finished.

---

## HIGH PRIORITY TASKS

### [ ] Task 1: Create Constants.h File

**File to Create**: `Constants.h`

**Description**: Extract all magic numbers and hardcoded values into a centralized constants header file.

**Steps**:
1. Create a new file `Constants.h` in the project root
2. Add the following namespaces and constants:

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

3. Update files to use these constants:
   - [`SimpleSampler.cpp:27`](SimpleSampler.cpp:27): Replace `DISPLAY_FPS = 3` with `Constants::Display::FPS`
   - [`SimpleSampler.cpp:55`](SimpleSampler.cpp:55): Replace `CUSTOM_POOL_SIZE (48*1024*1024)` with `Constants::Memory::CUSTOM_POOL_SIZE`
   - [`SimpleSampler.cpp:170-171`](SimpleSampler.cpp:170-171): Replace BPM mapping values with `Constants::UI::MIN_BPM` and `Constants::UI::BPM_RANGE`
   - [`SimpleSampler.cpp:204`](SimpleSampler.cpp:204): Replace `500` with `Constants::UI::HOLD_DETECT_MS`
   - [`DisplayManager.cpp:33-37`](DisplayManager.cpp:33-37): Replace all display constants
   - [`Menus.h:96`](Menus.h:96): Replace `MAX_CHARS_PER_LINE = 17` with `Constants::Display::MAX_CHARS_PER_LINE`
   - [`Sequencer.h:9-12`](Sequencer.h:9-12): Replace NUM_STEPS, NUM_TRACKS, MIN_BPM, MAX_BPM with constants
   - [`SampleLibrary.h:15`](SampleLibrary.h:15): Replace `MAX_SAMPLES 64` with constant

**Files to Modify**:
- Create: `Constants.h`
- Modify: `SimpleSampler.cpp`
- Modify: `DisplayManager.cpp`
- Modify: `Menus.h`
- Modify: `Sequencer.h`
- Modify: `SampleLibrary.h`

---

### [ ] Task 2: Fix Repeated const_cast Usage

**File to Modify**: [`UIManager.h`](UIManager.h)

**Description**: Add non-const accessor to `UIManager` to eliminate repeated `const_cast` calls in [`SimpleSampler.cpp`](SimpleSampler.cpp:188-207).

**Steps**:
1. Open `UIManager.h`
2. Find the `getState()` method (around line 180)
3. Add a non-const version of the method:

```cpp
// Get UI state (non-const version for modification)
UIState& getState() { return state_; }

// Get UI state (const version for read-only access)
const UIState& getState() const { return state_; }
```

4. Update [`SimpleSampler.cpp`](SimpleSampler.cpp:188-207) to use the non-const accessor:
   - Line 188: Change `const_cast<UIState&>(uiManager->getState()).encoderPressed = true;` to `uiManager->getState().encoderPressed = true;`
   - Line 189: Change `const_cast<UIState&>(uiManager->getState()).encoderPressTime = System::GetNow();` to `uiManager->getState().encoderPressTime = System::GetNow();`
   - Line 190: Change `const_cast<UIState&>(uiManager->getState()).encoderHeld = false;` to `uiManager->getState().encoderHeld = false;`
   - Line 206: Change `const_cast<UIState&>(uiManager->getState()).encoderHeld = true;` to `uiManager->getState().encoderHeld = true;`
   - Line 197: Change `const_cast<UIState&>(uiManager->getState()).encoderPressed = false;` to `uiManager->getState().encoderPressed = false;`
   - Line 198: Change `const_cast<UIState&>(uiManager->getState()).encoderHeld = false;` to `uiManager->getState().encoderHeld = false;`

**Files to Modify**:
- `UIManager.h`
- `SimpleSampler.cpp`

---

### [ ] Task 3: Remove Unused Variables

**File to Modify**: [`SimpleSampler.cpp`](SimpleSampler.cpp)

**Description**: Remove unused variables `inc` and `lastUpdateTime`.

**Steps**:
1. Open `SimpleSampler.cpp`
2. Find line 33: `static int32_t  inc;` - Delete this entire line
3. Find line 96: `uint32_t lastUpdateTime = System::GetNow();` - Delete this entire line

**Files to Modify**:
- `SimpleSampler.cpp`

---

### [ ] Task 4: Standardize Include Guards

**Files to Modify**: All header files

**Description**: Standardize all header files to use `#pragma once` instead of `#ifndef` guards.

**Steps**:
1. Open each header file and check the include guard style
2. Convert any `#ifndef` guards to `#pragma once`:

Files to convert from `#ifndef` to `#pragma once`:
- `UIManager.h` (currently uses `#ifndef UI_MANAGER_H`)
- `Menus.h` (currently uses `#ifndef MENUS_H`)
- `Sequencer.h` (currently uses `#ifndef SEQUENCER_H`)
- `SampleLibrary.h` (currently uses `#ifndef SAMPLE_LIBRARY_H`)
- `DisplayManager.h` (currently uses `#ifndef DISPLAY_MANAGER_H`)
- `Metronome.h` (currently uses `#ifndef METRONOME_H`)

For each file:
1. Remove the `#ifndef` line
2. Remove the `#define` line
3. Remove the `#endif // ...` line at the end of the file
4. Add `#pragma once` at the very top

**Files to Modify**:
- `UIManager.h`
- `Menus.h`
- `Sequencer.h`
- `SampleLibrary.h`
- `DisplayManager.h`
- `Metronome.h`

---

### [ ] Task 5: Fix Direct Display Access in SimpleSampler.cpp

**File to Modify**: [`SimpleSampler.cpp`](SimpleSampler.cpp:136-140)

**Description**: Replace direct display access with `DisplayManager` methods.

**Steps**:
1. Open `SimpleSampler.cpp`
2. Find lines 136-140:
```cpp
display.SetCursor(0, 0);
display.WriteString((char*)"SD Card Error!", Font_7x10, true);
display.Update();
while(1);  // Halt
```

3. Replace with:
```cpp
display_.showMessage("SD Card Error!", 0);
while(1);  // Halt
```

**Files to Modify**:
- `SimpleSampler.cpp`

---

## MEDIUM PRIORITY TASKS

### [ ] Task 6: Add Default Implementations to BaseMenu

**File to Modify**: [`UIManager.h`](UIManager.h)

**Description**: Add default empty implementations for button handlers in `BaseMenu` to eliminate redundant empty methods in derived classes.

**Steps**:
1. Open `UIManager.h`
2. Find the `BaseMenu` class definition (around line 78)
3. Add default implementations for the following virtual methods:

```cpp
// Handle encoder hold (long press - exit)
virtual void onEncoderHold() {}  // Default: do nothing

// Handle button1 press
virtual void onButton1Press() {}  // Default: do nothing

// Handle button2 press
virtual void onButton2Press() {}  // Default: do nothing
```

4. Remove the following empty implementations from derived classes in [`Menus.cpp`](Menus.cpp):
   - `TrackSelectMenu::onEncoderHold()` (lines 83-86)
   - `TrackSelectMenu::onButton1Press()` (lines 88-91)
   - `TrackSelectMenu::onButton2Press()` (lines 93-96)
   - `TrackEditMenu::onButton1Press()` (lines 193-196)
   - `TrackEditMenu::onButton2Press()` (lines 198-201)
   - `SampleSelectMenu::onButton1Press()` (lines 355-358)
   - `SampleSelectMenu::onButton2Press()` (lines 360-363)

5. Remove the corresponding declarations from [`Menus.h`](Menus.h):
   - `TrackSelectMenu::onEncoderHold()` (line 38)
   - `TrackSelectMenu::onButton1Press()` (line 41)
   - `TrackSelectMenu::onButton2Press()` (line 42)
   - `TrackEditMenu::onButton1Press()` (line 80)
   - `TrackEditMenu::onButton2Press()` (line 81)
   - `SampleSelectMenu::onButton1Press()` (line 123)
   - `SampleSelectMenu::onButton2Press()` (line 124)

**Files to Modify**:
- `UIManager.h`
- `Menus.cpp`
- `Menus.h`

---

### [ ] Task 7: Refactor getTrack() to Eliminate Duplication

**File to Modify**: [`Sequencer.cpp`](Sequencer.cpp)

**Description**: Use const_cast pattern to eliminate duplicate bounds checking in const and non-const `getTrack()` methods.

**Steps**:
1. Open `Sequencer.cpp`
2. Find the const version of `getTrack()` (lines 160-166)
3. Replace the entire method body with:

```cpp
const Track* Sequencer::getTrack(int index) const {
    return const_cast<Sequencer*>(this)->getTrack(index);
}
```

**Files to Modify**:
- `Sequencer.cpp`

---

### [ ] Task 8: Update Hardcoded Menu Array Size

**File to Modify**: [`UIManager.h`](UIManager.h)

**Description**: Replace hardcoded array size `4` with a named constant.

**Steps**:
1. Open `UIManager.h`
2. Find the `UIManager` class private section (around line 126)
3. Add a constant before the `menus_` array:

```cpp
static constexpr int NUM_SCREENS = 4;
```

4. Change line 139 from:
```cpp
BaseMenu* menus_[4];  // One for each ScreenType
```
to:
```cpp
BaseMenu* menus_[NUM_SCREENS];  // One for each ScreenType
```

**Files to Modify**:
- `UIManager.h`

---

### [ ] Task 9: Extract DisplayManager Constants

**File to Modify**: [`DisplayManager.h`](DisplayManager.h)

**Description**: Move display constants from local function scope to class-level static constants.

**Steps**:
1. Open `DisplayManager.h`
2. Find the `DisplayManager` class private section (around line 79)
3. Add static constants before the member variables:

```cpp
private:
    // Display Constants
    static constexpr uint8_t CHAR_WIDTH = 7;
    static constexpr uint8_t LINE_HEIGHT = 10;
    static constexpr uint8_t SCREEN_WIDTH = 128;
    static constexpr uint8_t SCREEN_HEIGHT = 64;
    static constexpr uint8_t MAX_CHARS_PER_LINE = SCREEN_WIDTH / CHAR_WIDTH;
```

4. Open `DisplayManager.cpp`
5. Find the `showMessage()` method (around line 30)
6. Remove the local constant definitions (lines 33-37):
```cpp
const uint8_t CHAR_WIDTH = 7;
const uint8_t LINE_HEIGHT = 10;
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const uint8_t MAX_CHARS_PER_LINE = SCREEN_WIDTH / CHAR_WIDTH;
```

7. Update references in `showMessage()` to use the class constants (they're accessible as `CHAR_WIDTH`, etc., since they're in the same class)

**Files to Modify**:
- `DisplayManager.h`
- `DisplayManager.cpp`

---

### [ ] Task 10: Extract LED Update Logic

**File to Modify**: [`SimpleSampler.cpp`](SimpleSampler.cpp)

**Description**: Extract LED update logic into a separate helper function.

**Steps**:
1. Open `SimpleSampler.cpp`
2. Add a new function before the `main()` function (around line 88):

```cpp
void updateSequencerLED(DaisyPod& hw, Sequencer* sequencer)
{
    if(sequencer->isRunning()) {
        // Flash LED on each step
        int currentStep = sequencer->getCurrentStep();
        if(currentStep % 4 == 0) {
            // Bright on beat
            hw.led1.Set(1.0f, 1.0f, 1.0f);
        } else {
            // Dim on off-beat
            hw.led1.Set(0.2f, 0.2f, 0.2f);
        }
    } else {
        // Red when stopped
        hw.led1.Set(0.5f, 0.0f, 0.0f);
    }
}
```

3. In the main loop (around line 223-237), replace the LED update code with:
```cpp
// === LED Feedback ===
updateSequencerLED(hw, sequencer);

// LED2 shows metronome volume level
hw.led2.Set(knob2_value, 0.0f, 0.0f);
```

**Files to Modify**:
- `SimpleSampler.cpp`

---

### [ ] Task 11: Refactor SampleLibrary::init() into Smaller Functions

**File to Modify**: [`SampleLibrary.cpp`](SampleLibrary.cpp)

**Description**: Break down the 110-line `init()` function into smaller, more manageable functions.

**Steps**:
1. Open `SampleLibrary.cpp`
2. Add the following private helper method declarations to `SampleLibrary.h` (in the private section, around line 45):

```cpp
// Helper: Open root directory
bool openDirectory();

// Helper: Scan and load WAV files
bool scanAndLoadFiles();

// Helper: Load a single WAV file
bool loadWavFile(const char* filename, int index);
```

3. In `SampleLibrary.cpp`, implement these helper functions:

```cpp
bool SampleLibrary::openDirectory()
{
    DIR dir;
    if (f_opendir(&dir, "/") != FR_OK) {
        display_.showMessage("Dir open failed!", 200);
        return false;
    }
    return true;
}

bool SampleLibrary::scanAndLoadFiles()
{
    DIR dir;
    FILINFO fno;
    FRESULT result;
    int fileCount = 0;

    // Open the root directory
    if (f_opendir(&dir, "/") != FR_OK) {
        display_.showMessage("Dir open failed!", 200);
        return false;
    }

    display_.showMessage("Scanning files...", 200);

    // Read entries in a loop
    while (f_readdir(&dir, &fno) == FR_OK) {
        if (fno.fname[0] == 0) break;  // No more files

        // Check if filename contains .wav or .WAV
        if (strstr(fno.fname, ".wav") != nullptr || strstr(fno.fname, ".WAV") != nullptr) {
            display_.showMessagef("Found WAV: %s", 200, fno.fname);
            if (fileCount < MAX_SAMPLES) {
                if (loadWavFile(fno.fname, fileCount)) {
                    fileCount++;
                }
            }
        }
    }

    // Close directory
    f_closedir(&dir);

    // Store the number of loaded samples
    sampleCount_ = fileCount;

    return true;
}

bool SampleLibrary::loadWavFile(const char* filename, int index)
{
    FIL SDFile;

    if(f_open(&SDFile, filename, (FA_OPEN_EXISTING | FA_READ)) != FR_OK) {
        display_.showMessagef("Open failed!", 200);
        return false;
    }

    int size = f_size(&SDFile);
    display_.showMessagef("Size: %d bytes", 200, size);

    // Allocate memory from custom pool
    char* memoryBuffer = (char*) custom_pool_allocate(size);

    if(!memoryBuffer) {
        display_.showMessagef("Alloc failed!", 200);
        f_close(&SDFile);
        return false;
    }

    display_.showMessagef("Alloc OK, reading...", 200);

    UINT bytesRead;
    if(f_read(&SDFile, memoryBuffer, size, &bytesRead) != FR_OK || bytesRead != size) {
        display_.showMessagef("Read failed!", 200);
        f_close(&SDFile);
        return false;
    }

    display_.showMessagef("Read OK, parsing...", 200);

    samples_[index].dataSource = MemoryDataSource(memoryBuffer, size);
    samples_[index].reader.getWavInfo(samples_[index].dataSource);

    // Copy filename to SampleInfo
    strncpy(samples_[index].name, filename, sizeof(samples_[index].name) - 1);
    samples_[index].name[sizeof(samples_[index].name) - 1] = '\0';

    // Copy WAV metadata from reader to SampleInfo
    samples_[index].numFrames = samples_[index].reader.getNumFrames();
    samples_[index].channels = samples_[index].reader.getChannels();
    samples_[index].sampleRate = (int)samples_[index].reader.getFileDataRate();
    samples_[index].bitsPerSample = samples_[index].reader.getBitsPerSample();

    // Mark sample as loaded
    samples_[index].loaded = true;
    samples_[index].audioDataLoaded = true;

    display_.showMessagef("Parsed OK, creating ticker...", 200);
    wavTickers[index] = samples_[index].reader.createWavTicker(Config::samplerate);
    display_.showMessagef("Ticker created!", 200);
    wavTickers[index].finished_ = true;

    display_.showMessagef("Loaded: %s", 200, filename);

    f_close(&SDFile);
    return true;
}
```

4. Replace the `init()` method body (lines 26-136) with:

```cpp
bool SampleLibrary::init()
{
    display_.showMessage("Initializing Library...", 200);

    if (!openDirectory()) {
        return false;
    }

    if (!scanAndLoadFiles()) {
        return false;
    }

    display_.showMessage("Files scanned", 300);
    char msg[64];
    snprintf(msg, sizeof(msg), "WAV Files: %d", sampleCount_);
    display_.showMessage(msg, 200);

    return true;
}
```

**Files to Modify**:
- `SampleLibrary.h`
- `SampleLibrary.cpp`

---

### [ ] Task 12: Fix Magic Number for Filename Length

**File to Modify**: [`SampleLibrary.cpp`](SampleLibrary.cpp)

**Description**: Replace magic number `31` with proper sizeof expression.

**Steps**:
1. Open `SampleLibrary.cpp`
2. Find line 81:
```cpp
strncpy(samples_[fileCount].name, fno.fname, 31);
```

3. Replace with:
```cpp
strncpy(samples_[fileCount].name, fno.fname, sizeof(samples_[fileCount].name) - 1);
```

4. Find line 82:
```cpp
samples_[fileCount].name[31] = '\0';
```

5. Replace with:
```cpp
samples_[fileCount].name[sizeof(samples_[fileCount].name) - 1] = '\0';
```

**Files to Modify**:
- `SampleLibrary.cpp`

---

## LOW PRIORITY TASKS

### [ ] Task 13: Create Utility Helper for Clamping

**File to Create**: `Utils.h`

**Description**: Create a utility header with a clamp function to eliminate duplicate clamping logic.

**Steps**:
1. Create a new file `Utils.h` in the project root
2. Add the following content:

```cpp
#pragma once

namespace Utils {
    /**
     * Clamp a value between min and max
     * @param value The value to clamp
     * @param min Minimum allowed value
     * @param max Maximum allowed value
     * @return The clamped value
     */
    inline float clamp(float value, float min, float max) {
        return (value < min) ? min : (value > max) ? max : value;
    }
}
```

3. Update [`Sequencer.cpp`](Sequencer.cpp:224-234) to use the utility:
   - Replace lines 227-231 with: `state_.metronomeVolume = Utils::clamp(volume, 0.0f, 1.0f);`

4. Update [`Metronome.cpp`](Metronome.cpp:73-83) to use the utility:
   - Replace lines 76-82 with: `volume_ = Utils::clamp(volume, 0.0f, 1.0f);`

5. Add `#include "Utils.h"` to both files at the top

**Files to Create**:
- `Utils.h`

**Files to Modify**:
- `Sequencer.cpp`
- `Metronome.cpp`

---

### [ ] Task 14: Extract Selection Indicator Rendering

**File to Modify**: [`Menus.cpp`](Menus.cpp)

**Description**: Extract repeated selection indicator rendering into a helper function.

**Steps**:
1. Open `Menus.cpp`
2. Add a private helper method declaration to `BaseMenu` in `UIManager.h` (around line 78):

```cpp
protected:
    // Helper: Render selection indicator
    void renderSelectionIndicator(int yPos, bool isSelected);
```

3. In `Menus.cpp`, implement the helper function:

```cpp
void BaseMenu::renderSelectionIndicator(int yPos, bool isSelected)
{
    display_->setCursor(0, yPos);
    display_->writeString(isSelected ? ">" : " ", Font_7x10);
}
```

4. Replace the repeated code in `TrackSelectMenu::render()` (lines 30-36):
```cpp
// Show selection indicator
renderSelectionIndicator(yPos, i == selectedIndex_);
```

5. Replace the repeated code in `TrackEditMenu::render()` (lines 134-140 and 148-154):
```cpp
// Sample option
renderSelectionIndicator(yPos, selectedOption_ == Option::Sample);
```

6. Replace the repeated code in `SampleSelectMenu::render()` (lines 262-268):
```cpp
// Show selection indicator
renderSelectionIndicator(yPos, sampleIndex == selectedIndex_);
```

**Files to Modify**:
- `UIManager.h`
- `Menus.cpp`

---

### [ ] Task 15: Use strncpy for Scrolling Text

**File to Modify**: [`Menus.cpp`](Menus.cpp)

**Description**: Replace manual character copying with `strncpy` for scrolling text.

**Steps**:
1. Open `Menus.cpp`
2. Find the `SampleSelectMenu::render()` method (around line 239)
3. Find lines 285-292:
```cpp
for (int j = 0; j < charsToCopy; j++) {
    displayBuffer[j] = sampleName[state_->scrollOffset + j];
}
displayBuffer[charsToCopy] = '\0';
```

4. Replace with:
```cpp
strncpy(displayBuffer, sampleName + state_->scrollOffset, charsToCopy);
displayBuffer[charsToCopy] = '\0';
```

**Files to Modify**:
- `Menus.cpp`

---

### [ ] Task 16: Standardize Naming Convention

**Files to Modify**: All header and source files

**Description**: Ensure all private member variables use trailing underscore convention consistently.

**Steps**:
1. Review all class private member variables
2. Ensure they all follow the `variableName_` convention

Files to check and potentially fix:
- `UIManager.h`: Already uses trailing underscore
- `Menus.h`: Already uses trailing underscore
- `Sequencer.h`: Already uses trailing underscore
- `SampleLibrary.h`: Already uses trailing underscore
- `DisplayManager.h`: Already uses trailing underscore
- `Metronome.h`: Already uses trailing underscore

**Note**: After review, all files already follow the trailing underscore convention. Mark this task as complete.

**Files to Review**:
- `UIManager.h`
- `Menus.h`
- `Sequencer.h`
- `SampleLibrary.h`
- `DisplayManager.h`
- `Metronome.h`

---

## COMPLETION CHECKLIST

After completing all tasks, verify:

- [ ] All code compiles without errors: `make clean && make`
- [ ] No new warnings are introduced
- [ ] All features still work as expected
- [ ] Code is tested on the Daisy hardware: `make program`

---

## NOTES FOR IMPLEMENTERS

1. **Order of Operations**: Tasks are listed in priority order. Complete HIGH priority tasks first, then MEDIUM, then LOW.

2. **Testing**: After each task or group of related tasks, compile and test to ensure nothing is broken.

3. **Feature Preservation**: The goal is code cleanliness WITHOUT affecting features. If a change appears to affect functionality, review carefully before proceeding.

4. **Incremental Progress**: Mark tasks as complete as you finish them to track progress.

5. **Dependencies**: Some tasks depend on others. For example, Task 1 (Constants.h) should be done before other tasks that reference those constants.

---

## TASK COMPLETION SUMMARY

| Priority | Total Tasks | Completed |
|----------|-------------|-----------|
| HIGH | 5 | [ ] |
| MEDIUM | 7 | [ ] |
| LOW | 4 | [ ] |
| **TOTAL** | **16** | **[ ]** |

Last Updated: 2025-12-29
