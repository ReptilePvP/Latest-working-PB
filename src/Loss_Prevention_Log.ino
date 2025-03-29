#include <Arduino.h>
#include <M5Unified.h>      // Must be before lvgl
#include "m5gfx_lvgl.hpp"   // Includes lvgl.h and SemaphoreHandle_t
#include "lvgl.h"       // Include for full lv_timer_t definition (LVGL v9)
#include <WiFiManager.h>   // Uses background task now
#include <HTTPClient.h>
#include <Wire.h>
#include <WiFi.h>           // Keep for WIFI_AUTH_OPEN etc.
#include <Preferences.h>
#include <SPI.h>
SPIClass SPI_SD;            // Custom SPI instance for SD card
#include <SD.h>
#include <time.h>
#include <vector>           // Ensure vector is included for NetworkInfo

// Last Edited 3/27/25 (WiFi Management Refactor - Restore Includes)

// Forward declarations for loading screen
void createLoadingScreen();
void updateLoadingProgress(lv_timer_t* timer);
void createLockScreen();
void updateLockScreenTime();
LV_IMG_DECLARE(LossPrev2);
LV_IMG_DECLARE(LossPrev1);

// Forward declarations for date and time screens
static void createDateSelectionScreen();
static void createTimeSelectionScreen();
static void confirm_dialog_event_cb(lv_event_t* e);
static void save_time_to_rtc();

// Static callback handlers for date and time selection
static void on_year_change(lv_event_t* e);
static void on_month_change(lv_event_t* e);
static void on_day_change(lv_event_t* e);
static void on_hour_change(lv_event_t* e);
static void on_minute_change(lv_event_t* e);

// Static pointers to remember rollers between callbacks
static lv_obj_t* g_year_roller = nullptr;
static lv_obj_t* g_month_roller = nullptr;
static lv_obj_t* g_day_roller = nullptr;
static lv_obj_t* g_selected_date_label = nullptr;
static lv_obj_t* g_hour_roller = nullptr;
static lv_obj_t* g_minute_roller = nullptr;
static lv_obj_t* g_am_pm_roller = nullptr;
static lv_obj_t* g_selected_time_label = nullptr;
lv_obj_t* lock_screen = nullptr; // <<< ADDED: Pointer for the lock screen


// Add these constants near the top of your file with other defines
#define AXP2101_ADDR 0x34
#define AW9523_ADDR 0x58

// UI elements
lv_obj_t* battery_icon = nullptr;
lv_obj_t* battery_label = nullptr;
lv_obj_t* wifi_label = nullptr; // For main menu wifi strength %
lv_obj_t* time_label = nullptr; // New time display label (Main Menu)
lv_obj_t* g_lock_screen_datetime_label = nullptr; // <<< ADDED: Global label for lock screen time

// Styles for hybrid UI
static lv_style_t style_card_action;
static lv_style_t style_card_info;
static lv_style_t style_card_pressed;
static lv_style_t style_gender_card;
static lv_style_t style_gender_card_pressed;
static lv_style_t style_network_item;
static lv_style_t style_network_item_pressed;


// Global WiFiManager instance
WiFiManager wifiManager;

// Global variables to store selected time temporarily
static int selected_hour = 0, selected_minute = 0, selected_is_pm = 0; // 0 = AM, 1 = PM

// Global variables to store selected date/time temporarily
static lv_calendar_date_t selected_date = {2025, 3, 18}; // Default date


// Debug flag - set to true to enable debug output
#define DEBUG_ENABLED true

// Debug macros
#include "lvgl.h" // Add LVGL header for proper type definitions

#define DEBUG_PRINT(x) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.println(x); }
#define DEBUG_PRINTF(x, ...) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.printf(x, __VA_ARGS__); }

// SD Card pins for M5Stack CoreS3
#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN   4
#define TFT_DC 35

// Default WiFi credentials (if none saved) - ADDED
#define DEFAULT_SSID "YourDefaultSSID"
#define DEFAULT_PASS "YourDefaultPassword"

// Forward declarations for timer callbacks
void wifi_btn_event_callback(lv_event_t* e);
void cleanupWiFiResources();

// Function declarations
void createMainMenu();
void createGenderMenu();
void createColorMenuShirt();
void createColorMenuPants();
void createColorMenuShoes();
void createItemMenu();
void createConfirmScreen();
void scanNetworks();
void showWiFiKeyboard();
void connectToWiFi();
void createWiFiScreen();
void createWiFiManagerScreen();
void loadSavedNetworks();
void saveNetworks();
void connectToSavedNetworks();
String getFormattedEntry(const String& entry);
String getTimestamp();
bool appendToLog(const String& entry); // Already correct in your code
void createViewLogsScreen();
void initFileSystem();
void listSavedEntries();
void saveEntry(const String& entry);
void updateStatus(const char* message, uint32_t color);
void sendWebhook(const String& entry);
void onWiFiScanComplete(const std::vector<NetworkInfo>& results);
void updateWiFiLoadingScreen(bool success, const String& message);
void showWiFiLoadingScreen(const String& ssid);
void addTimeDisplay(lv_obj_t *screen); // New function to add a time display
void updateTimeDisplay(); // Function to update the time display periodically


void createSettingsScreen(); // Added forward declaration
void createSoundSettingsScreen(); // Added forward declaration
void createBrightnessSettingsScreen(); // Added forward declaration
void createPowerManagementScreen(); // Added forward declaration

void wifi_btn_event_callback(lv_event_t* e);


// Global variables for scrolling
lv_obj_t *current_scroll_obj = nullptr;
const int SCROLL_AMOUNT = 40;

// Global variables for color selection
String selectedShirtColors = "";
String selectedPantsColors = "";
String selectedShoesColors = "";
String selectedGender = "";
String selectedItem = "";

// Global variables for network processing (UI related)
static lv_obj_t* g_spinner = nullptr; // Global spinner object

// Current entry for loss prevention logging
static String currentEntry = "";

// WiFi connection management
unsigned long lastWiFiConnectionAttempt = 0;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000; // 10 seconds between connection attempts
const int MAX_WIFI_CONNECTION_ATTEMPTS = 5; // Maximum number of consecutive connection attempts
int wifiConnectionAttempts = 0;
bool wifiReconnectEnabled = true;

// WiFi scan management
bool wifiScanInProgress = false;
unsigned long lastScanStartTime = 0;
const unsigned long SCAN_TIMEOUT = 30000; // 30 seconds timeout for WiFi scan

// Global variables for network processing
static int currentBatch = 0;
static int totalNetworksFound = 0;
// static lv_obj_t* g_spinner = nullptr; // Global spinner object - REMOVED DUPLICATE
static lv_timer_t* scan_timer = nullptr; // Global scan timer

// Declare the Montserrat font
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);

// LVGL Refresh time
static const uint32_t screenTickPeriod = 10;
static uint32_t lastLvglTick = 0;

// For log pagination
static std::vector<String> logEntries;
static int currentLogPage = 0;
static int totalLogPages = 0;
static const int LOGS_PER_PAGE = 8;

lv_obj_t* status_bar = nullptr;
char current_status_msg[64] = ""; // Increased size for longer messages

uint32_t current_status_color = 0xFFFFFF;
uint32_t error_status_color = 0xFF0000;



// Moved items array declaration earlier
const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};
const int NUM_ITEMS = sizeof(items) / sizeof(items[0]); // Calculate number of items


// Screen dimensions for CoreS3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LV_TICK_PERIOD_MS 10

// WiFi network management
#define MAX_NETWORKS 5

struct WifiNetwork {
    char ssid[33];
    char password[65];
};

// Structure to hold parsed log entry with timestamp
struct LogEntry {
    String text;
    time_t timestamp;
};

void setSystemTimeFromRTC() {
    m5::rtc_date_t DateStruct;
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getDate(&DateStruct);
    M5.Rtc.getTime(&TimeStruct);

    struct tm timeinfo = {0};
    timeinfo.tm_year = DateStruct.year - 1900;
    timeinfo.tm_mon = DateStruct.month - 1;
    timeinfo.tm_mday = DateStruct.date;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    timeinfo.tm_isdst = -1;

    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    DEBUG_PRINT("System time set from RTC");

    struct tm timeinfo_check;
    if (getLocalTime(&timeinfo_check)) {
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %I:%M:%S %p", &timeinfo_check);
        DEBUG_PRINTF("Current local time: %s", timeStr);
    }
}

// Global vector for parsed log entries
static std::vector<LogEntry> parsedLogEntries;

lv_obj_t *item_list = nullptr;

bool wifiEnabled = true; // Should be loaded from prefs
uint8_t displayBrightness = 128;

static WifiNetwork savedNetworks[MAX_NETWORKS];
static int numSavedNetworks = 0;
static bool wifiConfigured = false;
bool manualDisconnect = false; // Flag to track if disconnection was user-initiated

// Menu options
const char* genders[] = {"Male", "Female"};

// Global screen pointers
lv_obj_t* main_menu_screen = nullptr;
lv_obj_t* genderMenu = nullptr;
lv_obj_t* colorMenu = nullptr;
lv_obj_t* itemMenu = nullptr;
lv_obj_t* confirmScreen = nullptr;
lv_obj_t *wifi_screen = nullptr;
lv_obj_t *wifi_manager_screen = nullptr;
lv_obj_t *wifi_list = nullptr;
lv_obj_t *wifi_status_label = nullptr;
lv_obj_t *saved_networks_list = nullptr;
lv_obj_t* settingsScreen = nullptr;
lv_obj_t* view_logs_screen = nullptr; // Added for consistency
static lv_style_t style_network;
static lv_style_t style_card;

// Global next button pointers (for color menus)
lv_obj_t* shirt_next_btn = nullptr;
lv_obj_t* pants_next_btn = nullptr;
lv_obj_t* shoes_next_btn = nullptr;


// WiFi UI components
static lv_obj_t* wifi_keyboard = nullptr;
static char selected_ssid[33] = ""; // Max SSID length is 32 characters + null terminator
static char selected_password[65] = ""; // Max WPA2 password length is 64 characters + null terminator
static Preferences preferences; // NOTE: This was previously duplicated and removed from line 310. Keep this one.
// WiFi Loading Screen UI elements - ADDED
static lv_obj_t* wifi_loading_screen = nullptr;
static lv_obj_t* wifi_loading_spinner = nullptr;
static lv_obj_t* wifi_loading_label = nullptr;
static lv_obj_t* wifi_result_label = nullptr;


// Battery monitoring
static int lastBatteryLevel = -1;
static unsigned long lastBatteryReadTime = 0;
const unsigned long BATTERY_READ_INTERVAL = 30000;
 
// static Preferences preferences; // Used in various places - REMOVED DUPLICATE
 
// Styles
static lv_style_t style_screen, style_btn, style_btn_pressed, style_title, style_text;
static lv_style_t style_keyboard_btn;

// Keyboard definitions (unchanged)
static int keyboard_page_index = 0;
const lv_btnmatrix_ctrl_t keyboard_ctrl_map[] = {
    LV_BTNMATRIX_CTRL_POPOVER, LV_BTNMATRIX_CTRL_POPOVER, LV_BTNMATRIX_CTRL_POPOVER,
    LV_BTNMATRIX_CTRL_POPOVER, LV_BTNMATRIX_CTRL_POPOVER, LV_BTNMATRIX_CTRL_POPOVER,
    LV_BTNMATRIX_CTRL_POPOVER, LV_BTNMATRIX_CTRL_POPOVER, LV_BTNMATRIX_CTRL_POPOVER,
    LV_BTNMATRIX_CTRL_CHECKABLE, LV_BTNMATRIX_CTRL_CHECKED, LV_BTNMATRIX_CTRL_CHECKED, LV_BTNMATRIX_CTRL_CHECKED
};
const char *btnm_mapplus[11][23] = {
    { "a", "b", "c", "\n", "d", "e", "f", "\n", "g", "h", "i", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "j", "k", "l", "\n", "m", "n", "o", "\n", "p", "q", "r", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "s", "t", "u", "\n", "v", "w", "x", "\n", "y", "z", " ", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "A", "B", "C", "\n", "D", "E", "F", "\n", "G", "H", "I", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "J", "K", "L", "\n", "N", "M", "O", "\n", "P", "Q", "R", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "S", "T", "U", "\n", "V", "W", "X", "\n", "Y", "Z", " ", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "0", "+", "-", "\n", "/", "*", "=", "\n", "!", "?", " ", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "<", ">", "@", "\n", "%", "$", "(", "\n", ")", "{", "}", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "[", "]", ";", "\n", "\"", "'", ".", "\n", ",", ":", " ", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "\\", "_", "~", "\n", "|", "&", "^", "\n", "`", "#", " ", "\n", LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" }
};
const int NUM_KEYBOARD_PAGES = sizeof(btnm_mapplus) / sizeof(btnm_mapplus[0]);

// File system settings
#define LOG_FILENAME "/loss_prevention_log.txt"
bool fileSystemInitialized = false;

#define LVGL_TASK_CORE 1
#define LVGL_TASK_PRIORITY 5
#define LVGL_STACK_SIZE 32768


// --- Function Implementations ---

// Update parseTimestamp (unchanged)
time_t parseTimestamp(const String& entry) {
    struct tm timeinfo = {0};
    int day, year, hour, minute, second;
    char monthStr[4];
    if (sscanf(entry.c_str(), "%d-%3s-%d %d:%d:%d", &day, monthStr, &year, &hour, &minute, &second) == 6) {
        timeinfo.tm_mday = day;
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = second;
        timeinfo.tm_isdst = -1;
        static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        for (int i = 0; i < 12; i++) {
            if (strncmp(monthStr, months[i], 3) == 0) {
                timeinfo.tm_mon = i;
                break;
            }
        }
        return mktime(&timeinfo);
    }
    return 0;
}

// getTimestamp (unchanged)
String getTimestamp() {
    m5::rtc_date_t DateStruct;
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getDate(&DateStruct);
    M5.Rtc.getTime(&TimeStruct);
    struct tm timeinfo = {0};
    timeinfo.tm_year = DateStruct.year - 1900;
    timeinfo.tm_mon = DateStruct.month - 1;
    timeinfo.tm_mday = DateStruct.date;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    timeinfo.tm_isdst = -1;
    if (timeinfo.tm_year > 70) {
        char buffer[25];
        strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M:%S", &timeinfo);
        return String(buffer);
    }
    return "NoTime";
}

// appendToLog (unchanged)
bool appendToLog(const String& entry) {
    if (!fileSystemInitialized) {
        DEBUG_PRINT("File system not initialized, attempting to initialize...");
        initFileSystem();
        if (!fileSystemInitialized) {
            DEBUG_PRINT("File system not initialized, cannot save entry");
            return false;
        }
    }
    SPI.end(); delay(100);
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1);
    pinMode(SD_SPI_CS_PIN, OUTPUT); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(100);
    File file = SD.open(LOG_FILENAME, FILE_APPEND);
    if (!file) {
        DEBUG_PRINT("Failed to open log file for writing");
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);
        return false;
    }
    String timestamp = getTimestamp();
    String formattedEntry = timestamp + ": " + entry;
    size_t bytesWritten = file.println(formattedEntry);
    file.close();
    SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);
    if (bytesWritten == 0) {
        DEBUG_PRINT("Failed to write to log file");
        return false;
    }
    DEBUG_PRINTF("Entry saved to file (%d bytes): %s\n", bytesWritten, formattedEntry.c_str());
    return true;
}

// saveEntry (unchanged)
void saveEntry(const String& entry) {
    Serial.println("New Entry:"); Serial.println(entry);
    bool saved = appendToLog(entry);
    if (wifiManager.isConnected()) {
        sendWebhook(entry);
        updateStatus(saved ? "Entry Saved & Sent" : "Error Saving Entry", saved ? 0x00FF00 : 0xFF0000);
    } else {
        updateStatus(saved ? "Offline: Entry Saved Locally" : "Error: Failed to Save Entry", saved ? 0xFFFF00 : 0xFF0000);
    }
}

