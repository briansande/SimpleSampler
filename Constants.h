#pragma once

#include <cstdint>  // For uint8_t, uint32_t, size_t

namespace Constants {
    // Display Constants
    namespace Display {
        constexpr uint8_t WIDTH = 128;
        constexpr uint8_t HEIGHT = 64;
        constexpr uint8_t CHAR_WIDTH = 7;
        constexpr uint8_t LINE_HEIGHT = 10;
        constexpr uint8_t MAX_CHARS_PER_LINE = WIDTH / CHAR_WIDTH;  // ~18 chars
        constexpr uint32_t FPS = 3;
    }

    // UI Constants
    namespace UI {
        constexpr uint32_t HOLD_DETECT_MS = 500;
        constexpr float MIN_BPM = 60.0f;
        constexpr float MAX_BPM = 180.0f;
        constexpr float BPM_RANGE = MAX_BPM - MIN_BPM;
    }

    // Memory Constants
    namespace Memory {
        constexpr size_t CUSTOM_POOL_SIZE = 48 * 1024 * 1024;  // 48MB
    }

    // Sequencer Constants
    namespace Sequencer {
        constexpr int NUM_STEPS = 16;
        constexpr int NUM_TRACKS = 3;
        constexpr int MIN_BPM = 60;
        constexpr int MAX_BPM = 180;
    }

    // Sample Library Constants
    namespace SampleLibrary {
        constexpr int MAX_SAMPLES = 64;
        constexpr int MAX_GRAINS = 8;  // Maximum simultaneous grains (reduced for embedded safety)
    }

    // Granular Randomness Constants
    namespace Granular {
        constexpr float SPAWN_RATE_RANDOM_STEP = 1.0f;   // 1 grain/sec step
        constexpr float DURATION_RANDOM_STEP = 0.01f;      // 0.01 seconds step
        constexpr float SPEED_RANDOM_STEP = 0.1f;          // 0.1x step
        constexpr float POSITION_RANDOM_STEP = 0.01f;       // 0.01 step
        
        constexpr float SPAWN_RATE_RANDOM_MAX = 50.0f;    // Max randomness for spawn rate
        constexpr float DURATION_RANDOM_MAX = 0.5f;        // Max randomness for duration
        constexpr float SPEED_RANDOM_MAX = 2.0f;           // Max randomness for speed
        constexpr float POSITION_RANDOM_MAX = 0.5f;        // Max randomness for position
    }
}
