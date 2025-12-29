# Task 3: Remove Unused Variables - Implementation Plan

## Overview

This plan details the removal of two unused variables from `SimpleSampler.cpp` as part of the ongoing code refactoring effort for the SimpleSampler project.

## Analysis Summary

### Variable 1: `inc`
- **Location**: Line 33 in `SimpleSampler.cpp`
- **Declaration**: `static int32_t inc;`
- **Assignment**: Line 93: `inc = 0;`
- **Usage**: Never read, only written once
- **Verification**: Confirmed via code search that `inc` appears only twice (declaration and assignment), with no other references in any `.cpp` or `.h` files

### Variable 2: `lastUpdateTime`
- **Location**: Line 95 in `SimpleSampler.cpp` (Note: refactor.md states line 96, but actual line is 95 due to previous refactoring changes)
- **Declaration**: `uint32_t lastUpdateTime = System::GetNow();`
- **Usage**: Never referenced after declaration
- **Verification**: Confirmed via code search that `lastUpdateTime` appears only once (declaration), with no other references in any `.cpp` or `.h` files

## Implementation Steps

### Step 1: Remove `inc` Variable
**File**: `SimpleSampler.cpp`

**Action 1.1**: Delete line 33
```cpp
static int32_t  inc;
```

**Action 1.2**: Delete line 93 (the assignment)
```cpp
inc     = 0;
```

**Note**: After removing line 33, line 93 will shift to line 92. The coding agent should search for the actual content rather than relying solely on line numbers.

### Step 2: Remove `lastUpdateTime` Variable
**File**: `SimpleSampler.cpp`

**Action 2.1**: Delete line 95
```cpp
uint32_t lastUpdateTime = System::GetNow();             // Initialize lastUpdateTime to the current time
```

**Note**: This includes the inline comment which is no longer relevant after variable removal.

### Step 3: Verify Changes
After removing the variables, verify:
1. No compiler warnings about unused variables remain
2. Code compiles successfully with `make clean && make`
3. No functionality is affected (this is a cleanup-only change)

## Code Context

### Before Removal (Lines 88-96)
```cpp
int main(void)
{
    hw.Init();
    // Set the sample rate now that hw is initialized
    Config::samplerate = hw.AudioSampleRate();
    inc     = 0;

    uint32_t lastUpdateTime = System::GetNow();             // Initialize lastUpdateTime to the current time

    /** Configure then initialize the Display */
```

### After Removal (Expected Result)
```cpp
int main(void)
{
    hw.Init();
    // Set the sample rate now that hw is initialized
    Config::samplerate = hw.AudioSampleRate();

    /** Configure then initialize the Display */
```

## Search Patterns for Coding Agent

The coding agent should use these search patterns to locate the exact content to remove:

1. **For `inc` declaration**:
   - Pattern: `static int32_t  inc;`
   - Location: Around line 33 (file scope, before SD card declarations)

2. **For `inc` assignment**:
   - Pattern: `inc     = 0;`
   - Location: Around line 93 (inside `main()` function, after `Config::samplerate` assignment)

3. **For `lastUpdateTime`**:
   - Pattern: `uint32_t lastUpdateTime = System::GetNow();`
   - Location: Around line 95 (inside `main()` function, after `inc` assignment)

## Risk Assessment

**Risk Level**: LOW

**Rationale**:
- These variables are confirmed to be unused via comprehensive code search
- No other code references these variables
- Removal is purely a cleanup operation with no functional impact
- Previous refactoring tasks (1 and 2) have been completed successfully

## Testing Checklist

After implementation, verify:

- [ ] Code compiles without errors: `make clean && make`
- [ ] No new warnings are introduced
- [ ] All features still work as expected (test on Daisy hardware if possible)
- [ ] Task 3 in `refactor.md` is marked as complete `[x]`

## Dependencies

- **Prerequisites**: None (independent task)
- **Blocked By**: None
- **Blocking**: None (can be done in parallel with other independent tasks)

## Files to Modify

| File | Lines to Remove | Change Type |
|------|-----------------|-------------|
| `SimpleSampler.cpp` | 33, 93, 95 | Delete |

## Notes for Implementer

1. **Line Number Variance**: The refactor.md document mentions line 96 for `lastUpdateTime`, but actual analysis shows it's on line 95. This discrepancy is likely due to previous refactoring changes (Tasks 1 and 2). Use content-based search rather than absolute line numbers.

2. **Whitespace Preservation**: When removing lines, maintain proper code formatting and blank lines to keep the code clean and readable.

3. **Verification**: After removal, perform a fresh search for `inc` and `lastUpdateTime` to confirm complete removal.

## Completion Criteria

Task is complete when:
1. Both unused variables are removed from `SimpleSampler.cpp`
2. Code compiles without errors
3. No unused variable warnings remain
4. Task 3 in `refactor.md` is marked as complete
