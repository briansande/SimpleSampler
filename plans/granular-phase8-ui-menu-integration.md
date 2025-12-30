# Phase 8: UI Menu Integration

**Parent Plan:** [`granular-synth-main.md`](granular-synth-main.md)

---

## Goal

Create UI menus for granular parameter control.

---

## Step 8.1: Add Granular Screen Type

**File:** `UIManager.h`

Add to `ScreenType` enum:

```cpp
enum ScreenType {
    SCREEN_MAIN_MENU,         // Main menu: select Granular or Sequencer
    SCREEN_GRANULAR_PLACEHOLDER,  // Placeholder screen for granular synth
    SCREEN_GRANULAR_PARAMS,    // Granular parameter control screen (NEW)
    SCREEN_TRACK_SELECT,      // List all 3 tracks for selection
    SCREEN_TRACK_EDIT,        // Edit selected track (sample/sequence options)
    SCREEN_SAMPLE_SELECT,     // Choose sample for track
    SCREEN_SEQUENCE_EDITOR    // Edit 16-step pattern
};
```

---

## Step 8.2: Create GranularParamsMenu Class

**File:** `Menus.h`

Add new class:

```cpp
class GranularParamsMenu : public BaseMenu {
private:
    int selectedParam_;  // Which parameter is selected (0-7)
    
    // Parameter names for display
    const char* paramNames_[8] = {
        "Position",
        "Duration",
        "Spawn Rate",
        "Pitch",
        "Pos Rand",
        "Dur Rand",
        "Pitch Rand",
        "Freeze"
    };
    
public:
    GranularParamsMenu(DisplayManager* display, Sequencer* sequencer,
                      SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager);
    
    void render() override;
    void onEncoderIncrement() override;
    void onEncoderDecrement() override;
    void onEncoderClick() override;
    void onEncoderHold() override;
};
```

---

## Step 8.3: Implement GranularParamsMenu

**File:** `Menus.cpp`

Implement the menu:

```cpp
GranularParamsMenu::GranularParamsMenu(DisplayManager* display, Sequencer* sequencer,
                                     SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : BaseMenu(display, sequencer, sampleLibrary, state, uiManager),
      selectedParam_(0) {
}

void GranularParamsMenu::render() {
    display_->clear();
    
    // Title
    display_->setCursor(0, 0);
    display_->writeString("Granular Params", Font_7x10);
    
    // Show active grain count
    display_->setCursor(0, 12);
    char grainCountMsg[32];
    snprintf(grainCountMsg, sizeof(grainCountMsg), "Grains: %d", sampleLibrary_->getActiveGrainCount());
    display_->writeString(grainCountMsg, Font_7x10);
    
    // Show parameters
    for (int i = 0; i < 8; i++) {
        int yPos = 24 + (i * 6);
        if (yPos > 58) break;  // Don't go off screen
        
        display_->setCursor(0, yPos);
        
        // Show selection indicator
        if (i == selectedParam_) {
            display_->writeString(">", Font_7x10);
        } else {
            display_->writeString(" ", Font_7x10);
        }
        
        // Show parameter name and value
        char paramMsg[32];
        switch (i) {
            case 0:  // Position
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.2f", paramNames_[i], sampleLibrary_->getGrainPosition());
                break;
            case 1:  // Duration
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.3f", paramNames_[i], sampleLibrary_->getGrainDuration());
                break;
            case 2:  // Spawn Rate
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.1f", paramNames_[i], sampleLibrary_->getSpawnRate());
                break;
            case 3:  // Pitch
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.2f", paramNames_[i], sampleLibrary_->getGrainPitch());
                break;
            case 4:  // Position Randomness
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.2f", paramNames_[i], sampleLibrary_->getPositionRandomness());
                break;
            case 5:  // Duration Randomness
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.2f", paramNames_[i], sampleLibrary_->getDurationRandomness());
                break;
            case 6:  // Pitch Randomness
                snprintf(paramMsg, sizeof(paramMsg), "%s: %.2f", paramNames_[i], sampleLibrary_->getPitchRandomness());
                break;
            case 7:  // Freeze
                snprintf(paramMsg, sizeof(paramMsg), "%s: %s", paramNames_[i], sampleLibrary_->getFreezeMode() ? "ON" : "OFF");
                break;
        }
        display_->writeString(paramMsg, Font_7x10);
    }
    
    display_->update();
}

void GranularParamsMenu::onEncoderIncrement() {
    // Cycle to next parameter
    selectedParam_ = (selectedParam_ + 1) % 8;
}

void GranularParamsMenu::onEncoderDecrement() {
    // Cycle to previous parameter
    selectedParam_ = (selectedParam_ - 1 + 8) % 8;
}

void GranularParamsMenu::onEncoderClick() {
    // Toggle boolean parameters or increment numeric parameters
    switch (selectedParam_) {
        case 0:  // Position
            {
                double newPos = sampleLibrary_->getGrainPosition() + 0.1;
                if (newPos > 1.0) newPos = 0.0;
                sampleLibrary_->setGrainPosition(newPos);
            }
            break;
        case 1:  // Duration
            {
                double newDur = sampleLibrary_->getGrainDuration() + 0.05;
                if (newDur > 0.5) newDur = 0.01;
                sampleLibrary_->setGrainDuration(newDur);
            }
            break;
        case 2:  // Spawn Rate
            {
                double newRate = sampleLibrary_->getSpawnRate() + 5.0;
                if (newRate > 100.0) newRate = 1.0;
                sampleLibrary_->setSpawnRate(newRate);
            }
            break;
        case 3:  // Pitch
            {
                double newPitch = sampleLibrary_->getGrainPitch() + 0.1;
                if (newPitch > 2.0) newPitch = 0.5;
                sampleLibrary_->setGrainPitch(newPitch);
            }
            break;
        case 4:  // Position Randomness
            {
                double newRand = sampleLibrary_->getPositionRandomness() + 0.1;
                if (newRand > 1.0) newRand = 0.0;
                sampleLibrary_->setPositionRandomness(newRand);
            }
            break;
        case 5:  // Duration Randomness
            {
                double newRand = sampleLibrary_->getDurationRandomness() + 0.1;
                if (newRand > 1.0) newRand = 0.0;
                sampleLibrary_->setDurationRandomness(newRand);
            }
            break;
        case 6:  // Pitch Randomness
            {
                double newRand = sampleLibrary_->getPitchRandomness() + 0.1;
                if (newRand > 1.0) newRand = 0.0;
                sampleLibrary_->setPitchRandomness(newRand);
            }
            break;
        case 7:  // Freeze
            sampleLibrary_->setFreezeMode(!sampleLibrary_->getFreezeMode());
            break;
    }
}

void GranularParamsMenu::onEncoderHold() {
    // Return to main menu
    uiManager_->setAppMode(MODE_MAIN_MENU);
    uiManager_->setCurrentScreen(SCREEN_MAIN_MENU);
    uiManager_->getState().displayDirty = true;
}
```