// createViewLogsScreen (Fixed casts in callbacks)
void createViewLogsScreen() {
    DEBUG_PRINT("Creating new view logs screen");
    parsedLogEntries.clear(); // Clear previous entries when recreating the screen

    lv_obj_t* logs_screen = lv_obj_create(NULL);
    lv_obj_add_style(logs_screen, &style_screen, 0);
    // Ensure the main screen itself starts fully opaque
    lv_obj_set_style_bg_opa(logs_screen, LV_OPA_COVER, 0);

    // --- Header ---
    lv_obj_t* header = lv_obj_create(logs_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE); // Ensure header is not scrollable

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Log Entries - Last 3 Days");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_center(title);

    // --- TabView ---
    lv_obj_t* tabview = lv_tabview_create(logs_screen);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tabview, 30);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - 40); // Fill remaining space
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x2D2D2D), 0); // Tabview background

    // --- Date & Time Logic ---
    m5::rtc_date_t currentDateStruct;
    M5.Rtc.getDate(&currentDateStruct);
    m5::rtc_time_t currentTimeStruct;
    M5.Rtc.getTime(&currentTimeStruct); // Need time too for mktime

    struct tm currentTimeInfo = {0};
    currentTimeInfo.tm_year = currentDateStruct.year - 1900;
    currentTimeInfo.tm_mon = currentDateStruct.month - 1;
    currentTimeInfo.tm_mday = currentDateStruct.date;
    currentTimeInfo.tm_hour = currentTimeStruct.hours;
    currentTimeInfo.tm_min = currentTimeStruct.minutes;
    currentTimeInfo.tm_sec = currentTimeStruct.seconds;
    time_t now = mktime(&currentTimeInfo); // Get current time as time_t

    // --- File System & Log Reading ---
    if (!fileSystemInitialized) {
        initFileSystem(); // Attempt to initialize if not already done
    }

    if (fileSystemInitialized) {
        // Manage SPI bus access carefully
        SPI.end(); // End default SPI
        delay(50); // Small delay might help bus release
        SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
        digitalWrite(SD_SPI_CS_PIN, HIGH); // Ensure CS is initially high
        delay(50);

        if (!SD.exists(LOG_FILENAME)) {
            DEBUG_PRINT("Log file not found, creating...");
            File file = SD.open(LOG_FILENAME, FILE_WRITE);
            if (file) {
                file.println("# Log Created " + getTimestamp());
                file.close();
                DEBUG_PRINT("Log file created.");
            } else {
                DEBUG_PRINT("Failed to create log file!");
            }
        }

        File log_file = SD.open(LOG_FILENAME, FILE_READ);
        if (log_file) {
            DEBUG_PRINT("Reading log file...");
            while (log_file.available()) {
                String line = log_file.readStringUntil('\n');
                line.trim();
                if (line.length() > 0 && !line.startsWith("#")) {
                    time_t timestamp = parseTimestamp(line);
                    if (timestamp != 0) { // Ensure timestamp was parsed correctly
                        parsedLogEntries.push_back({line, timestamp});
                    }
                }
            }
            log_file.close();
            DEBUG_PRINT("Finished reading log file.");
        } else {
            DEBUG_PRINT("Failed to open log file for reading.");
        }
        SPI_SD.end(); // End SD SPI
        delay(50);
        SPI.begin(); // Reinitialize default SPI (assuming it's needed elsewhere)
        pinMode(TFT_DC, OUTPUT); // Re-init TFT DC pin if SPI.begin() resets it
        digitalWrite(TFT_DC, HIGH);
    } else {
        DEBUG_PRINT("Filesystem not initialized, cannot read logs.");
    }

    // --- Prepare Tabs and Sort Entries by Day ---
    char tab_names[3][16];
    lv_obj_t* tabs[3];
    std::vector<LogEntry*> entries_by_day[3]; // Store pointers for sorting

    for (int i = 0; i < 3; i++) {
        time_t day_target = now - (i * 86400); // Approx i days ago
        struct tm day_time = *localtime(&day_target);

        // Calculate start and end of the target day
        day_time.tm_hour = 0; day_time.tm_min = 0; day_time.tm_sec = 0;
        time_t day_start = mktime(&day_time);
        day_time.tm_hour = 23; day_time.tm_min = 59; day_time.tm_sec = 59;
        time_t day_end = mktime(&day_time);

        strftime(tab_names[i], sizeof(tab_names[i]), "%m/%d/%y", &day_time); // Format tab name
        tabs[i] = lv_tabview_add_tab(tabview, tab_names[i]);
        lv_obj_set_style_bg_opa(tabs[i], LV_OPA_TRANSP, 0); // Make tab content area transparent
        lv_obj_set_scroll_dir(tabs[i], LV_DIR_VER); // Allow scrolling within tab content area
        lv_obj_set_scrollbar_mode(tabs[i], LV_SCROLLBAR_MODE_AUTO);
        lv_obj_add_flag(tabs[i], LV_OBJ_FLAG_SCROLLABLE);


        // Filter entries for this day
        for (auto& entry : parsedLogEntries) {
            if (entry.timestamp >= day_start && entry.timestamp <= day_end) {
                entries_by_day[i].push_back(&entry);
            }
        }
        // Sort entries within the day by timestamp (ascending)
        std::sort(entries_by_day[i].begin(), entries_by_day[i].end(),
                  [](const LogEntry* a, const LogEntry* b) {
                      return a->timestamp < b->timestamp;
                  });
    }

    // --- Populate Tabs with Log Entries ---
    for (int i = 0; i < 3; i++) {
        // Use the tab itself as the container
        lv_obj_t* container = tabs[i];
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN); // Arrange items vertically
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(container, 5, 0); // Spacing between entries
        lv_obj_set_style_pad_column(container, 0, 0);
        lv_obj_set_style_pad_all(container, 5, 0); // Padding around the edges


        if (entries_by_day[i].empty()) {
            lv_obj_t* no_logs = lv_label_create(container);
            lv_label_set_text(no_logs, "No entries for this day");
            lv_obj_add_style(no_logs, &style_text, 0);
            lv_obj_align(no_logs, LV_ALIGN_CENTER, 0, 20); // Center it roughly
        } else {
            int entry_num = 1;
            for (const auto* entry : entries_by_day[i]) {
                lv_obj_t* entry_btn = lv_btn_create(container);
                // Let flexbox handle positioning, set width relative to parent
                lv_obj_set_width(entry_btn, lv_pct(98));
                lv_obj_set_height(entry_btn, 25);

                lv_obj_add_style(entry_btn, &style_btn, 0);
                lv_obj_add_style(entry_btn, &style_btn_pressed, LV_STATE_PRESSED);
                lv_obj_set_style_bg_color(entry_btn, lv_color_hex(0x3A3A3A), 0);

                struct tm* entry_time = localtime(&entry->timestamp);
                char time_str[16];
                strftime(time_str, sizeof(time_str), "%H:%M", entry_time);
                String entry_label_str = "Entry " + String(entry_num++) + ": " + String(time_str);

                lv_obj_t* label = lv_label_create(entry_btn);
                lv_label_set_text(label, entry_label_str.c_str());
                lv_obj_add_style(label, &style_text, 0);
                lv_obj_align(label, LV_ALIGN_LEFT_MID, 5, 0);

                // Store pointer to the LogEntry struct in the button's user data
                lv_obj_set_user_data(entry_btn, (void*)entry);

                // --- Event handler for clicking a log entry button ---
                lv_obj_add_event_cb(entry_btn, [](lv_event_t* e) {
                    LogEntry* log_entry = (LogEntry*)lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e));
                    if (!log_entry || log_entry->text.isEmpty()) {
                        DEBUG_PRINT("Invalid log entry data");
                        return;
                    }

                    // Assuming log format is like "YYYY-MM-DD HH:MM:SS: Data..."
                    int colon_pos = log_entry->text.indexOf(": "); // Find first colon-space after timestamp
                     if (colon_pos != -1 && colon_pos < log_entry->text.length() - 2) {
                         // Find the third colon which should mark the end of seconds
                         int third_colon_pos = -1;
                         int first_colon = log_entry->text.indexOf(':');
                         int second_colon = -1;
                         if(first_colon != -1) second_colon = log_entry->text.indexOf(':', first_colon + 1);
                         if(second_colon != -1) third_colon_pos = log_entry->text.indexOf(':', second_colon + 1);

                         if (third_colon_pos != -1 && third_colon_pos < log_entry->text.length() - 2) {
                            String entry_data = log_entry->text.substring(third_colon_pos + 2); // Get text after "HH:MM:SS: "
                            entry_data.trim(); // Clean up whitespace

                            if (entry_data.isEmpty()) {
                                DEBUG_PRINT("Extracted log data is empty");
                                return;
                            }

                            String formatted_entry = getFormattedEntry(entry_data); // Use your formatting function

                            // Create message box (modal by default)
                            lv_obj_t* msgbox = lv_msgbox_create(NULL); // Parent NULL -> Attach to top layer, creates modal bg
                            lv_obj_set_size(msgbox, 280, 180);
                            lv_obj_center(msgbox);
                            // Style the message box window itself
                            lv_obj_set_style_bg_opa(msgbox, LV_OPA_COVER, 0); // Fully opaque msgbox window
                            lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x808080), 0); // Gray background
                            lv_obj_set_style_pad_all(msgbox, 10, 0); // Padding inside msgbox

                            // Title for the message box
                            lv_obj_t* msg_title = lv_msgbox_add_title(msgbox, "Log Entry Details");

                            // Text content for the message box
                            lv_obj_t* msg_text = lv_msgbox_add_text(msgbox, formatted_entry.c_str());
                            lv_obj_set_style_text_font(msg_text, &lv_font_montserrat_14, 0); // Example font
                            lv_obj_set_style_text_line_space(msg_text, 2, 0);


                            // Add a close button to the message box
                            lv_obj_t* close_btn = lv_msgbox_add_footer_button(msgbox, "Close");

                            // --- ** CORRECTED Close Button Event Callback ** ---
                            lv_obj_add_event_cb(close_btn, [](lv_event_t* e_close) {
                                // Get the message box object itself by navigating up the hierarchy
                                // Button -> footer -> msgbox
                                lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e_close);
                                lv_obj_t* footer = lv_obj_get_parent(btn);
                                lv_obj_t* current_msgbox = lv_obj_get_parent(footer);

                                // Delete the message box. LVGL handles removing the modal background automatically.
                                if (current_msgbox) {
                                     lv_msgbox_close(current_msgbox); // Preferred way for msgbox
                                     // OR delete directly if lv_msgbox_close isn't sufficient/available
                                     // lv_obj_delete(current_msgbox);
                                }

                                // *** NO NEED to manually set parent screen opacity ***
                                // The original screen (logs_screen) was never made transparent,
                                // only obscured by the msgbox's modal background, which is now gone.

                                lv_task_handler(); // Process the deletion and redraw
                                DEBUG_PRINT("Log entry message box closed.");

                            }, LV_EVENT_CLICKED, NULL); // No user data needed here anymore for this purpose

                            // lv_obj_move_foreground(msgbox); // msgbox_create(NULL) already puts it on top layer
                         } else {
                             DEBUG_PRINT("Could not find third colon in log entry text.");
                         }
                    } else {
                        DEBUG_PRINT("Could not find ': ' separator in log entry text.");
                    }
                }, LV_EVENT_CLICKED, NULL);
            } // End loop through entries
        } // End else (entries exist)
    } // End loop through tabs

    // --- Back Button ---
    lv_obj_t* back_btn = lv_btn_create(logs_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -5); // Position bottom left
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);

    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_move_foreground(back_btn); // Ensure it's above the tabview content potentially

    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* old_screen = lv_scr_act();
        createMainMenu(); // Go back to main menu
        if (old_screen && old_screen != lv_scr_act()) { // Clean up old screen if different
             lv_obj_delete(old_screen);
        }
        lv_task_handler();
    }, LV_EVENT_CLICKED, NULL);

    // --- Reset Button ---
    lv_obj_t* reset_btn = lv_btn_create(logs_screen);
    lv_obj_set_size(reset_btn, 80, 40);
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -5); // Position bottom right
    lv_obj_add_style(reset_btn, &style_btn, 0);
    lv_obj_add_style(reset_btn, &style_btn_pressed, LV_STATE_PRESSED);

    lv_obj_t* reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_center(reset_label);
    lv_obj_move_foreground(reset_btn); // Ensure it's above tabview

    lv_obj_add_event_cb(reset_btn, [](lv_event_t* e) {
        // --- Confirmation Message Box for Reset ---
        lv_obj_t* confirm_msgbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(confirm_msgbox, "Confirm Reset");
        lv_msgbox_add_text(confirm_msgbox, "Are you sure you want to reset the log file?");
        lv_obj_set_style_bg_opa(confirm_msgbox, LV_OPA_COVER, 0); // Opaque box
        lv_obj_set_style_bg_color(confirm_msgbox, lv_color_hex(0x808080), 0);

        // Add Yes/No buttons individually
        lv_obj_t* yes_btn = lv_msgbox_add_footer_button(confirm_msgbox, "Yes");
        lv_obj_t* no_btn = lv_msgbox_add_footer_button(confirm_msgbox, "No");
        lv_obj_set_width(confirm_msgbox, 250); // Adjust size
        lv_obj_center(confirm_msgbox);

        lv_obj_add_event_cb(confirm_msgbox, [](lv_event_t* e_confirm) {
            lv_obj_t* target_msgbox = (lv_obj_t*)lv_event_get_current_target(e_confirm); // Get the msgbox itself
            lv_obj_t* clicked_btn = (lv_obj_t*)lv_event_get_target(e_confirm);
            const char* btn_text = lv_label_get_text(lv_obj_get_child(clicked_btn, 0));

            if (btn_text && strcmp(btn_text, "Yes") == 0) {
                // Close confirmation FIRST
                 lv_msgbox_close(target_msgbox);
                 lv_task_handler(); // Process deletion

                // Perform Reset Logic
                DEBUG_PRINT("Resetting log file...");
                SPI.end(); delay(50);
                SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
                digitalWrite(SD_SPI_CS_PIN, HIGH); delay(50);
                bool reset_success = false;
                if (SD.remove(LOG_FILENAME)) {
                    File file = SD.open(LOG_FILENAME, FILE_WRITE);
                    if (file) {
                        file.println("# Log Reset " + getTimestamp());
                        file.close();
                        reset_success = true;
                        DEBUG_PRINT("Log file reset successful.");
                    } else {
                         DEBUG_PRINT("Failed to create new log file after removing old one.");
                    }
                } else {
                     DEBUG_PRINT("Failed to remove log file.");
                }
                SPI_SD.end(); delay(50);
                SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);

                parsedLogEntries.clear(); // Clear in-memory logs

                // --- Show Success/Failure Message Box ---
                lv_obj_t* result_msgbox = lv_msgbox_create(NULL);
                lv_msgbox_add_title(result_msgbox, reset_success ? "Reset Successful" : "Reset Failed");
                lv_msgbox_add_text(result_msgbox, reset_success ? "Log has been reset." : "Failed to reset log.");
                lv_obj_set_style_bg_opa(result_msgbox, LV_OPA_COVER, 0);
                lv_obj_set_style_bg_color(result_msgbox, lv_color_hex(0x808080), 0);
                const char* ok_btn_txt[] = {"OK", ""};
                lv_obj_t* ok_btn = lv_msgbox_add_footer_button(result_msgbox, "OK");
                lv_obj_set_width(result_msgbox, 250);
                lv_obj_center(result_msgbox);

                lv_obj_add_event_cb(result_msgbox, [](lv_event_t* e_result) {
                     lv_obj_t* target_result_msgbox = (lv_obj_t*)lv_event_get_current_target(e_result);
                     lv_msgbox_close(target_result_msgbox); // Close this msgbox

                     // Refresh the entire log view screen
                     lv_obj_t* old_screen = lv_scr_act();
                     createViewLogsScreen(); // Recreate the screen to show empty/new logs
                     if (old_screen && old_screen != lv_scr_act()) { // Clean up old screen if different
                          lv_obj_delete(old_screen);
                     }
                     lv_task_handler(); // Update UI

                }, LV_EVENT_VALUE_CHANGED, NULL); // Event when button clicked

            } else if (btn_text && strcmp(btn_text, "No") == 0) {
                 lv_msgbox_close(target_msgbox); // Just close the confirmation box
                 lv_task_handler();
            }
        }, LV_EVENT_VALUE_CHANGED, NULL); // Event when a button is clicked

    }, LV_EVENT_CLICKED, NULL);


    // --- Load Screen ---
    lv_scr_load(logs_screen);
    lv_task_handler(); // Process initial drawing
    DEBUG_PRINT("View logs screen created and loaded successfully");
}

// releaseSPIBus (unchanged)
void releaseSPIBus() { 
    SPI.end(); 
    delay(100); 
}

// initFileSystem (Fixed cast in callback)
void initFileSystem() {
    if (fileSystemInitialized) return;
    DEBUG_PRINT("Initializing SD card...");
    SPI.end(); delay(200);
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1);
    pinMode(SD_SPI_CS_PIN, OUTPUT); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(200);
    bool sdInitialized = false;
    for (int i = 0; i < 3 && !sdInitialized; i++) { sdInitialized = SD.begin(SD_SPI_CS_PIN, SPI_SD, 2000000); if (!sdInitialized) delay(100); }
    if (!sdInitialized) {
        DEBUG_PRINT("All SD card initialization attempts failed");
        lv_obj_t* msgbox = lv_msgbox_create(NULL); lv_obj_set_size(msgbox, 280, 150); lv_obj_center(msgbox);
        lv_obj_t* title = lv_label_create(msgbox); lv_label_set_text(title, "Storage Error"); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_t* text = lv_label_create(msgbox); lv_label_set_text(text, "SD card initialization failed."); lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
        lv_obj_t* close_btn = lv_button_create(msgbox); lv_obj_set_size(close_btn, 80, 40); lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_t* close_label = lv_label_create(close_btn); lv_label_set_text(close_label, "OK"); lv_obj_center(close_label);
        lv_obj_add_event_cb(close_btn, [](lv_event_t* e) { lv_obj_delete(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e))); }, LV_EVENT_CLICKED, NULL); // Re-added cast
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); return;
    }
    DEBUG_PRINT("SD card initialized successfully"); fileSystemInitialized = true;
    SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);
}

// lvgl_task (unchanged)
void lvgl_task(void *pvParameters) { while (1) { 
    lv_timer_handler(); 
    vTaskDelay(pdMS_TO_TICKS(5)); 
 } 
}

// LVGL tick task (unchanged)
static void lvgl_tick_task(void *arg) {
    (void)arg;
    static uint32_t last_tick = 0;
    uint32_t current_tick = millis();
    if (current_tick - last_tick > LV_TICK_PERIOD_MS) {
        last_tick = current_tick;
        lv_task_handler();
    }
}
// setup (Removed loadSavedNetworks call, WiFiManager handles it)
void setup() {
    Serial.begin(115200); 
    DEBUG_PRINT("Starting Loss Prevention Log...");
    
    auto cfg = M5.config(); 
    M5.begin(cfg);
    M5.Power.begin();
    
    DEBUG_PRINT("Before lv_init"); 
    lv_init(); 
    m5gfx_lvgl_init(); 
    DEBUG_PRINT("After lv_init");
    
    initFileSystem();
    
    DEBUG_PRINT("LVGL task created");
    
    initStyles();
    createLoadingScreen();
    
    M5.Power.setExtOutput(true);
    M5.Power.Axp2101.writeRegister8(0x94, 28); 
    uint8_t reg90 = M5.Power.Axp2101.readRegister8(0x90); 
    M5.Power.Axp2101.writeRegister8(0x90, reg90 | 0x08);
    M5.Speaker.begin(); 
    DEBUG_PRINT("Speaker initialized");
    
    // Load saved settings
    Preferences prefs; prefs.begin("settings", false);
    uint8_t saved_volume = prefs.getUChar("volume", 128); 
    bool sound_enabled = prefs.getBool("sound_enabled", true);
    M5.Speaker.setVolume(sound_enabled ? saved_volume : 0);
    displayBrightness = prefs.getUChar("brightness", 128); 
    M5.Display.setBrightness(displayBrightness);
    prefs.end();
    
    // Initialize RTC
    m5::rtc_date_t DateStruct; 
    M5.Rtc.getDate(&DateStruct);
    if (DateStruct.year < 2020) { DateStruct.year = 2025; DateStruct.month = 3; DateStruct.date = 15; DateStruct.weekDay = 5; M5.Rtc.setDate(&DateStruct); m5::rtc_time_t TimeStruct; TimeStruct.hours = 12; TimeStruct.minutes = 0; TimeStruct.seconds = 0; M5.Rtc.setTime(&TimeStruct); }
    setSystemTimeFromRTC();
    
    // Initialize WiFi Manager 
    wifiManager.setStatusCallback(onWiFiStatus);
    wifiManager.setScanCallback(onWiFiScanComplete);
    wifiManager.begin();
    DEBUG_PRINT("WiFi Manager initialized");
    DEBUG_PRINT("Setup complete!");
}

// loop (Simplified WiFi update logic)
void loop() {
    M5.update(); // Update button and touch states
    // --- WiFi Manager Update ---
    wifiManager.update(); // Process results from WiFi task queue
    uint32_t currentMillis = millis();
    lv_timer_handler();

    // Handle LVGL timing (unchanged)
    static uint32_t lastLvglTick = 0;
    if (currentMillis - lastLvglTick > 10) { // screenTickPeriod set to 10ms
        if (xSemaphoreTake(xGuiSemaphore, (TickType_t)10) == pdTRUE) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
            lastLvglTick = currentMillis;
        }
    }

    // --- Time Update Section ---
    static unsigned long lastTimeUpdate = 0;
    if (currentMillis - lastTimeUpdate > 1000) { // Update every second
        if (xSemaphoreTake(xGuiSemaphore, (TickType_t)100) == pdTRUE) {
            // Update time display based on the current screen
            if (lv_scr_act() == main_menu_screen) {
                updateTimeDisplay(); // Update main menu time display
            } else if (lv_scr_act() == lock_screen) {
                updateLockScreenTime(); // <<< ADDED: Update lock screen time display
            }
            xSemaphoreGive(xGuiSemaphore);
        }
        lastTimeUpdate = currentMillis;
    }

    /* --- Status Bar/Icon Update Section (Main Menu Specific) ---
    static unsigned long lastStatusUpdate = 0;
    if (currentMillis - lastStatusUpdate > 1000) { // Reduced frequency might be ok here
        // The code for updating battery/wifi on main menu seems okay,
        // but ensure currentWiFiState is updated correctly if you uncomment this section.
        // It's currently commented out in the provided code.
        // If you re-enable it, make sure it's also protected by the semaphore,
        // as you were doing before.
        /*
        if (xSemaphoreTake(xGuiSemaphore, (TickType_t)100) == pdTRUE) {
            if (lv_scr_act() == main_menu_screen) { // Only update these for main menu
                // ... (Battery and WiFi update code) ...
            }
            xSemaphoreGive(xGuiSemaphore);
        }
        
        lastStatusUpdate = currentMillis;
        
    }
    */
    delay(5); // Keep the small delay for cooperative multitasking
}


