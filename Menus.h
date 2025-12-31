#pragma once

#include "UIManager.h"
#include "DisplayManager.h"
#include "Sequencer.h"
#include "SampleLibrary.h"
#include "Constants.h"

// Forward declaration of UIManager (to avoid circular dependency)
class UIManager;

/**
 * MainMenu - Main menu screen for mode selection
 *
 * Displays Granular Synth and Step Sequencer options.
 * Encoder navigates between options, click enters selected mode.
 */
class MainMenu : public BaseMenu {
private:
    enum class Option {
        GRANULAR,
        SEQUENCER
    };
    Option selectedOption_;

public:
    // Constructor
    MainMenu(DisplayManager* display, Sequencer* sequencer,
             SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Render main menu options
    void render() override;

    // Navigate between options
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;

    // Enter selected mode
    void onEncoderClick() override;
};

/**
 * GranularSynthMenu - Granular synthesizer mode screen
 *
 * Displays current sample, gate status, active grain count, and selected parameter.
 * Button1 (hold): Open gate to spawn grains
 * Button2: Cycle through loaded samples
 * Encoder turn: Adjust selected parameter value
 * Encoder click: Cycle through parameters (Spawn Rate, Duration, Speed, Position)
 * Hold encoder: Return to main menu
 */
class GranularSynthMenu : public BaseMenu {
private:
    int granularSampleIndex_;  // Currently selected sample for granular
    
    // Parameter selection enum
    enum class GranularParam {
        SPAWN_RATE,  // Grains per second
        DURATION,    // Grain duration in seconds
        SPEED,       // Playback speed multiplier
        POSITION      // Start position within sample (0.0-1.0)
    };
    
    GranularParam selectedParam_;  // Currently selected parameter

public:
    // Constructor
    GranularSynthMenu(DisplayManager* display, Sequencer* sequencer,
                      SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Render granular synth screen
    void render() override;

    // Encoder turn: Adjust selected parameter value
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;

    // Encoder click: Cycle through parameters
    void onEncoderClick() override;

    // Return to main menu on hold
    void onEncoderHold() override;

    // Button1: Open gate (grains spawn while held)
    void onButton1Press() override;

    // Button2: Cycle to next sample
    void onButton2Press() override;
};

/**
 * TrackSelectMenu - Screen to select a track for editing
 *
 * Displays all 3 tracks with their current sample assignment.
 * Encoder navigates between tracks, click enters track edit.
 */
class TrackSelectMenu : public BaseMenu {
private:
    int selectedIndex_;  // Currently selected track (0-2)

public:
    // Constructor
    TrackSelectMenu(DisplayManager* display, Sequencer* sequencer,
                    SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Render track list
    void render() override;

    // Navigate between tracks
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;

    // Enter track edit
    void onEncoderClick() override;
};

/**
 * TrackEditMenu - Screen to edit track settings
 *
 * Shows options for the selected track:
 * - Sample (enter sample selection)
 * - Sequence (enter sequence editor)
 */
class TrackEditMenu : public BaseMenu {
private:
    enum class Option {
        Sample,
        Sequence
    };

    Option selectedOption_;  // Currently selected option

public:
    // Constructor
    TrackEditMenu(DisplayManager* display, Sequencer* sequencer,
                  SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Render track options
    void render() override;

    // Navigate between options
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;

    // Enter selected option
    void onEncoderClick() override;

    // Go back to track select
    void onEncoderHold() override;
};

/**
 * SampleSelectMenu - Screen to select a sample for a track
 *
 * Lists available samples from SampleLibrary.
 * Supports scrolling for large sample lists.
 */
class SampleSelectMenu : public BaseMenu {
private:
    int selectedIndex_;      // Currently selected sample
    int windowStart_;        // First sample visible in window
    static const int ITEMS_PER_SCREEN = 4;  // Samples shown at once (kept as is - not in Constants.h)

    // Update window for scrolling
    void updateWindow();

    // Reset horizontal scroll offset when selection changes
    void resetScroll();

public:
    // Constructor
    SampleSelectMenu(DisplayManager* display, Sequencer* sequencer,
                     SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Render sample list
    void render() override;

    // Navigate between samples
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;

    // Select sample and assign to track
    void onEncoderClick() override;

    // Go back to track edit
    void onEncoderHold() override;
};

/**
 * SequenceEditorMenu - Screen to edit 16-step pattern
 *
 * Displays 16 steps in a grid layout.
 * Current step is highlighted.
 * Encoder selects step, buttons activate/deactivate.
 */
class SequenceEditorMenu : public BaseMenu {
private:
    int selectedStep_;       // Currently selected step (0-15)
    static const int STEPS_PER_ROW = 8;  // 8 steps per row (2 rows)

public:
    // Constructor
    SequenceEditorMenu(DisplayManager* display, Sequencer* sequencer,
                      SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);

    // Render step grid
    void render() override;

    // Navigate between steps
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;

    // Exit to track edit
    void onEncoderClick() override;

    // Exit to track edit
    void onEncoderHold() override;

    // Activate selected step
    void onButton1Press() override;

    // Deactivate selected step
    void onButton2Press() override;
};
