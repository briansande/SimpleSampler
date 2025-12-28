#include "Sequencer.h"
#include <string.h>

Sequencer::Sequencer(SampleLibrary* sampleLibrary, int sampleRate)
    : sampleLibrary_(sampleLibrary)
    , sampleRate_(sampleRate)
    , samplesSinceLastStep_(0)
{
}

void Sequencer::init()
{
    // Initialize the sequencer state
    state_.init();

    // Calculate initial samples per step
    state_.samplesPerStep = calculateSamplesPerStep(state_.bpm);

    // Reset sample counter
    samplesSinceLastStep_ = 0;
}

uint32_t Sequencer::calculateSamplesPerStep(int bpm)
{
    // Formula: samplesPerStep = (sampleRate * 60) / (bpm * 4)
    // This gives samples per 16th note
    // - sampleRate * 60 = samples per minute
    // - bpm * 4 = 16th notes per minute (4 per quarter note)
    return (sampleRate_ * 60) / (bpm * 4);
}

void Sequencer::setBpm(float bpm)
{
    // Clamp BPM to valid range
    if (bpm < static_cast<float>(MIN_BPM)) {
        bpm = static_cast<float>(MIN_BPM);
    } else if (bpm > static_cast<float>(MAX_BPM)) {
        bpm = static_cast<float>(MAX_BPM);
    }

    state_.bpm = static_cast<int>(bpm);

    // Recalculate samples per step
    state_.samplesPerStep = calculateSamplesPerStep(state_.bpm);
}

void Sequencer::setRunning(bool running)
{
    state_.isRunning = running;

    if (running) {
        // Reset sample counter when starting
        samplesSinceLastStep_ = 0;
        state_.stepStartTime = daisy::System::GetNow();
    }
}

void Sequencer::processAudio(float** out, size_t size)
{
    // Delegate audio processing to SampleLibrary
    // The sequencer only handles timing and triggering
    sampleLibrary_->processAudio(out, size);

    // If not running, no step advancement
    if (!state_.isRunning) {
        return;
    }

    // Track samples processed in this audio callback
    samplesSinceLastStep_ += static_cast<uint32_t>(size);

    // Check if we need to advance to next step
    while (samplesSinceLastStep_ >= state_.samplesPerStep) {
        samplesSinceLastStep_ -= state_.samplesPerStep;

        // Advance to next step
        state_.currentStep = (state_.currentStep + 1) % NUM_STEPS;

        // Update step start time
        state_.stepStartTime = daisy::System::GetNow();

        // Trigger samples for active steps at the new step
        triggerStep(state_.currentStep);

        // Trigger metronome if enabled
        if (state_.metronomeEnabled) {
            triggerMetronome();
        }
    }
}

bool Sequencer::shouldTriggerTrack(int trackIndex, int step)
{
    // Check bounds
    if (trackIndex < 0 || trackIndex >= NUM_TRACKS) {
        return false;
    }
    if (step < 0 || step >= NUM_STEPS) {
        return false;
    }

    Track& track = state_.tracks[trackIndex];

    // Track triggers if:
    // 1. Step is active
    // 2. Sample is assigned
    // 3. Track is not muted
    return track.steps[step] &&
           track.sampleIndex >= 0 &&
           !track.mute;
}

void Sequencer::triggerTrack(int trackIndex)
{
    // Check bounds
    if (trackIndex < 0 || trackIndex >= NUM_TRACKS) {
        return;
    }

    Track& track = state_.tracks[trackIndex];

    // Check if sample is assigned
    if (track.sampleIndex < 0) {
        return;
    }

    // Trigger the sample via SampleLibrary
    // SampleLibrary will handle the actual playback
    sampleLibrary_->triggerSample(track.sampleIndex);

    // Mark track as playing
    track.isPlaying = true;
}

void Sequencer::triggerStep(int step)
{
    // Check all tracks and trigger those with active steps
    for (int trackIdx = 0; trackIdx < NUM_TRACKS; trackIdx++) {
        if (shouldTriggerTrack(trackIdx, step)) {
            triggerTrack(trackIdx);
        }
    }
}

void Sequencer::triggerMetronome()
{
    // Placeholder for metronome implementation
    // This will be implemented when the Metronome class is added
    // For now, this is a no-op
}

Track* Sequencer::getTrack(int index)
{
    if (index < 0 || index >= NUM_TRACKS) {
        return nullptr;
    }
    return &state_.tracks[index];
}

const Track* Sequencer::getTrack(int index) const
{
    if (index < 0 || index >= NUM_TRACKS) {
        return nullptr;
    }
    return &state_.tracks[index];
}

void Sequencer::setTrackSample(int trackIndex, int sampleIndex)
{
    Track* track = getTrack(trackIndex);
    if (track == nullptr) {
        return;
    }

    track->sampleIndex = sampleIndex;

    // Update sample name cache
    if (sampleIndex >= 0) {
        SampleInfo* sample = sampleLibrary_->getSample(sampleIndex);
        if (sample != nullptr) {
            strncpy(track->sampleName, sample->name, sizeof(track->sampleName) - 1);
            track->sampleName[sizeof(track->sampleName) - 1] = '\0';
        } else {
            track->sampleName[0] = '\0';
        }
    } else {
        track->sampleName[0] = '\0';
    }
}

void Sequencer::setStepActive(int trackIndex, int stepIndex, bool active)
{
    Track* track = getTrack(trackIndex);
    if (track == nullptr) {
        return;
    }

    if (stepIndex >= 0 && stepIndex < NUM_STEPS) {
        track->steps[stepIndex] = active;
    }
}

bool Sequencer::isStepActive(int trackIndex, int stepIndex) const
{
    const Track* track = getTrack(trackIndex);
    if (track == nullptr) {
        return false;
    }

    if (stepIndex >= 0 && stepIndex < NUM_STEPS) {
        return track->steps[stepIndex];
    }

    return false;
}

void Sequencer::reset()
{
    state_.currentStep = 0;
    samplesSinceLastStep_ = 0;
    state_.stepStartTime = daisy::System::GetNow();
}

void Sequencer::setMetronomeVolume(float volume)
{
    // Clamp volume to valid range
    if (volume < 0.0f) {
        volume = 0.0f;
    } else if (volume > 1.0f) {
        volume = 1.0f;
    }

    state_.metronomeVolume = volume;
}
