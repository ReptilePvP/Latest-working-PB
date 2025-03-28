# 1 "C:\\Users\\nickd\\AppData\\Local\\Temp\\tmp910oyba9"
#include <Arduino.h>
# 1 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"
#include <Arduino.h>
#include <M5Unified.h>
#include "m5gfx_lvgl.hpp"
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <WiFi.h>
#include <Preferences.h>
#include <SPI.h>
SPIClass SPI_SD;
#include <SD.h>
#include <time.h>
#include <vector>




void createLoadingScreen();
void updateLoadingProgress(lv_timer_t* timer);
void createLockScreen();
void updateLockScreenTime();
LV_IMG_DECLARE(LossPrev2);
LV_IMG_DECLARE(LossPrev1);


static void createDateSelectionScreen();
static void createTimeSelectionScreen();
static void confirm_dialog_event_cb(lv_event_t* e);
static void save_time_to_rtc();


static void on_year_change(lv_event_t* e);
static void on_month_change(lv_event_t* e);
static void on_day_change(lv_event_t* e);
static void on_hour_change(lv_event_t* e);
static void on_minute_change(lv_event_t* e);


static lv_obj_t* g_year_roller = nullptr;
static lv_obj_t* g_month_roller = nullptr;
static lv_obj_t* g_day_roller = nullptr;
static lv_obj_t* g_selected_date_label = nullptr;
static lv_obj_t* g_hour_roller = nullptr;
static lv_obj_t* g_minute_roller = nullptr;
static lv_obj_t* g_am_pm_roller = nullptr;
static lv_obj_t* g_selected_time_label = nullptr;
lv_obj_t* lock_screen = nullptr;



#define AXP2101_ADDR 0x34
#define AW9523_ADDR 0x58


lv_obj_t* battery_icon = nullptr;
lv_obj_t* battery_label = nullptr;
lv_obj_t* wifi_label = nullptr;
lv_obj_t* time_label = nullptr;
lv_obj_t* g_lock_screen_datetime_label = nullptr;


static lv_style_t style_card_action;
static lv_style_t style_card_info;
static lv_style_t style_card_pressed;
static lv_style_t style_gender_card;
static lv_style_t style_gender_card_pressed;
static lv_style_t style_network_item;
static lv_style_t style_network_item_pressed;


WiFiManager wifiManager(true);


static int selected_hour = 0, selected_minute = 0, selected_is_pm = 0;


static lv_calendar_date_t selected_date = {2025, 3, 18};



#define DEBUG_ENABLED true


#include "lvgl.h"

#define DEBUG_PRINT(x) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.println(x); }
#define DEBUG_PRINTF(x,...) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.printf(x, __VA_ARGS__); }


#define SD_SPI_SCK_PIN 36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN 4
#define TFT_DC 35




void initStyles();
void createMainMenu();
void createGenderMenu();
void createColorMenuShirt();
void createColorMenuPants();
void createColorMenuShoes();
void createItemMenu();
void createConfirmScreen();
static void showWiFiPasswordKeyboard(const String& ssid);
void createWiFiScreen();
void createWiFiManagementScreen();
String getFormattedEntry(const String& entry);
String getTimestamp();
bool appendToLog(const String& entry);
void createViewLogsScreen();
void initFileSystem();
void listSavedEntries();
void saveEntry(const String& entry);
void updateStatus(const char* message, uint32_t color);
void sendWebhook(const String& entry);
void onWiFiScanComplete(const std::vector<NetworkInfo>& results);
void updateWiFiLoadingScreen(bool success, const String& message);
void showWiFiLoadingScreen(const String& ssid);
void addTimeDisplay(lv_obj_t *screen);
void updateTimeDisplay();
void onWiFiStatus(WiFiState state, const String& message);
static void wifi_scan_results_update_cb(lv_timer_t* timer);
static void wifi_status_update_cb(lv_timer_t* timer);
void createNetworkDetailsScreen(const String& ssid);
static void connect_btn_event_cb(lv_event_t* e);
static void forget_btn_event_cb(lv_event_t* e);
void createSettingsScreen();
void createSoundSettingsScreen();
void createBrightnessSettingsScreen();
void createPowerManagementScreen();



lv_obj_t *current_scroll_obj = nullptr;
const int SCROLL_AMOUNT = 40;


String selectedShirtColors = "";
String selectedPantsColors = "";
String selectedShoesColors = "";
String selectedGender = "";
String selectedItem = "";


static lv_obj_t* g_spinner = nullptr;


static String currentEntry = "";


LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);


static const uint32_t screenTickPeriod = 10;
static uint32_t lastLvglTick = 0;


static std::vector<String> logEntries;
static int currentLogPage = 0;
static int totalLogPages = 0;
static const int LOGS_PER_PAGE = 8;

lv_obj_t* status_bar = nullptr;
char current_status_msg[64] = "";

uint32_t current_status_color = 0xFFFFFF;
uint32_t error_status_color = 0xFF0000;


lv_obj_t *wifi_management_status_label = nullptr;
static lv_obj_t* wifi_password_keyboard = nullptr;
static lv_obj_t* wifi_password_ta = nullptr;
static String wifi_keyboard_ssid = "";


const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};
const int NUM_ITEMS = sizeof(items) / sizeof(items[0]);



#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LV_TICK_PERIOD_MS 10

struct WiFiStateInfo {
    int strength;
    uint32_t color;
    bool connected;
};
WiFiStateInfo currentWiFiState = {0, 0x666666, false};

struct WiFiScanUpdateData {
    std::vector<NetworkInfo> results;
    bool scan_failed;
};


struct LogEntry {
    String text;
    time_t timestamp;
};
void setSystemTimeFromRTC();
static void lvgl_tick_task(void *arg);
time_t parseTimestamp(const String& entry);
void releaseSPIBus();
void lvgl_task(void *pvParameters);
void setup();
void loop();
void println_log(const char *str);
void printf_log(const char *format, ...);
void addStatusBar(lv_obj_t* screen);
void handleSwipeLeft();
void handleSwipeVertical(int amount);
#line 208 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"
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


static std::vector<LogEntry> parsedLogEntries;

lv_obj_t *item_list = nullptr;

bool wifiEnabled = true;
uint8_t displayBrightness = 128;


static void lvgl_tick_task(void *arg) {
    (void)arg;
    static uint32_t last_tick = 0;
    uint32_t current_tick = millis();
    if (current_tick - last_tick > LV_TICK_PERIOD_MS) {
        last_tick = current_tick;
        lv_task_handler();
    }
}


const char* genders[] = {"Male", "Female"};


lv_obj_t* main_menu_screen = nullptr;
lv_obj_t* genderMenu = nullptr;
lv_obj_t* colorMenu = nullptr;
lv_obj_t* itemMenu = nullptr;
lv_obj_t* confirmScreen = nullptr;
lv_obj_t* wifi_screen = nullptr;
lv_obj_t* wifi_list = nullptr;
lv_obj_t* wifi_status_label = nullptr;
lv_obj_t* wifi_management_screen = nullptr;
lv_obj_t* settingsScreen = nullptr;
lv_obj_t* view_logs_screen = nullptr;


lv_obj_t* shirt_next_btn = nullptr;
lv_obj_t* pants_next_btn = nullptr;
lv_obj_t* shoes_next_btn = nullptr;


lv_obj_t* wifi_loading_screen = nullptr;
lv_obj_t* wifi_loading_spinner = nullptr;
lv_obj_t* wifi_loading_label = nullptr;
lv_obj_t* wifi_result_label = nullptr;


struct WiFiIconState {
    bool connected;
    int strength;
    uint32_t color;
};
static WiFiIconState currentWiFiIconState = { false, 0, 0xFF0000 };


static int lastBatteryLevel = -1;
static unsigned long lastBatteryReadTime = 0;
const unsigned long BATTERY_READ_INTERVAL = 30000;

static Preferences preferences;


static lv_style_t style_screen, style_btn, style_btn_pressed, style_title, style_text;
static lv_style_t style_keyboard_btn;


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


#define LOG_FILENAME "/loss_prevention_log.txt"
bool fileSystemInitialized = false;

#define LVGL_TASK_CORE 1
#define LVGL_TASK_PRIORITY 5
#define LVGL_STACK_SIZE 32768





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


