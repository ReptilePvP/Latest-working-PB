#ifndef UI_H
#define UI_H

#include "globals.h" // Include common definitions

// --- Style Initialization ---
void initStyles();

// --- Screen Creation Functions ---
void createLoadingScreen();
void updateLoadingProgress(lv_timer_t* timer); // Belongs with loading screen logic
void createLockScreen();
void createMainMenu();
void createSettingsScreen();
void createViewLogsScreen();
void createGenderMenu();
void createApparelTypeMenu();
void createColorMenuShirt();
void createPantsTypeMenu();
void createColorMenuPants();
void createShoeStyleMenu();
void createColorMenuShoes();
void createItemMenu();
void createConfirmScreen();
void createWiFiManagerScreen();
void createWiFiScreen();
void createNetworkDetailsScreen(const String& ssid);
void createSoundSettingsScreen();
void createBrightnessSettingsScreen();
void createPowerManagementScreen();
void createDateSelectionScreen();
void createTimeSelectionScreen();
void showWiFiKeyboard();
void showWiFiLoadingScreen(const String& ssid);
void updateWiFiLoadingScreen(bool success, const String& message); // Belongs with loading screen logic

// --- UI Update Functions ---
void addStatusBar(lv_obj_t* screen);
void updateStatus(const char* message, uint32_t color);
void addTimeDisplay(lv_obj_t *screen);
void updateTimeDisplay();
void updateLockScreenTime();

// --- UI Helper/Callback Functions (Static ones might stay in ui.cpp or move with their screen) ---
// Example:
// static void confirm_dialog_event_cb(lv_event_t* e); // Keep static in ui.cpp if only used there

#endif // UI_H