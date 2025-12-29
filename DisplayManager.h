#pragma once

#include "daisy_pod.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;

// Forward declaration of the display type
using MyOledDisplay = OledDisplay<SSD130x4WireSpi128x64Driver>;

/**
 * DisplayManager - Handles all OLED display operations
 * 
 * This class encapsulates display functionality, making it easy to share
 * across different parts of the application while maintaining clean
 * separation of concerns.
 */
class DisplayManager {
public:
    /**
     * Constructor - Takes references to the display and hardware objects
     * 
     * @param display Reference to the OLED display object
     * @param hw Reference to the DaisyPod hardware object (needed for delays)
     */
    DisplayManager(MyOledDisplay& display, DaisyPod& hw);

    /**
     * Display a message on the OLED screen with a delay
     *
     * @param message The text message to display
     * @param delayMs How long to display the message in milliseconds
     */
    void showMessage(const char* message, uint32_t delayMs);

    /**
     * Display a formatted message on the OLED screen with a delay
     * Works like printf - supports format specifiers (e.g., %s, %d, %f)
     *
     * @param format Printf-style format string (e.g., "Value: %d*Name: %s")
     * @param delayMs How long to display the message in milliseconds
     * @param ... Variable arguments for format string
     */
    void showMessagef(const char* format, uint32_t delayMs, ...);

    /**
     * Update the display (call this after any drawing operations)
     */
    void update();

    /**
     * Clear the display
     * @param fill Whether to fill with white (true) or black (false)
     */
    void clear(bool fill = false);

    /**
     * Set cursor position for text
     * @param x X coordinate (0-127)
     * @param y Y coordinate (0-63)
     */
    void setCursor(uint8_t x, uint8_t y);

    /**
     * Write text at current cursor position
     * @param text The text to write
     * @param font The font to use
     * @param on Whether text is on (white) or off (black)
     */
    void writeString(const char* text, FontDef font, bool on = true);

    /**
     * Get direct access to the display object for advanced operations
     * @return Reference to the underlying display
     */
    MyOledDisplay& getDisplay() { return display_; }

private:
    MyOledDisplay& display_;  // Reference to the OLED display
    DaisyPod& hw_;             // Reference to hardware (for delays)
};