void createViewLogsScreen() {
    DEBUG_PRINT("Creating new view logs screen");
    parsedLogEntries.clear();
    lv_obj_t* logs_screen = lv_obj_create(NULL);
    lv_obj_add_style(logs_screen, &style_screen, 0);
    lv_obj_t* header = lv_obj_create(logs_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40); lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Log Entries - Last 3 Days");
    lv_obj_add_style(title, &style_title, 0); lv_obj_center(title);
    lv_obj_t* tabview = lv_tabview_create(logs_screen);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP); lv_tabview_set_tab_bar_size(tabview, 30);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - 40); lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x2D2D2D), 0);
    m5::rtc_date_t currentDateStruct; M5.Rtc.getDate(&currentDateStruct);
    struct tm currentTimeInfo = {0}; currentTimeInfo.tm_year = currentDateStruct.year - 1900;
    currentTimeInfo.tm_mon = currentDateStruct.month - 1; currentTimeInfo.tm_mday = currentDateStruct.date;
    time_t now = mktime(&currentTimeInfo);
    if (!fileSystemInitialized) initFileSystem();
    if (fileSystemInitialized) {
        SPI.end(); delay(100); SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
        digitalWrite(SD_SPI_CS_PIN, HIGH); delay(100);
        if (!SD.exists(LOG_FILENAME)) { File file = SD.open(LOG_FILENAME, FILE_WRITE); if (file) { file.println("# Log Created " + getTimestamp()); file.close(); } }
        File log_file = SD.open(LOG_FILENAME, FILE_READ);
        if (log_file) { while (log_file.available()) { String line = log_file.readStringUntil('\n'); line.trim(); if (line.length() > 0 && !line.startsWith("#")) { time_t timestamp = parseTimestamp(line); if (timestamp != 0) parsedLogEntries.push_back({line, timestamp}); } } log_file.close(); }
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);
    }
    char tab_names[3][16]; lv_obj_t* tabs[3]; std::vector<LogEntry*> entries_by_day[3];
    for (int i = 0; i < 3; i++) {
        struct tm day_time = *localtime(&now); day_time.tm_mday -= i; day_time.tm_hour = 0; day_time.tm_min = 0; day_time.tm_sec = 0; mktime(&day_time);
        strftime(tab_names[i], sizeof(tab_names[i]), "%m/%d/%y", &day_time); tabs[i] = lv_tabview_add_tab(tabview, tab_names[i]);
        time_t day_start = mktime(&day_time); day_time.tm_hour = 23; day_time.tm_min = 59; day_time.tm_sec = 59; mktime(&day_time); time_t day_end = mktime(&day_time);
        for (auto& entry : parsedLogEntries) { if (entry.timestamp >= day_start && entry.timestamp <= day_end) entries_by_day[i].push_back(&entry); }
        std::sort(entries_by_day[i].begin(), entries_by_day[i].end(), [](const LogEntry* a, const LogEntry* b) { return a->timestamp < b->timestamp; });
    }
    for (int i = 0; i < 3; i++) {
        lv_obj_t* container = lv_obj_create(tabs[i]); lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 70); lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0); lv_obj_set_scroll_dir(container, LV_DIR_VER); lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_AUTO); lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
        if (entries_by_day[i].empty()) { lv_obj_t* no_logs = lv_label_create(container); lv_label_set_text(no_logs, "No entries for this day"); lv_obj_add_style(no_logs, &style_text, 0); lv_obj_center(no_logs); }
        else { int y_pos = 0; int entry_num = 1; for (const auto* entry : entries_by_day[i]) {
            lv_obj_t* entry_btn = lv_btn_create(container); lv_obj_set_size(entry_btn, SCREEN_WIDTH - 30, 25); lv_obj_set_pos(entry_btn, 5, y_pos);
            lv_obj_add_style(entry_btn, &style_btn, 0); lv_obj_add_style(entry_btn, &style_btn_pressed, LV_STATE_PRESSED); lv_obj_set_style_bg_color(entry_btn, lv_color_hex(0x3A3A3A), 0);
            struct tm* entry_time = localtime(&entry->timestamp); char time_str[16]; strftime(time_str, sizeof(time_str), "%H:%M", entry_time); String entry_label = "Entry " + String(entry_num++) + ": " + String(time_str);
            lv_obj_t* label = lv_label_create(entry_btn); lv_label_set_text(label, entry_label.c_str()); lv_obj_add_style(label, &style_text, 0); lv_obj_align(label, LV_ALIGN_LEFT_MID, 5, 0);
            lv_obj_set_user_data(entry_btn, (void*)entry);
            lv_obj_add_event_cb(entry_btn, [](lv_event_t* e) {
                LogEntry* entry = (LogEntry*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
                if (!entry || entry->text.isEmpty()) return;
                int colon_pos = entry->text.indexOf(": ");
                if (colon_pos != -1 && colon_pos < entry->text.length() - 2) {
                    String entry_data = entry->text.substring(colon_pos + 2);
                    if (entry_data.isEmpty()) return;
                    String formatted_entry = getFormattedEntry(entry_data);
                    lv_obj_t* msgbox = lv_msgbox_create(NULL); lv_obj_set_size(msgbox, 280, 180); lv_obj_center(msgbox);
                    lv_obj_t* title = lv_label_create(msgbox); lv_label_set_text(title, "Log Entry Details"); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
                    lv_obj_t* text = lv_label_create(msgbox); lv_label_set_text(text, formatted_entry.c_str()); lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
                    lv_obj_set_style_text_font(text, &lv_font_montserrat_14, 0); lv_obj_set_style_text_line_space(text, 2, 0); lv_obj_set_style_pad_all(msgbox, 10, 0);
                    lv_obj_t* close_btn = lv_btn_create(msgbox); lv_obj_set_size(close_btn, 80, 40); lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
                    lv_obj_t* close_label = lv_label_create(close_btn); lv_label_set_text(close_label, "Close"); lv_obj_center(close_label);
                    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) { lv_obj_delete(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e))); }, LV_EVENT_CLICKED, NULL);
                }
            }, LV_EVENT_CLICKED, NULL); y_pos += 30; } }
    }
    lv_obj_t* back_btn = lv_btn_create(logs_screen); lv_obj_set_size(back_btn, 80, 40); lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, 0);
    lv_obj_add_style(back_btn, &style_btn, 0); lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn); lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back"); lv_obj_center(back_label); lv_obj_move_foreground(back_btn);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) { createMainMenu(); }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* reset_btn = lv_btn_create(logs_screen); lv_obj_set_size(reset_btn, 80, 40); lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_RIGHT, -10, 0);
    lv_obj_add_style(reset_btn, &style_btn, 0); lv_obj_add_style(reset_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* reset_label = lv_label_create(reset_btn); lv_label_set_text(reset_label, "Reset"); lv_obj_center(reset_label); lv_obj_move_foreground(reset_btn);
    lv_obj_add_event_cb(reset_btn, [](lv_event_t* e) {
        lv_obj_t* msgbox = lv_msgbox_create(NULL); lv_obj_set_size(msgbox, 250, 150); lv_obj_center(msgbox);
        lv_obj_set_style_bg_opa(msgbox, LV_OPA_COVER, 0); lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x808080), 0);
        lv_obj_t* title = lv_label_create(msgbox); lv_label_set_text(title, "Confirm Reset"); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_t* text = lv_label_create(msgbox); lv_label_set_text(text, "Are you sure?"); lv_obj_align(text, LV_ALIGN_CENTER, 0, -10);
        lv_obj_t* yes_btn = lv_btn_create(msgbox); lv_obj_set_size(yes_btn, 80, 40); lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
        lv_obj_t* yes_label = lv_label_create(yes_btn); lv_label_set_text(yes_label, "Yes"); lv_obj_center(yes_label);
        lv_obj_t* no_btn = lv_btn_create(msgbox); lv_obj_set_size(no_btn, 80, 40); lv_obj_align(no_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
        lv_obj_t* no_label = lv_label_create(no_btn); lv_label_set_text(no_label, "No"); lv_obj_center(no_label);
        lv_obj_add_event_cb(yes_btn, [](lv_event_t* e) {
            lv_obj_t* confirm_msgbox = lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e));
            SPI.end(); delay(100); SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(100);
            bool reset_success = false; if (SD.remove(LOG_FILENAME)) { File file = SD.open(LOG_FILENAME, FILE_WRITE); if (file) { file.println("# Log Reset " + getTimestamp()); file.close(); reset_success = true; } }
            SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); parsedLogEntries.clear();
            lv_obj_delete(confirm_msgbox);
            lv_obj_t* success_msgbox = lv_msgbox_create(NULL); lv_obj_set_size(success_msgbox, 250, 150); lv_obj_center(success_msgbox);
            lv_obj_set_style_bg_opa(success_msgbox, LV_OPA_COVER, 0); lv_obj_set_style_bg_color(success_msgbox, lv_color_hex(0x808080), 0);
            lv_obj_t* success_title = lv_label_create(success_msgbox); lv_label_set_text(success_title, reset_success ? "Reset Successful" : "Reset Failed"); lv_obj_align(success_title, LV_ALIGN_TOP_MID, 0, 10);
            lv_obj_t* success_text = lv_label_create(success_msgbox); lv_label_set_text(success_text, reset_success ? "Log has been reset." : "Failed to reset log."); lv_obj_align(success_text, LV_ALIGN_CENTER, 0, -10);
            lv_obj_t* ok_btn = lv_btn_create(success_msgbox); lv_obj_set_size(ok_btn, 80, 40); lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_obj_t* ok_label = lv_label_create(ok_btn); lv_label_set_text(ok_label, "OK"); lv_obj_center(ok_label);
            lv_obj_add_event_cb(ok_btn, [](lv_event_t* e) { lv_obj_t* success_msgbox = lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e)); lv_obj_t* old_screen = lv_scr_act(); lv_obj_delete(success_msgbox); createViewLogsScreen(); if (old_screen && old_screen != lv_scr_act()) lv_obj_delete(old_screen); lv_scr_load(lv_scr_act()); lv_task_handler(); }, LV_EVENT_CLICKED, NULL);
            lv_obj_move_foreground(success_msgbox); lv_task_handler();
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(no_btn, [](lv_event_t* e) { lv_obj_delete(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e))); lv_task_handler(); }, LV_EVENT_CLICKED, NULL);
        lv_obj_move_foreground(msgbox);
    }, LV_EVENT_CLICKED, NULL);
    lv_scr_load(logs_screen); lv_task_handler();
    DEBUG_PRINT("View logs screen created successfully");
}


void releaseSPIBus() { SPI.end(); delay(100); }


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
        lv_obj_add_event_cb(close_btn, [](lv_event_t* e) { lv_obj_delete(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e))); }, LV_EVENT_CLICKED, NULL);
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); return;
    }
    DEBUG_PRINT("SD card initialized successfully"); fileSystemInitialized = true;
    SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);
}


void lvgl_task(void *pvParameters) { while (1) { lv_timer_handler(); vTaskDelay(pdMS_TO_TICKS(5)); } }


void setup() {
    Serial.begin(115200); DEBUG_PRINT("Starting Loss Prevention Log...");
    auto cfg = M5.config(); M5.begin(cfg); M5.Power.begin();
    DEBUG_PRINT("Before lv_init"); lv_init(); m5gfx_lvgl_init(); DEBUG_PRINT("After lv_init");
    initFileSystem();
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, LVGL_TASK_CORE); DEBUG_PRINT("LVGL task created");
    initStyles();
    createLoadingScreen();
    M5.Power.setExtOutput(true);
    M5.Power.Axp2101.writeRegister8(0x94, 28); uint8_t reg90 = M5.Power.Axp2101.readRegister8(0x90); M5.Power.Axp2101.writeRegister8(0x90, reg90 | 0x08);
    M5.Speaker.begin(); DEBUG_PRINT("Speaker initialized");
    Preferences prefs; prefs.begin("settings", false);
    uint8_t saved_volume = prefs.getUChar("volume", 128); bool sound_enabled = prefs.getBool("sound_enabled", true);
    M5.Speaker.setVolume(sound_enabled ? saved_volume : 0);
    displayBrightness = prefs.getUChar("brightness", 128); M5.Display.setBrightness(displayBrightness);
    prefs.end();
    m5::rtc_date_t DateStruct; M5.Rtc.getDate(&DateStruct);
    if (DateStruct.year < 2020) { DateStruct.year = 2025; DateStruct.month = 3; DateStruct.date = 15; DateStruct.weekDay = 5; M5.Rtc.setDate(&DateStruct); m5::rtc_time_t TimeStruct; TimeStruct.hours = 12; TimeStruct.minutes = 0; TimeStruct.seconds = 0; M5.Rtc.setTime(&TimeStruct); }
    setSystemTimeFromRTC();
    wifiManager.begin();
    wifiManager.setStatusCallback(onWiFiStatus);
    wifiManager.setScanCallback(onWiFiScanComplete);

    DEBUG_PRINT("WiFi Manager initialized");
    DEBUG_PRINT("Setup complete!");
}


void loop() {
    M5.update();
    uint32_t currentMillis = millis();
# 609 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"
    wifiManager.update();
# 648 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"
    lv_timer_handler();
    delay(5);
}

