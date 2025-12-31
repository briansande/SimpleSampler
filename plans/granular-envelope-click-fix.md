# Granular Envelope Click Fix

## Problem

Grains in the granular synth produce clicking sounds when they start and stop. This is because the audio begins and ends abruptly without any amplitude smoothing.

## Root Cause Analysis

The `b3WavTicker` struct has built-in envelope methods:
- `env_volume()`: Linear fade from 1.0 to 0.0 (fade out only)
- `env_volume2()`: Triangular envelope (ramp up to 0.5, then ramp down to 0.0)

However, the `tick()` method in `b3ReadWavFile.cpp` does NOT apply any envelope. It simply passes the volume parameter directly to the `interpolate()` function without calling the envelope methods.

## Solution

Apply the triangular envelope (`env_volume2()`) to each grain during playback. This envelope:
- Ramps up from 0 to 1 during the first half of the grain
- Ramps down from 1 to 0 during the second half of the grain

This ensures smooth transitions at both the start and end of each grain, eliminating clicking.

## Implementation

### File: b3ReadWavFile.cpp

Modify the `tick()` method to apply the envelope:

```cpp
void b3ReadWavFile::tick(b3WavTicker *ticker, b3DataSource& dataSource, double speed, double volume, int size, float* out0, float* out1)
{
    if (ticker->finished_) 
      return;
    if (ticker->time_ < ticker->starttime_ || ticker->time_ > ticker->endtime_)
    {
        ticker->finished_ = true;
        return;
    }

    for (int xx=0;xx<size;xx++)
    {
        // Apply triangular envelope to smooth grain boundaries
        double envelope = ticker->env_volume2();
        double envelopeVolume = volume * envelope;
        
        interpolate(ticker, dataSource, speed, envelopeVolume, size, out0, out1, xx);
        ticker->time_ += ticker->rate_*speed;
        if (ticker->time_ < ticker->starttime_ || ticker->time_ > ticker->endtime_)
        {
            ticker->finished_ = true;
            return;
        }
    }
}
```

## How the Triangular Envelope Works

The `env_volume2()` method calculates the envelope value:

```cpp
double env_volume2()
{
    double frac = (time_ - starttime_) / (endtime_ - starttime_);
    if (frac > 0.5)
        return 1. - frac;
    return frac;
}
```

- `frac` is the normalized position within the grain (0.0 to 1.0)
- When `frac < 0.5` (first half): returns `frac` (0 to 1, ramping up)
- When `frac >= 0.5` (second half): returns `1 - frac` (1 to 0, ramping down)

## Benefits

1. **Eliminates clicking**: Smooth fade-in and fade-out prevents discontinuities
2. **Simple implementation**: Uses existing `env_volume2()` method
3. **No performance impact**: Minimal computational overhead
4. **Works for all grains**: Applied automatically to all grain playback

## Impact on Existing Code

- **No changes needed** to `SampleLibrary.cpp` or `SampleLibrary.h`
- **No changes needed** to `SimpleSampler.cpp`
- The envelope is applied transparently in the `tick()` method
- Regular sample playback (non-granular) will also benefit from the envelope

## Testing Checklist

- [ ] Build the project with `make clean && make`
- [ ] Program the Daisy with `make program`
- [ ] Enter granular mode
- [ ] Hold Button1 to open the gate and spawn grains
- [ ] Verify clicking is eliminated
- [ ] Test with different grain durations (short and long)
- [ ] Test with different spawn rates
- [ ] Verify regular sample playback still works correctly

## Notes

- The triangular envelope is the standard choice for granular synthesis
- If you want to disable the envelope later, you can add a parameter to control it
- The envelope is applied to ALL playback, not just granular grains
