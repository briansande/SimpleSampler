#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "b3ReadWavFile.h"
#include "SampleLibrary.h"
#include "daisy_core.h"

// Sequencer Configuration
#define NUM_STEPS 16
#define NUM_TRACKS 3
#define MIN_BPM 60
#define MAX_BPM 180

/**
 * Track - Represents a single sequencer track with sample assignment and step pattern
 *
 * Each track has:
 * - A sample assigned from the SampleLibrary
 * - A 16-step pattern (active/inactive)
 * - A b3WavTicker for independent polyphonic playback
 * - Volume, mute, and solo controls
 */
struct Track {
    // Sample Assignment
    int sampleIndex;              // Index into SampleLibrary (-1 = none assigned)
    char sampleName[32];          // Cached sample name for display

    // Step Pattern (16 steps, each can be active/inactive)
    bool steps[NUM_STEPS];

    // Playback State
    b3WavTicker ticker;          // Independent ticker for polyphonic playback
    bool isPlaying;              // Is this track currently playing?

    // Track Properties
    float volume;                // Track volume (0.0 - 1.0)
    bool mute;                   // Track mute state
    bool solo;                   // Track solo state

    // Initialization
    void init() {
        sampleIndex = -1;
        sampleName[0] = '\0';
        for (int i = 0; i < NUM_STEPS; i++) {
            steps[i] = false;
        }
        isPlaying = false;
        volume = 1.0f;
        mute = false;
        solo = false;
    }
};

/**
 * SequencerState - Holds all sequencer state data
 *
 * Contains timing information, track data, and metronome settings.
 * This structure can be serialized for saving/loading patterns.
 */
struct SequencerState {
    // Timing
    int bpm;                     // Current tempo (60-180)
    int currentStep;             // Current step position (0-15)
    uint32_t stepStartTime;      // Timestamp when current step started
    uint32_t samplesPerStep;     // Number of audio samples per step
    bool isRunning;              // Sequencer running state

    // Track Data
    Track tracks[NUM_TRACKS];

    // Metronome
    bool metronomeEnabled;       // Metronome on/off
    float metronomeVolume;       // Metronome volume (0.0 - 1.0)

    // Initialization
    void init() {
        bpm = 120;               // Default BPM
        currentStep = 0;
        stepStartTime = 0;
        samplesPerStep = 0;
        isRunning = false;
        metronomeEnabled = true;
        metronomeVolume = 0.5f;

        for (int i = 0; i < NUM_TRACKS; i++) {
            tracks[i].init();
        }
    }
};

/**
 * Sequencer - Core timing and step logic for 16-step sequencer
 *
 * Manages:
 * - BPM-based timing calculation
 * - Step advancement synchronized to audio callback
 * - Track triggering when steps become active
 * - Integration with SampleLibrary for sample playback
 *
 * The sequencer handles timing and triggering, but audio mixing
 * is delegated to SampleLibrary::processAudio().
 */
class Sequencer {
private:
    SequencerState state_;
    SampleLibrary* sampleLibrary_;
    int sampleRate_;

    // Sample count tracking for step timing
    uint32_t samplesSinceLastStep_;

    // Calculate samples per step based on BPM
    // Formula: samplesPerStep = (sampleRate * 60) / (bpm * 4)
    // This gives samples per 16th note
    uint32_t calculateSamplesPerStep(int bpm);

    // Check if step should trigger for a track
    bool shouldTriggerTrack(int trackIndex, int step);

    // Trigger all active samples at current step
    void triggerStep(int step);

    // Trigger a specific track
    void triggerTrack(int trackIndex);

public:
    // Constructor
    Sequencer(SampleLibrary* sampleLibrary, int sampleRate);

    // Initialize sequencer
    void init();

    // Set BPM (clamped to MIN_BPM - MAX_BPM)
    void setBpm(float bpm);

    // Get current BPM
    float getBpm() const { return static_cast<float>(state_.bpm); }

    // Start/stop sequencer
    void setRunning(bool running);

    // Get running state
    bool isRunning() const { return state_.isRunning; }

    // Process audio callback (called from AudioCallback)
    // This handles step advancement and triggers samples
    void processAudio(float** out, size_t size);

    // Get current step index (0-15)
    int getCurrentStep() const { return state_.currentStep; }

    // Trigger metronome (for future metronome integration)
    void triggerMetronome();

    // Get track by index (0-2)
    Track* getTrack(int index);

    // Get track by index (const version)
    const Track* getTrack(int index) const;

    // Assign sample to track
    void setTrackSample(int trackIndex, int sampleIndex);

    // Set step active/inactive
    void setStepActive(int trackIndex, int stepIndex, bool active);

    // Check if step is active
    bool isStepActive(int trackIndex, int stepIndex) const;

    // Reset sequencer to step 0
    void reset();

    // Get sequencer state (for saving/loading)
    const SequencerState& getState() const { return state_; }

    // Metronome control
    void setMetronomeEnabled(bool enabled) { state_.metronomeEnabled = enabled; }
    bool isMetronomeEnabled() const { return state_.metronomeEnabled; }
    void setMetronomeVolume(float volume);
    float getMetronomeVolume() const { return state_.metronomeVolume; }
};

#endif // SEQUENCER_H