static void wifi_status_update_cb(lv_timer_t* timer) {
    WiFiState state = (WiFiState)(uintptr_t)lv_timer_get_user_data(timer);
    String message = String(current_status_msg);

    DEBUG_PRINTF("wifi_status_update_cb: State=%d, Msg=%s\n", (int)state, message.c_str());

    if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(500)) == pdTRUE) {

        if (status_bar && lv_obj_is_valid(status_bar)) {
            lv_label_set_text(status_bar, message.c_str());
            lv_obj_set_style_text_color(status_bar, lv_color_hex(current_status_color), 0);
        }


        if (main_menu_screen && lv_obj_is_valid(main_menu_screen) && lv_scr_act() == main_menu_screen) {
            if (wifi_label && lv_obj_is_valid(wifi_label)) {
                char wifi_text[16];
                snprintf(wifi_text, sizeof(wifi_text), "%d%%", currentWiFiState.strength);
                lv_label_set_text(wifi_label, wifi_text);
                lv_obj_set_style_text_color(wifi_label, lv_color_hex(currentWiFiState.color), 0);
            }
            lv_obj_t* wifi_card = (wifi_label && lv_obj_is_valid(wifi_label)) ? lv_obj_get_parent(wifi_label) : nullptr;
            if (wifi_card && lv_obj_is_valid(wifi_card)) {
                lv_obj_t* wifi_container = lv_obj_get_child(wifi_card, 0);
                if (wifi_container && lv_obj_is_valid(wifi_container)) {
                    const int BAR_COUNT = 4;
                    for (int i = 0; i < BAR_COUNT; i++) {
                        lv_obj_t* bar = lv_obj_get_child(wifi_container, i);
                        if (bar && lv_obj_is_valid(bar)) {
                            bool active = currentWiFiState.connected && currentWiFiState.strength > (i + 1) * (100 / BAR_COUNT);
                            lv_obj_set_style_bg_color(bar, active ? lv_color_hex(currentWiFiState.color) : lv_color_hex(0x666666), 0);
                        }
                    }
                }
            }
        }


        if (wifi_management_screen && lv_obj_is_valid(wifi_management_screen) && lv_scr_act() == wifi_management_screen) {
            if (wifi_management_status_label && lv_obj_is_valid(wifi_management_status_label)) {
                lv_label_set_text(wifi_management_status_label, message.c_str());
                lv_obj_set_style_text_color(wifi_management_status_label, lv_color_hex(current_status_color), 0);
            }
        }


        if (wifi_screen && lv_obj_is_valid(wifi_screen) && lv_scr_act() == wifi_screen) {
            if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {

                if (state != WiFiState::WIFI_SCANNING && state != WiFiState::WIFI_SCAN_REQUESTED) {
                    lv_label_set_text(wifi_status_label, message.c_str());
                }
            }
        }


        if (wifi_loading_screen && lv_obj_is_valid(wifi_loading_screen) && lv_scr_act() == wifi_loading_screen) {
            if (state == WiFiState::WIFI_CONNECTED) {
                updateWiFiLoadingScreen(true, "Connected!");
            } else if (state == WiFiState::WIFI_CONNECTION_FAILED) {
                updateWiFiLoadingScreen(false, "Connection Failed!");
            } else if (state == WiFiState::WIFI_CONNECTING || state == WiFiState::WIFI_CONNECT_REQUESTED) {

            } else {
                updateWiFiLoadingScreen(false, message);
            }
        }


        if (lv_scr_act() && lv_obj_get_child_cnt(lv_scr_act()) > 0) {
            lv_obj_t* header_obj = lv_obj_get_child(lv_scr_act(), 0);
            if (header_obj && lv_obj_get_child_cnt(header_obj) > 0) {
                lv_obj_t* screen_title_obj = lv_obj_get_child(header_obj, 0);
                if (screen_title_obj && lv_obj_check_type(screen_title_obj, &lv_label_class)) {
                    String screen_title = lv_label_get_text(screen_title_obj);
                    if (screen_title.startsWith("Network: ")) {
                        String current_ssid_on_screen = screen_title.substring(9);
                        if (message.indexOf(current_ssid_on_screen) != -1 ||
                            state == WiFiState::WIFI_CONNECTED ||
                            state == WiFiState::WIFI_DISCONNECTED) {
                            DEBUG_PRINTF("Refreshing Network Details screen for %s due to state %d\n",
                                         current_ssid_on_screen.c_str(), (int)state);
                            createNetworkDetailsScreen(current_ssid_on_screen);
                        }
                    }
                }
            }
        }

        xSemaphoreGive(xGuiSemaphore);
    } else {
        DEBUG_PRINT("Failed to take xGuiSemaphore in wifi_status_update_cb after 500ms");
    }
    lv_timer_del(timer);
}


void onWiFiStatus(WiFiState state, const String& message) {
    DEBUG_PRINT("WiFi Status: " + String((int)state) + ", Msg: " + message);
    switch (state) {
        case WiFiState::WIFI_CONNECTED:
            currentWiFiState.connected = true;
            currentWiFiState.strength = wifiManager.getRSSI() * -1;
            currentWiFiState.color = 0x00FF00;
            break;
        case WiFiState::WIFI_DISCONNECTED:
        case WiFiState::WIFI_CONNECTION_FAILED:
            currentWiFiState.connected = false;
            currentWiFiState.strength = 0;
            currentWiFiState.color = 0x666666;
            break;
        case WiFiState::WIFI_CONNECTING:
        case WiFiState::WIFI_CONNECT_REQUESTED:
            currentWiFiState.connected = false;
            currentWiFiState.strength = 0;
            currentWiFiState.color = 0xFFFF00;
            break;
        case WiFiState::WIFI_SCANNING:
        case WiFiState::WIFI_SCAN_REQUESTED:
            currentWiFiState.connected = wifiManager.isConnected();
            currentWiFiState.strength = wifiManager.isConnected() ? wifiManager.getRSSI() * -1 : 0;
            currentWiFiState.color = 0x0000FF;
            break;
        case WiFiState::WIFI_DISABLED:
            currentWiFiState.connected = false;
            currentWiFiState.strength = 0;
            currentWiFiState.color = 0xFF0000;
            break;
    }
}

void onWiFiScanComplete(const std::vector<NetworkInfo>& results) {
    DEBUG_PRINT("WiFi Scan Complete: " + String(results.size()) + " networks found");
    if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (lv_scr_act() == wifi_screen && wifi_list && lv_obj_is_valid(wifi_list)) {
            lv_obj_clean(wifi_list);
            if (results.empty()) {
                lv_label_set_text(wifi_status_label, "No networks found");
            } else {
                for (const auto& network : results) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%s (%d)", network.ssid.c_str(), network.rssi);
                    lv_obj_t* item = lv_list_add_btn(wifi_list, network.connected ? LV_SYMBOL_WIFI : NULL, buf);
                    lv_obj_add_event_cb(item, [](lv_event_t* e) {
                        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
                        const char* text = lv_list_get_btn_text(wifi_list, btn);
                        String ssid = String(text).substring(0, String(text).indexOf(" ("));
                        DEBUG_PRINT("Network clicked: " + ssid);
                        showWiFiPasswordKeyboard(ssid);
                    }, LV_EVENT_CLICKED, NULL);
                }
                lv_label_set_text(wifi_status_label, "Scan complete");
            }
            if (g_spinner && lv_obj_is_valid(g_spinner)) {
                lv_obj_del(g_spinner);
                g_spinner = nullptr;
            }
        }
        xSemaphoreGive(xGuiSemaphore);
    }
}


static void wifi_scan_results_update_cb(lv_timer_t* timer) {
    DEBUG_PRINT("Entering wifi_scan_results_update_cb");
    WiFiScanUpdateData* data = (WiFiScanUpdateData*)lv_timer_get_user_data(timer);

    if (!data) {
        DEBUG_PRINT("Error: Invalid data pointer in wifi_scan_results_update_cb");
        if (xSemaphoreTake(xGuiSemaphore, (TickType_t)100) == pdTRUE) {
            if (g_spinner != nullptr && lv_obj_is_valid(g_spinner)) { lv_obj_del(g_spinner); g_spinner = nullptr; }
            xSemaphoreGive(xGuiSemaphore);
        }
        lv_timer_del(timer);
        return;
    }

    if (xSemaphoreTake(xGuiSemaphore, (TickType_t)100) == pdTRUE) {
        if (wifi_screen && lv_obj_is_valid(wifi_screen) && lv_scr_act() == wifi_screen) {
            DEBUG_PRINTF("Updating UI with %d scan results, failed=%d\n", data->results.size(), data->scan_failed);

            if (g_spinner != nullptr && lv_obj_is_valid(g_spinner)) {
                lv_obj_del(g_spinner);
                g_spinner = nullptr;
            }

            if (wifi_list && lv_obj_is_valid(wifi_list)) {
                lv_obj_clean(wifi_list);
            }

            if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                if (data->scan_failed) {
                    lv_label_set_text(wifi_status_label, "Scan Failed/Timed Out");
                } else if (data->results.empty()) {
                    lv_label_set_text(wifi_status_label, "No networks found");
                } else {
                    lv_label_set_text(wifi_status_label, "Scan complete. Select network.");
                    for (const auto& net : data->results) {
                        String displayText = net.ssid + (net.encryptionType != WIFI_AUTH_OPEN ? " *" : "");
                                                lv_obj_t* btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, displayText.c_str());

                        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                            lv_obj_t* clicked_btn = (lv_obj_t*)lv_event_get_target(e);

                            const char* text = lv_list_get_btn_text(wifi_list, clicked_btn);
                            if (text) {
                                String fullText = String(text);
                                String ssid = fullText;

                                if (fullText.endsWith(" *")) {
                                    ssid = fullText.substring(0, fullText.length() - 2);
                                }
                                DEBUG_PRINT("Network button clicked: " + ssid);
                                showWiFiPasswordKeyboard(ssid);
                            }
                        }, LV_EVENT_CLICKED, NULL);

                    }
                }
            }
        }
        xSemaphoreGive(xGuiSemaphore);
    }
    delete data;
    lv_timer_del(timer);
}

void createGenderMenu() {

    lv_obj_t* gender_screen = lv_obj_create(NULL);
    lv_obj_add_style(gender_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(gender_screen, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(gender_screen, LV_OPA_COVER, 0);
    lv_obj_add_flag(gender_screen, LV_OBJ_FLAG_SCROLLABLE);


    lv_obj_t* back_btn = lv_btn_create(gender_screen);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_t* title = lv_label_create(gender_screen);
    lv_label_set_text(title, "Select Gender");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);


    static lv_style_t style_card;
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(0x2D2D2D));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, lv_color_hex(0x505050));
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, 5);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x000000));
    lv_style_set_shadow_width(&style_card, 10);
    lv_style_set_shadow_spread(&style_card, 2);
    lv_style_set_pad_all(&style_card, 5);

    static lv_style_t style_card_pressed;
    lv_style_init(&style_card_pressed);
    lv_style_set_bg_color(&style_card_pressed, lv_color_hex(0x404040));
    lv_style_set_bg_opa(&style_card_pressed, LV_OPA_COVER);


    const char* genders[] = {"Male", "Female"};
    int y_offset = 80;
    for (int i = 0; i < 2; i++) {
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

        y_offset += 70;
    }

    lv_scr_load(gender_screen);
}

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


    lv_obj_t* grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 5, 0);


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


    lv_obj_t* color_buttons[numColors] = {nullptr};
    selectedShirtColors = "";

    for (int i = 0; i < numColors; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colorMap[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
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

                    lv_obj_set_size(btn, 140, 60);
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

                    lv_obj_set_size(btn, 150, 65);
                    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
                    lv_obj_set_style_border_width(btn, 4, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x00FFFF), 0);
                    lv_obj_set_style_shadow_width(btn, 12, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x00FFFF), 0);
                    if (!selectedShirtColors.isEmpty()) selectedShirtColors += "+";
                    selectedShirtColors += color;
                }
                DEBUG_PRINTF("Shirt colors updated: %s\n", selectedShirtColors.c_str());
            }
        }, LV_EVENT_CLICKED, NULL);

        DEBUG_PRINTF("Created color button %d: %s\n", i, colorMap[i].name);
    }


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


    lv_obj_t* grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 5, 0);


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


    lv_obj_t* color_buttons[numColors] = {nullptr};
    selectedPantsColors = "";

    for (int i = 0; i < numColors; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colorMap[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
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

                    lv_obj_set_size(btn, 140, 60);
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

                    lv_obj_set_size(btn, 150, 65);
                    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
                    lv_obj_set_style_border_width(btn, 4, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x00FFFF), 0);
                    lv_obj_set_style_shadow_width(btn, 12, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x00FFFF), 0);
                    if (!selectedPantsColors.isEmpty()) selectedPantsColors += "+";
                    selectedPantsColors += color;
                }
                DEBUG_PRINTF("Pants colors updated: %s\n", selectedPantsColors.c_str());
            }
        }, LV_EVENT_CLICKED, NULL);

        DEBUG_PRINTF("Created color button %d: %s\n", i, colorMap[i].name);
    }


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


    lv_obj_t* grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(grid, 5, 0);


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


    lv_obj_t* color_buttons[numColors] = {nullptr};
    selectedShoesColors = "";

    for (int i = 0; i < numColors; i++) {
        lv_obj_t* btn = lv_btn_create(grid);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_style_bg_color(btn, lv_color_hex(colorMap[i].color), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_80, 0);
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

                    lv_obj_set_size(btn, 140, 60);
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

                    lv_obj_set_size(btn, 150, 65);
                    lv_obj_set_style_bg_opa(btn, LV_OPA_100, 0);
                    lv_obj_set_style_border_width(btn, 4, 0);
                    lv_obj_set_style_border_color(btn, lv_color_hex(0x00FFFF), 0);
                    lv_obj_set_style_shadow_width(btn, 12, 0);
                    lv_obj_set_style_shadow_opa(btn, LV_OPA_60, 0);
                    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x00FFFF), 0);
                    if (!selectedShoesColors.isEmpty()) selectedShoesColors += "+";
                    selectedShoesColors += color;
                }
                DEBUG_PRINTF("Shoes colors updated: %s\n", selectedShoesColors.c_str());
            }
        }, LV_EVENT_CLICKED, NULL);

        DEBUG_PRINTF("Created color button %d: %s\n", i, colorMap[i].name);
    }


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

