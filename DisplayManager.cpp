#include "DisplayManager.h"

/**
 * Constructor - Initialize references to display and hardware
 * 
 * Note: We use member initializer lists (display_(display), hw_(hw))
 * which is the preferred way to initialize reference members in C++.
 * References cannot be assigned after construction, so they MUST be
 * initialized in the initializer list.
 */
DisplayManager::DisplayManager(MyOledDisplay& display, DaisyPod& hw)
    : display_(display), hw_(hw) {
    // Nothing else to initialize - references are set above
}

/**
 * Display a message with a delay
 * 
 * This is the function you originally had in SimpleSampler.cpp.
 * Now it's encapsulated in the DisplayManager class.
 * 
 * @param message The text to display
 * @param delayMs How long to show the message (in milliseconds)
 */
void DisplayManager::showMessage(const char* message, uint32_t delayMs) {
    display_.Fill(false);                      // Clear the screen (black)
    display_.SetCursor(0, 0);                   // Position at top-left
    display_.WriteString((char*)message, Font_7x10, true);  // Write white text
    display_.Update();                          // Send to OLED
    hw_.DelayMs(delayMs);                       // Wait for specified time
}

/**
 * Update the display
 * Call this after any drawing operations to make them visible
 */
void DisplayManager::update() {
    display_.Update();
}

/**
 * Clear the display
 * @param fill If true, fill with white; if false, fill with black
 */
void DisplayManager::clear(bool fill) {
    display_.Fill(fill);
}

/**
 * Set cursor position for text writing
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 */
void DisplayManager::setCursor(uint8_t x, uint8_t y) {
    display_.SetCursor(x, y);
}

/**
 * Write text at the current cursor position
 * @param text The text to write
 * @param font Pointer to font data (e.g., Font_7x10, Font_6x8, Font_11x18)
 * @param on If true, text is white; if false, text is black
 */
void DisplayManager::writeString(const char* text, FontDef font, bool on) {
    display_.WriteString((char*)text, font, on);
}