// adding old wifi back
void wifi_btn_event_callback(lv_event_t* e) {
    if (e == nullptr) {
        return;
    }
    
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Cast void* to lv_obj_t* for LVGL v9
    if (btn == nullptr) {
        return;
    }
    
    // Make sure the button is still valid
    if (!lv_obj_is_valid(btn)) {
        DEBUG_PRINT("Button is no longer valid");
        return;
    }
    
    char* ssid = (char*)lv_obj_get_user_data(btn);
    if (ssid && (intptr_t)ssid > 100) { // Check if it's a valid pointer and not just an integer cast
        strncpy(selected_ssid, ssid, 32);
        selected_ssid[32] = '\0';
        DEBUG_PRINTF("Selected SSID: %s\n", selected_ssid);
        
        // Don't rescan networks when selecting one
        // scanNetworks();
        
        showWiFiKeyboard();
    } else {
        DEBUG_PRINT("Invalid SSID data in button");
    }
}

void showWiFiKeyboard() {
    lv_obj_t* kb_screen = lv_obj_create(NULL);
    lv_obj_add_style(kb_screen, &style_screen, 0);

    // Password Input
    lv_obj_t* ta = lv_textarea_create(kb_screen);
    lv_obj_set_size(ta, 260, 40);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 20);
    lv_textarea_set_password_mode(ta, true);
    lv_textarea_set_max_length(ta, 64);
    lv_textarea_set_placeholder_text(ta, "Enter Password");

    // Keyboard
    lv_obj_t* kb = lv_keyboard_create(kb_screen);
    lv_obj_set_size(kb, SCREEN_WIDTH, SCREEN_HEIGHT / 2);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, ta);

    // Connect Button
    lv_obj_t* connect_btn = lv_btn_create(kb_screen);
    lv_obj_set_size(connect_btn, 100, 40);
    lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -60);
    lv_obj_add_style(connect_btn, &style_btn, 0);
    lv_obj_t* connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_center(connect_label);
    lv_obj_set_user_data(connect_btn, ta); // Store the textarea pointer as user data
    lv_obj_add_event_cb(connect_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* textarea = (lv_obj_t*)lv_obj_get_user_data(btn);
        const char* password = lv_textarea_get_text(textarea);
        wifiManager.connect(selected_ssid, password);
        createWiFiManagerScreen(); // Return to WiFi screen
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(kb_screen);
}

void connectToWiFi() {
    DEBUG_PRINTF("Connecting to %s\n", selected_ssid);
    wifiManager.connect(selected_ssid, selected_password);
    if (wifi_keyboard) {
        lv_obj_del(wifi_keyboard);
        wifi_keyboard = nullptr;
    }
}

