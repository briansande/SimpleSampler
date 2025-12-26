#include "Voice.h"

Voice::Voice(double hardwareSampleRate)
    : hardwareSampleRate_(hardwareSampleRate),
      sample_(nullptr),
      active_(false),
      volume_(1.0f),
      speed_(1.0f)
{
    // Ticker will be initialized when start() is called
}

void Voice::start(SampleInfo* sample, float volume, float speed) {
    if (sample == nullptr || !sample->loaded) {
        return;  // Invalid sample
    }
    
    // Store sample info
    sample_ = sample;
    volume_ = volume;
    speed_ = speed;
    
    // Create a new ticker for this playback instance
    ticker_ = sample_->reader.createWavTicker(hardwareSampleRate_);
    
    // Mark voice as active
    active_ = true;
}

void Voice::stop() {
    active_ = false;
    ticker_.finished_ = true;
}

int Voice::process(int numSamples, float* outLeft, float* outRight) {
    // Don't process if inactive
    if (!active_ || sample_ == nullptr) {
        return 0;
    }
    
    // Generate audio samples using b3ReadWavFile::tick()
    sample_->reader.tick(
        &ticker_,          // Our playback position tracker
        sample_->data,      // The audio data in SDRAM
        speed_,            // Playback speed/pitch
        volume_,            // Volume
        numSamples,         // How many samples to generate
        outLeft,            // Left output buffer (adds to existing!)
        outRight            // Right output buffer (adds to existing!)
    );
    
    // Check if playback finished
    if (ticker_.finished_) {
        active_ = false;  // Voice is now idle
    }
    
    return numSamples;
}
