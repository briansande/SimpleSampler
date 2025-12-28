#include "Metronome.h"

// Default parameters
static constexpr float DEFAULT_FREQUENCY = 800.0f;  // Hz
static constexpr float DEFAULT_DURATION = 0.01f;   // 10ms
static constexpr float DEFAULT_VOLUME = 0.5f;

// Envelope time constants (in seconds)
static constexpr float ATTACK_TIME = 0.001f;   // 1ms - fast attack
static constexpr float DECAY_TIME = 0.009f;   // 9ms - short decay
static constexpr float SUSTAIN_LEVEL = 0.0f;  // No sustain
static constexpr float RELEASE_TIME = 0.0f;  // No release

Metronome::Metronome()
    : volume_(DEFAULT_VOLUME)
    , frequency_(DEFAULT_FREQUENCY)
    , duration_(DEFAULT_DURATION)
    , sampleRate_(48000.0f)
{
}

void Metronome::init(float sampleRate)
{
    // Store sample rate
    sampleRate_ = sampleRate;

    // Initialize oscillator as sine wave
    osc_.Init(sampleRate_);
    osc_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    osc_.SetFreq(frequency_);
    osc_.SetAmp(1.0f);

    // Initialize ADSR envelope
    // Fast attack (1ms), short decay (9ms), no sustain, no release
    // Total duration = attack + decay = 10ms
    env_.Init(sampleRate_);
    env_.SetTime(daisysp::ADSR_SEG_ATTACK, ATTACK_TIME);
    env_.SetTime(daisysp::ADSR_SEG_DECAY, DECAY_TIME);
    env_.SetSustainLevel(SUSTAIN_LEVEL);
    env_.SetTime(daisysp::ADSR_SEG_RELEASE, RELEASE_TIME);
}

void Metronome::trigger()
{
    // Retrigger the envelope to start a new click
    // Using hard=false to avoid clicking
    env_.Retrigger(false);
}

void Metronome::process(float** out, size_t size)
{
    // Process each sample
    for (size_t i = 0; i < size; i++) {
        // Get envelope value
        // Gate is false since we want the envelope to decay naturally
        float envValue = env_.Process(false);

        // Generate oscillator output
        float oscValue = osc_.Process();

        // Apply envelope to oscillator output
        float clickSample = oscValue * envValue;

        // Apply volume control
        clickSample *= volume_;

        // Mix into both output channels
        out[0][i] += clickSample;  // Left channel
        out[1][i] += clickSample;  // Right channel
    }
}

void Metronome::setVolume(float volume)
{
    // Clamp volume to valid range
    if (volume < 0.0f) {
        volume_ = 0.0f;
    } else if (volume > 1.0f) {
        volume_ = 1.0f;
    } else {
        volume_ = volume;
    }
}

void Metronome::setFrequency(float freq)
{
    frequency_ = freq;
    osc_.SetFreq(frequency_);
}

void Metronome::setDuration(float duration)
{
    duration_ = duration;

    // Adjust envelope decay time to match desired duration
    // Keep attack time fixed, adjust decay to achieve total duration
    float newDecay = duration_ - ATTACK_TIME;

    // Ensure decay is at least a small positive value
    if (newDecay < 0.001f) {
        newDecay = 0.001f;
    }

    env_.SetTime(daisysp::ADSR_SEG_DECAY, newDecay);
}
