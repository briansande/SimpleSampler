#include "UIManager.h"
#include "Menus.h"
#include <string.h>

// BaseMenu Implementation
BaseMenu::BaseMenu(DisplayManager* display, Sequencer* sequencer,
                   SampleLibrary* sampleLibrary, UIState* state, UIManager* uiManager)
    : display_(display)
    , sequencer_(sequencer)
    , sampleLibrary_(sampleLibrary)
    , state_(state)
    , uiManager_(uiManager)
{
}

// UIManager Implementation
UIManager::UIManager(DisplayManager* display, Sequencer* sequencer,
                     SampleLibrary* sampleLibrary)
    : display_(display)
    , sequencer_(sequencer)
    , sampleLibrary_(sampleLibrary)
    , stackDepth_(0)
    , currentMenu_(nullptr)
{
    // Initialize all menu pointers to null
    for (int i = 0; i < UIManager::NUM_SCREENS; i++) {
        menus_[i] = nullptr;
    }
}

UIManager::~UIManager()
{
    // Clean up menu instances
    for (int i = 0; i < UIManager::NUM_SCREENS; i++) {
        if (menus_[i] != nullptr) {
            delete menus_[i];
            menus_[i] = nullptr;
        }
    }
}

void UIManager::createMenus()
{
    // Create each menu instance, passing 'this' as UIManager reference
    menus_[SCREEN_MAIN_MENU] = new MainMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_GRANULAR_PLACEHOLDER] = new GranularPlaceholder(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_TRACK_SELECT] = new TrackSelectMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_TRACK_EDIT] = new TrackEditMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_SAMPLE_SELECT] = new SampleSelectMenu(display_, sequencer_, sampleLibrary_, &state_, this);
    menus_[SCREEN_SEQUENCE_EDITOR] = new SequenceEditorMenu(display_, sequencer_, sampleLibrary_, &state_, this);

    // Set current menu to main menu
    currentMenu_ = menus_[SCREEN_MAIN_MENU];
}

void UIManager::init()
{
    // Initialize UI state
    state_.init();

    // Create menu instances
    createMenus();

    // Initial render
    render();
}

void UIManager::update()
{
    // Update scrolling state when on sample select screen
    if (state_.currentScreen == SCREEN_SAMPLE_SELECT) {
        updateScrolling();
    }

    // Check if display needs updating
    if (state_.displayDirty) {
        render();
        state_.displayDirty = false;
    }
}

void UIManager::handleEncoderIncrement()
{
    if (currentMenu_ != nullptr) {
        currentMenu_->onEncoderIncrement();
        state_.displayDirty = true;
    }
}

void UIManager::handleEncoderDecrement()
{
    if (currentMenu_ != nullptr) {
        currentMenu_->onEncoderDecrement();
        state_.displayDirty = true;
    }
}

void UIManager::handleEncoderClick()
{
    if (currentMenu_ != nullptr) {
        currentMenu_->onEncoderClick();
        state_.displayDirty = true;
    }
}

void UIManager::handleEncoderHold()
{
    // Special handling: if in sequencer mode and at track select, return to main menu
    if (state_.currentMode == MODE_SEQUENCER && state_.currentScreen == SCREEN_TRACK_SELECT) {
        // Stop the sequencer when returning to main menu
        sequencer_->setRunning(false);
        setAppMode(MODE_MAIN_MENU);
        setCurrentScreen(SCREEN_MAIN_MENU);
        // Clear navigation stack when returning to main menu
        stackDepth_ = 0;
        state_.displayDirty = true;
        return;
    }

    if (currentMenu_ != nullptr) {
        currentMenu_->onEncoderHold();
        state_.displayDirty = true;
    }
}

void UIManager::handleButton1Press()
{
    if (currentMenu_ != nullptr) {
        currentMenu_->onButton1Press();
        state_.displayDirty = true;
    }
}

void UIManager::handleButton2Press()
{
    if (currentMenu_ != nullptr) {
        currentMenu_->onButton2Press();
        state_.displayDirty = true;
    }
}

void UIManager::pushScreen(ScreenType screen)
{
    // Push current screen to navigation stack
    if (stackDepth_ < MAX_STACK_DEPTH) {
        navigationStack_[stackDepth_] = state_.currentScreen;
        stackDepth_++;
    }

    // Set new current screen
    setCurrentScreen(screen);
}

void UIManager::popScreen()
{
    if (stackDepth_ > 0) {
        stackDepth_--;
        ScreenType previousScreen = navigationStack_[stackDepth_];
        setCurrentScreen(previousScreen);
    }
}

void UIManager::setCurrentScreen(ScreenType screen)
{
    state_.previousScreen = state_.currentScreen;
    state_.currentScreen = screen;

    // Update current menu pointer
    currentMenu_ = menus_[screen];
}

void UIManager::render()
{
    if (currentMenu_ != nullptr) {
        currentMenu_->render();
    }
}

void UIManager::updateScrolling()
{
    uint32_t now = System::GetNow();
    
    // Check if enough time has passed since last scroll update
    if (now - state_.lastScrollUpdate >= UIState::SCROLL_DELAY_MS) {
        state_.lastScrollUpdate = now;
        
        // Increment scroll offset
        state_.scrollOffset++;
        
        // Mark display as dirty so it will be re-rendered
        state_.displayDirty = true;
    }
}

void UIManager::setAppMode(AppMode mode)
{
    state_.currentMode = mode;
}
