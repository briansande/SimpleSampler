#include "AudioEngine.h"

AudioEngine::AudioEngine(SampleLibrary* library)
    : library_(library),
      activeVoiceCount_(0)
{
    // Initialize all voice pointers to nullptr
    // They will be allocated in init()
    for (int i = 0; i < MAX_VOICES; i++) {
        voices_[i] = nullptr;
    }
}

AudioEngine::~AudioEngine() {
    // Clean up all allocated voices
    for (int i = 0; i < MAX_VOICES; i++) {
        delete voices_[i];
        voices_[i] = nullptr;
    }
}

bool AudioEngine::init(double sampleRate) {
    // Allocate and initialize all voices in the pool
    for (int i = 0; i < MAX_VOICES; i++) {
        voices_[i] = new Voice(sampleRate);
    }
    
    return true;
}

Voice* AudioEngine::findFreeVoice() {
    // Find first inactive voice
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices_[i]->isActive()) {
            return voices_[i];
        }
    }
    return nullptr;  // All voices are busy!
}

void AudioEngine::triggerSample(int sampleIndex, float volume, float speed) {
    // Get the sample
    SampleInfo* sample = library_->getSample(sampleIndex);
    if (sample == nullptr) {
        return;  // Invalid sample index
    }
    
    // Find a free voice
    Voice* voice = findFreeVoice();
    if (voice == nullptr) {
        return;  // Voice stealing not implemented yet
    }
    
    // Start playback
    voice->start(sample, volume, speed);
}

void AudioEngine::playSample(int index, float volume, float speed) {
    triggerSample(index, volume, speed);
}

void AudioEngine::stopAll() {
    for (int i = 0; i < MAX_VOICES; i++) {
        voices_[i]->stop();
    }
    activeVoiceCount_ = 0;
}

void AudioEngine::audioCallback(float** out, size_t size) {
    // Clear output buffers (start with silence)
    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;  // Left channel
        out[1][i] = 0.0f;  // Right channel
        out[2][i] = 0.0f;  // Channel 2
        out[3][i] = 0.0f;  // Channel 3
    }
    
    // Process all active voices
    activeVoiceCount_ = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices_[i]->isActive()) {
            activeVoiceCount_++;
            
            // Generate audio samples and add to output
            voices_[i]->process(size, out[0], out[1]);
        }
    }
}

