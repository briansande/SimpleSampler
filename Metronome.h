#pragma once

#include "daisysp.h"

/**
 * Metronome - Synthesized click/beep using daisysp oscillator
 *
 * Generates a simple click sound synchronized with sequencer steps.
 * Uses a sine wave oscillator with ADSR envelope for short decay.
 * The click is triggered by the Sequencer on each step.
 */
class Metronome {
public:
    /**
     * Constructor
     */
    Metronome();

    /**
     * Initialize the metronome with sample rate
     * Sets up the oscillator and envelope with default parameters
     *
     * @param sampleRate Audio sample rate (typically 48000 Hz)
     */
    void init(float sampleRate);

    /**
     * Trigger a click sound
     * Resets the envelope to start a new click
     */
    void trigger();

    /**
     * Process audio and mix into output buffers
     * Generates the click sound and adds it to both output channels
     *
     * @param out Output buffer array (2 channels: left, right)
     * @param size Number of samples to process
     */
    void process(float** out, size_t size);

    /**
     * Set the output volume
     *
     * @param volume Volume level (0.0 to 1.0)
     */
    void setVolume(float volume);

    /**
     * Get current volume
     *
     * @return Current volume (0.0 to 1.0)
     */
    float getVolume() const { return volume_; }

    /**
     * Set the click frequency
     *
     * @param freq Frequency in Hz (default ~800Hz)
     */
    void setFrequency(float freq);

    /**
     * Get current frequency
     *
     * @return Current frequency in Hz
     */
    float getFrequency() const { return frequency_; }

    /**
     * Set the click duration in seconds
     * This affects the envelope decay time
     *
     * @param duration Duration in seconds (default ~0.01s)
     */
    void setDuration(float duration);

    /**
     * Get current duration
     *
     * @return Current duration in seconds
     */
    float getDuration() const { return duration_; }

private:
    daisysp::Oscillator osc_;    // Oscillator for click sound (sine wave)
    daisysp::Adsr env_;           // ADSR envelope for short decay
    float volume_;                // Output volume (0.0 - 1.0)
    float frequency_;             // Click frequency in Hz
    float duration_;              // Click duration in seconds
    float sampleRate_;            // Sample rate for calculations
};