void createItemMenu() {
    if (itemMenu) { lv_obj_del(itemMenu); itemMenu = nullptr; }
    itemMenu = lv_obj_create(NULL); lv_obj_add_style(itemMenu, &style_screen, 0); lv_scr_load(itemMenu);
    lv_obj_t *header = lv_obj_create(itemMenu); lv_obj_set_size(header, 320, 50); lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header); lv_label_set_text(title, "Select Item"); lv_obj_add_style(title, &style_title, 0); lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *list_cont = lv_obj_create(itemMenu); lv_obj_set_size(list_cont, 280, 180); lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(list_cont, lv_color_hex(0x4A4A4A), 0); lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list_cont, 5, 0); lv_obj_set_scroll_dir(list_cont, LV_DIR_VER); lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(list_cont, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_MOMENTUM));
    for (int i = 0; i < NUM_ITEMS; i++) {
        lv_obj_t *btn = lv_btn_create(list_cont); lv_obj_set_width(btn, lv_pct(100)); lv_obj_set_height(btn, 40);
        lv_obj_add_style(btn, &style_btn, 0); lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn); lv_label_set_text(label, items[i]); lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t* target_btn = (lv_obj_t*)lv_event_get_target(e);
            lv_obj_t* label = lv_obj_get_child(target_btn, 0);
            if (label) { currentEntry += String(lv_label_get_text(label)); createConfirmScreen(); }
        }, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_update_layout(list_cont);
    if (lv_obj_get_content_height(list_cont) > lv_obj_get_height(list_cont)) current_scroll_obj = list_cont; else current_scroll_obj = nullptr;
}

void createConfirmScreen() {
    DEBUG_PRINT("Creating confirmation screen");


    if (confirmScreen) {
        DEBUG_PRINT("Cleaning existing confirm screen");
        lv_obj_del(confirmScreen);
        confirmScreen = nullptr;
        lv_timer_handler();
    }


    confirmScreen = lv_obj_create(NULL);
    if (!confirmScreen) {
        DEBUG_PRINT("Failed to create confirmation screen");
        createMainMenu();
        return;
    }

    lv_obj_add_style(confirmScreen, &style_screen, 0);
    lv_scr_load(confirmScreen);
    DEBUG_PRINTF("Confirm screen created: %p\n", confirmScreen);

    lv_timer_handler();

    lv_obj_t *header = lv_obj_create(confirmScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Confirm Entry");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);


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


    String entryToSave = currentEntry;

    lv_obj_t *btn_confirm = lv_btn_create(btn_container);
    lv_obj_set_size(btn_confirm, 120, 40);
    lv_obj_add_style(btn_confirm, &style_btn, 0);
    lv_obj_add_style(btn_confirm, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *confirm_label = lv_label_create(btn_confirm);
    lv_label_set_text(confirm_label, "Confirm");
    lv_obj_center(confirm_label);


    lv_obj_set_user_data(btn_confirm, (void*)strdup(formattedEntryStr.c_str()));

    lv_obj_add_event_cb(btn_confirm, [](lv_event_t *e) {
        lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
        saveEntry(currentEntry);
        createMainMenu();
        DEBUG_PRINT("Returning to main menu after confirmation");
    }, LV_EVENT_CLICKED, NULL);


    lv_timer_handler();
}




void createWiFiScreen() {
    if (wifi_screen && lv_obj_is_valid(wifi_screen)) {
        lv_scr_load(wifi_screen);
        return;
    }

    wifi_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_screen, &style_screen, 0);

    lv_obj_t* header = lv_obj_create(wifi_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi Networks");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_center(title);

    wifi_status_label = lv_label_create(wifi_screen);
    lv_label_set_text(wifi_status_label, "Ready");
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(wifi_status_label, &style_text, 0);

    wifi_list = lv_list_create(wifi_screen);
    lv_obj_set_size(wifi_list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 120);
    lv_obj_align(wifi_list, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_add_style(wifi_list, &style_network_item, 0);

    lv_obj_t* scan_btn = lv_btn_create(wifi_screen);
    lv_obj_set_size(scan_btn, 80, 40);
    lv_obj_align(scan_btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_add_style(scan_btn, &style_btn, 0);
    lv_obj_add_style(scan_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan");
    lv_obj_center(scan_label);

    lv_obj_add_event_cb(scan_btn, [](lv_event_t* e) {
        if (wifiManager.isScanning()) {
            DEBUG_PRINT("Scan already in progress");
            return;
        }

        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
            lv_label_set_text(wifi_status_label, "Scanning...");
            if (!g_spinner || !lv_obj_is_valid(g_spinner)) {
                g_spinner = lv_spinner_create(wifi_screen);
                lv_obj_set_size(g_spinner, 50, 50);
                lv_obj_center(g_spinner);
                lv_spinner_set_anim_params(g_spinner, 1000, 60);
            }
            xSemaphoreGive(xGuiSemaphore);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        if (wifiManager.startScan()) {
            DEBUG_PRINT("WiFi scan started");
        } else {
            DEBUG_PRINT("Failed to start WiFi scan");
            if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                lv_label_set_text(wifi_status_label, "Scan failed to start");
                if (g_spinner && lv_obj_is_valid(g_spinner)) {
                    lv_obj_del(g_spinner);
                    g_spinner = nullptr;
                }
                xSemaphoreGive(xGuiSemaphore);
            }
        }

        lv_timer_create([](lv_timer_t* timer) {
            if (wifiManager.isScanning()) {
                DEBUG_PRINT("Scan timed out after 15s");
                if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                    lv_label_set_text(wifi_status_label, "Scan timed out");
                    if (g_spinner && lv_obj_is_valid(g_spinner)) {
                        lv_obj_del(g_spinner);
                        g_spinner = nullptr;
                    }
                    xSemaphoreGive(xGuiSemaphore);
                }
            }
            lv_timer_del(timer);
        }, 15000, NULL);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* back_btn = lv_btn_create(wifi_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createWiFiManagementScreen();
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(wifi_screen);
    DEBUG_PRINT("WiFi screen created");
}


void createWiFiManagementScreen() {
    if (wifi_management_screen && lv_obj_is_valid(wifi_management_screen)) {
        lv_obj_del(wifi_management_screen);
        wifi_management_screen = nullptr;
    }
    wifi_management_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_management_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(wifi_management_screen, lv_color_hex(0x1A1A1A), 0);
    lv_obj_add_flag(wifi_management_screen, LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = wifi_management_screen;

    lv_obj_t* back_btn = lv_btn_create(wifi_management_screen);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* title = lv_label_create(wifi_management_screen);
    lv_label_set_text(title, "WiFi Manager");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);


    lv_obj_t* enable_label = lv_label_create(wifi_management_screen);
    lv_label_set_text(enable_label, "WiFi Enabled:");
    lv_obj_align(enable_label, LV_ALIGN_TOP_LEFT, 20, 60);
    lv_obj_add_style(enable_label, &style_text, 0);
    lv_obj_t* enable_switch = lv_switch_create(wifi_management_screen);
    lv_obj_align(enable_switch, LV_ALIGN_TOP_RIGHT, -20, 55);
    if (wifiManager.isEnabled()) lv_obj_add_state(enable_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(enable_switch, [](lv_event_t* e) {
        bool enabled = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
        if (!enabled && wifiManager.isScanning()) {
            lv_obj_t* msgbox = lv_msgbox_create(NULL);
            lv_obj_set_size(msgbox, 250, 150);
            lv_obj_center(msgbox);
            lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING);
            lv_obj_t* title = lv_label_create(msgbox);
            lv_label_set_text(title, "Scan in Progress");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
            lv_obj_t* text = lv_label_create(msgbox);
            lv_label_set_text(text, "Cannot disable WiFi during scan.");
            lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
            lv_obj_t* ok_btn = lv_btn_create(msgbox);
            lv_obj_set_size(ok_btn, 80, 40);
            lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_obj_t* ok_label = lv_label_create(ok_btn);
            lv_label_set_text(ok_label, "OK");
            lv_obj_center(ok_label);
            lv_obj_add_event_cb(ok_btn, [](lv_event_t* e) {
                lv_obj_delete(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e)));
            }, LV_EVENT_CLICKED, NULL);
            lv_obj_clear_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
        } else {
            wifiManager.setEnabled(enabled);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);


    wifi_management_status_label = lv_label_create(wifi_management_screen);
    lv_label_set_text(wifi_management_status_label, wifiManager.getStateString().c_str());
    lv_obj_align(wifi_management_status_label, LV_ALIGN_TOP_LEFT, 20, 90);
    lv_obj_add_style(wifi_management_status_label, &style_text, 0);


    lv_obj_t* saved_label = lv_label_create(wifi_management_screen);
    lv_label_set_text(saved_label, "Saved Networks:");
    lv_obj_align(saved_label, LV_ALIGN_TOP_LEFT, 20, 120);
    lv_obj_add_style(saved_label, &style_text, 0);


    lv_obj_t* list_cont = lv_obj_create(wifi_management_screen);
    lv_obj_set_size(list_cont, SCREEN_WIDTH - 40, 100);
    lv_obj_align(list_cont, LV_ALIGN_TOP_LEFT, 20, 145);
    lv_obj_set_style_bg_opa(list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list_cont, 5, 0);

    auto savedNetworks = wifiManager.getSavedNetworks();
    if (savedNetworks.empty()) {
        lv_obj_t* no_saved = lv_label_create(list_cont);
        lv_label_set_text(no_saved, "No saved networks.");
        lv_obj_add_style(no_saved, &style_text, 0);
    } else {
        for (size_t i = 0; i < savedNetworks.size(); i++) {
            lv_obj_t* network_item = lv_btn_create(list_cont);
            lv_obj_set_width(network_item, lv_pct(100));
            lv_obj_set_height(network_item, LV_SIZE_CONTENT);
            lv_obj_add_style(network_item, &style_network_item, 0);
            lv_obj_add_style(network_item, &style_network_item_pressed, LV_STATE_PRESSED);

            lv_obj_t* ssid_label = lv_label_create(network_item);
            String label_text = savedNetworks[i].ssid;
            if (wifiManager.isConnected() && wifiManager.getCurrentSSID() == savedNetworks[i].ssid) {
                label_text += " (Connected)";
                lv_obj_set_style_text_color(ssid_label, lv_color_hex(0x00FF00), 0);
            } else {
                lv_obj_set_style_text_color(ssid_label, lv_color_hex(0xFFFFFF), 0);
            }
            lv_label_set_text(ssid_label, label_text.c_str());
            lv_obj_align(ssid_label, LV_ALIGN_LEFT_MID, 5, 0);
            lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_16, 0);


            String* ssid_ptr = new String(savedNetworks[i].ssid);
            lv_obj_set_user_data(network_item, ssid_ptr);

            lv_obj_add_event_cb(network_item, [](lv_event_t* e) {
                String* ssid = static_cast<String*>(lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e)));
                if (ssid) {
                    createNetworkDetailsScreen(*ssid);
                }

            }, LV_EVENT_CLICKED, NULL);
        }
    }


    lv_obj_t* scan_btn = lv_btn_create(wifi_management_screen);
    lv_obj_set_size(scan_btn, 120, 45);
    lv_obj_align(scan_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_style(scan_btn, &style_btn, 0);
    lv_obj_add_style(scan_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(scan_label);
    lv_obj_add_event_cb(scan_btn, [](lv_event_t* e) {
        DEBUG_PRINT("Scan button pressed on WiFi Manager screen.");
        if (wifiManager.isEnabled()) {
            createWiFiScreen();
        } else {
            lv_obj_t* msgbox = lv_msgbox_create(NULL);
            lv_obj_set_size(msgbox, 250, 150);
            lv_obj_center(msgbox);
            lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING);
            lv_obj_t* title = lv_label_create(msgbox);
            lv_label_set_text(title, "WiFi Disabled");
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
            lv_obj_t* text = lv_label_create(msgbox);
            lv_label_set_text(text, "Please enable WiFi to scan.");
            lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
            lv_obj_t* ok_btn = lv_btn_create(msgbox);
            lv_obj_set_size(ok_btn, 80, 40);
            lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_obj_t* ok_label = lv_label_create(ok_btn);
            lv_label_set_text(ok_label, "OK");
            lv_obj_center(ok_label);
            lv_obj_add_event_cb(ok_btn, [](lv_event_t* e) {
                lv_obj_delete(lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e)));
            }, LV_EVENT_CLICKED, NULL);
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(wifi_management_screen);
}


void createNetworkDetailsScreen(const String& ssid) {

    lv_obj_t* old_screen = lv_scr_act();

    lv_obj_t* details_screen = lv_obj_create(NULL);
    lv_obj_add_style(details_screen, &style_screen, 0);

    lv_obj_t* header = lv_obj_create(details_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40); lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header); lv_label_set_text_fmt(title, "Network: %s", ssid.c_str());
    lv_obj_add_style(title, &style_title, 0); lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* container = lv_obj_create(details_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60); lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x222222), 0); lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);


    bool isConnected = wifiManager.isConnected() && wifiManager.getCurrentSSID() == ssid;
    WiFiState currentState = wifiManager.getState();
    String statusText = wifiManager.getStateString();
    if (isConnected) statusText = "Connected";
    else if (currentState == WiFiState::WIFI_CONNECTING && wifiManager.getCurrentSSID() == ssid) statusText = "Connecting...";
    else if (currentState == WiFiState::WIFI_CONNECT_REQUESTED && wifiManager.getCurrentSSID() == ssid) statusText = "Connect Requested...";
    else statusText = "Disconnected";

    lv_obj_t* status_label = lv_label_create(container);
    lv_label_set_text_fmt(status_label, "Status: %s", statusText.c_str());
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 10, 10); lv_obj_add_style(status_label, &style_text, 0);

    lv_obj_t* strength_label = lv_label_create(container);
    int rssi = isConnected ? wifiManager.getRSSI() : 0;
    int strength = isConnected ? map(rssi, -100, -50, 0, 100) : 0; strength = constrain(strength, 0, 100);
    lv_label_set_text_fmt(strength_label, "Signal: %d%%", strength);
    lv_obj_align(strength_label, LV_ALIGN_TOP_LEFT, 10, 40); lv_obj_add_style(strength_label, &style_text, 0);

    lv_obj_t* btn_container = lv_obj_create(container);
    lv_obj_set_size(btn_container, SCREEN_WIDTH - 40, 180); lv_obj_align(btn_container, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0); lv_obj_set_style_border_width(btn_container, 0, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_COLUMN); lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn_container, 15, 0);


    lv_obj_t* connect_btn = lv_btn_create(btn_container);
    lv_obj_set_size(connect_btn, 200, 50); lv_obj_add_style(connect_btn, &style_btn, 0); lv_obj_add_style(connect_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, isConnected ? "Disconnect" : "Connect"); lv_obj_center(connect_label);

    if ((currentState == WiFiState::WIFI_CONNECTING || currentState == WiFiState::WIFI_CONNECT_REQUESTED) && wifiManager.getCurrentSSID() == ssid) {
         lv_obj_add_state(connect_btn, LV_STATE_DISABLED);
    }
    String* ssid_ptr_connect = new String(ssid);
    lv_obj_set_user_data(connect_btn, ssid_ptr_connect);
    lv_obj_add_event_cb(connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, NULL);


    lv_obj_t* forget_btn = lv_btn_create(btn_container);
    lv_obj_set_size(forget_btn, 200, 50); lv_obj_add_style(forget_btn, &style_btn, 0); lv_obj_add_style(forget_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(forget_btn, lv_color_hex(0xE74C3C), 0);
    lv_obj_t* forget_label = lv_label_create(forget_btn); lv_label_set_text(forget_label, "Forget Network"); lv_obj_center(forget_label);
    String* ssid_ptr_forget = new String(ssid);
    lv_obj_set_user_data(forget_btn, ssid_ptr_forget);
    lv_obj_add_event_cb(forget_btn, forget_btn_event_cb, LV_EVENT_CLICKED, NULL);


    lv_obj_t* back_btn = lv_btn_create(btn_container);
    lv_obj_set_size(back_btn, 200, 50); lv_obj_add_style(back_btn, &style_btn, 0); lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x777777), 0);
    lv_obj_t* back_label = lv_label_create(back_btn); lv_label_set_text(back_label, "Back"); lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {

        lv_obj_t* btn_cont = lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e));
        String* ptr1 = (String*)lv_obj_get_user_data(lv_obj_get_child(btn_cont, 0));
        String* ptr2 = (String*)lv_obj_get_user_data(lv_obj_get_child(btn_cont, 1));
        delete ptr1;
        delete ptr2;
        createWiFiManagementScreen();
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(details_screen);


    if (old_screen && lv_obj_is_valid(old_screen)) {
        lv_obj_del_async(old_screen);
    }
}


