# Task 5: Fix Direct Display Access in SimpleSampler.cpp

## Overview
Replace direct display access with `DisplayManager` methods to maintain proper abstraction and code consistency.

## Problem Analysis

### Current Code Location
**File**: [`SimpleSampler.cpp`](../SimpleSampler.cpp)
**Lines**: 130-133 (Note: Task description mentions 136-140, but actual location is 130-133)

```cpp
if (!library->init()) {
    display.SetCursor(0, 0);
    display.WriteString((char*)"SD Card Error!", Font_7x10, true);
    display.Update();
    while(1);  // Halt
}
```

### Issue
The code directly accesses the raw `display` object (`MyOledDisplay`), bypassing the `DisplayManager` abstraction layer. This breaks the encapsulation pattern established elsewhere in the codebase.

### Solution
Replace the direct display calls with the `DisplayManager::showMessage()` method.

## Implementation Steps

### Step 1: Locate the Code Block
- Open [`SimpleSampler.cpp`](../SimpleSampler.cpp)
- Navigate to line 129-134
- Identify the `if (!library->init())` block

### Step 2: Replace the Code
Replace lines 130-133 with the following:

**FROM:**
```cpp
display.SetCursor(0, 0);
display.WriteString((char*)"SD Card Error!", Font_7x10, true);
display.Update();
```

**TO:**
```cpp
display_.showMessage("SD Card Error!", 0);
```

**Note on delayMs=0**: The `while(1);` halts execution immediately after, so the delay value doesn't matter. The message will remain visible indefinitely.

### Step 3: Verify the Context
After replacement, the complete block should look like:
```cpp
if (!library->init()) {
    display_.showMessage("SD Card Error!", 0);
    while(1);  // Halt
}
```

## Verification Checklist

### Pre-Implementation Checks
- [ ] Confirm `DisplayManager` is properly initialized at line 99: `new (&display_) DisplayManager(display, hw);`
- [ ] Verify `DisplayManager::showMessage()` signature: `void showMessage(const char* message, uint32_t delayMs);`
- [ ] Confirm no other direct `display` accesses exist in the file

### Post-Implementation Checks
- [ ] Code compiles without errors: `make clean && make`
- [ ] No new warnings are introduced
- [ ] The error message still displays correctly when SD card initialization fails
- [ ] The program still halts as expected on error

## Technical Details

### DisplayManager::showMessage() Behavior
The `showMessage()` method (defined in [`DisplayManager.cpp`](../DisplayManager.cpp:31-86)) performs the following:
1. Clears the display with `display_.Fill(false)`
2. Sets cursor position to (0, 0)
3. Writes the message text using `Font_7x10`
4. Calls `display_.Update()` to refresh the screen
5. Calls `hw_.DelayMs(delayMs)` for the specified delay

This matches exactly what the original code was doing, but with proper abstraction.

### Why This Change is Important
1. **Consistency**: All other display operations in [`SimpleSampler.cpp`](../SimpleSampler.cpp) use `display_` (DisplayManager)
2. **Encapsulation**: Direct access to `display` breaks the abstraction layer
3. **Maintainability**: Future changes to display handling only need to be made in DisplayManager
4. **Code Cleanliness**: Reduces code duplication and improves organization

## Related Files
- [`SimpleSampler.cpp`](../SimpleSampler.cpp) - File to modify
- [`DisplayManager.h`](../DisplayManager.h) - Contains showMessage() declaration
- [`DisplayManager.cpp`](../DisplayManager.cpp) - Contains showMessage() implementation
- [`refactor.md`](../refactor.md) - Task definition document

## Notes for Implementer
- The line numbers in the task description (136-140) differ from the actual location (130-133). Use the actual line numbers when making the change.
- The `display` object (raw `MyOledDisplay`) is still declared at line 30 and is needed for the DisplayManager constructor. Do not remove this declaration.
- After this change, there should be no more direct calls to `display.SetCursor()`, `display.WriteString()`, or `display.Update()` in [`SimpleSampler.cpp`](../SimpleSampler.cpp).