---

## Step 8.4: Update MainMenu to Navigate to Granular Params

**File:** `Menus.cpp`

Update `MainMenu::onEncoderClick()`:

```cpp
void MainMenu::onEncoderClick() {
    if (state_->selectedOption == 0) {
        // Granular Synthesis
        uiManager_->setAppMode(MODE_GRANULAR);
        uiManager_->setCurrentScreen(SCREEN_GRANULAR_PARAMS);
        
        // Enable granular mode
        sampleLibrary_->setGranularMode(true);
        
        // Set default sample if needed
        if (sampleLibrary_->getSampleCount() > 0) {
            sampleLibrary_->setGranularSampleIndex(0);
        }
    } else if (state_->selectedOption == 1) {
        // Sequencer
        uiManager_->setAppMode(MODE_SEQUENCER);
        uiManager_->pushScreen(SCREEN_TRACK_SELECT);
    }
    
    state_->displayDirty = true;
}
```

---

## Step 8.5: Update UIManager to Create Granular Menu

**File:** `UIManager.cpp`

Update `createMenus()`:

```cpp
void UIManager::createMenus() {
    // Create each menu instance, passing 'this' as UIManager reference
    menus_[SCREEN_MAIN_MENU] = new MainMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_GRANULAR_PLACEHOLDER] = new GranularPlaceholder(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_GRANULAR_PARAMS] = new GranularParamsMenu(display_, sequencer_, sampleLibrary_, &state_, this);  // NEW
    menus_[SCREEN_TRACK_SELECT] = new TrackSelectMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_TRACK_EDIT] = new TrackEditMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_SAMPLE_SELECT] = new SampleSelectMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_SEQUENCE_EDITOR] = new SequenceEditorMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    
    // Set current menu to main menu
    currentMenu_ = menus_[SCREEN_MAIN_MENU];
}
```

Update `NUM_SCREENS` constant:

```cpp
static constexpr int NUM_SCREENS = 7;  // Updated from 6 to 7
```

---

## Success Criteria

- [ ] Granular parameter menu is accessible from main menu
- [ ] Parameters can be adjusted via encoder
- [ ] Values are displayed in real-time
- [ ] Hold encoder returns to main menu
- [ ] Granular mode is enabled when entering granular menu