static void connect_btn_event_cb(lv_event_t* e) {
    String* ssid = static_cast<String*>(lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e)));
    if (!ssid) return;

    bool isConnected = wifiManager.isConnected() && wifiManager.getCurrentSSID() == *ssid;

    if (isConnected) {
        DEBUG_PRINTF("Disconnecting from %s\n", ssid->c_str());
        wifiManager.disconnect(true);

    } else {

        auto savedNetworks = wifiManager.getSavedNetworks();
        String password = "";
        bool found = false;
        for (const auto& net : savedNetworks) {
            if (net.ssid == *ssid) {
                password = net.password;
                found = true;
                break;
            }
        }
        if (found) {
             DEBUG_PRINTF("Connecting to %s\n", ssid->c_str());
             showWiFiLoadingScreen(*ssid);
             wifiManager.connect(*ssid, password, false);
        } else {

             DEBUG_PRINTF("Error: Network %s not found in saved list for connect.\n", ssid->c_str());
        }
    }

}


static void forget_btn_event_cb(lv_event_t* e) {
    String* ssid_forget = static_cast<String*>(lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e)));
    if (!ssid_forget) return;


    lv_obj_t* btn_cont = lv_obj_get_parent((lv_obj_t*)lv_event_get_target(e));
    String* ssid_connect = (String*)lv_obj_get_user_data(lv_obj_get_child(btn_cont, 0));

    DEBUG_PRINTF("Forgetting network %s\n", ssid_forget->c_str());
    wifiManager.removeNetwork(*ssid_forget);


    delete ssid_connect;
    delete ssid_forget;

    createWiFiManagementScreen();
}


String getFormattedEntry(const String& entry) {
    String entryData = entry;
    String timestamp = getTimestamp();


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


    String formatted = "Time: " + timestamp + "\n";
    formatted += "Gender: " + (partCount > 0 ? parts[0] : "N/A") + "\n";
    formatted += "Shirt: " + (partCount > 1 ? parts[1] : "N/A") + "\n";
    formatted += "Pants: " + (partCount > 2 ? parts[2] : "N/A") + "\n";
    formatted += "Shoes: " + (partCount > 3 ? parts[3] : "N/A") + "\n";
    formatted += "Item: " + (partCount > 4 ? parts[4] : "N/A");

    DEBUG_PRINTF("Formatted: %s\n", formatted.c_str());
    return formatted;
}

void initStyles() {

    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x121212));
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);


    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0xE31B23));
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_radius(&style_btn, 8);


    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xA81419));
    lv_style_set_bg_opa(&style_btn_pressed, LV_OPA_COVER);


    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));


    lv_style_init(&style_card_action);
    lv_style_set_bg_color(&style_card_action, lv_color_hex(0x2A2A2A));
    lv_style_set_bg_opa(&style_card_action, LV_OPA_COVER);
    lv_style_set_border_color(&style_card_action, lv_color_hex(0xE31B23));
    lv_style_set_border_width(&style_card_action, 2);
    lv_style_set_radius(&style_card_action, 10);
    lv_style_set_text_color(&style_card_action, lv_color_hex(0xFFFFFF));


    lv_style_init(&style_card_pressed);
    lv_style_set_bg_color(&style_card_pressed, lv_color_hex(0x3A3A3A));
    lv_style_set_bg_opa(&style_card_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&style_card_pressed, 3);
    lv_style_set_text_color(&style_card_pressed, lv_color_hex(0xFFFFFF));


    lv_style_init(&style_card_info);
    lv_style_set_bg_color(&style_card_info, lv_color_hex(0x8B0000));
    lv_style_set_bg_opa(&style_card_info, LV_OPA_COVER);
    lv_style_set_border_color(&style_card_info, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&style_card_info, 1);
    lv_style_set_radius(&style_card_info, 10);
    lv_style_set_text_color(&style_card_info, lv_color_hex(0xFFFFFF));
    lv_style_set_shadow_color(&style_card_info, lv_color_hex(0x000000));
    lv_style_set_shadow_width(&style_card_info, 10);
    lv_style_set_shadow_opa(&style_card_info, LV_OPA_30);


    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);


    lv_style_init(&style_keyboard_btn);
    lv_style_set_bg_color(&style_keyboard_btn, lv_color_hex(0x3D3D3D));
    lv_style_set_text_color(&style_keyboard_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&style_keyboard_btn, 8);
    lv_style_set_border_width(&style_keyboard_btn, 0);

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

void println_log(const char *str) {
    Serial.println(str);

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

    if (status_bar != nullptr) {
        lv_label_set_text(status_bar, buf);
    }
}

