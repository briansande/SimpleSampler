#include "Menus.h"
#include "UIManager.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// BaseMenu Helper Functions
// ============================================================================

void BaseMenu::renderSelectionIndicator(int yPos, bool isSelected)
{
    display_->setCursor(0, yPos);
    display_->writeString(isSelected ? ">" : " ", Font_7x10);
}

// ============================================================================
// MainMenu Implementation
// ============================================================================

MainMenu::MainMenu(DisplayManager* display, Sequencer* sequencer,
                  SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager)
    , selectedOption_(Option::SEQUENCER)
{
}

void MainMenu::render()
{
    display_->clear();

    // Display header
    display_->setCursor(0, 0);
    display_->writeString("MAIN MENU", Font_7x10);

    // Display options
    int yPos = 20;

    // Granular option
    renderSelectionIndicator(yPos, selectedOption_ == Option::GRANULAR);
    display_->setCursor(8, yPos);
    display_->writeString("Granular Synth", Font_7x10);

    // Sequencer option
    yPos = 32;
    renderSelectionIndicator(yPos, selectedOption_ == Option::SEQUENCER);
    display_->setCursor(8, yPos);
    display_->writeString("Step Sequencer", Font_7x10);

    // Display footer
    display_->setCursor(0, 54);
    display_->writeString("Click to Select*", Font_7x10);

    display_->update();
}

void MainMenu::onEncoderIncrement()
{
    selectedOption_ = (selectedOption_ == Option::GRANULAR) ? Option::SEQUENCER : Option::GRANULAR;
}

void MainMenu::onEncoderDecrement()
{
    selectedOption_ = (selectedOption_ == Option::SEQUENCER) ? Option::GRANULAR : Option::SEQUENCER;
}

void MainMenu::onEncoderClick()
{
    // Enter selected mode
    if (selectedOption_ == Option::GRANULAR) {
        uiManager_->setAppMode(MODE_GRANULAR);
        uiManager_->setCurrentScreen(SCREEN_GRANULAR_PLACEHOLDER);
    } else if (selectedOption_ == Option::SEQUENCER) {
        uiManager_->setAppMode(MODE_SEQUENCER);
        uiManager_->setCurrentScreen(SCREEN_TRACK_SELECT);
    }
}


// ============================================================================
// GranularPlaceholder Implementation
// ============================================================================

GranularPlaceholder::GranularPlaceholder(DisplayManager* display, Sequencer* sequencer,
                                         SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager)
{
}

void GranularPlaceholder::render()
{
    display_->clear();

    // Display header
    display_->setCursor(0, 0);
    display_->writeString("GRANULAR SYNTH", Font_7x10);

    // Display message
    display_->setCursor(0, 24);
    display_->writeString("Coming Soon...", Font_7x10);

    // Display footer
    display_->setCursor(0, 54);
    display_->writeString("Hold: Main Menu*", Font_7x10);

    display_->update();
}

void GranularPlaceholder::onEncoderIncrement()
{
    // No-op - no navigation in placeholder
}

void GranularPlaceholder::onEncoderDecrement()
{
    // No-op - no navigation in placeholder
}

void GranularPlaceholder::onEncoderClick()
{
    // No-op - no action on click in placeholder
}

void GranularPlaceholder::onEncoderHold()
{
    // Return to main menu
    uiManager_->setAppMode(MODE_MAIN_MENU);
    uiManager_->setCurrentScreen(SCREEN_MAIN_MENU);
}


// ============================================================================
// TrackSelectMenu Implementation
// ============================================================================

TrackSelectMenu::TrackSelectMenu(DisplayManager* display, Sequencer* sequencer,
                                 SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager)
    , selectedIndex_(0)
{
}

