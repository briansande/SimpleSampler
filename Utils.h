#pragma once

namespace Utils {
    /**
     * Clamp a value between min and max
     * @param value The value to clamp
     * @param min Minimum allowed value
     * @param max Maximum allowed value
     * @return The clamped value
     */
    inline float clamp(float value, float min, float max) {
        return (value < min) ? min : (value > max) ? max : value;
    }
}
