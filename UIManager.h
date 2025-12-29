#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "DisplayManager.h"
#include "Sequencer.h"
#include "SampleLibrary.h"
#include "daisy_core.h"

/**
 * ScreenType - Enum defining all available screen types in the UI
 * 
 * Each screen represents a different menu or view in the UI system.
 */
enum ScreenType {
    SCREEN_TRACK_SELECT,      // List all 3 tracks for selection
    SCREEN_TRACK_EDIT,        // Edit selected track (sample/sequence options)
    SCREEN_SAMPLE_SELECT,     // Choose sample for track
    SCREEN_SEQUENCE_EDITOR    // Edit 16-step pattern
};

/**
 * UIState - Holds all UI state information
 * 
 * Contains current screen, selection indices, navigation stack,
 * and display update timing.
 */
struct UIState {
    // Current Screen
    ScreenType currentScreen;
    ScreenType previousScreen;   // For navigation stack

    // Selection State
    int selectedTrack;           // Currently selected track index (0-2)
    int selectedStep;            // Currently selected step (0-15)
    int selectedSample;          // Currently selected sample index
    int windowStart;             // For scrolling sample list

    // Navigation State
    bool encoderPressed;         // Track encoder press state
    uint32_t encoderPressTime;   // For detecting long press (hold)
    bool encoderHeld;            // Encoder is being held

    // Display Update Timing
    uint32_t lastDisplayUpdate;
    bool displayDirty;           // Flag to trigger display refresh

    // Horizontal Text Scrolling State
    int scrollOffset;            // Current character offset for scrolling (0 = start)
    uint32_t lastScrollUpdate;  // Timestamp of last scroll position update
    static const uint32_t SCROLL_DELAY_MS = 200;  // Milliseconds per character scroll

    // Initialization
    void init() {
        currentScreen = SCREEN_TRACK_SELECT;
        previousScreen = SCREEN_TRACK_SELECT;
        selectedTrack = 0;
        selectedStep = 0;
        selectedSample = 0;
        windowStart = 0;
        encoderPressed = false;
        encoderPressTime = 0;
        encoderHeld = false;
        lastDisplayUpdate = 0;
        displayDirty = true;
        scrollOffset = 0;
        lastScrollUpdate = 0;
    }
};

/**
 * BaseMenu - Abstract base class for all menu screens
 *
 * Defines the interface that all menu screens must implement.
 * Each menu handles its own rendering and input processing.
 */
class UIManager;  // Forward declaration

class BaseMenu {
protected:
    DisplayManager* display_;
    Sequencer* sequencer_;
    SampleLibrary* sampleLibrary_;
    UIState* state_;
    UIManager* uiManager_;  // Reference to UIManager for navigation

public:
    // Constructor
    BaseMenu(DisplayManager* display, Sequencer* sequencer,
             SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Virtual destructor
    virtual ~BaseMenu() = default;

    // Render the menu screen
    virtual void render() = 0;

    // Handle encoder rotation (right turn)
    virtual void onEncoderIncrement() = 0;

    // Handle encoder rotation (left turn)
    virtual void onEncoderDecrement() = 0;

    // Handle encoder click (enter/select)
    virtual void onEncoderClick() = 0;

    // Handle encoder hold (long press - exit)
    virtual void onEncoderHold() = 0;

    // Handle button1 press
    virtual void onButton1Press() = 0;

    // Handle button2 press
    virtual void onButton2Press() = 0;
};

/**
 * UIManager - Main UI manager for navigation and display rendering
 * 
 * Manages the menu system, processes encoder/button input,
 * and renders all screens to the OLED display.
 * 
 * Maintains a navigation stack for proper exit behavior
 * and delegates input handling to the current active menu.
 */
class UIManager {
private:
    DisplayManager* display_;
    Sequencer* sequencer_;
    SampleLibrary* sampleLibrary_;
    UIState state_;

    // Navigation stack (simple array for tracking history)
    static const int MAX_STACK_DEPTH = 8;
    ScreenType navigationStack_[MAX_STACK_DEPTH];
    int stackDepth_;

    // Menu instances (owned by UIManager)
    BaseMenu* currentMenu_;
    BaseMenu* menus_[4];  // One for each ScreenType

    // Helper to create menu instances
    void createMenus();

    // Update horizontal text scrolling state
    void updateScrolling();

public:
    // Constructor
    UIManager(DisplayManager* display, Sequencer* sequencer,
             SampleLibrary* sampleLibrary);

    // Destructor
    ~UIManager();

    // Initialize UI system
    void init();

    // Update UI (call from main loop)
    void update();

    // Handle encoder input
    void handleEncoderIncrement();
    void handleEncoderDecrement();
    void handleEncoderClick();
    void handleEncoderHold();

    // Handle button presses
    void handleButton1Press();
    void handleButton2Press();

    // Navigation stack management
    void pushScreen(ScreenType screen);
    void popScreen();
    ScreenType getCurrentScreen() const { return state_.currentScreen; }

    // Render current screen
    void render();

    // Get UI state
    const UIState& getState() const { return state_; }

    // Set current screen directly (for navigation)
    void setCurrentScreen(ScreenType screen);
};

#endif // UI_MANAGER_H