void TrackSelectMenu::render()
{
    display_->clear();

    // Display header
    display_->setCursor(0, 0);
    display_->writeString("TRACK SELECT", Font_7x10);

    // Display track list
    for (int i = 0; i < 3; i++) {
        int yPos = 12 + (i * 12);

        // Show selection indicator
        renderSelectionIndicator(yPos, i == selectedIndex_);

        // Get track info
        const Track* track = sequencer_->getTrack(i);
        char trackLine[32];

        if (track->sampleIndex >= 0) {
            const SampleInfo* sample = sampleLibrary_->getSample(track->sampleIndex);
            if (sample && sample->loaded) {
                snprintf(trackLine, sizeof(trackLine), "Track %d: %s", i + 1, sample->name);
            } else {
                snprintf(trackLine, sizeof(trackLine), "Track %d: None", i + 1);
            }
        } else {
            snprintf(trackLine, sizeof(trackLine), "Track %d: None", i + 1);
        }

        display_->setCursor(8, yPos);
        display_->writeString(trackLine, Font_7x10);
    }

    // Display footer
    display_->setCursor(0, 54);
    display_->writeString("Click to Edit*", Font_7x10);

    display_->update();
}

void TrackSelectMenu::onEncoderIncrement()
{
    selectedIndex_ = (selectedIndex_ + 1) % 3;
    state_->selectedTrack = selectedIndex_;
}

void TrackSelectMenu::onEncoderDecrement()
{
    selectedIndex_ = (selectedIndex_ - 1 + 3) % 3;
    state_->selectedTrack = selectedIndex_;
}

void TrackSelectMenu::onEncoderClick()
{
    // Store selected track and navigate to track edit menu
    state_->selectedTrack = selectedIndex_;
    uiManager_->pushScreen(SCREEN_TRACK_EDIT);
}


// ============================================================================
// TrackEditMenu Implementation
// ============================================================================

TrackEditMenu::TrackEditMenu(DisplayManager* display, Sequencer* sequencer,
                              SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager)
    , selectedOption_(Option::Sample)
{
}

void TrackEditMenu::render()
{
    display_->clear();

    // Display header
    display_->setCursor(0, 0);
    char header[32];
    snprintf(header, sizeof(header), "TRACK %d EDIT", state_->selectedTrack + 1);
    display_->writeString(header, Font_7x10);

    // Get track info for sample name display
    const Track* track = sequencer_->getTrack(state_->selectedTrack);
    char sampleName[32] = "None";
    if (track->sampleIndex >= 0) {
        const SampleInfo* sample = sampleLibrary_->getSample(track->sampleIndex);
        if (sample && sample->loaded) {
            strncpy(sampleName, sample->name, sizeof(sampleName) - 1);
            sampleName[sizeof(sampleName) - 1] = '\0';
        }
    }

    // Display options
    int yPos = 12;

    // Sample option
    renderSelectionIndicator(yPos, selectedOption_ == Option::Sample);
    display_->setCursor(8, yPos);
    char sampleLine[32];
    snprintf(sampleLine, sizeof(sampleLine), "Sample: %s", sampleName);
    display_->writeString(sampleLine, Font_7x10);

    // Sequence option
    yPos = 24;
    renderSelectionIndicator(yPos, selectedOption_ == Option::Sequence);
    display_->setCursor(8, yPos);
    display_->writeString("Sequence", Font_7x10);

    // Display footer
    display_->setCursor(0, 54);
    display_->writeString("Click: Enter*", Font_7x10);
    display_->setCursor(0, 64);
    display_->writeString("Hold: Back*", Font_7x10);

    display_->update();
}

void TrackEditMenu::onEncoderIncrement()
{
    selectedOption_ = (selectedOption_ == Option::Sample) ? Option::Sequence : Option::Sample;
}

void TrackEditMenu::onEncoderDecrement()
{
    selectedOption_ = (selectedOption_ == Option::Sequence) ? Option::Sample : Option::Sequence;
}

void TrackEditMenu::onEncoderClick()
{
    // Navigate to selected submenu
    if (selectedOption_ == Option::Sample) {
        uiManager_->pushScreen(SCREEN_SAMPLE_SELECT);
    } else if (selectedOption_ == Option::Sequence) {
        uiManager_->pushScreen(SCREEN_SEQUENCE_EDITOR);
    }
}