void createWiFiManagerScreen() {
    if (wifi_manager_screen) {
        lv_obj_del(wifi_manager_screen);
        wifi_manager_screen = nullptr;
    }
    wifi_manager_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_manager_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(wifi_manager_screen, lv_color_hex(0x1A1A1A), 0); // Dark gray background
    lv_obj_set_style_bg_opa(wifi_manager_screen, LV_OPA_COVER, 0);
    lv_obj_add_flag(wifi_manager_screen, LV_OBJ_FLAG_SCROLLABLE); // Screen scrolls if needed
    current_scroll_obj = wifi_manager_screen;

    // Back Button (Top-Left)
    lv_obj_t* back_btn = lv_btn_create(wifi_manager_screen);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0); // Darker gray button
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT); // Left arrow icon
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);

    // Title
    lv_obj_t* title = lv_label_create(wifi_manager_screen);
    lv_label_set_text(title, "WiFi Manager");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Saved Networks Label
    lv_obj_t* saved_label = lv_label_create(wifi_manager_screen);
    lv_label_set_text(saved_label, "Saved Networks");
    lv_obj_set_style_text_color(saved_label, lv_color_hex(0xBBBBBB), 0); // Light gray text
    lv_obj_set_style_text_font(saved_label, &lv_font_montserrat_16, 0);
    lv_obj_align(saved_label, LV_ALIGN_TOP_LEFT, 20, 60); // Aligned with cards

    // Define styles for network items
    static lv_style_t style_network;
    lv_style_init(&style_network);
    lv_style_set_bg_color(&style_network, lv_color_hex(0x2D2D2D)); // Dark gray for cards
    lv_style_set_bg_opa(&style_network, LV_OPA_COVER);
    lv_style_set_border_color(&style_network, lv_color_hex(0x505050)); // Light gray border
    lv_style_set_border_width(&style_network, 1);
    lv_style_set_radius(&style_network, 5);
    lv_style_set_shadow_color(&style_network, lv_color_hex(0x000000)); // Black shadow
    lv_style_set_shadow_width(&style_network, 10);
    lv_style_set_shadow_spread(&style_network, 2);
    lv_style_set_pad_all(&style_network, 5);

    static lv_style_t style_network_pressed;
    lv_style_init(&style_network_pressed);
    lv_style_set_bg_color(&style_network_pressed, lv_color_hex(0x404040)); // Lighter gray when pressed
    lv_style_set_bg_opa(&style_network_pressed, LV_OPA_COVER);

    // Populate Saved Networks
    auto savedNetworks = wifiManager.getSavedNetworks();
    int y_offset = 90; // Start below the "Saved Networks" label
    for (size_t i = 0; i < savedNetworks.size(); i++) {
        lv_obj_t* network_item = lv_obj_create(wifi_manager_screen);
        lv_obj_set_size(network_item, SCREEN_WIDTH - 40, 60);
        lv_obj_set_pos(network_item, 20, y_offset); // Consistent left margin
        lv_obj_add_style(network_item, &style_network, 0);
        lv_obj_add_style(network_item, &style_network_pressed, LV_STATE_PRESSED);

        // SSID Label
        lv_obj_t* ssid_label = lv_label_create(network_item);
        lv_label_set_text(ssid_label, savedNetworks[i].ssid.c_str());
        lv_obj_align(ssid_label, LV_ALIGN_LEFT_MID, 5, 0);
        lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xFFFFFF), 0); // White text
        lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_16, 0); // Consistent font

        // Status Indicator
        if (savedNetworks[i].connected) {
            lv_obj_t* status_icon = lv_label_create(network_item);
            lv_label_set_text(status_icon, LV_SYMBOL_OK); // Checkmark for connected
            lv_obj_set_style_text_color(status_icon, lv_color_hex(0x00CC00), 0); // Green
            lv_obj_set_style_text_font(status_icon, &lv_font_montserrat_16, 0); // Match font size
            lv_obj_align(status_icon, LV_ALIGN_RIGHT_MID, -5, 0);
        }

        // Clickable area
        lv_obj_add_event_cb(network_item, [](lv_event_t* e) {
            lv_obj_t* item = (lv_obj_t*)lv_event_get_target(e); // Cast void* to lv_obj_t* for LVGL v9
            size_t idx = (size_t)lv_obj_get_user_data(item);
            auto savedNetworks = wifiManager.getSavedNetworks();
            if (idx < savedNetworks.size()) {
                createNetworkDetailsScreen(savedNetworks[idx].ssid);
            }
        }, LV_EVENT_CLICKED, (void*)i);
        lv_obj_set_user_data(network_item, (void*)i);

        y_offset += 70; // Spacing between cards
    }

    // Scan Button (Top-Right)
    lv_obj_t* scan_btn = lv_btn_create(wifi_manager_screen);
    lv_obj_set_size(scan_btn, 60, 40);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x333333), 0); // Darker gray button
    lv_obj_set_style_radius(scan_btn, 5, 0);
    lv_obj_add_style(scan_btn, &style_btn, 0);
    lv_obj_add_style(scan_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH); // Search icon
    lv_obj_center(scan_label);
    lv_obj_set_style_text_color(scan_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_add_event_cb(scan_btn, [](lv_event_t* e) {
        if (wifiManager.isEnabled()) {
            wifiManager.startScan();
            createWiFiScreen();
        } else {
            // LVGL v9: Create msgbox first, then add elements
            lv_obj_t* msgbox = lv_msgbox_create(NULL);
            lv_msgbox_add_title(msgbox, "WiFi Disabled");
            lv_msgbox_add_text(msgbox, "Please enable WiFi to scan.");
            lv_obj_t* close_btn = lv_msgbox_add_footer_button(msgbox, "Close");
            lv_obj_add_event_cb(close_btn, [](lv_event_t* e_close) {
                // Correctly get msgbox from button parent in LVGL v9 msgbox structure
                lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e_close);
                lv_obj_t* mbox = lv_obj_get_parent(lv_obj_get_parent(btn));
                if(mbox) lv_msgbox_close(mbox);
            }, LV_EVENT_CLICKED, NULL);
            lv_obj_center(msgbox);
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(wifi_manager_screen);
}

void createNetworkDetailsScreen(const String& ssid) {
    lv_obj_t* details_screen = lv_obj_create(NULL);
    lv_obj_add_style(details_screen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(details_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text_fmt(title, "Network: %s", ssid.c_str());
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container for details
    lv_obj_t* container = lv_obj_create(details_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0); // Black background
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0); // Opaque background

    // Connection Status
    bool isConnected = wifiManager.isConnected() && wifiManager.getCurrentSSID() == ssid;
    lv_obj_t* status_label = lv_label_create(container);
    lv_label_set_text(status_label, isConnected ? "Status: Connected" : "Status: Disconnected");
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_style(status_label, &style_text, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0); // White text

    // Signal Strength
    lv_obj_t* strength_label = lv_label_create(container);
    int rssi = isConnected ? wifiManager.getRSSI() : 0; // Only show RSSI if connected
    int strength = isConnected ? map(rssi, -100, -50, 0, 100) : 0;
    strength = constrain(strength, 0, 100);
    lv_label_set_text_fmt(strength_label, "Signal Strength: %d%%", strength);
    lv_obj_align(strength_label, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_add_style(strength_label, &style_text, 0);
    lv_obj_set_style_text_color(strength_label, lv_color_hex(0xFFFFFF), 0); // White text

    // Connect/Disconnect Button
    lv_obj_t* connect_btn = lv_btn_create(container);
    lv_obj_set_size(connect_btn, 140, 50);
    lv_obj_align(connect_btn, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_add_style(connect_btn, &style_btn, 0);
    lv_obj_add_style(connect_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, isConnected ? "Disconnect" : "Connect");
    lv_obj_center(connect_label);
    String* ssid_ptr = new String(ssid); // Allocate on heap to persist
    lv_obj_add_event_cb(connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, ssid_ptr);

    // Forget Button Removed - Functionality not supported by current WiFiManager library.
    /*
    lv_obj_t* forget_btn = lv_btn_create(container);
    lv_obj_set_size(forget_btn, 140, 50);
    lv_obj_align(forget_btn, LV_ALIGN_TOP_MID, 0, 140); // Positioned below connect button
    lv_obj_add_style(forget_btn, &style_btn, 0);
    lv_obj_add_style(forget_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* forget_label = lv_label_create(forget_btn);
    lv_label_set_text(forget_label, "Forget Network");
    lv_obj_center(forget_label);
    String* forget_ssid_ptr = new String(ssid); // Allocate on heap to persist
    lv_obj_add_event_cb(forget_btn, forget_btn_event_cb, LV_EVENT_CLICKED, forget_ssid_ptr);
    */

    // Back Button (Positioned at bottom)
    lv_obj_t* back_btn = lv_btn_create(details_screen);
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10); // Keep at bottom center
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createWiFiManagerScreen();
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(details_screen);
}

void createWiFiScreen() {
    if (wifi_screen) {
        lv_obj_del(wifi_screen);
        wifi_screen = nullptr;
    }
    wifi_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_screen, &style_screen, 0);

    lv_obj_t* header = lv_obj_create(wifi_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi Networks");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container with subtle shadow
    lv_obj_t* container = lv_obj_create(wifi_screen);
    lv_obj_set_size(container, 300, 180);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2A2A40), 0);
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_shadow_color(container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_width(container, 15, 0);
    lv_obj_set_style_pad_all(container, 20, 0);

    wifi_status_label = lv_label_create(container);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 5);
    lv_label_set_text(wifi_status_label, "Scanning...");
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFFFF), 0);

    wifi_list = lv_list_create(container);
    lv_obj_set_size(wifi_list, 280, 140);
    lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x2A2A40), 0);
    lv_obj_set_style_border_width(wifi_list, 0, 0);
    lv_obj_set_scroll_dir(wifi_list, LV_DIR_VER);
    current_scroll_obj = wifi_list;

    // Bottom button container
    lv_obj_t* btnContainer = lv_obj_create(wifi_screen);
    lv_obj_set_size(btnContainer, 300, 50);
    lv_obj_align(btnContainer, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(btnContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btnContainer, 0, 0);
    lv_obj_set_flex_flow(btnContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btnContainer, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* back_btn = lv_btn_create(btnContainer);
    lv_obj_set_size(back_btn, 120, 45);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createWiFiManagerScreen();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* refresh_btn = lv_btn_create(btnContainer);
    lv_obj_set_size(refresh_btn, 120, 45);
    lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_style(refresh_btn, &style_btn, 0);
    lv_obj_add_style(refresh_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* refresh_label = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_label, "Scan");
    lv_obj_center(refresh_label);

    lv_obj_add_event_cb(refresh_btn, [](lv_event_t* e) {
        // Clear list and update status
        lv_obj_clean(wifi_list);
        lv_label_set_text(wifi_status_label, "Scanning...");
        
        
        // Create spinner to indicate scanning
        if (g_spinner == nullptr) {
            g_spinner = lv_spinner_create(wifi_screen); // LVGL v9: Only parent argument
            lv_obj_set_size(g_spinner, 50, 50);
            lv_obj_align(g_spinner, LV_ALIGN_CENTER, 0, 0);
        }
        // Start scan
        wifiManager.startScan();
        
        // Create a timeout timer
        static uint32_t scan_start_time = 0;
        scan_start_time = millis();
        
        if (scan_timer != nullptr) {
            lv_timer_del(scan_timer);
            scan_timer = nullptr;
        }
        
        scan_timer = lv_timer_create([](lv_timer_t* timer) {
            static uint32_t* start_time_ptr = (uint32_t*)lv_timer_get_user_data(timer);
            uint32_t elapsed = millis() - *start_time_ptr;
            
            // Check if scan is taking too long (more than 10 seconds)
            if (elapsed > 10000) {
                // Update status label
                if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text(wifi_status_label, "Scan timed out. Try again.");
                }
                
                // Remove spinner
                if (g_spinner && lv_obj_is_valid(g_spinner)) {
                    lv_obj_del(g_spinner);
                    g_spinner = nullptr;
                }
                
                // Delete timer
                lv_timer_del(timer);
                scan_timer = nullptr;
            }
        }, 1000, &scan_start_time);
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(wifi_screen);
}

static void connect_btn_event_cb(lv_event_t* e) {
    String* ssid = static_cast<String*>(lv_event_get_user_data(e));
    bool isConnected = wifiManager.isConnected() && wifiManager.getCurrentSSID() == *ssid;
    if (isConnected) {
        wifiManager.disconnect(true); // Manual disconnect
        createNetworkDetailsScreen(*ssid); // Refresh screen
    } else {
        auto savedNetworks = wifiManager.getSavedNetworks();
        for (const auto& net : savedNetworks) {
            if (net.ssid == *ssid) {
                showWiFiLoadingScreen(*ssid);
                wifiManager.connect(*ssid, net.password, false);
                break;
            }
        }
    }
    delete ssid; // Clean up allocated memory
}

static void forget_btn_event_cb(lv_event_t* e) {
    String* ssid = static_cast<String*>(lv_event_get_user_data(e));
    // Functionality to remove/forget networks is not available in this WiFiManager library.
    // Consider adding this feature to lib/WiFiManager/WiFiManager.cpp/h if needed.
    DEBUG_PRINTF("Attempted to forget network '%s', but functionality is not implemented.\n", ssid->c_str());
    // Don't transition back immediately, let user see the debug message (if enabled) or just stay on details screen.
    // createWiFiManagerScreen(); // Return to WiFi Manager screen
    delete ssid; // Clean up allocated memory
}

void cleanupWiFiResources() {
    DEBUG_PRINT("Cleaning up WiFi resources");
    
    // Delete the spinner if it exists
    if (g_spinner != nullptr) {
        lv_obj_del(g_spinner);
        g_spinner = nullptr;
    }
    
    // Delete the scan timer if it exists
    if (scan_timer != nullptr) {
        lv_timer_del(scan_timer);
        scan_timer = nullptr;
    }
    
    // Clean up keyboard if it exists
    if (wifi_keyboard != nullptr) {
        lv_obj_del(wifi_keyboard);
        wifi_keyboard = nullptr;
    }
    
    // Clean up the WiFi list and free memory for network buttons
    if (wifi_list != nullptr) {
        // Check if wifi_list is still a valid object before accessing its children
        if (lv_obj_is_valid(wifi_list)) {
            uint32_t child_count = lv_obj_get_child_cnt(wifi_list);
            for (uint32_t i = 0; i < child_count; i++) {
                lv_obj_t* btn = lv_obj_get_child(wifi_list, i);
                if (btn != nullptr && lv_obj_is_valid(btn)) {
                    // Get user data safely - only free if not NULL
                    void* user_data = lv_obj_get_user_data(btn);
                    if (user_data != nullptr) {
                        // Only free memory that was dynamically allocated
                        // Check if the pointer is not just a cast of an integer
                        if ((intptr_t)user_data > 100) {  // Assuming small integers are used as indices
                            free(user_data); // Use standard free() for memory allocated with strdup/malloc
                        }
                        lv_obj_set_user_data(btn, NULL); // Clear the pointer after freeing
                    }
                }
            }
        }
        // We don't delete wifi_list here as it's a child of wifi_screen
        // and will be deleted when wifi_screen is deleted
    }
    
    // Reset scan state
    wifiScanInProgress = false;
    
    // Cancel any ongoing WiFi scan
    WiFi.scanDelete();
    
    // Reset batch processing variables
    currentBatch = 0;
    totalNetworksFound = 0;
    
    // Ensure we're connected to the previous network if we have credentials
    if (WiFi.status() != WL_CONNECTED && strlen(selected_ssid) > 0 && strlen(selected_password) > 0) {
        DEBUG_PRINTF("Will reconnect to %s after scan\n", selected_ssid);
    }
}



void createMainMenu() {
    DEBUG_PRINTF("Free heap before main menu: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    
    // Ensure SPI bus is set for display
    releaseSPIBus();
    SPI.begin();
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);

    // Clean up existing main menu screen if it exists
    if (main_menu_screen) {
        lv_obj_clean(main_menu_screen); // Remove all child objects
        lv_obj_del(main_menu_screen);
        main_menu_screen = nullptr;
    }
    main_menu_screen = lv_obj_create(NULL);
    lv_obj_add_style(main_menu_screen, &style_screen, 0);

    // Reset global pointers to avoid dangling references
    current_scroll_obj = nullptr;
    status_bar = nullptr;
    battery_icon = nullptr;
    battery_label = nullptr;
    wifi_label = nullptr;
    time_label = nullptr;

    // Header
    lv_obj_t* header = lv_obj_create(main_menu_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Loss Prevention");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_all(title, 5, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Scrollable card grid (2x3, vertical scrolling)
    lv_obj_t* grid = lv_obj_create(main_menu_screen);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST}; // 2 columns
    static lv_coord_t row_dsc[] = {80, 80, 80, LV_GRID_TEMPLATE_LAST}; // 3 rows
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_set_content_width(grid, 300);
    // Scroll improvements
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);  // Vertical scrolling
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_scroll_snap_y(grid, LV_SCROLL_SNAP_NONE);  // Disable snap scrolling
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLL_MOMENTUM);  // Add momentum
    current_scroll_obj = grid;

    // Card 1: New Entry (Row 0, Col 0)
    lv_obj_t* new_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(new_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_style(new_card, &style_card_action, 0);
    lv_obj_add_style(new_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* new_icon = lv_label_create(new_card);
    lv_label_set_text(new_icon, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_font(new_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(new_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* new_label = lv_label_create(new_card);
    lv_label_set_text(new_label, "New Entry");
    lv_obj_align(new_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(new_card, [](lv_event_t* e) {
        createGenderMenu(); 
    }, LV_EVENT_CLICKED, NULL);

    // Card 2: Logs (Row 0, Col 1)
    lv_obj_t* logs_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(logs_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_style(logs_card, &style_card_action, 0);
    lv_obj_add_style(logs_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* logs_icon = lv_label_create(logs_card);
    lv_label_set_text(logs_icon, LV_SYMBOL_LIST); // List icon
    lv_obj_set_style_text_font(logs_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(logs_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* logs_label = lv_label_create(logs_card);
    lv_label_set_text(logs_label, "Logs");
    lv_obj_align(logs_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(logs_card, [](lv_event_t* e) {
        createViewLogsScreen();
    }, LV_EVENT_CLICKED, NULL);

    // Card 3: Settings (Row 1, Col 0)
    lv_obj_t* settings_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(settings_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_style(settings_card, &style_card_action, 0);
    lv_obj_add_style(settings_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* settings_icon = lv_label_create(settings_card);
    lv_label_set_text(settings_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(settings_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(settings_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* settings_label = lv_label_create(settings_card);
    lv_label_set_text(settings_label, "Settings");
    lv_obj_align(settings_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(settings_card, [](lv_event_t* e) {
        createSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);

    // Card 4: Battery (Row 1, Col 1)
    lv_obj_t* battery_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(battery_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_style(battery_card, &style_card_info, 0);
    battery_icon = lv_label_create(battery_card);
    int battery_level = M5.Power.getBatteryLevel();
    const char* battery_symbol = (battery_level > 75) ? LV_SYMBOL_BATTERY_FULL :
                                 (battery_level > 50) ? LV_SYMBOL_BATTERY_3 :
                                 (battery_level > 25) ? LV_SYMBOL_BATTERY_2 :
                                 (battery_level > 10) ? LV_SYMBOL_BATTERY_1 : LV_SYMBOL_BATTERY_EMPTY;
    lv_label_set_text(battery_icon, battery_symbol);
    lv_obj_set_style_text_font(battery_icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(battery_icon, lv_color_hex(0x4A90E2), 0);
    lv_obj_align(battery_icon, LV_ALIGN_CENTER, -20, 0);
    battery_label = lv_label_create(battery_card);
    char battery_text[5];
    snprintf(battery_text, sizeof(battery_text), "%d%%", battery_level);
    lv_label_set_text(battery_label, battery_text);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_16, 0);
    lv_obj_align(battery_label, LV_ALIGN_CENTER, 20, 0);

    // Card 5: WiFi (Row 2, Col 0)
    lv_obj_t* wifi_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(wifi_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_style(wifi_card, &style_card_info, 0);

    // Create container for signal bars
    lv_obj_t* wifi_container = lv_obj_create(wifi_card);
    lv_obj_set_size(wifi_container, 90, 40);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_container, 0, 0);
    lv_obj_set_style_pad_all(wifi_container, 0, 0);
    lv_obj_align(wifi_container, LV_ALIGN_CENTER, -10, 0);

    // Get WiFi strength and determine color
    int rssi = WiFi.RSSI();
    int wifi_strength = map(rssi, -100, -50, 0, 100);
    wifi_strength = constrain(wifi_strength, 0, 100);

    // Set color based on connection quality
    uint32_t wifi_color;
    if (WiFi.status() != WL_CONNECTED) {
        wifi_color = 0xFF0000; // Red for no connection
    } else if (wifi_strength < 30) {
        wifi_color = 0xFF0000; // Red for poor connection
    } else if (wifi_strength < 70) {
        wifi_color = 0xFFAA00; // Orange/yellow for moderate connection
    } else {
        wifi_color = 0x00FF00; // Green for good connection
    }

    // Create signal bars
    const int BAR_WIDTH = 8;
    const int BAR_GAP = 2;
    const int BAR_COUNT = 4;
    const int MAX_BAR_HEIGHT = 30;

    for (int i = 0; i < BAR_COUNT; i++) {
        int bar_height = 6 + ((i + 1) * (MAX_BAR_HEIGHT - 6) / BAR_COUNT);
        lv_obj_t* bar = lv_obj_create(wifi_container);
        lv_obj_set_size(bar, BAR_WIDTH, bar_height);
        lv_obj_set_pos(bar, i * (BAR_WIDTH + BAR_GAP), MAX_BAR_HEIGHT - bar_height);
        
        // Calculate if this bar should be active based on signal strength
        bool active = wifi_strength > (i + 1) * (100 / BAR_COUNT);
        
        if (WiFi.status() != WL_CONNECTED) {
            // All bars empty/gray if not connected
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x666666), 0);
        } else if (active) {
            // Active bars get the signal color
            lv_obj_set_style_bg_color(bar, lv_color_hex(wifi_color), 0);
        } else {
            // Inactive bars are dimmed/gray
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x666666), 0);
        }
        
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 1, 0);
    }

    // Display signal strength text next to bars
    wifi_label = lv_label_create(wifi_card);
    char wifi_text[16];
    snprintf(wifi_text, sizeof(wifi_text), "%d%%", wifi_strength);
    lv_label_set_text(wifi_label, wifi_text);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(wifi_color), 0);
    lv_obj_align(wifi_label, LV_ALIGN_CENTER, 40, 0);

    // Card 6: Power Management (Row 2, Col 1)
    lv_obj_t* power_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(power_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_style(power_card, &style_card_action, 0);
    lv_obj_add_style(power_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* power_icon = lv_label_create(power_card);
    lv_label_set_text(power_icon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_font(power_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(power_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* power_label = lv_label_create(power_card);
    lv_label_set_text(power_label, "Power");
    lv_obj_align(power_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(power_card, [](lv_event_t* e) {
        createPowerManagementScreen();
    }, LV_EVENT_CLICKED, NULL);

    // Add status bar before loading the screen
    addStatusBar(main_menu_screen);
    updateStatus("Ready", 0xFFFFFF); // Set initial status

    // Load the screen and add time display
    lv_scr_load(main_menu_screen);
    addTimeDisplay(main_menu_screen);

    DEBUG_PRINTF("Free heap after main menu: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}
// createGenderMenu (unchanged)
void createGenderMenu() {
    // Create the screen
    lv_obj_t* gender_screen = lv_obj_create(NULL);
    lv_obj_add_style(gender_screen, &style_screen, 0); // Assuming this sets basic screen style
    lv_obj_set_style_bg_color(gender_screen, lv_color_hex(0x1A1A1A), 0); // Dark gray background
    lv_obj_set_style_bg_opa(gender_screen, LV_OPA_COVER, 0);
    lv_obj_add_flag(gender_screen, LV_OBJ_FLAG_SCROLLABLE); // Screen scrolls if needed

    // Back Button (Top-Left)
    lv_obj_t* back_btn = lv_btn_create(gender_screen);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0); // Darker gray button
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT); // Left arrow icon
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    // Title
    lv_obj_t* title = lv_label_create(gender_screen);
    lv_label_set_text(title, "Select Gender");
    lv_obj_add_style(title, &style_title, 0); // Assuming this sets font/size
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Gender Options
    const char* genders[] = {"Male", "Female"};
    int y_offset = 80; // Start below title
    for (int i = 0; i < 2; i++) { // Only Male and Female from your original
        lv_obj_t* card = lv_obj_create(gender_screen);
        lv_obj_set_size(card, SCREEN_WIDTH - 40, 60);
        lv_obj_set_pos(card, 20, y_offset);
        lv_obj_add_style(card, &style_card, 0);
        lv_obj_add_style(card, &style_card_pressed, LV_STATE_PRESSED);

        lv_obj_t* label = lv_label_create(card);
        lv_label_set_text(label, genders[i]);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

        // Event callback based on gender
        if (strcmp(genders[i], "Male") == 0) {
            lv_obj_add_event_cb(card, [](lv_event_t* e) {
                DEBUG_PRINT("Male selected");
                currentEntry = "Male,";
                DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                createColorMenuShirt();
            }, LV_EVENT_CLICKED, NULL);
        } else if (strcmp(genders[i], "Female") == 0) {
            lv_obj_add_event_cb(card, [](lv_event_t* e) {
                DEBUG_PRINT("Female selected");
                currentEntry = "Female,";
                DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                createColorMenuShirt();
            }, LV_EVENT_CLICKED, NULL);
        }

        y_offset += 70; // Spacing between cards
    }

    lv_scr_load(gender_screen);
}
// createColorMenuShirt (unchanged)
void createColorMenuShirt() {
    DEBUG_PRINT("Creating Shirt Color Menu");

    if (colorMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: 0x%08x\n", (unsigned)colorMenu);
        lv_obj_del_async(colorMenu);
        colorMenu = nullptr;
    }
    shirt_next_btn = nullptr;
    pants_next_btn = nullptr;
    shoes_next_btn = nullptr;
    current_scroll_obj = nullptr;

    DEBUG_PRINTF("Free heap before creation: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    colorMenu = lv_obj_create(NULL);
    if (!colorMenu) {
        DEBUG_PRINT("Failed to create colorMenu");
        return;
    }
    DEBUG_PRINTF("New color menu created: 0x%08x\n", (unsigned)colorMenu);
    lv_obj_add_style(colorMenu, &style_screen, 0);
    lv_obj_set_style_bg_color(colorMenu, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_grad_color(colorMenu, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_grad_dir(colorMenu, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(colorMenu, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, SCREEN_WIDTH - 20, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 10, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Shirt Color");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title);

    // Grid of color buttons
    lv_obj_t* grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST}; // 2 columns
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST}; // 4 rows
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 5, 0);

    // Color mappings
    struct ColorInfo {
        const char* name;
        uint32_t color;
    };
    ColorInfo colorMap[] = {
        {"Red", 0xFF0000}, {"Orange", 0xFFA500}, {"Yellow", 0xFFFF00},
        {"Green", 0x00FF00}, {"Blue", 0x0000FF}, {"Purple", 0x800080},
        {"Black", 0x000000}, {"White", 0xFFFFFF}
    };
    const int numColors = sizeof(colorMap) / sizeof(colorMap[0]);
    DEBUG_PRINTF("Creating %d shirt color buttons\n", numColors);

    // Array to track selected buttons
    lv_obj_t* color_buttons[numColors] = {nullptr};
    selectedShirtColors = ""; // Reset selection

    for (int i = 0; i < numColors; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colorMap[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0); // Faded by default
        lv_obj_set_style_radius(btn, 15, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 5, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, colorMap[i].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(label);

        color_buttons[i] = btn;

        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
            lv_obj_t* label = lv_obj_get_child(btn, 0);
            if (label) {
                String color = lv_label_get_text(label);
                bool is_selected = lv_obj_get_style_border_width(btn, 0) > 0;
                if (is_selected) {
                    // Deselect
                    lv_obj_set_size(btn, 140, 60); // Reset size
                    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
                    lv_obj_set_style_border_width(btn, 0, 0);
                    lv_obj_set_style_shadow_width(btn, 5, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
                    if (selectedShirtColors.indexOf(color) != -1) {
                        selectedShirtColors.replace(color + "+", "");
                        selectedShirtColors.replace("+" + color, "");
                        selectedShirtColors.replace(color, "");
                    }
                } else {
                    // Select
                    lv_obj_set_size(btn, 150, 65); // Slightly larger
                    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
                    lv_obj_set_style_border_width(btn, 4, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x00FFFF), 0); // Cyan outline
                    lv_obj_set_style_shadow_width(btn, 12, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x00FFFF), 0); // Cyan glow
                    if (!selectedShirtColors.isEmpty()) selectedShirtColors += "+";
                    selectedShirtColors += color;
                }
                DEBUG_PRINTF("Shirt colors updated: %s\n", selectedShirtColors.c_str());
            }
        }, LV_EVENT_CLICKED, NULL);

        DEBUG_PRINTF("Created color button %d: %s\n", i, colorMap[i].name);
    }

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(colorMenu);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x6A6A6A), LV_STATE_PRESSED);
    lv_obj_set_style_radius(back_btn, 20, 0);
    lv_obj_set_style_shadow_width(back_btn, 5, 0);
    lv_obj_set_style_shadow_color(back_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(back_btn, LV_OPA_20, 0);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back " LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* old_menu = colorMenu;
        colorMenu = nullptr;
        selectedShirtColors = "";
        createGenderMenu();
        if (old_menu && old_menu != lv_scr_act()) {
            lv_obj_del_async(old_menu);
        }
    }, LV_EVENT_CLICKED, NULL);
    DEBUG_PRINT("Back button created");

    // Next Button
    shirt_next_btn = lv_btn_create(colorMenu);
    lv_obj_set_size(shirt_next_btn, 120, 40);
    lv_obj_align(shirt_next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(shirt_next_btn, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_bg_opa(shirt_next_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(shirt_next_btn, lv_color_hex(0x6A6A6A), LV_STATE_PRESSED);
    lv_obj_set_style_radius(shirt_next_btn, 20, 0);
    lv_obj_set_style_shadow_width(shirt_next_btn, 5, 0);
    lv_obj_set_style_shadow_color(shirt_next_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(shirt_next_btn, LV_OPA_20, 0);
    lv_obj_t* next_label = lv_label_create(shirt_next_btn);
    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(next_label, &lv_font_montserrat_16, 0);
    lv_obj_center(next_label);
    lv_obj_add_event_cb(shirt_next_btn, [](lv_event_t* e) {
        if (selectedShirtColors.isEmpty()) {
            lv_obj_t* header = lv_obj_get_child(lv_scr_act(), 0);
            lv_obj_set_style_bg_color(header, lv_color_hex(0xFF0000), 0);
            lv_timer_create([](lv_timer_t* timer) {
                lv_obj_t* header = static_cast<lv_obj_t*>(lv_timer_get_user_data(timer));
                lv_obj_set_style_bg_color(header, lv_color_hex(0x3A3A3A), 0);
                lv_timer_del(timer);
            }, 200, header);
        } else {
            lv_obj_t* old_menu = colorMenu;
            colorMenu = nullptr;
            currentEntry += selectedShirtColors + ",";
            DEBUG_PRINTF("Transitioning to pants menu with Shirt: %s\n", selectedShirtColors.c_str());
            createColorMenuPants();
            if (old_menu && old_menu != lv_scr_act()) {
                lv_obj_del_async(old_menu);
            }
        }
    }, LV_EVENT_CLICKED, NULL);
    DEBUG_PRINT("Next button created");

    lv_scr_load(colorMenu);
    DEBUG_PRINT("Shirt color menu loaded");
}
// createColorMenuPants (unchanged)
void createColorMenuPants() {
    DEBUG_PRINT("Creating Pants Color Menu");

    if (colorMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: 0x%08x\n", (unsigned)colorMenu);
        lv_obj_del_async(colorMenu);
        colorMenu = nullptr;
    }
    shirt_next_btn = nullptr;
    pants_next_btn = nullptr;
    shoes_next_btn = nullptr;
    current_scroll_obj = nullptr;

    DEBUG_PRINTF("Free heap before creation: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    colorMenu = lv_obj_create(NULL);
    if (!colorMenu) {
        DEBUG_PRINT("Failed to create colorMenu");
        return;
    }
    DEBUG_PRINTF("New color menu created: 0x%08x\n", (unsigned)colorMenu);
    lv_obj_add_style(colorMenu, &style_screen, 0);
    lv_obj_set_style_bg_color(colorMenu, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_grad_color(colorMenu, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_grad_dir(colorMenu, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(colorMenu, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, SCREEN_WIDTH - 20, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 10, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Pants Color");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title);

    // Grid of color buttons
    lv_obj_t* grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST}; // 2 columns
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST}; // 4 rows
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 5, 0);

    // Color mappings
    struct ColorInfo {
        const char* name;
        uint32_t color;
    };
    ColorInfo colorMap[] = {
        {"Red", 0xFF0000}, {"Orange", 0xFFA500}, {"Yellow", 0xFFFF00},
        {"Green", 0x00FF00}, {"Blue", 0x0000FF}, {"Purple", 0x800080},
        {"Black", 0x000000}, {"White", 0xFFFFFF}
    };
    const int numColors = sizeof(colorMap) / sizeof(colorMap[0]);
    DEBUG_PRINTF("Creating %d pants color buttons\n", numColors);

    // Array to track selected buttons
    lv_obj_t* color_buttons[numColors] = {nullptr};
    selectedPantsColors = ""; // Reset selection

    for (int i = 0; i < numColors; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colorMap[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0); // Faded by default
        lv_obj_set_style_radius(btn, 15, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 5, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, colorMap[i].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(label);

        color_buttons[i] = btn;

        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
            lv_obj_t* label = lv_obj_get_child(btn, 0);
            if (label) {
                String color = lv_label_get_text(label);
                bool is_selected = lv_obj_get_style_border_width(btn, 0) > 0;
                if (is_selected) {
                    // Deselect
                    lv_obj_set_size(btn, 140, 60); // Reset size
                    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
                    lv_obj_set_style_border_width(btn, 0, 0);
                    lv_obj_set_style_shadow_width(btn, 5, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
                    if (selectedPantsColors.indexOf(color) != -1) {
                        selectedPantsColors.replace(color + "+", "");
                        selectedPantsColors.replace("+" + color, "");
                        selectedPantsColors.replace(color, "");
                    }
                } else {
                    // Select
                    lv_obj_set_size(btn, 150, 65); // Slightly larger
                    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
                    lv_obj_set_style_border_width(btn, 4, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x00FFFF), 0); // Cyan outline
                    lv_obj_set_style_shadow_width(btn, 12, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x00FFFF), 0); // Cyan glow
                    if (!selectedPantsColors.isEmpty()) selectedPantsColors += "+";
                    selectedPantsColors += color;
                }
                DEBUG_PRINTF("Pants colors updated: %s\n", selectedPantsColors.c_str());
            }
        }, LV_EVENT_CLICKED, NULL);

        DEBUG_PRINTF("Created color button %d: %s\n", i, colorMap[i].name);
    }

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(colorMenu);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x6A6A6A), LV_STATE_PRESSED);
    lv_obj_set_style_radius(back_btn, 20, 0);
    lv_obj_set_style_shadow_width(back_btn, 5, 0);
    lv_obj_set_style_shadow_color(back_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(back_btn, LV_OPA_20, 0);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back " LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* old_menu = colorMenu;
        colorMenu = nullptr;
        selectedPantsColors = "";
        createColorMenuShirt();
        if (old_menu && old_menu != lv_scr_act()) {
            lv_obj_del_async(old_menu);
        }
    }, LV_EVENT_CLICKED, NULL);
    DEBUG_PRINT("Back button created");

    // Next Button
    pants_next_btn = lv_btn_create(colorMenu);
    lv_obj_set_size(pants_next_btn, 120, 40);
    lv_obj_align(pants_next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(pants_next_btn, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_bg_opa(pants_next_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(pants_next_btn, lv_color_hex(0x6A6A6A), LV_STATE_PRESSED);
    lv_obj_set_style_radius(pants_next_btn, 20, 0);
    lv_obj_set_style_shadow_width(pants_next_btn, 5, 0);
    lv_obj_set_style_shadow_color(pants_next_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(pants_next_btn, LV_OPA_20, 0);
    lv_obj_t* next_label = lv_label_create(pants_next_btn);
    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(next_label, &lv_font_montserrat_16, 0);
    lv_obj_center(next_label);
    lv_obj_add_event_cb(pants_next_btn, [](lv_event_t* e) {
        if (selectedPantsColors.isEmpty()) {
            lv_obj_t* header = lv_obj_get_child(lv_scr_act(), 0);
            lv_obj_set_style_bg_color(header, lv_color_hex(0xFF0000), 0);
            lv_timer_create([](lv_timer_t* timer) {
                lv_obj_t* header = static_cast<lv_obj_t*>(lv_timer_get_user_data(timer));
                lv_obj_set_style_bg_color(header, lv_color_hex(0x3A3A3A), 0);
                lv_timer_del(timer);
            }, 200, header);
        } else {
            lv_obj_t* old_menu = colorMenu;
            colorMenu = nullptr;
            currentEntry += selectedPantsColors + ",";
            DEBUG_PRINTF("Transitioning to shoes menu with Pants: %s\n", selectedPantsColors.c_str());
            createColorMenuShoes();
            if (old_menu && old_menu != lv_scr_act()) {
                lv_obj_del_async(old_menu);
            }
        }
    }, LV_EVENT_CLICKED, NULL);
    DEBUG_PRINT("Next button created");

    lv_scr_load(colorMenu);
    DEBUG_PRINT("Pants color menu loaded");
}
// createColorMenuShoes (unchanged)
void createColorMenuShoes() {
    DEBUG_PRINT("Creating Shoes Color Menu");

    if (colorMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: 0x%08x\n", (unsigned)colorMenu);
        lv_obj_del_async(colorMenu);
        colorMenu = nullptr;
    }
    shirt_next_btn = nullptr;
    pants_next_btn = nullptr;
    shoes_next_btn = nullptr;
    current_scroll_obj = nullptr;

    DEBUG_PRINTF("Free heap before creation: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    colorMenu = lv_obj_create(NULL);
    if (!colorMenu) {
        DEBUG_PRINT("Failed to create colorMenu");
        return;
    }
    DEBUG_PRINTF("New color menu created: 0x%08x\n", (unsigned)colorMenu);
    lv_obj_add_style(colorMenu, &style_screen, 0);
    lv_obj_set_style_bg_color(colorMenu, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_grad_color(colorMenu, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_grad_dir(colorMenu, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(colorMenu, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, SCREEN_WIDTH - 20, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x3A3A3A), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 10, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Shoes Color");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(title);

    // Grid of color buttons
    lv_obj_t* grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST}; // 2 columns
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST}; // 4 rows
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 5, 0);

    // Color mappings
    struct ColorInfo {
        const char* name;
        uint32_t color;
    };
    ColorInfo colorMap[] = {
        {"Red", 0xFF0000}, {"Orange", 0xFFA500}, {"Yellow", 0xFFFF00},
        {"Green", 0x00FF00}, {"Blue", 0x0000FF}, {"Purple", 0x800080},
        {"Black", 0x000000}, {"White", 0xFFFFFF}
    };
    const int numColors = sizeof(colorMap) / sizeof(colorMap[0]);
    DEBUG_PRINTF("Creating %d shoes color buttons\n", numColors);

    // Array to track selected buttons
    lv_obj_t* color_buttons[numColors] = {nullptr};
    selectedShoesColors = ""; // Reset selection

    for (int i = 0; i < numColors; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colorMap[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0); // Faded by default
        lv_obj_set_style_radius(btn, 15, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 5, 0);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);

        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, colorMap[i].name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(label);

        color_buttons[i] = btn;

        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
            lv_obj_t* label = lv_obj_get_child(btn, 0);
            if (label) {
                String color = lv_label_get_text(label);
                bool is_selected = lv_obj_get_style_border_width(btn, 0) > 0;
                if (is_selected) {
                    // Deselect
                    lv_obj_set_size(btn, 140, 60); // Reset size
                    lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
                    lv_obj_set_style_border_width(btn, 0, 0);
                    lv_obj_set_style_shadow_width(btn, 5, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), 0);
                    if (selectedShoesColors.indexOf(color) != -1) {
                        selectedShoesColors.replace(color + "+", "");
                        selectedShoesColors.replace("+" + color, "");
                        selectedShoesColors.replace(color, "");
                    }
                } else {
                    // Select
                    lv_obj_set_size(btn, 150, 65); // Slightly larger
                    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
                    lv_obj_set_style_border_width(btn, 4, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x00FFFF), 0); // Cyan outline
                    lv_obj_set_style_shadow_width(btn, 12, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x00FFFF), 0); // Cyan glow
                    if (!selectedShoesColors.isEmpty()) selectedShoesColors += "+";
                    selectedShoesColors += color;
                }
                DEBUG_PRINTF("Shoes colors updated: %s\n", selectedShoesColors.c_str());
            }
        }, LV_EVENT_CLICKED, NULL);

        DEBUG_PRINTF("Created color button %d: %s\n", i, colorMap[i].name);
    }

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(colorMenu);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x6A6A6A), LV_STATE_PRESSED);
    lv_obj_set_style_radius(back_btn, 20, 0);
    lv_obj_set_style_shadow_width(back_btn, 5, 0);
    lv_obj_set_style_shadow_color(back_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(back_btn, LV_OPA_20, 0);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back " LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* old_menu = colorMenu;
        colorMenu = nullptr;
        selectedShoesColors = "";
        createColorMenuPants();
        if (old_menu && old_menu != lv_scr_act()) {
            lv_obj_del_async(old_menu);
        }
    }, LV_EVENT_CLICKED, NULL);
    DEBUG_PRINT("Back button created");

    // Next Button
    shoes_next_btn = lv_btn_create(colorMenu);
    lv_obj_set_size(shoes_next_btn, 120, 40);
    lv_obj_align(shoes_next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(shoes_next_btn, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_bg_opa(shoes_next_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(shoes_next_btn, lv_color_hex(0x6A6A6A), LV_STATE_PRESSED);
    lv_obj_set_style_radius(shoes_next_btn, 20, 0);
    lv_obj_set_style_shadow_width(shoes_next_btn, 5, 0);
    lv_obj_set_style_shadow_color(shoes_next_btn, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(shoes_next_btn, LV_OPA_20, 0);
    lv_obj_t* next_label = lv_label_create(shoes_next_btn);
    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(next_label, &lv_font_montserrat_16, 0);
    lv_obj_center(next_label);
    lv_obj_add_event_cb(shoes_next_btn, [](lv_event_t* e) {
        if (selectedShoesColors.isEmpty()) {
            lv_obj_t* header = lv_obj_get_child(lv_scr_act(), 0);
            lv_obj_set_style_bg_color(header, lv_color_hex(0xFF0000), 0);
            lv_timer_create([](lv_timer_t* timer) {
                lv_obj_t* header = static_cast<lv_obj_t*>(lv_timer_get_user_data(timer));
                lv_obj_set_style_bg_color(header, lv_color_hex(0x3A3A3A), 0);
                lv_timer_del(timer);
            }, 200, header);
        } else {
            lv_obj_t* old_menu = colorMenu;
            colorMenu = nullptr;
            currentEntry += selectedShoesColors + ",";
            DEBUG_PRINTF("Transitioning to item menu with Shoes: %s\n", selectedShoesColors.c_str());
            createItemMenu();
            if (old_menu && old_menu != lv_scr_act()) {
                lv_obj_del_async(old_menu);
            }
        }
    }, LV_EVENT_CLICKED, NULL);
    DEBUG_PRINT("Next button created");

    lv_scr_load(colorMenu);
    DEBUG_PRINT("Shoes color menu loaded");
}
// createItemMenu (Updated scroll logic & loop condition)
void createItemMenu() {
    if (itemMenu) { lv_obj_del(itemMenu); itemMenu = nullptr; }
    itemMenu = lv_obj_create(NULL); lv_obj_add_style(itemMenu, &style_screen, 0); lv_scr_load(itemMenu);
    lv_obj_t *header = lv_obj_create(itemMenu); lv_obj_set_size(header, 320, 50); lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header); lv_label_set_text(title, "Select Item"); lv_obj_add_style(title, &style_title, 0); lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *list_cont = lv_obj_create(itemMenu); lv_obj_set_size(list_cont, 280, 180); lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(list_cont, lv_color_hex(0x4A4A4A), 0); lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list_cont, 5, 0); lv_obj_set_scroll_dir(list_cont, LV_DIR_VER); lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(list_cont, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_MOMENTUM)); // Corrected flag usage with cast
    for (int i = 0; i < NUM_ITEMS; i++) { // Use calculated NUM_ITEMS
        lv_obj_t *btn = lv_btn_create(list_cont); lv_obj_set_width(btn, lv_pct(100)); lv_obj_set_height(btn, 40);
        lv_obj_add_style(btn, &style_btn, 0); lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn); lv_label_set_text(label, items[i]); lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t* target_btn = (lv_obj_t*)lv_event_get_target(e); // Corrected cast
            lv_obj_t* label = lv_obj_get_child(target_btn, 0);
            if (label) { currentEntry += String(lv_label_get_text(label)); createConfirmScreen(); }
        }, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_update_layout(list_cont); // Update layout before checking scroll height
    if (lv_obj_get_content_height(list_cont) > lv_obj_get_height(list_cont)) current_scroll_obj = list_cont; else current_scroll_obj = nullptr;
}
// createConfirmScreen (unchanged)
void createConfirmScreen() {
    DEBUG_PRINT("Creating confirmation screen");
    
    // Clean up any existing confirm screen first
    if (confirmScreen) {
        DEBUG_PRINT("Cleaning existing confirm screen");
        lv_obj_del(confirmScreen);
        confirmScreen = nullptr;
        lv_timer_handler(); // Process deletion immediately
    }
    
    // Create new confirmation screen
    confirmScreen = lv_obj_create(NULL);
    if (!confirmScreen) {
        DEBUG_PRINT("Failed to create confirmation screen");
        createMainMenu(); // Fallback to main menu
        return;
    }
    
    lv_obj_add_style(confirmScreen, &style_screen, 0);
    lv_scr_load(confirmScreen);
    DEBUG_PRINTF("Confirm screen created: %p\n", confirmScreen);
    
    lv_timer_handler(); // Process any pending UI updates

    lv_obj_t *header = lv_obj_create(confirmScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Confirm Entry");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Create a local copy of the formatted entry to avoid memory issues
    String formattedEntryStr = getFormattedEntry(currentEntry);
    
    lv_obj_t *preview = lv_label_create(confirmScreen);
    lv_label_set_text(preview, formattedEntryStr.c_str());
    lv_obj_add_style(preview, &style_text, 0);
    lv_obj_set_size(preview, 280, 140);
    lv_obj_align(preview, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(preview, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_pad_all(preview, 10, 0);

    lv_obj_t *btn_container = lv_obj_create(confirmScreen);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, 320, 50);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Create a local copy of the current entry for the event callbacks
    String entryToSave = currentEntry;
    
    lv_obj_t *btn_confirm = lv_btn_create(btn_container);
    lv_obj_set_size(btn_confirm, 120, 40);
    lv_obj_add_style(btn_confirm, &style_btn, 0);
    lv_obj_add_style(btn_confirm, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *confirm_label = lv_label_create(btn_confirm);
    lv_label_set_text(confirm_label, "Confirm");
    lv_obj_center(confirm_label);
    
    // Store a copy of the current entry in user data to avoid using the global variable
    lv_obj_set_user_data(btn_confirm, (void*)strdup(formattedEntryStr.c_str()));
    
    lv_obj_add_event_cb(btn_confirm, [](lv_event_t *e) {
        lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
        saveEntry(currentEntry); // Save raw entry
        createMainMenu();
        DEBUG_PRINT("Returning to main menu after confirmation");
    }, LV_EVENT_CLICKED, NULL);
    
    // Process UI updates to ensure everything is properly initialized
    lv_timer_handler();
}

void loadSavedNetworks() {
    Preferences prefs;
    prefs.begin("wifi_config", false);
    
    numSavedNetworks = prefs.getInt("numNetworks", 0);
    numSavedNetworks = min(numSavedNetworks, MAX_NETWORKS);
    
    for (int i = 0; i < numSavedNetworks; i++) {
        char ssidKey[16];
        char passKey[16];
        sprintf(ssidKey, "ssid%d", i);
        sprintf(passKey, "pass%d", i);
        
        String ssid = prefs.getString(ssidKey, "");
        String pass = prefs.getString(passKey, "");
        
        strncpy(savedNetworks[i].ssid, ssid.c_str(), 32);
        savedNetworks[i].ssid[32] = '\0';
        strncpy(savedNetworks[i].password, pass.c_str(), 64);
        savedNetworks[i].password[64] = '\0';
    }
    
    if (numSavedNetworks == 0) {
        strncpy(savedNetworks[0].ssid, DEFAULT_SSID, 32);
        strncpy(savedNetworks[0].password, DEFAULT_PASS, 64);
        numSavedNetworks = 1;
        saveNetworks();
    }
    
    prefs.end();
}

void saveNetworks() {
    Preferences prefs;
    prefs.begin("wifi_config", false);
    
    prefs.putInt("numNetworks", numSavedNetworks);
    
    for (int i = 0; i < numSavedNetworks; i++) {
        char ssidKey[16];
        char passKey[16];
        
        sprintf(ssidKey, "ssid%d", i);
        sprintf(passKey, "pass%d", i);
        
        prefs.putString(ssidKey, savedNetworks[i].ssid);
        prefs.putString(passKey, savedNetworks[i].password);
        
        DEBUG_PRINTF("Saved network %d: %s\n", i, savedNetworks[i].ssid);
    }
    
    prefs.end();
}

void connectToSavedNetworks() {
    DEBUG_PRINT("Attempting to connect to saved networks");
    
    if (!wifiEnabled) {
        DEBUG_PRINT("WiFi is disabled in settings, not connecting");
        return;
    }
    
    if (numSavedNetworks == 0 || WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINT("No saved networks or already connected");
        return;
    }
    
    WiFi.disconnect();
    delay(100);
    
    bool connected = false;
    
    for (int i = 0; i < numSavedNetworks; i++) {
        DEBUG_PRINTF("Trying to connect to %s\n", savedNetworks[i].ssid);
        
        WiFi.begin(savedNetworks[i].ssid, savedNetworks[i].password);
        
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20) {
            delay(500);
            DEBUG_PRINT(".");
            timeout++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            DEBUG_PRINTF("Connected to %s\n", savedNetworks[i].ssid);
            connected = true;
            wifiConnectionAttempts = 0;
            break;
        } else {
            DEBUG_PRINTF("Failed to connect to %s\n", savedNetworks[i].ssid);
            lastWiFiConnectionAttempt = millis();
            wifiConnectionAttempts = 1;
        }
    }
    
    if (!connected) {
        DEBUG_PRINT("Could not connect to any saved networks");
    }
}

void onWiFiStatus(WiFiState state, const String& message) {
    DEBUG_PRINTF("WiFi Status: %s - %s\n", wifiManager.getStateString().c_str(), message.c_str());
    
    // Update status bar if visible
    if (status_bar) {
        lv_label_set_text(status_bar, message.c_str());
        lv_obj_set_style_text_color(status_bar, 
            state == WiFiState::WIFI_CONNECTED ? lv_color_hex(0x00FF00) : 
            state == WiFiState::WIFI_CONNECTING ? lv_color_hex(0xFFFF00) : 
            lv_color_hex(0xFF0000), 0);
    }
    
    // Update loading screen if active
    if (wifi_loading_screen != nullptr && lv_obj_is_valid(wifi_loading_screen) && 
        lv_scr_act() == wifi_loading_screen) {
        if (state == WiFiState::WIFI_CONNECTED) {
            updateWiFiLoadingScreen(true, "WiFi Connected!");
        } else if (state == WiFiState::WIFI_DISCONNECTED && 
                  (message.indexOf("failed") >= 0 || message.indexOf("Failed") >= 0)) {
            updateWiFiLoadingScreen(false, "Connection Failed!");
        }
    }
}

void onWiFiScanComplete(const std::vector<NetworkInfo>& results) {
    // Attempt to take the semaphore
    if (xSemaphoreTake(xGuiSemaphore, (TickType_t)100 / portTICK_PERIOD_MS) == pdTRUE) {
        DEBUG_PRINTF("onWiFiScanComplete: Got semaphore. Processing %d results.\n", results.size());

        if (wifi_screen && lv_obj_is_valid(wifi_screen) && wifi_list && lv_obj_is_valid(wifi_list)) {
            // Remove spinner
            if (g_spinner != nullptr && lv_obj_is_valid(g_spinner)) {
                 lv_obj_del(g_spinner);
                 g_spinner = nullptr;
            } else if (g_spinner != nullptr) { g_spinner = nullptr; }

            DEBUG_PRINT("Cleaning wifi_list");
            lv_obj_clean(wifi_list); // Clear previous list items

            if (results.empty()) {
                if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text(wifi_status_label, "No networks found");
                }
                DEBUG_PRINT("No networks found.");
            } else {
                 if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text(wifi_status_label, "Populating list..."); // Indicate start
                 }
                 DEBUG_PRINTF("Populating list with %d networks...\n", results.size());

                 int count = 0;
                 const int BATCH_SIZE = 5; // Process N items before yielding slightly

                 for (const auto& net : results) {
                    if (net.ssid.isEmpty()) continue;

                    String displayText = net.ssid;
                    if (net.encryptionType != WIFI_AUTH_OPEN) {
                        displayText += " *";
                    }
                    // DEBUG_PRINTF("  Adding: %s\n", displayText.c_str());

                    lv_obj_t* btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, displayText.c_str());

                    if (btn) {
                        lv_obj_add_style(btn, &style_btn, 0);

                        char* ssid_copy = strdup(net.ssid.c_str());
                        if (ssid_copy) {
                            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                                // ... (callback content unchanged) ...
                                char* stored_ssid = (char*)lv_event_get_user_data(e);
                                if (stored_ssid) {
                                    strncpy(selected_ssid, stored_ssid, sizeof(selected_ssid) - 1);
                                    selected_ssid[sizeof(selected_ssid) - 1] = '\0';
                                    DEBUG_PRINTF("Selected network: %s\n", selected_ssid);
                                    showWiFiKeyboard();
                                    free(stored_ssid);
                                    lv_obj_set_user_data((lv_obj_t*)lv_event_get_target(e), NULL);
                                }
                            }, LV_EVENT_CLICKED, (void*)ssid_copy);
                        } else {
                             DEBUG_PRINT("Failed to allocate memory for SSID copy");
                             lv_obj_del(btn);
                             continue;
                        }
                        count++;

                        // --- ADDED: Incremental Processing ---
                        if (count % BATCH_SIZE == 0) {
                            DEBUG_PRINTF("Processed batch of %d items, calling lv_task_handler...\n", BATCH_SIZE);
                            lv_task_handler(); // Allow LVGL to process UI updates for this batch
                            // Optional small delay if still having issues, but task_handler might be enough
                            // vTaskDelay(pdMS_TO_TICKS(5));
                        }
                        // --- END ADDED ---

                    } else {
                         DEBUG_PRINTF("Failed to create list button for: %s\n", displayText.c_str());
                    }
                 } // End of for loop

                 DEBUG_PRINTF("Finished adding %d networks to the list widget.\n", count);

                 // Call handler one last time after the loop finishes for any remaining items
                 DEBUG_PRINT("Calling final lv_task_handler() after loop.");
                 lv_task_handler();
                 DEBUG_PRINT("Finished final lv_task_handler() call.");

                 // Update status label after populating and processing
                 if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text_fmt(wifi_status_label, "Scan complete (%d found)", results.size());
                 }
            }
        } else {
             DEBUG_PRINT("onWiFiScanComplete: wifi_screen or wifi_list no longer valid after taking semaphore.");
        }

        xSemaphoreGive(xGuiSemaphore);
        DEBUG_PRINT("onWiFiScanComplete: Released semaphore.");

    } else {
        DEBUG_PRINT("onWiFiScanComplete: Failed to get GUI semaphore. Skipping UI update.");
    }
}

void showWiFiLoadingScreen(const String& ssid) {
    if (wifi_loading_screen != nullptr) {
        lv_obj_del(wifi_loading_screen);
    }
    
    wifi_loading_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_loading_screen, &style_screen, 0);
    
    // Create header
    lv_obj_t* header = lv_obj_create(wifi_loading_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi Connection");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Create spinner
    wifi_loading_spinner = lv_spinner_create(wifi_loading_screen); // LVGL v9: Only parent argument
    lv_obj_set_size(wifi_loading_spinner, 100, 100);
    lv_obj_align(wifi_loading_spinner, LV_ALIGN_CENTER, 0, -20);
    
    // Create connecting label
    wifi_loading_label = lv_label_create(wifi_loading_screen);
    lv_label_set_text_fmt(wifi_loading_label, "Connecting to %s...", ssid.c_str());
    lv_obj_align(wifi_loading_label, LV_ALIGN_CENTER, 0, 60);
    
    // Create result label (hidden initially)
    wifi_result_label = lv_label_create(wifi_loading_screen);
    lv_obj_set_style_text_font(wifi_result_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(wifi_result_label, "");
    lv_obj_align(wifi_result_label, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_flag(wifi_result_label, LV_OBJ_FLAG_HIDDEN);
    
    // Create back button (hidden initially)
    lv_obj_t* back_btn = lv_btn_create(wifi_loading_screen);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createWiFiManagerScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Store back button reference for later use
    lv_obj_set_user_data(wifi_loading_screen, back_btn);
    
    lv_scr_load(wifi_loading_screen);
}

void updateWiFiLoadingScreen(bool success, const String& message) {
    if (wifi_loading_screen == nullptr || !lv_obj_is_valid(wifi_loading_screen)) {
        return;
    }
    
    // Hide spinner
    if (wifi_loading_spinner != nullptr && lv_obj_is_valid(wifi_loading_spinner)) {
        lv_obj_add_flag(wifi_loading_spinner, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Update result label
    if (wifi_result_label != nullptr && lv_obj_is_valid(wifi_result_label)) {
        lv_obj_clear_flag(wifi_result_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(wifi_result_label, message.c_str());
        lv_obj_set_style_text_color(wifi_result_label, 
            success ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    }
    
    // Show back button
    lv_obj_t* back_btn = (lv_obj_t*)lv_obj_get_user_data(wifi_loading_screen);
    if (back_btn != nullptr && lv_obj_is_valid(back_btn)) {
        lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Auto return to WiFi manager screen after successful connection
    if (success) {
        static lv_timer_t* return_timer = nullptr;
        if (return_timer != nullptr) {
            lv_timer_del(return_timer);
        }
        return_timer = lv_timer_create([](lv_timer_t* timer) {
            createWiFiManagerScreen();
            lv_timer_del(timer);
        }, 2000, NULL);
    }
}


// getFormattedEntry (unchanged)
String getFormattedEntry(const String& entry) {
    String entryData = entry;
    String timestamp = getTimestamp(); // Default to current timestamp

    // Check if the entry contains a timestamp
    int colonPos = entry.indexOf(", ");
    if (colonPos > 0 && colonPos <= 19 && entry.substring(0, 2).toInt() > 0) {
        timestamp = entry.substring(0, colonPos + 1);
        entryData = entry.substring(colonPos + 2);
        DEBUG_PRINTF("Extracted timestamp: %s\n", timestamp.c_str());
        DEBUG_PRINTF("Extracted entry data: %s\n", entryData.c_str());
    } else {
        entryData = entry;
        DEBUG_PRINTF("No timestamp found, using raw entry: %s\n", entryData.c_str());
    }

    // Parse the entry data into parts
    String parts[5];
    int partCount = 0, startIdx = 0;
    DEBUG_PRINTF("Parsing entryData: %s (length: %d)\n", entryData.c_str(), entryData.length());
    for (int i = 0; i < entryData.length() && partCount < 5; i++) {
        if (entryData.charAt(i) == ',') {
            parts[partCount] = entryData.substring(startIdx, i);
            DEBUG_PRINTF("Part %d: %s (from %d to %d)\n", partCount, parts[partCount].c_str(), startIdx, i);
            partCount++;
            startIdx = i + 1;
        }
    }
    if (startIdx < entryData.length()) {
        parts[partCount] = entryData.substring(startIdx);
        DEBUG_PRINTF("Part %d: %s (from %d to end)\n", partCount, parts[partCount].c_str(), startIdx);
        partCount++;
    }
    DEBUG_PRINTF("Total parts found: %d\n", partCount);

    // Format the output
    String formatted = "Time: " + timestamp + "\n";
    formatted += "Gender: " + (partCount > 0 ? parts[0] : "N/A") + "\n";
    formatted += "Shirt: " + (partCount > 1 ? parts[1] : "N/A") + "\n";
    formatted += "Pants: " + (partCount > 2 ? parts[2] : "N/A") + "\n";
    formatted += "Shoes: " + (partCount > 3 ? parts[3] : "N/A") + "\n";
    formatted += "Item: " + (partCount > 4 ? parts[4] : "N/A");

    DEBUG_PRINTF("Formatted: %s\n", formatted.c_str());
    return formatted;
}
// initStyles (unchanged)
void initStyles() {
    // Screen style - Deep black background
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x121212));
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);

    // Button style - Vibrant red
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0xE31B23));
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_radius(&style_btn, 8);

    // Button pressed style - Darker red
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xA81419));
    lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);

    // Title style - Bright white text
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));

    // Card action style - Dark gray with red accent
    lv_style_init(&style_card_action);
    lv_style_set_bg_color(&style_card_action, lv_color_hex(0x2A2A2A));
    lv_style_set_bg_opa(&style_card_action, LV_OPA_COVER);
    lv_style_set_border_color(&style_card_action, lv_color_hex(0xE31B23));
    lv_style_set_border_width(&style_card_action, 2);
    lv_style_set_radius(&style_card_action, 10);
    lv_style_set_text_color(&style_card_action, lv_color_hex(0xFFFFFF)); // Ensure text is white

    // Card pressed style - Highlighted state
    lv_style_init(&style_card_pressed);
    lv_style_set_bg_color(&style_card_pressed, lv_color_hex(0x3A3A3A));
    lv_style_set_bg_opa(&style_card_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&style_card_pressed, 3);
    lv_style_set_text_color(&style_card_pressed, lv_color_hex(0xFFFFFF)); // Ensure text is white

    // Card info style (for WiFi/Battery cards) - Dark red with white text
    lv_style_init(&style_card_info);
    lv_style_set_bg_color(&style_card_info, lv_color_hex(0x8B0000));  // Deep red background
    lv_style_set_bg_opa(&style_card_info, LV_OPA_COVER);
    lv_style_set_border_color(&style_card_info, lv_color_hex(0xFFFFFF));  // White border
    lv_style_set_border_width(&style_card_info, 1);
    lv_style_set_radius(&style_card_info, 10);
    lv_style_set_text_color(&style_card_info, lv_color_hex(0xFFFFFF));  // Pure white text
    lv_style_set_shadow_color(&style_card_info, lv_color_hex(0x000000));
    lv_style_set_shadow_width(&style_card_info, 10);
    lv_style_set_shadow_opa(&style_card_info, LV_OPA_30);

    // Text style - Clear white
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);

    // Keyboard button style
    lv_style_init(&style_keyboard_btn);
    lv_style_set_bg_color(&style_keyboard_btn, lv_color_hex(0x3D3D3D));
    lv_style_set_text_color(&style_keyboard_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&style_keyboard_btn, 8);
    lv_style_set_border_width(&style_keyboard_btn, 0);

    // Define styles for gender cards
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(0x2D2D2D)); // Dark gray for cards
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, lv_color_hex(0x505050)); // Light gray border
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, 5);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000)); // Black shadow
    lv_style_set_shadow_width(&style_card, 10);
    lv_style_set_shadow_spread(&style_card, 2);
    lv_style_set_pad_all(&style_card, 5);
    
    lv_style_init(&style_card_pressed);
    lv_style_set_bg_color(&style_card_pressed, lv_color_hex(0x404040)); // Lighter gray when pressed
    lv_style_set_bg_opa(&style_card_pressed, LV_OPA_COVER);

    DEBUG_PRINT("Styles initialized with red, white & black theme");
}
void listSavedEntries() {
    if (!fileSystemInitialized) {
        DEBUG_PRINT("File system not initialized - skipping listSavedEntries");
        return;
    }

    releaseSPIBus();
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    File file = SD.open(LOG_FILENAME, FILE_READ);
    if (!file) {
        DEBUG_PRINT("Failed to open log file for reading");
        SPI_SD.end();
        SPI.begin();
        return;
    }

    parsedLogEntries.clear();
    String entryText = "";

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() == 0 && entryText.length() > 0) {
            LogEntry entry;
            entry.text = entryText;
            entry.timestamp = parseTimestamp(entryText);
            parsedLogEntries.push_back(entry);
            entryText = "";
        } else if (line.length() > 0) {
            entryText += line + "\n";
        }
    }

    if (entryText.length() > 0) {
        LogEntry entry;
        entry.text = entryText;
        entry.timestamp = parseTimestamp(entryText);
        parsedLogEntries.push_back(entry);
    }

    file.close();
    SPI_SD.end();
    SPI.begin();
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);

    DEBUG_PRINTF("Total entries loaded: %d\n", parsedLogEntries.size());
}
// println_log / printf_log (unchanged)
void println_log(const char *str) {
    Serial.println(str);
    // If we have a status label, update it
    if (status_bar != nullptr) {
        lv_label_set_text(status_bar, str);
    }
}
void printf_log(const char *format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 256, format, args);
    va_end(args);
    Serial.print(buf);
    // If we have a status label, update it
    if (status_bar != nullptr) {
        lv_label_set_text(status_bar, buf);
    }
}
// sendWebhook (unchanged)
void sendWebhook(const String& entry) {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINT("WiFi not connected, skipping webhook");
        return;
    }

    HTTPClient http;
    const char* zapierUrl = "https://hooks.zapier.com/hooks/catch/21957602/2qk3799/"; // Replace with your Zapier URL
    Serial.println("Starting HTTP client...");
    DEBUG_PRINTF("Webhook URL: %s\n", zapierUrl);
    http.setReuse(false);
    if (!http.begin(zapierUrl)) {
        DEBUG_PRINT("Failed to begin HTTP client");
        return;
    }
    http.addHeader("Content-Type", "application/json");

    // Structure payload as key-value pairs for Zapier
    String timestamp = getTimestamp();
    String jsonPayload = "{";
    jsonPayload += "\"timestamp\":\"" + timestamp + "\",";
    int colonPos = entry.indexOf(",");
    int lastIndex = 0;
    jsonPayload += "\"gender\":\"" + entry.substring(lastIndex, colonPos) + "\",";
    lastIndex = colonPos + 1;
    colonPos = entry.indexOf(",", lastIndex);
    jsonPayload += "\"shirt\":\"" + entry.substring(lastIndex, colonPos) + "\",";
    lastIndex = colonPos + 1;
    colonPos = entry.indexOf(",", lastIndex);
    jsonPayload += "\"pants\":\"" + entry.substring(lastIndex, colonPos) + "\",";
    lastIndex = colonPos + 1;
    colonPos = entry.indexOf(",", lastIndex);
    jsonPayload += "\"shoes\":\"" + entry.substring(lastIndex, colonPos) + "\",";
    lastIndex = colonPos + 1;
    jsonPayload += "\"item\":\"" + entry.substring(lastIndex) + "\"";
    jsonPayload += "}";

    DEBUG_PRINTF("Sending webhook payload: %s\n", jsonPayload.c_str());
    int httpCode = http.POST(jsonPayload);
    DEBUG_PRINTF("HTTP response code: %d\n", httpCode);
    if (httpCode > 0) {
        if (httpCode == 200 || httpCode == 201) { // Zapier returns 200/201
            DEBUG_PRINT("Webhook sent successfully");
            updateStatus("Webhook Sent", 0x00FF00);
        } else {
            String response = http.getString();
            DEBUG_PRINTF("Webhook failed, response: %s\n", response.c_str());
            updateStatus("Webhook Failed", 0xFF0000);
        }
    } else {
        DEBUG_PRINTF("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        updateStatus("Webhook Failed", 0xFF0000);
    }
    http.end();
}
// createSettingsScreen (unchanged)
void createSettingsScreen() {
    if (settingsScreen) {
        lv_obj_del(settingsScreen);
        settingsScreen = nullptr;
    }
    
    settingsScreen = lv_obj_create(NULL);
    lv_obj_add_style(settingsScreen, &style_screen, 0);
    
    // Header
    lv_obj_t* header = lv_obj_create(settingsScreen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
        // Settings list
    lv_obj_t* list = lv_list_create(settingsScreen);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 72);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_radius(list, 15, 0); // Add rounded corners with 15px radius
    lv_obj_set_style_border_width(list, 0, 0); // Optional: remove border

    lv_obj_set_style_shadow_width(list, 10, 0);
    lv_obj_set_style_shadow_opa(list, LV_OPA_20, 0); // 20% opacity shadow
    lv_obj_set_style_shadow_color(list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_ofs_y(list, 5, 0); // Shadow slightly below
    
    // WiFi settings
    lv_obj_t* wifi_btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi Settings");
    lv_obj_add_event_cb(wifi_btn, [](lv_event_t* e) {
        createWiFiManagerScreen(); // Corrected typo
    }, LV_EVENT_CLICKED, NULL);
    
    // Sound settings
    lv_obj_t* sound_btn = lv_list_add_btn(list, LV_SYMBOL_AUDIO, "Sound Settings");
    lv_obj_add_event_cb(sound_btn, [](lv_event_t* e) {
        createSoundSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Display settings
    lv_obj_t* display_btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Display Settings");
    lv_obj_add_event_cb(display_btn, [](lv_event_t* e) {
        createBrightnessSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Date & Time settings
    lv_obj_t* datetime_btn = lv_list_add_btn(list, LV_SYMBOL_CALL, "Date & Time");
    lv_obj_add_event_cb(datetime_btn, [](lv_event_t* e) {
        createDateSelectionScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Power management
    lv_obj_t* power_btn = lv_list_add_btn(list, LV_SYMBOL_BATTERY_FULL, "Power Management");
    lv_obj_add_event_cb(power_btn, [](lv_event_t* e) {
        createPowerManagementScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Back Button (Top-Left)
    lv_obj_t* back_btn = lv_btn_create(settingsScreen);
    lv_obj_set_size(back_btn, 35, 23);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0); // Darker gray button
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT); // Left arrow icon
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0); // White text
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(settingsScreen);
}
// createSoundSettingsScreen (unchanged)
void createSoundSettingsScreen() {
    static lv_obj_t* sound_settings_screen = nullptr;
    if (sound_settings_screen) {
        lv_obj_del(sound_settings_screen);
        sound_settings_screen = nullptr;
    }
    sound_settings_screen = lv_obj_create(NULL);
    if (!sound_settings_screen) {
        DEBUG_PRINT("Failed to create sound_settings_screen");
        return;
    }
    DEBUG_PRINTF("Created sound_settings_screen: %p\n", sound_settings_screen);
    
    lv_obj_add_style(sound_settings_screen, &style_screen, 0);
    lv_scr_load(sound_settings_screen);
    DEBUG_PRINT("Screen loaded");

    // Header with gradient
    lv_obj_t* header = lv_obj_create(sound_settings_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0x357ABD), 0);
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Sound Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Sound Toggle Switch
    lv_obj_t* sound_toggle_label = lv_label_create(sound_settings_screen);
    lv_label_set_text(sound_toggle_label, "Sound Enable");
    lv_obj_align(sound_toggle_label, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_obj_add_style(sound_toggle_label, &style_text, 0);

    lv_obj_t* sound_switch = lv_switch_create(sound_settings_screen);
    lv_obj_align(sound_switch, LV_ALIGN_TOP_RIGHT, -20, 65);
    lv_obj_set_size(sound_switch, 50, 25);
    if (M5.Speaker.isEnabled()) {
        lv_obj_add_state(sound_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sound_switch, [](lv_event_t* e) {
        Preferences prefs;
        prefs.begin("settings", false);
        bool enabled = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
        uint8_t current_volume = prefs.getUChar("volume", 128); // Get saved volume
        M5.Speaker.setVolume(enabled ? current_volume : 0); // Restore volume or mute
        if (enabled) M5.Speaker.tone(440, 100); // Play 440Hz test tone
        prefs.putBool("sound_enabled", enabled); // Save state
        DEBUG_PRINTF("Sound %s\n", enabled ? "Enabled" : "Disabled");
        prefs.end();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Volume Slider
    lv_obj_t* volume_label = lv_label_create(sound_settings_screen);
    lv_label_set_text(volume_label, "Volume");
    lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 20, 120);
    lv_obj_add_style(volume_label, &style_text, 0);

    lv_obj_t* volume_slider = lv_slider_create(sound_settings_screen);
    lv_obj_set_size(volume_slider, SCREEN_WIDTH - 60, 10);
    lv_obj_align(volume_slider, LV_ALIGN_TOP_MID, 0, 150);
    lv_slider_set_range(volume_slider, 0, 255);
    lv_slider_set_value(volume_slider, M5.Speaker.getVolume(), LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_slider, [](lv_event_t* e) {
        Preferences prefs;
        prefs.begin("settings", false);
        lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
        uint8_t volume = lv_slider_get_value(slider);
        M5.Speaker.setVolume(volume);
        if (M5.Speaker.isEnabled()) {
            M5.Speaker.tone(440, 100); // Play 440Hz test tone
        }
        prefs.putUChar("volume", volume); // Save volume
        DEBUG_PRINTF("Volume set to %d\n", volume);
        prefs.end();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Volume Value Display
    static lv_obj_t* volume_value_label = nullptr;
    if (volume_value_label) lv_obj_del(volume_value_label);
    volume_value_label = lv_label_create(sound_settings_screen);
    char volume_text[10];
    snprintf(volume_text, sizeof(volume_text), "%d", M5.Speaker.getVolume());
    lv_label_set_text(volume_value_label, volume_text);
    lv_obj_align_to(volume_value_label, volume_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_style(volume_value_label, &style_text, 0);

    lv_obj_add_event_cb(volume_slider, [](lv_event_t* e) {
        uint8_t volume = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
        char volume_text[10];
        snprintf(volume_text, sizeof(volume_text), "%d", volume);
        lv_label_set_text(volume_value_label, volume_text);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(sound_settings_screen);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 90, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);

    DEBUG_PRINT("Sound settings screen created");
}

void createBrightnessSettingsScreen() {
    DEBUG_PRINT("Entering createBrightnessSettingsScreen");
    
    // Create a static variable for the brightness screen
    static lv_obj_t* brightness_settings_screen = nullptr;
    
    // Clean up previous instance if it exists
    if (brightness_settings_screen) {
        DEBUG_PRINTF("Cleaning and deleting existing brightness_settings_screen: %p\n", brightness_settings_screen);
        lv_obj_clean(brightness_settings_screen);
        lv_obj_del(brightness_settings_screen);
        brightness_settings_screen = nullptr;
        lv_task_handler();
        delay(10);
    }
    
    // Create a new screen
    brightness_settings_screen = lv_obj_create(NULL);
    if (!brightness_settings_screen) {
        DEBUG_PRINT("Failed to create brightness_settings_screen");
        return;
    }
    DEBUG_PRINTF("Created brightness_settings_screen: %p\n", brightness_settings_screen);
    
    lv_obj_add_style(brightness_settings_screen, &style_screen, 0);
    lv_scr_load(brightness_settings_screen);
    DEBUG_PRINT("Screen loaded");

    // Header with gradient
    lv_obj_t* header = lv_obj_create(brightness_settings_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0x357ABD), 0);
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Display Brightness");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Container with subtle shadow
    lv_obj_t* container = lv_obj_create(brightness_settings_screen);
    lv_obj_set_size(container, 300, 200);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2A2A40), 0);
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_shadow_color(container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_width(container, 15, 0);
    lv_obj_set_style_pad_all(container, 20, 0);
    
    // Brightness value label
    lv_obj_t* brightnessValueLabel = lv_label_create(container);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (displayBrightness * 100) / 255);
    lv_label_set_text(brightnessValueLabel, buf);
    lv_obj_align(brightnessValueLabel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(brightnessValueLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(brightnessValueLabel, lv_color_hex(0xFFFFFF), 0);

    // Brightness slider with custom colors
    lv_obj_t* brightnessSlider = lv_slider_create(container);
    lv_obj_set_width(brightnessSlider, 260);
    lv_obj_align(brightnessSlider, LV_ALIGN_TOP_MID, 0, 40);
    lv_slider_set_range(brightnessSlider, 10, 255);
    lv_slider_set_value(brightnessSlider, displayBrightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x4A90E2), LV_PART_KNOB);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
    
    // Preset buttons
    lv_obj_t* presetContainer = lv_obj_create(container);
    lv_obj_set_size(presetContainer, 260, 50);
    lv_obj_align(presetContainer, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_opa(presetContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(presetContainer, 0, 0);
    lv_obj_set_flex_flow(presetContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(presetContainer, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    const char* presets[] = {"Low", "Medium", "High"};
    const uint8_t presetValues[] = {50, 150, 250};
    
    for (int i = 0; i < 3; i++) {
        lv_obj_t* btn = lv_btn_create(presetContainer);
        lv_obj_set_size(btn, 80, 40);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, presets[i]);
        lv_obj_center(label);
        
        lv_obj_set_user_data(btn, (void*)(uintptr_t)presetValues[i]);
        
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
            uint8_t value = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);
            lv_obj_t* presetContainer = lv_obj_get_parent(btn);
            lv_obj_t* container = lv_obj_get_parent(presetContainer);
            lv_obj_t* slider = lv_obj_get_child(container, 1);
            lv_obj_t* valueLabel = lv_obj_get_child(container, 0);
            
            displayBrightness = value;
            lv_slider_set_value(slider, value, LV_ANIM_ON);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d%%", (value * 100) / 255);
            lv_label_set_text(valueLabel, buf);
            M5.Display.setBrightness(value);
            
            Preferences prefs;
            prefs.begin("settings", false);
            prefs.putUChar("brightness", value);
            prefs.end();
            
            DEBUG_PRINTF("Brightness preset set to %d\n", value);
        }, LV_EVENT_CLICKED, NULL);
    }
    
    // Slider event handler
    lv_obj_add_event_cb(brightnessSlider, [](lv_event_t* e) {
        lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
        displayBrightness = lv_slider_get_value(slider);
        lv_obj_t* container = lv_obj_get_parent(slider);
        lv_obj_t* valueLabel = lv_obj_get_child(container, 0);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (displayBrightness * 100) / 255);
        lv_label_set_text(valueLabel, buf);
        M5.Display.setBrightness(displayBrightness);
        
        Preferences prefs;
        prefs.begin("settings", false);
        prefs.putUChar("brightness", displayBrightness);
        prefs.end();
        
        DEBUG_PRINTF("Brightness set to %d\n", displayBrightness);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Auto-brightness option
    lv_obj_t* auto_container = lv_obj_create(container);
    lv_obj_set_size(auto_container, 260, 50);
    lv_obj_align(auto_container, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_set_style_bg_opa(auto_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(auto_container, 0, 0);
    
    lv_obj_t* auto_label = lv_label_create(auto_container);
    lv_label_set_text(auto_label, "Auto Brightness");
    lv_obj_align(auto_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(auto_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(auto_label, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t* auto_switch = lv_switch_create(auto_container);
    lv_obj_align(auto_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(auto_switch, lv_color_hex(0x4A90E2), LV_PART_KNOB);
    
    Preferences prefs;
    prefs.begin("settings", false);
    bool auto_brightness = prefs.getBool("auto_bright", false);
    prefs.end();
    
    if (auto_brightness) {
        lv_obj_add_state(auto_switch, LV_STATE_CHECKED);
    }
    
    lv_obj_add_event_cb(auto_switch, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e);
        bool auto_brightness = lv_obj_has_state(sw, LV_STATE_CHECKED);
        lv_obj_t* container = lv_obj_get_parent(lv_obj_get_parent(sw));
        lv_obj_t* slider = lv_obj_get_child(container, 1);
        lv_obj_t* presetContainer = lv_obj_get_child(container, 2);
        
        if (auto_brightness) {
            lv_obj_add_state(slider, LV_STATE_DISABLED);
            for (int i = 0; i < lv_obj_get_child_cnt(presetContainer); i++) {
                lv_obj_add_state(lv_obj_get_child(presetContainer, i), LV_STATE_DISABLED);
            }
        } else {
            lv_obj_clear_state(slider, LV_STATE_DISABLED);
            for (int i = 0; i < lv_obj_get_child_cnt(presetContainer); i++) {
                lv_obj_clear_state(lv_obj_get_child(presetContainer, i), LV_STATE_DISABLED);
            }
        }
        
        Preferences prefs;
        prefs.begin("settings", false);
        prefs.putBool("auto_bright", auto_brightness);
        prefs.end();
        
        DEBUG_PRINTF("Auto brightness set to %d\n", auto_brightness);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Back button
    lv_obj_t* back_btn = lv_btn_create(container); // Change from brightness_settings_screen to container
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_align(back_btn, LV_ALIGN_TOP_MID, 0, 200); // Position it below auto brightness
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    DEBUG_PRINT("Finished createBrightnessSettingsScreen");
}
// createPowerManagementScreen (unchanged)
void createPowerManagementScreen() {
    lv_obj_t* power_screen = lv_obj_create(NULL);
    lv_obj_add_style(power_screen, &style_screen, 0);
    
    // Header
    lv_obj_t* header = lv_obj_create(power_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Power Management");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Container for buttons
    lv_obj_t* container = lv_obj_create(power_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(container, 15, 0);
    
    // Power Off Button
    lv_obj_t* power_off_btn = lv_btn_create(container);
    lv_obj_set_size(power_off_btn, 280, 60);
    lv_obj_add_style(power_off_btn, &style_btn, 0);
    lv_obj_add_style(power_off_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(power_off_btn, lv_color_hex(0xE74C3C), 0); // Red color
    
    lv_obj_t* power_off_label = lv_label_create(power_off_btn);
    lv_label_set_text(power_off_label, LV_SYMBOL_POWER " Power Off");
    lv_obj_set_style_text_font(power_off_label, &lv_font_montserrat_20, 0);
    lv_obj_center(power_off_label);
    
    // Restart Button
    lv_obj_t* restart_btn = lv_btn_create(container);
    lv_obj_set_size(restart_btn, 280, 60);
    lv_obj_add_style(restart_btn, &style_btn, 0);
    lv_obj_add_style(restart_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(restart_btn, lv_color_hex(0xF39C12), 0); // Orange color
    
    lv_obj_t* restart_label = lv_label_create(restart_btn);
    lv_label_set_text(restart_label, LV_SYMBOL_REFRESH " Restart");
    lv_obj_set_style_text_font(restart_label, &lv_font_montserrat_20, 0);
    lv_obj_center(restart_label);
    
    // Sleep Button
    lv_obj_t* sleep_btn = lv_btn_create(container);
    lv_obj_set_size(sleep_btn, 280, 60);
    lv_obj_add_style(sleep_btn, &style_btn, 0);
    lv_obj_add_style(sleep_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(sleep_btn, lv_color_hex(0x3498DB), 0); // Blue color
    
    lv_obj_t* sleep_label = lv_label_create(sleep_btn);
    lv_label_set_text(sleep_label, LV_SYMBOL_DOWNLOAD " Sleep Mode");
    lv_obj_set_style_text_font(sleep_label, &lv_font_montserrat_20, 0);
    lv_obj_center(sleep_label);
    
    // Back Button
    lv_obj_t* back_btn = lv_btn_create(container);
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    
    // Event handlers
    lv_obj_add_event_cb(power_off_btn, [](lv_event_t* e) {
        // Create countdown screen
        lv_obj_t* countdown_screen = lv_obj_create(NULL);
        lv_obj_add_style(countdown_screen, &style_screen, 0);
        lv_scr_load(countdown_screen);
        
        lv_obj_t* countdown_label = lv_label_create(countdown_screen);
        lv_label_set_text_fmt(countdown_label, "Powering off in %d...", 3);
        lv_obj_set_style_text_font(countdown_label, &lv_font_montserrat_24, 0);
        lv_obj_align(countdown_label, LV_ALIGN_CENTER, 0, 0);
        lv_task_handler();
        delay(1000);
        
        lv_label_set_text_fmt(countdown_label, "Powering off in %d...", 2);
        lv_task_handler();
        delay(1000);
        
        lv_label_set_text_fmt(countdown_label, "Powering off in %d...", 1);
        lv_task_handler();
        delay(1000);
        
        // Power off
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.sleep();
        M5.Lcd.waitDisplay();
        
        // Use AXP to power off
        M5.In_I2C.bitOn(AXP2101_ADDR, 0x41, 1 << 1, 100000L);
        M5.In_I2C.writeRegister8(AXP2101_ADDR, 0x25, 0b00011011, 100000L);
        M5.In_I2C.writeRegister8(AXP2101_ADDR, 0x10, 0b00110001, 100000L);
        
    }, LV_EVENT_CLICKED, NULL);
    
    lv_obj_add_event_cb(restart_btn, [](lv_event_t* e) {
        // Create countdown screen
        lv_obj_t* countdown_screen = lv_obj_create(NULL);
        lv_obj_add_style(countdown_screen, &style_screen, 0);
        lv_scr_load(countdown_screen);
        
        lv_obj_t* countdown_label = lv_label_create(countdown_screen);
        lv_label_set_text_fmt(countdown_label, "Restarting in %d...", 3);
        lv_obj_set_style_text_font(countdown_label, &lv_font_montserrat_24, 0);
        lv_obj_align(countdown_label, LV_ALIGN_CENTER, 0, 0);
        lv_task_handler();
        delay(1000);
        
        lv_label_set_text_fmt(countdown_label, "Restarting in %d...", 2);
        lv_task_handler();
        delay(1000);
        
        lv_label_set_text_fmt(countdown_label, "Restarting in %d...", 1);
        lv_task_handler();
        delay(1000);
        
        // Restart
        ESP.restart();
        
    }, LV_EVENT_CLICKED, NULL);
    
    lv_obj_add_event_cb(sleep_btn, [](lv_event_t* e) {
        // Create sleep screen
        lv_obj_t* sleep_screen = lv_obj_create(NULL);
        lv_obj_add_style(sleep_screen, &style_screen, 0);
        lv_scr_load(sleep_screen);
        
        lv_obj_t* sleep_label = lv_label_create(sleep_screen);
        lv_label_set_text(sleep_label, "Entering sleep mode...\nTouch screen to wake");
        lv_obj_set_style_text_font(sleep_label, &lv_font_montserrat_24, 0);
        lv_obj_align(sleep_label, LV_ALIGN_CENTER, 0, 0);
        lv_task_handler();
        delay(2000);
        
        // Prepare for deep sleep
        // INTEN_P0 TF_DETECT ES7210 EN[0] DIS[1]
        M5.In_I2C.writeRegister8(AW9523_ADDR, 0x06, 0b11111111, 100000L);
        // INTEN_P1 AW88298 FT6336 EN[0] DIS[1] Clear INT
        M5.In_I2C.writeRegister8(AW9523_ADDR, 0x07, 0b11111011, 100000L);
        M5.In_I2C.readRegister8(AW9523_ADDR, 0x00, 100000L);
        M5.In_I2C.readRegister8(AW9523_ADDR, 0x01, 100000L);
        
        pinMode(21, INPUT_PULLUP);
        
        // GPIO21 <- AW9523 INT <- AW9523 P1.2 <- Touch INT
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 0);
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.sleep();
        M5.Lcd.waitDisplay();
        esp_deep_sleep_start();
        
    }, LV_EVENT_CLICKED, NULL);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(power_screen);
}
// save_time_to_rtc (unchanged)
static void save_time_to_rtc() {
    DEBUG_PRINTF("Saving time: %04d-%02d-%02d %02d:%02d:00 %s\n",
                 selected_date.year, selected_date.month, selected_date.day,
                 selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");

    m5::rtc_date_t DateStruct;
    DateStruct.year = selected_date.year;
    DateStruct.month = selected_date.month;
    DateStruct.date = selected_date.day;
    
    // Calculate weekday using Zeller's Congruence
    int q = selected_date.day;
    int m = selected_date.month;
    int y = selected_date.year;
    if (m < 3) { m += 12; y--; }
    int h = (q + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    DateStruct.weekDay = (h + 1) % 7;
    
    M5.Rtc.setDate(&DateStruct);

    m5::rtc_time_t TimeStruct;
    // Convert 12-hour format with AM/PM to 24-hour format for RTC
    int hour_24 = selected_hour;
    if (selected_is_pm && selected_hour != 12) hour_24 += 12;
    else if (!selected_is_pm && selected_hour == 12) hour_24 = 0;
    TimeStruct.hours = hour_24;
    TimeStruct.minutes = selected_minute;
    TimeStruct.seconds = 0;
    M5.Rtc.setTime(&TimeStruct);

    setSystemTimeFromRTC();
    DEBUG_PRINT("Time saved to RTC successfully");
}
// createDateSelectionScreen (unchanged)
static void createDateSelectionScreen() {
    static lv_obj_t* date_screen = nullptr;
    if (date_screen) {
        lv_obj_del(date_screen);
        date_screen = nullptr;
    }
    date_screen = lv_obj_create(NULL);
    lv_obj_add_style(date_screen, &style_screen, 0);

    // Header (unchanged)
    lv_obj_t* header = lv_obj_create(date_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(header, 10, 0);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Set Date");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container
    lv_obj_t* container = lv_obj_create(date_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);  // 300x140 for 320x240
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Get current date from RTC (unchanged)
    m5::rtc_date_t DateStruct;
    M5.Rtc.getDate(&DateStruct);
    selected_date.year = DateStruct.year;
    selected_date.month = DateStruct.month;
    selected_date.day = DateStruct.date;

    // Selected date display (replacing Date and Current Time)
    g_selected_date_label = lv_label_create(container);
    lv_obj_set_style_text_font(g_selected_date_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(g_selected_date_label, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(g_selected_date_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(g_selected_date_label, 5, 0);
    lv_obj_set_style_radius(g_selected_date_label, 5, 0);
    lv_obj_set_style_text_color(g_selected_date_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_selected_date_label, LV_ALIGN_TOP_MID, 0, 0);  // Top center like date screen

    // Year picker
    lv_obj_t* year_label = lv_label_create(container);
    lv_label_set_text(year_label, "Year:");
    lv_obj_align(year_label, LV_ALIGN_TOP_LEFT, 10, 25);  // Left-aligned with padding
    
    g_year_roller = lv_roller_create(container);
    char years_str[512] = {0};
    for (int year = 2020; year <= 2040; year++) {
        char year_str[8];
        snprintf(year_str, sizeof(year_str), "%04d\n", year);
        strcat(years_str, year_str);
    }
    if (strlen(years_str) > 0) {
        years_str[strlen(years_str) - 1] = '\0';
    }
    lv_roller_set_options(g_year_roller, years_str, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_year_roller, 2);
    lv_obj_set_width(g_year_roller, 95);  // Adjusted to ~1/3 of 300px with padding
    lv_obj_align(year_label, LV_ALIGN_TOP_LEFT, 10, 25);  // Align label above roller
    lv_obj_align(g_year_roller, LV_ALIGN_TOP_LEFT, 10, 40);  // Position roller below label
    lv_roller_set_selected(g_year_roller, DateStruct.year - 2020, LV_ANIM_OFF);

    // Month picker
    lv_obj_t* month_label = lv_label_create(container);
    lv_label_set_text(month_label, "Month:");
    lv_obj_align(month_label, LV_ALIGN_TOP_MID, 0, 25);  // Centered
    
    g_month_roller = lv_roller_create(container);
    lv_roller_set_options(g_month_roller, 
                         "01-Jan\n02-Feb\n03-Mar\n04-Apr\n05-May\n06-Jun\n"
                         "07-Jul\n08-Aug\n09-Sep\n10-Oct\n11-Nov\n12-Dec", 
                         LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_month_roller, 2);
    lv_obj_set_width(g_month_roller, 95);  // Adjusted to ~1/3 of 300px with padding
    lv_obj_align(g_month_roller, LV_ALIGN_TOP_MID, 0, 40);  // Centered below label
    lv_roller_set_selected(g_month_roller, DateStruct.month - 1, LV_ANIM_OFF);

    // Day picker
    lv_obj_t* day_label = lv_label_create(container);
    lv_label_set_text(day_label, "Day:");
    lv_obj_align(day_label, LV_ALIGN_TOP_RIGHT, -10, 25);  // Right-aligned with padding
    
    g_day_roller = lv_roller_create(container);
    char days_str[256] = {0};
    for (int day = 1; day <= 31; day++) {
        char day_str[8];
        snprintf(day_str, sizeof(day_str), "%02d\n", day);
        strcat(days_str, day_str);
    }
    if (strlen(days_str) > 0) {
        days_str[strlen(days_str) - 1] = '\0';
    }
    lv_roller_set_options(g_day_roller, days_str, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_day_roller, 2);
    lv_obj_set_width(g_day_roller, 95);  // Adjusted to ~1/3 of 300px with padding
    lv_obj_align(g_day_roller, LV_ALIGN_TOP_RIGHT, -10, 40);  // Right-aligned below label
    lv_roller_set_selected(g_day_roller, DateStruct.date - 1, LV_ANIM_OFF);

    // Initial update for selected date (unchanged)
    static auto updateSelectedDateFunc = []() {
        int year = 2020 + lv_roller_get_selected(g_year_roller);
        int month = lv_roller_get_selected(g_month_roller) + 1;
        int day = lv_roller_get_selected(g_day_roller) + 1;
        selected_date.year = year;
        selected_date.month = month;
        selected_date.day = day;
        char date_str[64];
        snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", year, month, day);
        lv_label_set_text(g_selected_date_label, date_str);
    };
    updateSelectedDateFunc();

    // Event handlers (unchanged)
    lv_obj_add_event_cb(g_year_roller, on_year_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_month_roller, on_month_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_day_roller, on_day_change, LV_EVENT_VALUE_CHANGED, NULL);

    // Continue button (unchanged)
    lv_obj_t* continue_btn = lv_btn_create(date_screen);
    lv_obj_set_size(continue_btn, 120, 40);
    lv_obj_align(continue_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(continue_btn, &style_btn, 0);
    lv_obj_add_style(continue_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* continue_label = lv_label_create(continue_btn);
    lv_label_set_text(continue_label, "Next");
    lv_obj_center(continue_label);
    lv_obj_add_event_cb(continue_btn, [](lv_event_t* e) {
        createTimeSelectionScreen();
    }, LV_EVENT_CLICKED, NULL);

    // Back button (unchanged)
    lv_obj_t* back_btn = lv_btn_create(date_screen);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(date_screen);
}

// Improved time selection screen
static void createTimeSelectionScreen() {
    static lv_obj_t* time_screen = nullptr;
    if (time_screen) {
        lv_obj_del(time_screen);
        time_screen = nullptr;
    }
    time_screen = lv_obj_create(NULL);
    lv_obj_add_style(time_screen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(time_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(header, 10, 0);
    
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Set Time");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container
    lv_obj_t* container = lv_obj_create(time_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);  // 300x140 for 320x240
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Get current time from RTC
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);

    // Selected time display (replacing Date and Current Time)
    g_selected_time_label = lv_label_create(container);
    lv_obj_set_style_text_font(g_selected_time_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(g_selected_time_label, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(g_selected_time_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(g_selected_time_label, 5, 0);
    lv_obj_set_style_radius(g_selected_time_label, 5, 0);
    lv_obj_set_style_text_color(g_selected_time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_selected_time_label, LV_ALIGN_TOP_MID, 0, 0);  // Top center like date screen

    // Hour picker (1-12)
    lv_obj_t* hour_label = lv_label_create(container);
    lv_label_set_text(hour_label, "Hour:");
    lv_obj_align(hour_label, LV_ALIGN_TOP_LEFT, 10, 25);  // Raised to match date screen
    
    g_hour_roller = lv_roller_create(container);
    char hours_str[64] = {0};
    for (int hour = 1; hour <= 12; hour++) {
        char hour_str[8];
        snprintf(hour_str, sizeof(hour_str), "%d\n", hour);
        strcat(hours_str, hour_str);
    }
    if (strlen(hours_str) > 0) hours_str[strlen(hours_str) - 1] = '\0';
    
    lv_roller_set_options(g_hour_roller, hours_str, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_hour_roller, 2);
    lv_obj_set_width(g_hour_roller, 95);
    lv_obj_align(g_hour_roller, LV_ALIGN_TOP_LEFT, 10, 40);  // Raised to match date screen
    int display_hour = (TimeStruct.hours == 0) ? 12 : (TimeStruct.hours > 12 ? TimeStruct.hours - 12 : TimeStruct.hours);
    lv_roller_set_selected(g_hour_roller, display_hour - 1, LV_ANIM_OFF);
    selected_hour = display_hour;

    // Minute picker
    lv_obj_t* minute_label = lv_label_create(container);
    lv_label_set_text(minute_label, "Minute:");
    lv_obj_align(minute_label, LV_ALIGN_TOP_MID, 0, 25);  // Raised to match date screen
    
    g_minute_roller = lv_roller_create(container);
    char minutes_str[256] = {0};
    for (int minute = 0; minute <= 59; minute++) {
        char minute_str[8];
        snprintf(minute_str, sizeof(minute_str), "%02d\n", minute);
        strcat(minutes_str, minute_str);
    }
    if (strlen(minutes_str) > 0) minutes_str[strlen(minutes_str) - 1] = '\0';
    
    lv_roller_set_options(g_minute_roller, minutes_str, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_minute_roller, 2);
    lv_obj_set_width(g_minute_roller, 95);
    lv_obj_align(g_minute_roller, LV_ALIGN_TOP_MID, 0, 40);  // Raised to match date screen
    lv_roller_set_selected(g_minute_roller, TimeStruct.minutes, LV_ANIM_OFF);
    selected_minute = TimeStruct.minutes;

    // AM/PM picker
    lv_obj_t* am_pm_label = lv_label_create(container);
    lv_label_set_text(am_pm_label, "Period:");
    lv_obj_align(am_pm_label, LV_ALIGN_TOP_RIGHT, -10, 25);  // Raised to match date screen
    
    g_am_pm_roller = lv_roller_create(container);
    lv_roller_set_options(g_am_pm_roller, "AM\nPM", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_am_pm_roller, 2);
    lv_obj_set_width(g_am_pm_roller, 95);
    lv_obj_align(g_am_pm_roller, LV_ALIGN_TOP_RIGHT, -10, 40);  // Raised to match date screen
    selected_is_pm = (TimeStruct.hours >= 12) ? 1 : 0;
    lv_roller_set_selected(g_am_pm_roller, selected_is_pm, LV_ANIM_OFF);

    // Initial update of selected time display
    char selected_time_str[32];
    snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s", 
             selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
    lv_label_set_text(g_selected_time_label, selected_time_str);

    // Event handlers
    lv_obj_add_event_cb(g_hour_roller, on_hour_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_minute_roller, on_minute_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_am_pm_roller, [](lv_event_t* e) {
        selected_is_pm = lv_roller_get_selected(g_am_pm_roller);
        char selected_time_str[32];
        snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s", 
                 selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
        lv_label_set_text(g_selected_time_label, selected_time_str);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Save button
    lv_obj_t* save_btn = lv_btn_create(time_screen);
    lv_obj_set_size(save_btn, 120, 40);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(save_btn, &style_btn, 0);
    lv_obj_add_style(save_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x00AA00), 0);
    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);
    
    lv_obj_add_event_cb(save_btn, [](lv_event_t* e) {
        lv_obj_t* saving_label = lv_label_create(lv_scr_act());
        lv_label_set_text(saving_label, "Saving...");
        lv_obj_set_style_text_font(saving_label, &lv_font_montserrat_16, 0);
        lv_obj_center(saving_label);
        lv_task_handler();
        
        save_time_to_rtc();
        
        lv_timer_t* timer = lv_timer_create([](lv_timer_t* t) {
            if (lv_scr_act()) createSettingsScreen();
            lv_timer_del(t);
        }, 500, NULL);
    }, LV_EVENT_CLICKED, NULL);

    // Back button
    lv_obj_t* back_btn = lv_btn_create(time_screen);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createDateSelectionScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(time_screen);
}
// Date/Time callbacks (on_year_change etc.) (unchanged)
static void on_year_change(lv_event_t* e) {
    // Update the selected date when year changes
    int year = 2020 + lv_roller_get_selected(g_year_roller);
    int month = lv_roller_get_selected(g_month_roller) + 1;
    int day = lv_roller_get_selected(g_day_roller) + 1;
    
    selected_date.year = year;
    selected_date.month = month;
    selected_date.day = day;
    
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", year, month, day);
    lv_label_set_text(g_selected_date_label, date_str);
}

static void on_month_change(lv_event_t* e) {
    // Update the selected date when month changes
    int year = 2020 + lv_roller_get_selected(g_year_roller);
    int month = lv_roller_get_selected(g_month_roller) + 1;
    int day = lv_roller_get_selected(g_day_roller) + 1;
    
    selected_date.year = year;
    selected_date.month = month;
    selected_date.day = day;
    
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", year, month, day);
    lv_label_set_text(g_selected_date_label, date_str);
}

static void on_day_change(lv_event_t* e) {
    // Update the selected date when day changes
    int year = 2020 + lv_roller_get_selected(g_year_roller);
    int month = lv_roller_get_selected(g_month_roller) + 1;
    int day = lv_roller_get_selected(g_day_roller) + 1;
    
    selected_date.year = year;
    selected_date.month = month;
    selected_date.day = day;
    
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", year, month, day);
    lv_label_set_text(g_selected_date_label, date_str);
}

// Define the callback functions for time selection
static void on_hour_change(lv_event_t* e) {
    selected_hour = lv_roller_get_selected(g_hour_roller) + 1; // 1-12
    char selected_time_str[32];
    snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s", 
             selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
    lv_label_set_text(g_selected_time_label, selected_time_str);
}

static void on_minute_change(lv_event_t* e) {
    selected_minute = lv_roller_get_selected(g_minute_roller);
    char selected_time_str[32];
    snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s", 
             selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
    lv_label_set_text(g_selected_time_label, selected_time_str);
}

// Implementation of the loading screen
void createLoadingScreen() {
    lv_obj_t* loading_screen = lv_obj_create(NULL);
    lv_obj_add_style(loading_screen, &style_screen, 0);

    // Set the background image
    lv_obj_t* bg_img = lv_img_create(loading_screen);
    lv_img_set_src(bg_img, &LossPrev2);
    lv_obj_center(bg_img);

    // Create a semi-transparent overlay for better text visibility
    lv_obj_t* overlay = lv_obj_create(loading_screen);
    lv_obj_set_size(overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_30, 0); // 30% opacity
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
    
    // Add a title
    lv_obj_t* title = lv_label_create(loading_screen);
    lv_label_set_text(title, "Loss Prevention Log");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Create the progress bar
    lv_obj_t* progress_bar = lv_bar_create(loading_screen);
    lv_obj_set_size(progress_bar, 200, 20);
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), LV_PART_MAIN); // Dark background
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x4A90E2), LV_PART_INDICATOR); // Blue indicator
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(progress_bar, 0, 100);
    
    // Add loading text
    lv_obj_t* loading_label = lv_label_create(loading_screen);
    lv_label_set_text(loading_label, "Loading...");
    lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(loading_label, LV_ALIGN_BOTTOM_MID, 0, -70);
    
    // Add version text
    lv_obj_t* version_label = lv_label_create(loading_screen);
    lv_label_set_text(version_label, "v1.0.0 - 2025");
    lv_obj_set_style_text_color(version_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(version_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    
    // Load the screen
    lv_scr_load(loading_screen);
    
    // Start the timer for progress updates
    // The timer will call updateLoadingProgress every 50ms
    // We'll pass the progress bar as user data
    lv_timer_t* timer = lv_timer_create(updateLoadingProgress, 50, progress_bar);
    
    // Store the timer in the screen's user data so we can access it later
    lv_obj_set_user_data(loading_screen, timer);
    
    DEBUG_PRINT("Loading screen created");
}

// Update the loading progress bar
void updateLoadingProgress(lv_timer_t* timer) {
    lv_obj_t* bar = (lv_obj_t*)lv_timer_get_user_data(timer);
    int32_t value = lv_bar_get_value(bar);
    value += 5;
    lv_bar_set_value(bar, value, LV_ANIM_ON);

    if (value >= 100) {
        lv_timer_del(timer); // Delete the timer

        // Take the GUI semaphore to ensure exclusive access
        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(500)) == pdTRUE) {
            lv_obj_t* old_screen = lv_scr_act(); // Store reference to loading screen
            createLockScreen();                  // Create and load the lock screen
            if (old_screen && old_screen != lv_scr_act()) {
                lv_obj_del(old_screen);          // Synchronously delete the loading screen
            }
            xSemaphoreGive(xGuiSemaphore);       // Release semaphore
        } else {
            DEBUG_PRINT("Failed to take xGuiSemaphore in updateLoadingProgress");
            // Optionally, retry or handle the failure gracefully
        }
    }
}
void createLockScreen() {
    if (lock_screen && lv_obj_is_valid(lock_screen)) {
        lv_obj_del(lock_screen); // Delete previous if exists (safety)
    }
    lock_screen = lv_obj_create(NULL); // Create the screen object
    lv_obj_set_style_bg_opa(lock_screen, LV_OPA_TRANSP, 0); // Make screen transparent if image covers all

    // Background Image
    lv_obj_t* bg_img = lv_img_create(lock_screen);
    lv_img_set_src(bg_img, &LossPrev1); // Use the LossPrev1 image
    lv_obj_center(bg_img); // Center the image (assuming it's 320x240)
    // If image is smaller, you might use: lv_obj_set_size(bg_img, LV_HOR_RES, LV_VER_RES); lv_img_set_zoom(bg_img, compute_zoom_factor_if_needed);

    // Date & Time Label
    g_lock_screen_datetime_label = lv_label_create(lock_screen);
    lv_obj_align(g_lock_screen_datetime_label, LV_ALIGN_TOP_MID, 0, 15); // Position near top center
    lv_obj_set_style_text_color(g_lock_screen_datetime_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_set_style_text_font(g_lock_screen_datetime_label, &lv_font_montserrat_16, 0); // Choose a font
    lv_obj_set_style_text_align(g_lock_screen_datetime_label, LV_TEXT_ALIGN_CENTER, 0); // Center align text
    updateLockScreenTime(); // Set initial time

    // Unlock Button
    lv_obj_t* unlock_btn = lv_btn_create(lock_screen);
    lv_obj_set_size(unlock_btn, 160, 45);
    lv_obj_align(unlock_btn, LV_ALIGN_BOTTOM_MID, 0, -25); // Position near bottom center
    lv_obj_add_style(unlock_btn, &style_btn, 0); // Use existing button style
    lv_obj_add_style(unlock_btn, &style_btn_pressed, LV_STATE_PRESSED); // Use existing pressed style
    // Optionally, make button semi-transparent
    // lv_obj_set_style_bg_opa(unlock_btn, LV_OPA_70, 0);

    lv_obj_t* unlock_label = lv_label_create(unlock_btn);
    lv_label_set_text(unlock_label, "Press to Unlock");
    lv_obj_center(unlock_label);

    // Button Click Event
    lv_obj_add_event_cb(unlock_btn, [](lv_event_t* e) {
        DEBUG_PRINT("Unlock button pressed.");
        // Transition to Main Menu
        createMainMenu(); // Create the main menu screen first
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* parent1 = lv_obj_get_parent(btn);
        lv_obj_t* old_screen = lv_obj_get_parent(parent1); // Get the lock screen object
        if (old_screen && old_screen == lock_screen) {
             g_lock_screen_datetime_label = nullptr; // Clear global pointer before deleting screen
             lv_obj_del_async(old_screen); // Delete the lock screen asynchronously
             lock_screen = nullptr; // Clear screen pointer
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(lock_screen); // Load the created lock screen
    DEBUG_PRINT("Lock screen created and loaded.");
}

void updateLockScreenTime() {
    if (!g_lock_screen_datetime_label || !lv_obj_is_valid(g_lock_screen_datetime_label) || lv_scr_act() != lock_screen) {
        return; // Only update if the label exists and lock screen is active
    }

    m5::rtc_date_t DateStruct;
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getDate(&DateStruct);
    M5.Rtc.getTime(&TimeStruct);

    struct tm timeinfo = {0};
    timeinfo.tm_year = DateStruct.year - 1900;
    timeinfo.tm_mon = DateStruct.month - 1;
    timeinfo.tm_mday = DateStruct.date;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    timeinfo.tm_isdst = -1; // Let mktime determine DST

    // Check if RTC time is valid before formatting
    if (DateStruct.year >= 2023) { // Basic validity check
        char date_buf[16]; // YYYY-MM-DD + null
        char time_buf[16]; // HH:MM:SS AM/PM + null
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%I:%M:%S %p", &timeinfo); // 12-hour format with AM/PM

        // Use lv_label_set_text_fmt for easier formatting with newline
        lv_label_set_text_fmt(g_lock_screen_datetime_label, "%s\n%s", date_buf, time_buf);
    } else {
        lv_label_set_text(g_lock_screen_datetime_label, "RTC Error\nSet Time");
    }
}

void addTimeDisplay(lv_obj_t *screen) {
    lv_obj_t* time_card = lv_obj_create(screen);
    lv_obj_set_size(time_card, 180, 40);
    lv_obj_align(time_card, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_style(time_card, &style_card_info, 0);

    lv_obj_t* time_icon = lv_label_create(time_card);
    lv_label_set_text(time_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(time_icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(time_icon, lv_color_hex(0x4A90E2), 0);
    lv_obj_align(time_icon, LV_ALIGN_LEFT_MID, 15, 0);

    time_label = lv_label_create(time_card);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_16, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 10, 0);

    updateTimeDisplay(); // Initial update
}

void updateTimeDisplay() {
    if (time_label == nullptr) return;

    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);
    
    // Convert 24-hour to 12-hour format
    int hour = TimeStruct.hours;
    const char* period = (hour >= 12) ? "PM" : "AM";
    if (hour == 0) {
        hour = 12; // Midnight
    } else if (hour > 12) {
        hour -= 12;
    }

    char timeStr[24];
    snprintf(timeStr, sizeof(timeStr), "%d:%02d:%02d %s", 
             hour, TimeStruct.minutes, TimeStruct.seconds, period);
    lv_label_set_text(time_label, timeStr);
}
// updateStatus (unchanged)
void addStatusBar(lv_obj_t* screen) {
    if (status_bar) lv_obj_del(status_bar);
    status_bar = lv_label_create(screen);
    lv_obj_set_style_text_font(status_bar, &lv_font_montserrat_14, 0);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(status_bar, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(status_bar, "Ready");
}

void updateStatus(const char* message, uint32_t color) {
    if (!status_bar) return; // Ensure status bar exists

    // Get current time from RTC
    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);

    // Format time in 12-hour format with AM/PM
    char time_str[12]; // Enough for "HH:MM:SS AM/PM" (e.g., "2:30 PM")
    struct tm timeinfo;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    strftime(time_str, sizeof(time_str), "%I:%M %p", &timeinfo);

    // Remove leading zero if present (e.g., "02:30 PM" -> "2:30 PM")
    if (time_str[0] == '0') {
        memmove(time_str, time_str + 1, strlen(time_str));
    }

    // Update time label
    if (!time_label) {
        time_label = lv_label_create(status_bar);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -10, 0);
    }
    lv_label_set_text(time_label, time_str);

    // Update status message if needed (assuming status_label exists)
    if (status_bar && current_status_msg[0] != '\0') {
        lv_label_set_text(status_bar, current_status_msg);
        lv_obj_set_style_text_color(status_bar, lv_color_hex(current_status_color), 0);
    }
}
