#ifndef VOICE_H
#define VOICE_H

#include "b3ReadWavFile.h"
#include "SampleLibrary.h"

/**
 * Voice: Manages ONE playback instance of a sample.
 * 
 * For polyphony, you create multiple Voice objects, each playing
 * potentially the same sample at different times.
 */
class Voice {
private:
    b3WavTicker ticker_;       // Tracks playback position
    SampleInfo* sample_;         // Which sample we're playing
    bool active_;                 // Is this voice currently playing?
    float volume_;                // Playback volume (0.0 to 1.0)
    float speed_;                 // Playback speed/pitch (1.0 = normal)
    double hardwareSampleRate_;   // Hardware sample rate (e.g., 48000)

public:
    // Constructor
    Voice(double hardwareSampleRate);
    
    // Start playing a sample
    void start(SampleInfo* sample, float volume = 1.0f, float speed = 1.0f);
    
    // Stop playback immediately
    void stop();
    
    // Generate audio samples (called from audio callback)
    // Returns number of samples generated (0 if voice is inactive)
    int process(int numSamples, float* outLeft, float* outRight);
    
    // Check if voice is currently playing
    bool isActive() const { return active_; }
    
    // Get/set volume
    void setVolume(float volume) { volume_ = volume; }
    float getVolume() const { return volume_; }
    
    // Get/set speed
    void setSpeed(float speed) { speed_ = speed; }
    float getSpeed() const { return speed_; }
    
    // Get current playback position (in sample frames)
    double getPosition() const { return ticker_.time_; }
    
    // Check if playback finished
    bool isFinished() const { return ticker_.finished_; }
};

#endif // VOICE_H