void sendWebhook(const String& entry) {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINT("WiFi not connected, skipping webhook");
        return;
    }

    HTTPClient http;
    const char* zapierUrl = "https://hooks.zapier.com/hooks/catch/21957602/2qk3799/";
    Serial.println("Starting HTTP client...");
    DEBUG_PRINTF("Webhook URL: %s\n", zapierUrl);
    http.setReuse(false);
    if (!http.begin(zapierUrl)) {
        DEBUG_PRINT("Failed to begin HTTP client");
        return;
    }
    http.addHeader("Content-Type", "application/json");


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
        if (httpCode == 200 || httpCode == 201) {
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

void createSettingsScreen() {
    if (settingsScreen) {
        lv_obj_del(settingsScreen);
        settingsScreen = nullptr;
    }

    settingsScreen = lv_obj_create(NULL);
    lv_obj_add_style(settingsScreen, &style_screen, 0);


    lv_obj_t* header = lv_obj_create(settingsScreen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);


    lv_obj_t* list = lv_list_create(settingsScreen);
    lv_obj_set_size(list, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 72);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_radius(list, 15, 0);
    lv_obj_set_style_border_width(list, 0, 0);

    lv_obj_set_style_shadow_width(list, 10, 0);
    lv_obj_set_style_shadow_opa(list, LV_OPA_20, 0);
    lv_obj_set_style_shadow_color(list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_ofs_y(list, 5, 0);


    lv_obj_t* wifi_btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "WiFi Settings");
    lv_obj_add_event_cb(wifi_btn, [](lv_event_t* e) {
        createWiFiManagementScreen();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_t* sound_btn = lv_list_add_btn(list, LV_SYMBOL_AUDIO, "Sound Settings");
    lv_obj_add_event_cb(sound_btn, [](lv_event_t* e) {
        createSoundSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_t* display_btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Display Settings");
    lv_obj_add_event_cb(display_btn, [](lv_event_t* e) {
        createBrightnessSettingsScreen();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_t* datetime_btn = lv_list_add_btn(list, LV_SYMBOL_CALL, "Date & Time");
    lv_obj_add_event_cb(datetime_btn, [](lv_event_t* e) {
        createDateSelectionScreen();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_t* power_btn = lv_list_add_btn(list, LV_SYMBOL_BATTERY_FULL, "Power Management");
    lv_obj_add_event_cb(power_btn, [](lv_event_t* e) {
        createPowerManagementScreen();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_t* back_btn = lv_btn_create(settingsScreen);
    lv_obj_set_size(back_btn, 35, 23);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0);

    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(settingsScreen);
}

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


    lv_obj_t* header = lv_obj_create(sound_settings_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0x357ABD), 0);
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Sound Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);


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
        uint8_t current_volume = prefs.getUChar("volume", 128);
        M5.Speaker.setVolume(enabled ? current_volume : 0);
        if (enabled) M5.Speaker.tone(440, 100);
        prefs.putBool("sound_enabled", enabled);
        DEBUG_PRINTF("Sound %s\n", enabled ? "Enabled" : "Disabled");
        prefs.end();
    }, LV_EVENT_VALUE_CHANGED, NULL);


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
            M5.Speaker.tone(440, 100);
        }
        prefs.putUChar("volume", volume);
        DEBUG_PRINTF("Volume set to %d\n", volume);
        prefs.end();
    }, LV_EVENT_VALUE_CHANGED, NULL);


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


    static lv_obj_t* brightness_settings_screen = nullptr;


    if (brightness_settings_screen) {
        DEBUG_PRINTF("Cleaning and deleting existing brightness_settings_screen: %p\n", brightness_settings_screen);
        lv_obj_clean(brightness_settings_screen);
        lv_obj_del(brightness_settings_screen);
        brightness_settings_screen = nullptr;
        lv_task_handler();
        delay(10);
    }


    brightness_settings_screen = lv_obj_create(NULL);
    if (!brightness_settings_screen) {
        DEBUG_PRINT("Failed to create brightness_settings_screen");
        return;
    }
    DEBUG_PRINTF("Created brightness_settings_screen: %p\n", brightness_settings_screen);

    lv_obj_add_style(brightness_settings_screen, &style_screen, 0);
    lv_scr_load(brightness_settings_screen);
    DEBUG_PRINT("Screen loaded");


    lv_obj_t* header = lv_obj_create(brightness_settings_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0x357ABD), 0);
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Display Brightness");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);


    lv_obj_t* container = lv_obj_create(brightness_settings_screen);
    lv_obj_set_size(container, 300, 200);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2A2A40), 0);
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_shadow_color(container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_width(container, 15, 0);
    lv_obj_set_style_pad_all(container, 20, 0);


    lv_obj_t* brightnessValueLabel = lv_label_create(container);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (displayBrightness * 100) / 255);
    lv_label_set_text(brightnessValueLabel, buf);
    lv_obj_align(brightnessValueLabel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(brightnessValueLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(brightnessValueLabel, lv_color_hex(0xFFFFFF), 0);


    lv_obj_t* brightnessSlider = lv_slider_create(container);
    lv_obj_set_width(brightnessSlider, 260);
    lv_obj_align(brightnessSlider, LV_ALIGN_TOP_MID, 0, 40);
    lv_slider_set_range(brightnessSlider, 10, 255);
    lv_slider_set_value(brightnessSlider, displayBrightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x4A90E2), LV_PART_KNOB);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);


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


    lv_obj_t* back_btn = lv_btn_create(container);
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_align(back_btn, LV_ALIGN_TOP_MID, 0, 200);
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

