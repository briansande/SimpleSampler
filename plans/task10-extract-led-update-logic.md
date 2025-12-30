# Task 10: Extract LED Update Logic - Implementation Plan

## Overview
Extract LED1 update logic from the main loop in [`SimpleSampler.cpp`](../SimpleSampler.cpp:214-229) into a separate helper function to improve code organization and readability.

## Current State Analysis

### Location of LED Update Logic
The LED update code is located in the main loop of [`SimpleSampler.cpp`](../SimpleSampler.cpp:214-232):

```cpp
// === LED Feedback ===
// Use LED1 to indicate sequencer state (running/stopped)
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

// LED2 shows metronome volume level
hw.led2.Set(knob2_value, 0.0f, 0.0f);
```

### LED Behavior Summary
- **LED1**: Sequencer status indicator
  - Running, beat (step % 4 == 0): Bright white (1.0, 1.0, 1.0)
  - Running, off-beat: Dim white (0.2, 0.2, 0.2)
  - Stopped: Red (0.5, 0.0, 0.0)
- **LED2**: Metronome volume indicator
  - Shows knob2_value (0.0-1.0) as red intensity

### Dependencies
- `DaisyPod::led1.Set(r, g, b)` - LED color setting method
- `Sequencer::isRunning()` - Returns sequencer running state (const method)
- `Sequencer::getCurrentStep()` - Returns current step index 0-15 (const method)

## Design Decisions

### Why Only Extract LED1?
LED2 uses `knob2_value`, which is a local variable in the main loop. Extracting LED2 would require passing the volume value as an additional parameter, adding complexity without significant benefit. The LED1 logic is self-contained and benefits most from extraction.

### Function Signature
```cpp
void updateSequencerLED(DaisyPod& hw, Sequencer* sequencer)
```

**Rationale:**
- Pass `hw` by reference to avoid copying the DaisyPod object
- Pass `sequencer` as pointer (matching existing code style)
- No return value needed (function has side effects only)
- Non-const `sequencer` pointer matches existing pattern, though only const methods are called

### Function Placement
Place the new function before `main()` at line 86 (after the `AudioCallback` function and before `main()`). This follows the convention of helper functions appearing before their usage point.

### Future Considerations
If additional LED feedback is added later, consider:
1. Creating a `LEDManager` class to encapsulate all LED logic
2. Using constants for LED colors (e.g., `LED_COLOR_BEAT`, `LED_COLOR_OFF_BEAT`, `LED_COLOR_STOPPED`)
3. Extracting LED2 logic if the metronome volume is needed elsewhere

## Implementation Steps

### Step 1: Add Helper Function
**Location:** [`SimpleSampler.cpp`](../SimpleSampler.cpp:86) - Insert before `main()`

**Action:** Add the following function between `AudioCallback` (line 84) and `main()` (line 86):

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

**Verification:**
- Function is placed at line 86 (before `main()`)
- Function signature matches the design
- Comments are preserved from original code

### Step 2: Replace Inline LED1 Update
**Location:** [`SimpleSampler.cpp`](../SimpleSampler.cpp:214-229)

**Action:** Replace lines 214-229 with:

```cpp
// === LED Feedback ===
updateSequencerLED(hw, sequencer);

// LED2 shows metronome volume level
hw.led2.Set(knob2_value, 0.0f, 0.0f);
```

**Verification:**
- LED1 update code is completely removed
- Function call to `updateSequencerLED()` is added
- LED2 update code remains unchanged
- `hw.UpdateLeds()` call (line 234) remains unchanged

### Step 3: Verify Compilation
**Action:** Compile the project to ensure no errors

```bash
make clean && make
```

**Expected Result:** Clean compilation with no errors or warnings

### Step 4: Update refactor.md
**Location:** [`refactor.md`](../refactor.md:333)

**Action:** Mark Task 10 as complete by changing `[ ]` to `[x]` on line 333

## Verification Checklist

After implementation, verify:

- [ ] Code compiles without errors: `make clean && make`
- [ ] No new warnings are introduced
- [ ] LED1 behavior is unchanged:
  - [ ] Bright white on beat when running
  - [ ] Dim white on off-beat when running
  - [ ] Red when stopped
- [ ] LED2 behavior is unchanged (shows metronome volume)
- [ ] No duplicate LED update logic elsewhere in codebase
- [ ] Task 10 is marked as complete in refactor.md

## Risk Assessment

### Low Risk
- This is a straightforward extraction of self-contained logic
- No behavior changes
- No new dependencies
- Single-threaded embedded system (no concurrency concerns)

### Potential Issues
- None identified

## Testing Recommendations

### Functional Testing
1. Start the sequencer - LED1 should flash bright white on beat, dim on off-beat
2. Stop the sequencer - LED1 should turn red
3. Adjust metronome volume knob - LED2 should show corresponding red intensity

### Regression Testing
- Ensure all other features (sequencer, sample playback, UI) still work correctly

## Files Modified

| File | Lines Changed | Description |
|------|---------------|-------------|
| `SimpleSampler.cpp` | Add 15 lines at line 86 | New helper function |
| `SimpleSampler.cpp` | Replace 16 lines at line 214 | Simplified LED feedback section |
| `refactor.md` | Line 333 | Mark task as complete |

## Related Tasks

This task is part of the MEDIUM PRIORITY refactoring tasks. After completion, the next task in sequence is Task 11: Refactor SampleLibrary::init() into Smaller Functions.