void TrackEditMenu::onEncoderHold()
{
    // Navigate back to track select
    uiManager_->popScreen();
}


// ============================================================================
// SampleSelectMenu Implementation
// ============================================================================

SampleSelectMenu::SampleSelectMenu(DisplayManager* display, Sequencer* sequencer,
                                    SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager)
    , selectedIndex_(0)
    , windowStart_(0)
{
}

void SampleSelectMenu::updateWindow()
{
    int numSamples = sampleLibrary_->getSampleCount();

    // Adjust window start if selection is out of view
    if (selectedIndex_ < windowStart_) {
        windowStart_ = selectedIndex_;
    } else if (selectedIndex_ >= windowStart_ + ITEMS_PER_SCREEN) {
        windowStart_ = selectedIndex_ - ITEMS_PER_SCREEN + 1;
    }

    // Ensure window doesn't go beyond bounds
    if (windowStart_ + ITEMS_PER_SCREEN > numSamples) {
        windowStart_ = (numSamples > ITEMS_PER_SCREEN) ? numSamples - ITEMS_PER_SCREEN : 0;
    }
}

void SampleSelectMenu::resetScroll()
{
    // Reset scroll offset to start
    state_->scrollOffset = 0;
    state_->lastScrollUpdate = System::GetNow();
}

void SampleSelectMenu::render()
{
    display_->clear();

    // Display header
    display_->setCursor(0, 0);
    display_->writeString("SELECT SAMPLE", Font_7x10);

    int numSamples = sampleLibrary_->getSampleCount();

    // Update window for scrolling
    updateWindow();

    // Display sample list
    for (int i = 0; i < ITEMS_PER_SCREEN; i++) {
        int sampleIndex = windowStart_ + i;
        if (sampleIndex >= numSamples) {
            break;
        }

        int yPos = 12 + (i * 12);

        // Show selection indicator
        renderSelectionIndicator(yPos, sampleIndex == selectedIndex_);

        // Get sample name
        const SampleInfo* sample = sampleLibrary_->getSample(sampleIndex);
        if (sample && sample->loaded) {
            display_->setCursor(8, yPos);
            
            // For selected sample, implement horizontal scrolling
            if (sampleIndex == selectedIndex_) {
                const char* sampleName = sample->name;
                int nameLength = strlen(sampleName);
                
                // Calculate how many characters we can display
                int availableChars = nameLength - state_->scrollOffset;
                
                if (availableChars > 0) {
                    // Create substring starting from scroll offset
                    char displayBuffer[32];
                    int charsToCopy = (availableChars > Constants::Display::MAX_CHARS_PER_LINE) ? Constants::Display::MAX_CHARS_PER_LINE : availableChars;
                    
                    // Copy characters starting from scroll offset
                    strncpy(displayBuffer, sampleName + state_->scrollOffset, charsToCopy);
                    displayBuffer[charsToCopy] = '\0';
                    
                    display_->writeString(displayBuffer, Font_7x10);
                }
                // If scroll offset is beyond name length, reset it
                else {
                    resetScroll();
                    display_->writeString(sample->name, Font_7x10);
                }
            } else {
                // For non-selected samples, display full name normally
                display_->writeString(sample->name, Font_7x10);
            }
        }
    }

    // Display footer with position
    display_->setCursor(0, 54);
    char footer[32];
    snprintf(footer, sizeof(footer), "%d/%d*", selectedIndex_ + 1, numSamples);
    display_->writeString(footer, Font_7x10);
    display_->setCursor(0, 64);
    display_->writeString("Hold: Back*", Font_7x10);

    display_->update();
}

void SampleSelectMenu::onEncoderIncrement()
{
    int numSamples = sampleLibrary_->getSampleCount();
    if (numSamples > 0) {
        selectedIndex_ = (selectedIndex_ + 1) % numSamples;
        state_->selectedSample = selectedIndex_;
        // Reset scroll offset when selection changes
        resetScroll();
    }
}