void createPowerManagementScreen() {
    lv_obj_t* power_screen = lv_obj_create(NULL);
    lv_obj_add_style(power_screen, &style_screen, 0);


    lv_obj_t* header = lv_obj_create(power_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Power Management");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);


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


    lv_obj_t* power_off_btn = lv_btn_create(container);
    lv_obj_set_size(power_off_btn, 280, 60);
    lv_obj_add_style(power_off_btn, &style_btn, 0);
    lv_obj_add_style(power_off_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(power_off_btn, lv_color_hex(0xE74C3C), 0);

    lv_obj_t* power_off_label = lv_label_create(power_off_btn);
    lv_label_set_text(power_off_label, LV_SYMBOL_POWER " Power Off");
    lv_obj_set_style_text_font(power_off_label, &lv_font_montserrat_20, 0);
    lv_obj_center(power_off_label);


    lv_obj_t* restart_btn = lv_btn_create(container);
    lv_obj_set_size(restart_btn, 280, 60);
    lv_obj_add_style(restart_btn, &style_btn, 0);
    lv_obj_add_style(restart_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(restart_btn, lv_color_hex(0xF39C12), 0);

    lv_obj_t* restart_label = lv_label_create(restart_btn);
    lv_label_set_text(restart_label, LV_SYMBOL_REFRESH " Restart");
    lv_obj_set_style_text_font(restart_label, &lv_font_montserrat_20, 0);
    lv_obj_center(restart_label);


    lv_obj_t* sleep_btn = lv_btn_create(container);
    lv_obj_set_size(sleep_btn, 280, 60);
    lv_obj_add_style(sleep_btn, &style_btn, 0);
    lv_obj_add_style(sleep_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(sleep_btn, lv_color_hex(0x3498DB), 0);

    lv_obj_t* sleep_label = lv_label_create(sleep_btn);
    lv_label_set_text(sleep_label, LV_SYMBOL_DOWNLOAD " Sleep Mode");
    lv_obj_set_style_text_font(sleep_label, &lv_font_montserrat_20, 0);
    lv_obj_center(sleep_label);


    lv_obj_t* back_btn = lv_btn_create(container);
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);

    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);


    lv_obj_add_event_cb(power_off_btn, [](lv_event_t* e) {

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


        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.sleep();
        M5.Lcd.waitDisplay();


        M5.In_I2C.bitOn(AXP2101_ADDR, 0x41, 1 << 1, 100000L);
        M5.In_I2C.writeRegister8(AXP2101_ADDR, 0x25, 0b00011011, 100000L);
        M5.In_I2C.writeRegister8(AXP2101_ADDR, 0x10, 0b00110001, 100000L);

    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(restart_btn, [](lv_event_t* e) {

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


        ESP.restart();

    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(sleep_btn, [](lv_event_t* e) {

        lv_obj_t* sleep_screen = lv_obj_create(NULL);
        lv_obj_add_style(sleep_screen, &style_screen, 0);
        lv_scr_load(sleep_screen);

        lv_obj_t* sleep_label = lv_label_create(sleep_screen);
        lv_label_set_text(sleep_label, "Entering sleep mode...\nTouch screen to wake");
        lv_obj_set_style_text_font(sleep_label, &lv_font_montserrat_24, 0);
        lv_obj_align(sleep_label, LV_ALIGN_CENTER, 0, 0);
        lv_task_handler();
        delay(2000);



        M5.In_I2C.writeRegister8(AW9523_ADDR, 0x06, 0b11111111, 100000L);

        M5.In_I2C.writeRegister8(AW9523_ADDR, 0x07, 0b11111011, 100000L);
        M5.In_I2C.readRegister8(AW9523_ADDR, 0x00, 100000L);
        M5.In_I2C.readRegister8(AW9523_ADDR, 0x01, 100000L);

        pinMode(21, INPUT_PULLUP);


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

static void save_time_to_rtc() {
    DEBUG_PRINTF("Saving time: %04d-%02d-%02d %02d:%02d:00 %s\n",
                 selected_date.year, selected_date.month, selected_date.day,
                 selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");

    m5::rtc_date_t DateStruct;
    DateStruct.year = selected_date.year;
    DateStruct.month = selected_date.month;
    DateStruct.date = selected_date.day;


    int q = selected_date.day;
    int m = selected_date.month;
    int y = selected_date.year;
    if (m < 3) { m += 12; y--; }
    int h = (q + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    DateStruct.weekDay = (h + 1) % 7;

    M5.Rtc.setDate(&DateStruct);

    m5::rtc_time_t TimeStruct;

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

static void createDateSelectionScreen() {
    static lv_obj_t* date_screen = nullptr;
    if (date_screen) {
        lv_obj_del(date_screen);
        date_screen = nullptr;
    }
    date_screen = lv_obj_create(NULL);
    lv_obj_add_style(date_screen, &style_screen, 0);


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


    lv_obj_t* container = lv_obj_create(date_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);


    m5::rtc_date_t DateStruct;
    M5.Rtc.getDate(&DateStruct);
    selected_date.year = DateStruct.year;
    selected_date.month = DateStruct.month;
    selected_date.day = DateStruct.date;


    g_selected_date_label = lv_label_create(container);
    lv_obj_set_style_text_font(g_selected_date_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(g_selected_date_label, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(g_selected_date_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(g_selected_date_label, 5, 0);
    lv_obj_set_style_radius(g_selected_date_label, 5, 0);
    lv_obj_set_style_text_color(g_selected_date_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_selected_date_label, LV_ALIGN_TOP_MID, 0, 0);


    lv_obj_t* year_label = lv_label_create(container);
    lv_label_set_text(year_label, "Year:");
    lv_obj_align(year_label, LV_ALIGN_TOP_LEFT, 10, 25);

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
    lv_obj_set_width(g_year_roller, 95);
    lv_obj_align(year_label, LV_ALIGN_TOP_LEFT, 10, 25);
    lv_obj_align(g_year_roller, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_roller_set_selected(g_year_roller, DateStruct.year - 2020, LV_ANIM_OFF);


    lv_obj_t* month_label = lv_label_create(container);
    lv_label_set_text(month_label, "Month:");
    lv_obj_align(month_label, LV_ALIGN_TOP_MID, 0, 25);

    g_month_roller = lv_roller_create(container);
    lv_roller_set_options(g_month_roller,
                         "01-Jan\n02-Feb\n03-Mar\n04-Apr\n05-May\n06-Jun\n"
                         "07-Jul\n08-Aug\n09-Sep\n10-Oct\n11-Nov\n12-Dec",
                         LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_month_roller, 2);
    lv_obj_set_width(g_month_roller, 95);
    lv_obj_align(g_month_roller, LV_ALIGN_TOP_MID, 0, 40);
    lv_roller_set_selected(g_month_roller, DateStruct.month - 1, LV_ANIM_OFF);


    lv_obj_t* day_label = lv_label_create(container);
    lv_label_set_text(day_label, "Day:");
    lv_obj_align(day_label, LV_ALIGN_TOP_RIGHT, -10, 25);

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
    lv_obj_set_width(g_day_roller, 95);
    lv_obj_align(g_day_roller, LV_ALIGN_TOP_RIGHT, -10, 40);
    lv_roller_set_selected(g_day_roller, DateStruct.date - 1, LV_ANIM_OFF);


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


    lv_obj_add_event_cb(g_year_roller, on_year_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_month_roller, on_month_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_day_roller, on_day_change, LV_EVENT_VALUE_CHANGED, NULL);


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


static void createTimeSelectionScreen() {
    static lv_obj_t* time_screen = nullptr;
    if (time_screen) {
        lv_obj_del(time_screen);
        time_screen = nullptr;
    }
    time_screen = lv_obj_create(NULL);
    lv_obj_add_style(time_screen, &style_screen, 0);


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


    lv_obj_t* container = lv_obj_create(time_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);


    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);


    g_selected_time_label = lv_label_create(container);
    lv_obj_set_style_text_font(g_selected_time_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(g_selected_time_label, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(g_selected_time_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(g_selected_time_label, 5, 0);
    lv_obj_set_style_radius(g_selected_time_label, 5, 0);
    lv_obj_set_style_text_color(g_selected_time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_selected_time_label, LV_ALIGN_TOP_MID, 0, 0);


    lv_obj_t* hour_label = lv_label_create(container);
    lv_label_set_text(hour_label, "Hour:");
    lv_obj_align(hour_label, LV_ALIGN_TOP_LEFT, 10, 25);

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
    lv_obj_align(g_hour_roller, LV_ALIGN_TOP_LEFT, 10, 40);
    int display_hour = (TimeStruct.hours == 0) ? 12 : (TimeStruct.hours > 12 ? TimeStruct.hours - 12 : TimeStruct.hours);
    lv_roller_set_selected(g_hour_roller, display_hour - 1, LV_ANIM_OFF);
    selected_hour = display_hour;


    lv_obj_t* minute_label = lv_label_create(container);
    lv_label_set_text(minute_label, "Minute:");
    lv_obj_align(minute_label, LV_ALIGN_TOP_MID, 0, 25);

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
    lv_obj_align(g_minute_roller, LV_ALIGN_TOP_MID, 0, 40);
    lv_roller_set_selected(g_minute_roller, TimeStruct.minutes, LV_ANIM_OFF);
    selected_minute = TimeStruct.minutes;


    lv_obj_t* am_pm_label = lv_label_create(container);
    lv_label_set_text(am_pm_label, "Period:");
    lv_obj_align(am_pm_label, LV_ALIGN_TOP_RIGHT, -10, 25);

    g_am_pm_roller = lv_roller_create(container);
    lv_roller_set_options(g_am_pm_roller, "AM\nPM", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_am_pm_roller, 2);
    lv_obj_set_width(g_am_pm_roller, 95);
    lv_obj_align(g_am_pm_roller, LV_ALIGN_TOP_RIGHT, -10, 40);
    selected_is_pm = (TimeStruct.hours >= 12) ? 1 : 0;
    lv_roller_set_selected(g_am_pm_roller, selected_is_pm, LV_ANIM_OFF);


    char selected_time_str[32];
    snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s",
             selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
    lv_label_set_text(g_selected_time_label, selected_time_str);


    lv_obj_add_event_cb(g_hour_roller, on_hour_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_minute_roller, on_minute_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_am_pm_roller, [](lv_event_t* e) {
        selected_is_pm = lv_roller_get_selected(g_am_pm_roller);
        char selected_time_str[32];
        snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s",
                 selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
        lv_label_set_text(g_selected_time_label, selected_time_str);
    }, LV_EVENT_VALUE_CHANGED, NULL);


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

static void on_year_change(lv_event_t* e) {

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


static void on_hour_change(lv_event_t* e) {
    selected_hour = lv_roller_get_selected(g_hour_roller) + 1;
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


void createLoadingScreen() {
    lv_obj_t* loading_screen = lv_obj_create(NULL);
    lv_obj_add_style(loading_screen, &style_screen, 0);


    lv_obj_t* bg_img = lv_img_create(loading_screen);
    lv_img_set_src(bg_img, &LossPrev2);
    lv_obj_center(bg_img);


    lv_obj_t* overlay = lv_obj_create(loading_screen);
    lv_obj_set_size(overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_30, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);


    lv_obj_t* title = lv_label_create(loading_screen);
    lv_label_set_text(title, "Loss Prevention Log");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);


    lv_obj_t* progress_bar = lv_bar_create(loading_screen);
    lv_obj_set_size(progress_bar, 200, 20);
    lv_obj_align(progress_bar, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x4A90E2), LV_PART_INDICATOR);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(progress_bar, 0, 100);


    lv_obj_t* loading_label = lv_label_create(loading_screen);
    lv_label_set_text(loading_label, "Loading...");
    lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(loading_label, LV_ALIGN_BOTTOM_MID, 0, -70);


    lv_obj_t* version_label = lv_label_create(loading_screen);
    lv_label_set_text(version_label, "v1.0.0 - 2025");
    lv_obj_set_style_text_color(version_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(version_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);


    lv_scr_load(loading_screen);




    lv_timer_t* timer = lv_timer_create(updateLoadingProgress, 50, progress_bar);


    lv_obj_set_user_data(loading_screen, timer);

    DEBUG_PRINT("Loading screen created");
}


void updateLoadingProgress(lv_timer_t* timer) {
    lv_obj_t* bar = (lv_obj_t*)lv_timer_get_user_data(timer);
    int32_t value = lv_bar_get_value(bar);
    value += 5;
    lv_bar_set_value(bar, value, LV_ANIM_ON);

    if (value >= 100) {
        lv_timer_del(timer);


        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(500)) == pdTRUE) {
            lv_obj_t* old_screen = lv_scr_act();
            createLockScreen();
            if (old_screen && old_screen != lv_scr_act()) {
                lv_obj_del(old_screen);
            }
            xSemaphoreGive(xGuiSemaphore);
        } else {
            DEBUG_PRINT("Failed to take xGuiSemaphore in updateLoadingProgress");

        }
    }
}
void createLockScreen() {
    if (lock_screen && lv_obj_is_valid(lock_screen)) {
        lv_obj_del(lock_screen);
    }
    lock_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_opa(lock_screen, LV_OPA_TRANSP, 0);


    lv_obj_t* bg_img = lv_img_create(lock_screen);
    lv_img_set_src(bg_img, &LossPrev1);
    lv_obj_center(bg_img);



    g_lock_screen_datetime_label = lv_label_create(lock_screen);
    lv_obj_align(g_lock_screen_datetime_label, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_color(g_lock_screen_datetime_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(g_lock_screen_datetime_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(g_lock_screen_datetime_label, LV_TEXT_ALIGN_CENTER, 0);
    updateLockScreenTime();


    lv_obj_t* unlock_btn = lv_btn_create(lock_screen);
    lv_obj_set_size(unlock_btn, 160, 45);
    lv_obj_align(unlock_btn, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_add_style(unlock_btn, &style_btn, 0);
    lv_obj_add_style(unlock_btn, &style_btn_pressed, LV_STATE_PRESSED);



    lv_obj_t* unlock_label = lv_label_create(unlock_btn);
    lv_label_set_text(unlock_label, "Press to Unlock");
    lv_obj_center(unlock_label);


    lv_obj_add_event_cb(unlock_btn, [](lv_event_t* e) {
        DEBUG_PRINT("Unlock button pressed.");

        createMainMenu();
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        lv_obj_t* parent1 = lv_obj_get_parent(btn);
        lv_obj_t* old_screen = lv_obj_get_parent(parent1);
        if (old_screen && old_screen == lock_screen) {
             g_lock_screen_datetime_label = nullptr;
             lv_obj_del_async(old_screen);
             lock_screen = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(lock_screen);
    DEBUG_PRINT("Lock screen created and loaded.");
}

void updateLockScreenTime() {
    if (!g_lock_screen_datetime_label || !lv_obj_is_valid(g_lock_screen_datetime_label) || lv_scr_act() != lock_screen) {
        return;
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
    timeinfo.tm_isdst = -1;


    if (DateStruct.year >= 2023) {
        char date_buf[16];
        char time_buf[16];
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%I:%M:%S %p", &timeinfo);


        lv_label_set_text_fmt(g_lock_screen_datetime_label, "%s\n%s", date_buf, time_buf);
    } else {
        lv_label_set_text(g_lock_screen_datetime_label, "RTC Error\nSet Time");
    }
}

void showWiFiLoadingScreen(const String& ssid) {
    if (wifi_loading_screen != nullptr) {
        lv_obj_del(wifi_loading_screen);
    }

    wifi_loading_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_loading_screen, &style_screen, 0);


    lv_obj_t* header = lv_obj_create(wifi_loading_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi Connection");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);


    wifi_loading_spinner = lv_spinner_create(wifi_loading_screen);
    lv_spinner_set_anim_params(wifi_loading_spinner, 1000, 60);
    lv_obj_set_size(wifi_loading_spinner, 100, 100);
    lv_obj_align(wifi_loading_spinner, LV_ALIGN_CENTER, 0, -20);


    wifi_loading_label = lv_label_create(wifi_loading_screen);
    lv_label_set_text_fmt(wifi_loading_label, "Connecting to %s...", ssid.c_str());
    lv_obj_align(wifi_loading_label, LV_ALIGN_CENTER, 0, 60);


    wifi_result_label = lv_label_create(wifi_loading_screen);
    lv_obj_set_style_text_font(wifi_result_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(wifi_result_label, "");
    lv_obj_align(wifi_result_label, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_flag(wifi_result_label, LV_OBJ_FLAG_HIDDEN);


    lv_obj_t* back_btn = lv_btn_create(wifi_loading_screen);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createWiFiManagementScreen();
    }, LV_EVENT_CLICKED, NULL);


    lv_obj_set_user_data(wifi_loading_screen, back_btn);

    lv_scr_load(wifi_loading_screen);
}

void updateWiFiLoadingScreen(bool success, const String& message) {
    if (wifi_loading_screen == nullptr || !lv_obj_is_valid(wifi_loading_screen)) {
        return;
    }


    if (wifi_loading_spinner != nullptr && lv_obj_is_valid(wifi_loading_spinner)) {
        lv_obj_add_flag(wifi_loading_spinner, LV_OBJ_FLAG_HIDDEN);
    }


    if (wifi_result_label != nullptr && lv_obj_is_valid(wifi_result_label)) {
        lv_obj_clear_flag(wifi_result_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(wifi_result_label, message.c_str());
        lv_obj_set_style_text_color(wifi_result_label,
            success ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    }


    lv_obj_t* back_btn = (lv_obj_t*)lv_obj_get_user_data(wifi_loading_screen);
    if (back_btn != nullptr && lv_obj_is_valid(back_btn)) {
        lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_HIDDEN);
    }


    if (success) {
        static lv_timer_t* return_timer = nullptr;
        if (return_timer != nullptr) {
            lv_timer_del(return_timer);
        }
        return_timer = lv_timer_create([](lv_timer_t* timer) {
            createWiFiManagementScreen();
            lv_timer_del(timer);
        }, 2000, NULL);
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

    updateTimeDisplay();
}

void updateTimeDisplay() {
    if (time_label == nullptr) return;

    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);


    int hour = TimeStruct.hours;
    const char* period = (hour >= 12) ? "PM" : "AM";
    if (hour == 0) {
        hour = 12;
    } else if (hour > 12) {
        hour -= 12;
    }

    char timeStr[24];
    snprintf(timeStr, sizeof(timeStr), "%d:%02d:%02d %s",
             hour, TimeStruct.minutes, TimeStruct.seconds, period);
    lv_label_set_text(time_label, timeStr);
}

void addStatusBar(lv_obj_t* screen) {
    if (status_bar) lv_obj_del(status_bar);
    status_bar = lv_label_create(screen);
    lv_obj_set_style_text_font(status_bar, &lv_font_montserrat_14, 0);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(status_bar, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(status_bar, "Ready");
}

void updateStatus(const char* message, uint32_t color) {
    if (!status_bar) return;


    m5::rtc_time_t TimeStruct;
    M5.Rtc.getTime(&TimeStruct);


    char time_str[12];
    struct tm timeinfo;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    strftime(time_str, sizeof(time_str), "%I:%M %p", &timeinfo);


    if (time_str[0] == '0') {
        memmove(time_str, time_str + 1, strlen(time_str));
    }


    if (!time_label) {
        time_label = lv_label_create(status_bar);
        lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -10, 0);
    }
    lv_label_set_text(time_label, time_str);


    if (status_bar && current_status_msg[0] != '\0') {
        lv_label_set_text(status_bar, current_status_msg);
        lv_obj_set_style_text_color(status_bar, lv_color_hex(current_status_color), 0);
    }
}

void handleSwipeLeft() {
    lv_obj_t* current_screen_obj = lv_scr_act();


    if (current_screen_obj == lock_screen) {
        DEBUG_PRINT("Swipe ignored on lock screen.");
        return;
    }

    if (current_screen_obj == wifi_management_screen) {
        DEBUG_PRINT("Swiping back to settings screen from WiFi Manager");
        createSettingsScreen();
    } else if (current_screen_obj == genderMenu) {
        DEBUG_PRINT("Swiping back to main menu from gender menu");
        createMainMenu();
    } else if (current_screen_obj == colorMenu) {

        if (shoes_next_btn != nullptr) createColorMenuPants();
        else if (pants_next_btn != nullptr) createColorMenuShirt();
        else createGenderMenu();
    } else if (current_screen_obj == itemMenu) {
        DEBUG_PRINT("Swiping back to shoes menu from item menu");
        createColorMenuShoes();
    } else if (current_screen_obj == confirmScreen) {
        DEBUG_PRINT("Swiping back to item menu from confirm screen");
        createItemMenu();
    }



    if (current_screen_obj && current_screen_obj != lv_scr_act() && lv_obj_is_valid(current_screen_obj)) {
        DEBUG_PRINTF("Cleaning old screen: %p\n", current_screen_obj);
        lv_obj_del_async(current_screen_obj);
    }
}

void handleSwipeVertical(int amount) {
    if (current_scroll_obj && lv_obj_is_valid(current_scroll_obj)) {
        lv_obj_scroll_by(current_scroll_obj, 0, amount, LV_ANIM_ON);
        DEBUG_PRINTF("Scrolled %p by %d pixels\n", current_scroll_obj, amount);
        lv_obj_invalidate(current_scroll_obj);
    } else {
        DEBUG_PRINT("No valid scrollable object");
    }
}

void createMainMenu() {
    DEBUG_PRINTF("Free heap before main menu: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));


    releaseSPIBus();
    SPI.begin();
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);


    if (main_menu_screen) {
        lv_obj_clean(main_menu_screen);
        lv_obj_del(main_menu_screen);
        main_menu_screen = nullptr;
    }
    main_menu_screen = lv_obj_create(NULL);
    lv_obj_add_style(main_menu_screen, &style_screen, 0);


    current_scroll_obj = nullptr;
    status_bar = nullptr;
    battery_icon = nullptr;
    battery_label = nullptr;
    wifi_label = nullptr;
    time_label = nullptr;


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


    lv_obj_t* grid = lv_obj_create(main_menu_screen);
    lv_obj_set_size(grid, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {80, 80, 80, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);
    lv_obj_set_content_width(grid, 300);

    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_scroll_snap_y(grid, LV_SCROLL_SNAP_NONE);
    lv_obj_add_flag(grid, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    current_scroll_obj = grid;


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


    lv_obj_t* logs_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(logs_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_add_style(logs_card, &style_card_action, 0);
    lv_obj_add_style(logs_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* logs_icon = lv_label_create(logs_card);
    lv_label_set_text(logs_icon, LV_SYMBOL_LIST);
    lv_obj_set_style_text_font(logs_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(logs_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* logs_label = lv_label_create(logs_card);
    lv_label_set_text(logs_label, "Logs");
    lv_obj_align(logs_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(logs_card, [](lv_event_t* e) {
        createViewLogsScreen();
    }, LV_EVENT_CLICKED, NULL);


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


    lv_obj_t* wifi_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(wifi_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_style(wifi_card, &style_card_info, 0);


    lv_obj_t* wifi_container = lv_obj_create(wifi_card);
    lv_obj_set_size(wifi_container, 90, 40);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_container, 0, 0);
    lv_obj_set_style_pad_all(wifi_container, 0, 0);
    lv_obj_align(wifi_container, LV_ALIGN_CENTER, -10, 0);


    int rssi = WiFi.RSSI();
    int wifi_strength = map(rssi, -100, -50, 0, 100);
    wifi_strength = constrain(wifi_strength, 0, 100);


    uint32_t wifi_color;
    if (WiFi.status() != WL_CONNECTED) {
        wifi_color = 0xFF0000;
    } else if (wifi_strength < 30) {
        wifi_color = 0xFF0000;
    } else if (wifi_strength < 70) {
        wifi_color = 0xFFAA00;
    } else {
        wifi_color = 0x00FF00;
    }


    const int BAR_WIDTH = 8;
    const int BAR_GAP = 2;
    const int BAR_COUNT = 4;
    const int MAX_BAR_HEIGHT = 30;

    for (int i = 0; i < BAR_COUNT; i++) {
        int bar_height = 6 + ((i + 1) * (MAX_BAR_HEIGHT - 6) / BAR_COUNT);
        lv_obj_t* bar = lv_obj_create(wifi_container);
        lv_obj_set_size(bar, BAR_WIDTH, bar_height);
        lv_obj_set_pos(bar, i * (BAR_WIDTH + BAR_GAP), MAX_BAR_HEIGHT - bar_height);


        bool active = wifi_strength > (i + 1) * (100 / BAR_COUNT);

        if (WiFi.status() != WL_CONNECTED) {

            lv_obj_set_style_bg_color(bar, lv_color_hex(0x666666), 0);
        } else if (active) {

            lv_obj_set_style_bg_color(bar, lv_color_hex(wifi_color), 0);
        } else {

            lv_obj_set_style_bg_color(bar, lv_color_hex(0x666666), 0);
        }

        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 1, 0);
    }


    wifi_label = lv_label_create(wifi_card);
    char wifi_text[16];
    snprintf(wifi_text, sizeof(wifi_text), "%d%%", wifi_strength);
    lv_label_set_text(wifi_label, wifi_text);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(wifi_color), 0);
    lv_obj_align(wifi_label, LV_ALIGN_CENTER, 40, 0);


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


    addStatusBar(main_menu_screen);
    updateStatus("Ready", 0xFFFFFF);


    lv_scr_load(main_menu_screen);
    addTimeDisplay(main_menu_screen);

    DEBUG_PRINTF("Free heap after main menu: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}


static void showWiFiPasswordKeyboard(const String& ssid) {

     if (wifi_password_keyboard != nullptr && lv_obj_is_valid(wifi_password_keyboard)) {
         lv_obj_del(wifi_password_keyboard);
         wifi_password_keyboard = nullptr;
     }


     wifi_keyboard_ssid = ssid;



     lv_obj_t* parent_screen = lv_scr_act();
     wifi_password_keyboard = lv_obj_create(parent_screen);
     lv_obj_set_size(wifi_password_keyboard, 320, 240);
     lv_obj_align(wifi_password_keyboard, LV_ALIGN_CENTER, 0, 0);
     lv_obj_set_style_bg_color(wifi_password_keyboard, lv_color_hex(0x1E1E1E), 0);
     lv_obj_set_style_bg_opa(wifi_password_keyboard, LV_OPA_90, 0);
     lv_obj_add_flag(wifi_password_keyboard, LV_OBJ_FLAG_FLOATING);


     lv_obj_t* kb_title = lv_label_create(wifi_password_keyboard);
     lv_label_set_text_fmt(kb_title, "Enter Password for %s", ssid.c_str());
     lv_obj_set_style_text_color(kb_title, lv_color_hex(0xFFFFFF), 0);
     lv_obj_align(kb_title, LV_ALIGN_TOP_MID, 0, 5);


     wifi_password_ta = lv_textarea_create(wifi_password_keyboard);
     lv_obj_set_size(wifi_password_ta, 260, 40);
     lv_obj_align(wifi_password_ta, LV_ALIGN_TOP_MID, 0, 30);
     lv_textarea_set_password_mode(wifi_password_ta, true);
     lv_textarea_set_max_length(wifi_password_ta, 64);
     lv_textarea_set_placeholder_text(wifi_password_ta, "Password");



     lv_obj_t* close_btn = lv_btn_create(wifi_password_keyboard);
     lv_obj_set_size(close_btn, 30, 30);
     lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
     lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFF0000), 0);
     lv_obj_set_style_radius(close_btn, 15, 0);
     lv_obj_t* close_label = lv_label_create(close_btn);
     lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
     lv_obj_center(close_label);
     lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
         if (wifi_password_keyboard != nullptr && lv_obj_is_valid(wifi_password_keyboard)) {
             lv_obj_del(wifi_password_keyboard);
             wifi_password_keyboard = nullptr;
             wifi_password_ta = nullptr;
         }
     }, LV_EVENT_CLICKED, NULL);


     lv_obj_t* kb = lv_btnmatrix_create(wifi_password_keyboard);
     lv_obj_set_size(kb, 300, 150);
     lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -5);
     keyboard_page_index = 0;
     lv_btnmatrix_set_map(kb, btnm_mapplus[keyboard_page_index]);
     lv_btnmatrix_set_ctrl_map(kb, keyboard_ctrl_map);
     lv_obj_set_style_pad_row(kb, 5, 0);
     lv_obj_set_style_pad_column(kb, 5, 0);
     lv_obj_add_style(kb, &style_keyboard_btn, LV_PART_ITEMS);


     lv_obj_add_event_cb(kb, [](lv_event_t* e) {
         lv_event_code_t code = lv_event_get_code(e);
         lv_obj_t* btnm = (lv_obj_t*)lv_event_get_target(e);

         if (code == LV_EVENT_VALUE_CHANGED) {
             uint32_t btn_id = lv_btnmatrix_get_selected_btn(btnm);
             const char* txt = lv_btnmatrix_get_btn_text(btnm, btn_id);

             if (txt && wifi_password_ta && lv_obj_is_valid(wifi_password_ta)) {
                 if (strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
                     keyboard_page_index = (keyboard_page_index + 1) % NUM_KEYBOARD_PAGES;
                     lv_btnmatrix_set_map(btnm, btnm_mapplus[keyboard_page_index]);
                 } else if (strcmp(txt, LV_SYMBOL_LEFT) == 0) {
                     keyboard_page_index = (keyboard_page_index - 1 + NUM_KEYBOARD_PAGES) % NUM_KEYBOARD_PAGES;
                     lv_btnmatrix_set_map(btnm, btnm_mapplus[keyboard_page_index]);
                 } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                     lv_textarea_delete_char(wifi_password_ta);
                 } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {

                     const char* password = lv_textarea_get_text(wifi_password_ta);
                     DEBUG_PRINTF("Connecting to %s with entered password.\n", wifi_keyboard_ssid.c_str());

                     showWiFiLoadingScreen(wifi_keyboard_ssid);
                     wifiManager.connect(wifi_keyboard_ssid, password, true);


                     if (wifi_password_keyboard != nullptr && lv_obj_is_valid(wifi_password_keyboard)) {
                         lv_obj_del(wifi_password_keyboard);
                         wifi_password_keyboard = nullptr;
                         wifi_password_ta = nullptr;
                     }
                 } else {

                     lv_textarea_add_text(wifi_password_ta, txt);
                 }
             }
         }
     }, LV_EVENT_VALUE_CHANGED, NULL);
 }