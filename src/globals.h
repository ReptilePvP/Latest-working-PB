#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <M5Unified.h>      // Must be before lvgl
#include "m5gfx_lvgl.hpp"   // Includes lvgl.h and SemaphoreHandle_t
#include "lvgl.h"           // Include for full lv_timer_t definition (LVGL v9)
#include <WiFiManager.h>   // Uses background task now
#include <HTTPClient.h>
#include <Wire.h>
#include <WiFi.h>           // Keep for WIFI_AUTH_OPEN etc.
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <vector>           // Ensure vector is included for NetworkInfo
#include <algorithm>        // For std::sort

// --- Debug ---
#define DEBUG_ENABLED true
#define DEBUG_PRINT(x) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.println(x); }
#define DEBUG_PRINTF(x, ...) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.printf(x, ##__VA_ARGS__); }
#define DEBUG_PRINTLN(x) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.println(x); } // Added LN version

// --- Pins & Hardware ---
// SD Card pins for M5Stack CoreS3
#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN   4
#define TFT_DC          35 // Note: Also used by SD MISO, managed by library/bus switching

// I2C Addresses
#define AXP2101_ADDR 0x34
#define AW9523_ADDR  0x58

// --- Screen ---
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LV_TICK_PERIOD_MS 10

// --- WiFi ---
#define MAX_NETWORKS 5
#define DEFAULT_SSID "YourDefaultSSID"
#define DEFAULT_PASS "YourDefaultPassword"
#define WIFI_RECONNECT_INTERVAL 10000 // 10 seconds
#define MAX_WIFI_CONNECTION_ATTEMPTS 5
#define SCAN_TIMEOUT 30000 // 30 seconds

// --- SD Card ---
#define LOG_FILENAME "/loss_prevention_log.txt"
extern SPIClass SPI_SD; // Custom SPI instance for SD card
extern bool fileSystemInitialized;

// --- LVGL ---
extern SemaphoreHandle_t xGuiSemaphore; // Declared in m5gfx_lvgl.hpp, make it accessible
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_28); // Added missing font declaration
LV_IMG_DECLARE(LossPrev1); // Forward declare images
LV_IMG_DECLARE(LossPrev2);

// --- Tasking ---
#define LVGL_TASK_CORE 1
#define LVGL_TASK_PRIORITY 5
#define LVGL_STACK_SIZE 32768

// --- Data Structures ---
struct WifiNetwork {
    char ssid[33];
    char password[65];
    bool connected = false; // Added for WiFiManager integration
};

struct LogEntry {
    String text;
    time_t timestamp;
};

// Forward declaration for NetworkInfo if needed elsewhere (defined in WiFiManager.h)
// struct NetworkInfo;

// --- Global Variables (Minimize where possible) ---

// Entry State
extern String selectedGender;
extern String selectedApparelType;
extern String selectedShirtColors;
extern String selectedPantsType;
extern String selectedPantsColors;
extern String selectedShoeStyle;
extern String selectedShoesColors;
extern String selectedItem;
extern String currentEntry; // Holds the entry being built

// Date/Time Selection State
extern lv_calendar_date_t selected_date;
extern int selected_hour, selected_minute, selected_is_pm;

// WiFi State
extern WiFiManager wifiManager;
extern char selected_ssid[33];
extern char selected_password[65];
extern bool manualDisconnect;

// Settings State
extern uint8_t displayBrightness;
extern bool wifiEnabled; // Should be loaded from prefs

// Log Viewing State
extern std::vector<LogEntry> parsedLogEntries;
extern int currentLogPage;
extern int totalLogPages;
extern const int LOGS_PER_PAGE;

// UI Element Pointers (Consider grouping these in a UI struct/class later)
extern lv_obj_t* main_menu_screen;
extern lv_obj_t* lock_screen;
extern lv_obj_t* settingsScreen;
extern lv_obj_t* view_logs_screen;
extern lv_obj_t* wifi_manager_screen;
// ... other screen pointers ...
extern lv_obj_t* status_bar;
extern lv_obj_t* time_label; // Main menu time
extern lv_obj_t* g_lock_screen_datetime_label; // Lock screen time
extern lv_obj_t* current_scroll_obj; // For scroll handling
extern lv_obj_t* battery_icon; // Status bar battery icon
extern lv_obj_t* battery_label; // Status bar battery label
extern lv_obj_t* wifi_label; // Status bar WiFi label
extern lv_obj_t* wifi_screen; // Added extern
extern lv_obj_t* wifi_list; // Added extern
extern lv_obj_t* wifi_status_label; // Added extern


// WiFi UI Components (Defined in ui.cpp)
extern lv_obj_t* wifi_loading_screen;
extern lv_obj_t* wifi_loading_spinner;
extern lv_obj_t* wifi_loading_label;
extern lv_obj_t* wifi_result_label;
extern lv_obj_t* g_spinner; // Global spinner object for scan
extern lv_obj_t* wifi_keyboard;
extern lv_timer_t* scan_timer; // Global scan timer

// Styles (Declare extern, define in ui.cpp)
extern lv_style_t style_screen, style_btn, style_btn_pressed, style_title, style_text;
extern lv_style_t style_card_action, style_card_info, style_card_pressed;
// ... other styles ...

// --- Function Prototypes (Moved to specific module headers later) ---
// Setup/Loop related
void lvgl_task(void *pvParameters);
void setSystemTimeFromRTC();

// UI related (will move to ui.h)
void initStyles();
void createLoadingScreen();
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
void updateWiFiLoadingScreen(bool success, const String& message);
void addStatusBar(lv_obj_t* screen);
void updateStatus(const char* message, uint32_t color);
void addTimeDisplay(lv_obj_t *screen);
void updateTimeDisplay();
void updateLockScreenTime();

// WiFi related (will move to wifi_handler.h)
void onWiFiStatus(WiFiState state, const String& message);
void onWiFiScanComplete(const std::vector<NetworkInfo>& results);
void sendWebhook(const String& entry);
// void connectToSavedNetworks(); // Handled by WiFiManager now

// SD Log related (will move to sd_logger.h)
void initFileSystem();
bool appendToLog(const String& entry);
void saveEntry(const String& entry);
time_t parseTimestamp(const String& entry);
String getTimestamp();
String getFormattedEntry(const String& entry);
void listSavedEntries(); // Maybe rename to loadLogEntries

// Time related (will move to time_utils.h)
// void setSystemTimeFromRTC(); // Already declared above
// String getTimestamp(); // Already declared above
// time_t parseTimestamp(const String& entry); // Already declared above

// Power related (will move to power_utils.h)
// void createPowerManagementScreen(); // Already declared above

// Misc
void releaseSPIBus();


#endif // GLOBALS_H
