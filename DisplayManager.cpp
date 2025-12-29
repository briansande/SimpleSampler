#include "DisplayManager.h"
#include <cstdarg>  // For va_list, va_start, va_end
#include "Constants.h"

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
 * Supports automatic line wrapping and manual line breaks using "*":
 * - Automatic: wraps to next line when text exceeds screen width (128px)
 * - Manual: use "*" character to force a new line
 *
 * @param message The text to display
 * @param delayMs How long to show the message (in milliseconds)
 */
void DisplayManager::showMessage(const char* message, uint32_t delayMs) {
    display_.Fill(false);                      // Clear the screen (black)
    
    uint8_t cursorX = 0;                       // Current X position
    uint8_t cursorY = 0;                       // Current Y position
    uint8_t charsOnLine = 0;                   // Characters written on current line
    
    display_.SetCursor(cursorX, cursorY);      // Position at top-left
    
    // Iterate through each character in the message
    for (int i = 0; message[i] != '\0'; i++) {
        char c = message[i];
        
        // Check for manual line break character "*"
        if (c == '*') {
            // Move to next line
            cursorY += Constants::Display::LINE_HEIGHT;
            cursorX = 0;
            charsOnLine = 0;
            
            // Check if we've exceeded screen height
            if (cursorY >= Constants::Display::HEIGHT) {
                break;  // Can't display more text
            }
            
            display_.SetCursor(cursorX, cursorY);
            continue;  // Skip writing the "*" character
        }
        
        // Check if we need automatic line wrapping
        if (charsOnLine >= Constants::Display::MAX_CHARS_PER_LINE) {
            // Move to next line
            cursorY += Constants::Display::LINE_HEIGHT;
            cursorX = 0;
            charsOnLine = 0;
            
            // Check if we've exceeded screen height
            if (cursorY >= Constants::Display::HEIGHT) {
                break;  // Can't display more text
            }
            
            display_.SetCursor(cursorX, cursorY);
        }
        
        // Write the character
        char charBuffer[2] = {c, '\0'};
        display_.WriteString(charBuffer, Font_7x10, true);
        
        // Update cursor position
        cursorX += Constants::Display::CHAR_WIDTH;
        charsOnLine++;
    }
    
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

/**
 * Display a formatted message with a delay (printf-style)
 *
 * This function allows you to display multiple variables using format specifiers.
 * Common format specifiers:
 *   %s - string
 *   %d - integer
 *   %f - float
 *   %c - character
 *   %u - unsigned integer
 *   %x - hexadecimal
 *
 * @param format Printf-style format string (use "*" for manual line breaks)
 * @param delayMs How long to show the message (in milliseconds)
 * @param ... Variable arguments matching format specifiers
 */
void DisplayManager::showMessagef(const char* format, uint32_t delayMs, ...) {
    char buffer[256];  // Buffer to hold the formatted string
    
    // Use va_list to handle variable arguments
    va_list args;
    va_start(args, delayMs);
    
    // Format the string into the buffer
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Clean up variable arguments
    va_end(args);
    
    // Call the regular showMessage with the formatted string
    showMessage(buffer, delayMs);
}
