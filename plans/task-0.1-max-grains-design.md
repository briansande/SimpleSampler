# Task 0.1: MAX_GRAINS Constant Addition - Design Document

**Phase:** Phase 0 - Constants  
**Task:** 0.1  
**File to Modify:** `Constants.h`

---

## Overview

Add the `MAX_GRAINS` constant to the `SampleLibrary` namespace in [`Constants.h`](../Constants.h) to define the maximum number of simultaneous grains for granular synthesis.

---

## Current State

The [`Constants.h`](../Constants.h) file currently contains:

```cpp
// Sample Library Constants
namespace SampleLibrary {
    constexpr int MAX_SAMPLES = 64;
}
```

Located at **lines 37-40**.

---

## Proposed Change

### Location
- **File:** `Constants.h`
- **Line:** After line 39 (after `MAX_SAMPLES = 64;`)
- **Namespace:** `Constants::SampleLibrary`

### Code to Add

```cpp
    constexpr int MAX_GRAINS = 8;  // Maximum simultaneous grains (reduced for embedded safety)
```

### Complete Modified Section

```cpp
    // Sample Library Constants
    namespace SampleLibrary {
        constexpr int MAX_SAMPLES = 64;
        constexpr int MAX_GRAINS = 8;  // Maximum simultaneous grains (reduced for embedded safety)
    }
```

---

## Implementation Considerations

### Why Value of 8?

The value of `8` is chosen for embedded safety reasons:

1. **Memory Fragmentation:** The `b3WavTicker` struct uses `std::vector<double> lastFrame_` which allocates memory dynamically. Each grain creates a new allocation. On embedded platforms, excessive allocations can cause fragmentation.

2. **CPU Load:** Real-time audio processing on Daisy has limited CPU cycles. Fewer grains reduce processing overhead.

3. **Sufficient Density:** 8 simultaneous grains provide enough density for most granular textures (clouds, pads, textures).

4. **Future Scalability:** The value can be increased later (8 → 12 → 16 → 24) if memory and CPU monitoring permits.

### Code Style Notes

- Uses `constexpr int` matching the style of `MAX_SAMPLES`
- Includes inline comment explaining the embedded safety rationale
- Placed immediately after `MAX_SAMPLES` for logical grouping

---

## Success Criteria

- [ ] `MAX_GRAINS` constant is defined in `Constants.h` within the `SampleLibrary` namespace
- [ ] Value is set to `8`
- [ ] Code compiles without errors using `make clean && make`
- [ ] No changes to other constants or namespaces

---

## Verification Steps

After implementation:

1. Run `make clean && make` to verify compilation
2. Check that no warnings or errors related to the new constant appear
3. Verify the constant can be accessed as `Constants::SampleLibrary::MAX_GRAINS`

---

## Next Steps

This is a standalone task. Upon successful implementation, proceed to Phase 0, Task 0.2 (if any) or Phase 1 tasks as outlined in [`granular-phase0-constants.md`](granular-phase0-constants.md).
