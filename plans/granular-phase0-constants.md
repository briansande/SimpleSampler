# Phase 0: Constants

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Add granular-specific constants to the project configuration.

---

## Step 0.1: Add MAX_GRAINS to Constants.h

**File:** `Constants.h`

Add to the `SampleLibrary` namespace:

```cpp
// Sample Library Constants
namespace SampleLibrary {
    constexpr int MAX_SAMPLES = 64;
    constexpr int MAX_GRAINS = 8;  // Maximum simultaneous grains (reduced for embedded safety)
}
```

---

## Why 8 grains?

The `b3WavTicker` struct contains `std::vector<double> lastFrame_` which uses dynamic memory allocation. Each grain creates a new vector allocation, which can cause memory fragmentation on the embedded Daisy platform.

**Reasons for 8 grains:**
- Sufficient granular density for most textures
- Minimizes memory fragmentation risk
- Reduces CPU load for real-time audio processing
- Can be increased later if memory permits

**If you need more grains:**
- Monitor memory usage and CPU load
- Consider pre-allocating `lastFrame_` buffers (requires modifying `b3WavTicker`)
- Increase `MAX_GRAINS` gradually (8 → 12 → 16 → 24)

---

## Success Criteria

- [x] `MAX_GRAINS` constant is defined in `Constants.h`
- [x] Value is 8 (or your chosen value)
- [x] Code compiles without errors
