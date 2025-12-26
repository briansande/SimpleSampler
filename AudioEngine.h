#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include "Voice.h"
#include "SampleLibrary.h"

// Maximum number of voices playing simultaneously
#define MAX_VOICES 16

/**
 * AudioEngine: Manages voice pool and real-time audio processing.
 * 
 * This is the core of your sampler - it ties together:
 * - SampleLibrary (sample data)
 * - Voice pool (polyphony)
 * - Audio callback (real-time processing)
 */
class AudioEngine {
private:
    Voice* voices_[MAX_VOICES];     // Pool of voice pointers (to avoid default constructor)
    SampleLibrary* library_;           // Access to loaded samples
    int activeVoiceCount_;             // How many voices are playing
    
    // Helper: Find an inactive voice from pool
    Voice* findFreeVoice();
    
    // Helper: Trigger a sample playback
    void triggerSample(int sampleIndex, float volume = 1.0f, float speed = 1.0f);

public:
    // Constructor
    AudioEngine(SampleLibrary* library);
    
    // Initialize the engine - allocates and initializes all voices
    bool init(double sampleRate);
    
    // Destructor - clean up allocated voices
    ~AudioEngine();
    
    // Audio callback: Called by hardware 48,000 times per second!
    // This is the REAL-TIME function - must be fast!
    void audioCallback(float** out, size_t size);
    
    // Get number of active voices
    int getActiveVoiceCount() const { return activeVoiceCount_; }
    
    // Trigger sample by index
    void playSample(int index, float volume = 1.0f, float speed = 1.0f);
    
    // Stop all voices
    void stopAll();
};

#endif // AUDIO_ENGINE_H