void SampleSelectMenu::onEncoderDecrement()
{
    int numSamples = sampleLibrary_->getSampleCount();
    if (numSamples > 0) {
        selectedIndex_ = (selectedIndex_ - 1 + numSamples) % numSamples;
        state_->selectedSample = selectedIndex_;
        // Reset scroll offset when selection changes
        resetScroll();
    }
}

void SampleSelectMenu::onEncoderClick()
{
    // Assign selected sample to current track
    sequencer_->setTrackSample(state_->selectedTrack, selectedIndex_);
    // Navigate back to track edit
    uiManager_->popScreen();
}

void SampleSelectMenu::onEncoderHold()
{
    // Navigate back to track edit
    uiManager_->popScreen();
}


// ============================================================================
// SequenceEditorMenu Implementation
// ============================================================================

SequenceEditorMenu::SequenceEditorMenu(DisplayManager* display, Sequencer* sequencer,
                                        SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager)
    , selectedStep_(0)
{
}

void SequenceEditorMenu::render()
{
    display_->clear();

    // Display header
    display_->setCursor(0, 0);
    char header[32];
    snprintf(header, sizeof(header), "TRACK %d PATTERN", state_->selectedTrack + 1);
    display_->writeString(header, Font_7x10);

    // Get track steps
    const Track* track = sequencer_->getTrack(state_->selectedTrack);

    // Display step pattern (2 rows of 8 steps each)
    // Row 1: Steps 0-7
    int yPos = 12;
    for (int i = 0; i < 8; i++) {
        int xPos = (i * 16) + 4;
        if (track->steps[i]) {
            display_->setCursor(xPos, yPos);
            display_->writeString("X", Font_7x10);
        } else {
            display_->setCursor(xPos, yPos);
            display_->writeString(".", Font_7x10);
        }
    }

    // Row 2: Steps 8-15
    yPos = 24;
    for (int i = 8; i < 16; i++) {
        int xPos = ((i - 8) * 16) + 4;
        if (track->steps[i]) {
            display_->setCursor(xPos, yPos);
            display_->writeString("X", Font_7x10);
        } else {
            display_->setCursor(xPos, yPos);
            display_->writeString(".", Font_7x10);
        }
    }

    // Show selected step indicator
    int selectedRow = (selectedStep_ < 8) ? 12 : 24;
    int selectedCol = (selectedStep_ < 8) ? selectedStep_ : (selectedStep_ - 8);
    int selectedX = (selectedCol * 16);
    display_->setCursor(selectedX, selectedRow + 10);
    display_->writeString("^", Font_7x10);

    // Display footer
    display_->setCursor(0, 54);
    char footer[32];
    snprintf(footer, sizeof(footer), "Step: %d*", selectedStep_);
    display_->writeString(footer, Font_7x10);
    display_->setCursor(0, 64);
    display_->writeString("B1:ON B2:OFF*", Font_7x10);

    display_->update();
}

void SequenceEditorMenu::onEncoderIncrement()
{
    selectedStep_ = (selectedStep_ + 1) % 16;
    state_->selectedStep = selectedStep_;
}

void SequenceEditorMenu::onEncoderDecrement()
{
    selectedStep_ = (selectedStep_ - 1 + 16) % 16;
    state_->selectedStep = selectedStep_;
}

void SequenceEditorMenu::onEncoderClick()
{
    // Navigate back to track edit
    uiManager_->popScreen();
}

void SequenceEditorMenu::onEncoderHold()
{
    // Navigate back to track edit
    uiManager_->popScreen();
}

void SequenceEditorMenu::onButton1Press()
{
    // Activate selected step
    sequencer_->setStepActive(state_->selectedTrack, selectedStep_, true);
}

void SequenceEditorMenu::onButton2Press()
{
    // Deactivate selected step
    sequencer_->setStepActive(state_->selectedTrack, selectedStep_, false);
}
