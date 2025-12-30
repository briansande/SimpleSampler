# Task 8: Update Hardcoded Menu Array Size - Implementation Plan

## Overview
Replace the hardcoded array size `4` with a named constant `NUM_SCREENS` to improve code readability and maintainability.

## Analysis

### Current State
- [`UIManager.h:138`](UIManager.h:138): `BaseMenu* menus_[4];` - Array declaration with magic number
- [`UIManager.cpp:26`](UIManager.cpp:26): `for (int i = 0; i < 4; i++)` - Constructor initialization loop
- [`UIManager.cpp:34`](UIManager.cpp:34): `for (int i = 0; i < 4; i++)` - Destructor cleanup loop

### ScreenType Enum Values
The `ScreenType` enum has exactly 4 values (lines 13-18 in UIManager.h):
1. `SCREEN_TRACK_SELECT`
2. `SCREEN_TRACK_EDIT`
3. `SCREEN_SAMPLE_SELECT`
4. `SCREEN_SEQUENCE_EDITOR`

### Why This Change Is Needed
- Magic numbers reduce code readability
- Hardcoded values make maintenance harder if screen count changes
- Named constants make the code self-documenting

## Implementation Steps

### Step 1: Add the NUM_SCREENS Constant to UIManager.h
**File**: [`UIManager.h`](UIManager.h)

**Location**: In the `UIManager` class private section, before the `menus_` array declaration (around line 137)

**Change**:
```cpp
// Add this line before line 138:
static constexpr int NUM_SCREENS = 4;
```

**Context**:
```cpp
private:
    DisplayManager* display_;
    Sequencer* sequencer_;
    SampleLibrary* sampleLibrary_;
    UIState state_;

    // Navigation stack (simple array for tracking history)
    static const int MAX_STACK_DEPTH = 8;
    ScreenType navigationStack_[MAX_STACK_DEPTH];
    int stackDepth_;

    // Menu instances (owned by UIManager)
    BaseMenu* currentMenu_;
    static constexpr int NUM_SCREENS = 4;  // <-- ADD THIS LINE
    BaseMenu* menus_[4];  // One for each ScreenType
```

### Step 2: Update the Array Declaration in UIManager.h
**File**: [`UIManager.h`](UIManager.h)

**Location**: Line 138

**Change**:
```cpp
// Before:
BaseMenu* menus_[4];  // One for each ScreenType

// After:
BaseMenu* menus_[NUM_SCREENS];  // One for each ScreenType
```

### Step 3: Update Constructor Loop in UIManager.cpp
**File**: [`UIManager.cpp`](UIManager.cpp)

**Location**: Lines 26-28 in the constructor

**Change**:
```cpp
// Before:
for (int i = 0; i < 4; i++) {
    menus_[i] = nullptr;
}

// After:
for (int i = 0; i < NUM_SCREENS; i++) {
    menus_[i] = nullptr;
}
```

### Step 4: Update Destructor Loop in UIManager.cpp
**File**: [`UIManager.cpp`](UIManager.cpp)

**Location**: Lines 34-38 in the destructor

**Change**:
```cpp
// Before:
for (int i = 0; i < 4; i++) {
    if (menus_[i] != nullptr) {
        delete menus_[i];
        menus_[i] = nullptr;
    }
}

// After:
for (int i = 0; i < NUM_SCREENS; i++) {
    if (menus_[i] != nullptr) {
        delete menus_[i];
        menus_[i] = nullptr;
    }
}
```

## Technical Considerations

### Why `static constexpr`?
- `static`: The constant is shared across all instances of `UIManager`
- `constexpr`: The value is known at compile time, allowing for optimizations
- `int`: Simple integer type appropriate for array sizes

### Constant Placement
- Placed in the private section since it's an internal implementation detail
- Not exposed publicly as external code doesn't need this information
- Positioned before its first use for readability

### Access in UIManager.cpp
- Can be referenced as `NUM_SCREENS` within class methods
- Or as `UIManager::NUM_SCREENS` if needed outside class methods
- No additional includes or modifications required

## Verification Steps

After implementation, verify:

1. **Compilation**: `make clean && make` should succeed without errors
2. **No new warnings**: Check that no compiler warnings are introduced
3. **Functionality**: All UI screens should still work correctly
4. **Hardware test**: `make program` and verify on Daisy hardware

## Edge Cases & Risks

### Identified Risks: None
- This is a straightforward refactoring with no functional changes
- The constant value (4) exactly matches the number of `ScreenType` enum values
- `static constexpr` ensures compile-time evaluation with zero runtime overhead

### Future Considerations
- If new screen types are added to `ScreenType`, `NUM_SCREENS` must be updated
- Consider using `sizeof(ScreenType)` or similar if the enum grows dynamic

## Files Modified

| File | Lines Changed | Type of Change |
|------|---------------|----------------|
| [`UIManager.h`](UIManager.h) | 137-138 | Add constant, update array declaration |
| [`UIManager.cpp`](UIManager.cpp) | 26, 34 | Update loop conditions |

## Completion Checklist

- [ ] Add `static constexpr int NUM_SCREENS = 4;` to UIManager.h private section
- [ ] Update `menus_[4]` to `menus_[NUM_SCREENS]` in UIManager.h
- [ ] Update constructor loop condition in UIManager.cpp
- [ ] Update destructor loop condition in UIManager.cpp
- [ ] Verify compilation: `make clean && make`
- [ ] Test on hardware: `make program`
- [ ] Mark Task 8 as complete in refactor.md

## Related Tasks

This task is independent of other refactoring tasks and can be completed in any order.
