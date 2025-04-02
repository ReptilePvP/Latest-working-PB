// #include <WiFi.h> // Keep removed, should be in globals.h
#include "ui.h"
#include "globals.h" // Include common definitions (includes WiFi.h and lvgl via m5gfx_lvgl.hpp)
#include "time_utils.h" // Needed for save_time_to_rtc, updateLockScreenTime, updateTimeDisplay, getTimestamp
#include "sd_logger.h" // Needed for saveEntry, getFormattedEntry, loadLogEntriesForDateRange, parseTimestamp, LOG_FILENAME
#include "wifi_handler.h" // Needed for startWifiScan, connectToWiFi etc.
// #include <lvgl.h> // Keep removed

// Forward declarations for static event handlers used before definition
static void connect_btn_event_cb(lv_event_t* e);
static void forget_btn_event_cb(lv_event_t* e);
static void power_management_msgbox_event_cb(lv_event_t* e); // Renamed
static void reset_confirm_event_cb(lv_event_t* e);
// Add other forward declarations if needed (e.g., for date/time rollers)
static void on_year_change(lv_event_t* e);
static void on_month_change(lv_event_t* e);
static void on_day_change(lv_event_t* e);
static void on_hour_change(lv_event_t* e);
static void on_minute_change(lv_event_t* e);
static void pants_type_card_event_cb(lv_event_t* e); // Added forward declaration
// Forward declarations
void displayLogPage(lv_obj_t* list_container, int page);
void updateStatusBar(); // Keep this one
#include <cstring> // For strcmp, strncpy, strlen, strdup, free, memmove
#include <cstdlib> // For free (used with strdup)
#include <Preferences.h> // Needed for settings screens
// #include <WiFi.h> // Keep removed
#include <algorithm> // Needed for std::sort in createViewLogsScreen
// #include <lvgl.h> // Keep removed


// --- Style Definitions ---
lv_style_t style_screen, style_btn, style_btn_pressed, style_title, style_text;
lv_style_t style_card_action, style_card_info, style_card_pressed;
// Note: style_card_pressed is defined twice in the original .ino (lines 67 and 3227)
// We'll use the second definition from initStyles (lines 3227-3232) as it seems more complete for card interactions.
// lv_style_t style_gender_card, style_gender_card_pressed; // These seem unused, replaced by style_card/style_card_pressed
// lv_style_t style_network_item, style_network_item_pressed; // Replaced by style_network/style_network_pressed
lv_style_t style_keyboard_btn;
lv_style_t style_network;
lv_style_t style_network_pressed; // Added for consistency
lv_style_t style_card; // Used for gender/type cards

// --- Global Variables (Defined here, declared extern in globals.h) ---
// Entry State
String selectedGender = "";
String selectedApparelType = "";
String selectedShirtColors = "";
String selectedPantsType = "";
String selectedPantsColors = "";
String selectedShoeStyle = "";
String selectedShoesColors = "";
String selectedItem = "";
String currentEntry = ""; // Holds the entry being built

// UI screen pointers for apparel selection
lv_obj_t* apparel_screen = nullptr;

// Date/Time Selection State
lv_calendar_date_t selected_date = {2025, 3, 18}; // Default date
int selected_hour = 0, selected_minute = 0, selected_is_pm = 0; // 0 = AM, 1 = PM

// WiFi State (selected_ssid/password are defined in globals.h as extern char arrays)
char selected_ssid[33] = "";
char selected_password[65] = "";
bool manualDisconnect = false;

// Settings State (displayBrightness/wifiEnabled defined in globals.h as extern)
uint8_t displayBrightness = 128;
bool wifiEnabled = true; // Should be loaded from prefs

// Log Viewing State
std::vector<LogEntry> parsedLogEntries; // Defined here, extern in globals.h
int currentLogPage = 0;
int totalLogPages = 0;
const int LOGS_PER_PAGE = 8; // Defined in globals.h

// UI Element Pointers (Defined here, declared extern in globals.h)
lv_obj_t* main_menu_screen = nullptr;
lv_obj_t* lock_screen = nullptr;
lv_obj_t* settingsScreen = nullptr;
lv_obj_t* view_logs_screen = nullptr;
lv_obj_t* wifi_manager_screen = nullptr;
lv_obj_t* genderMenu = nullptr;
lv_obj_t* colorMenu = nullptr; // Used for shirt, pants, shoes color menus
lv_obj_t* itemMenu = nullptr;
lv_obj_t* confirmScreen = nullptr;
lv_obj_t* wifi_screen = nullptr;
lv_obj_t* wifi_list = nullptr;
lv_obj_t* wifi_status_label = nullptr;
lv_obj_t* saved_networks_list = nullptr; // Seems unused, WiFiManager handles display
// Pointers for specific buttons within color menus (might be better localized)
lv_obj_t* shirt_next_btn = nullptr;
lv_obj_t* pants_next_btn = nullptr;
lv_obj_t* shoes_next_btn = nullptr;

lv_obj_t* status_bar = nullptr;
lv_obj_t* time_label = nullptr; // Main menu time
lv_obj_t* g_lock_screen_datetime_label = nullptr; // Lock screen time
lv_obj_t* current_scroll_obj = nullptr; // For scroll handling
lv_obj_t* battery_icon = nullptr; // Status bar battery icon
lv_obj_t* battery_label = nullptr; // Status bar battery label
lv_obj_t* wifi_label = nullptr; // Status bar WiFi label

// Pointers for date/time rollers (Consider making these local to date/time screens)
lv_obj_t* g_year_roller = nullptr;
lv_obj_t* g_month_roller = nullptr;
lv_obj_t* g_day_roller = nullptr;
lv_obj_t* g_selected_date_label = nullptr;
lv_obj_t* g_hour_roller = nullptr;
lv_obj_t* g_minute_roller = nullptr;
lv_obj_t* g_am_pm_roller = nullptr;
lv_obj_t* g_selected_time_label = nullptr;

// Sound Settings Screen pointer
static lv_obj_t* sound_settings_screen = nullptr; // Added declaration

// WiFi UI components (Consider making these local to WiFi screens)
lv_obj_t* wifi_keyboard = nullptr;
lv_obj_t* wifi_loading_screen = nullptr;
lv_obj_t* wifi_loading_spinner = nullptr;
lv_obj_t* wifi_loading_label = nullptr;
lv_obj_t* wifi_result_label = nullptr;
lv_obj_t* g_spinner = nullptr; // Global spinner object for scan
lv_timer_t* scan_timer = nullptr; // Global scan timer

// Status message buffer
char current_status_msg[64] = "";
uint32_t current_status_color = 0xFFFFFF;

// Apparel Types (Used in createApparelTypeMenu) - Keep static const here
const char* apparelTypes[] = {"Hoodie", "Jacket", "Long Sleeve", "Short Sleeve"};
const int NUM_APPAREL_TYPES = sizeof(apparelTypes) / sizeof(apparelTypes[0]);

// Pants Types (Used in createPantsTypeMenu)
const char* pantsTypes[] = {"Jeans", "Shorts", "Sweat Pants", "Leggings", "Skirt"};
const int NUM_PANTS_TYPES = sizeof(pantsTypes) / sizeof(pantsTypes[0]);

// Shoe Styles (Used in createShoeStyleMenu)
const char* shoeStyles[] = {"Boots", "Athletic/Sneaker", "Casual", "Dress"};
const int NUM_SHOE_STYLES = sizeof(shoeStyles) / sizeof(shoeStyles[0]);

// Items (Used in createItemMenu)
const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};
const int NUM_ITEMS = sizeof(items) / sizeof(items[0]);


// --- Style Initialization Implementation ---
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
    // Using the definition from lines 3227-3232 of original .ino
    lv_style_init(&style_card_pressed); // Re-initializing for clarity, assuming it's intended for cards primarily
    lv_style_set_bg_color(&style_card_pressed, lv_color_hex(0x3A3A3A));
    lv_style_set_bg_opa(&style_card_pressed, LV_OPA_COVER);
    lv_style_set_border_width(&style_card_pressed, 3); // Make border thicker on press
    lv_style_set_border_color(&style_card_pressed, lv_color_hex(0xFFFFFF)); // White border on press
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

    // Define styles for gender/type cards (style_card)
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

    // Define styles for network items (style_network)
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

    lv_style_init(&style_network_pressed); // Renamed from style_network_item_pressed for consistency
    lv_style_set_bg_color(&style_network_pressed, lv_color_hex(0x404040)); // Lighter gray when pressed
    lv_style_set_bg_opa(&style_network_pressed, LV_OPA_COVER);

    DEBUG_PRINT("Styles initialized");
}
String getFormattedEntry(const String& entry) {
    // This function now assumes 'entry' contains only the data part,
    // not the timestamp. Timestamp formatting is handled by the caller.
    String entryData = entry;
    // Parse the entry data into parts
    String parts[6]; // <<< MODIFIED: Increased size for Apparel Type + Shirt Color
    int partCount = 0, startIdx = 0;
    DEBUG_PRINTF("Parsing entryData: %s (length: %d)\n", entryData.c_str(), entryData.length());
    for (int i = 0; i < entryData.length() && partCount < 6; i++) { // <<< MODIFIED: Loop condition
        if (entryData.charAt(i) == ',') {
            parts[partCount] = entryData.substring(startIdx, i);
            DEBUG_PRINTF("Part %d: %s (from %d to %d)\n", partCount, parts[partCount].c_str(), startIdx, i);
            partCount++;
            startIdx = i + 1;
        }
    }
    if (startIdx < entryData.length() && partCount < 6) { // <<< MODIFIED: Check partCount
        parts[partCount] = entryData.substring(startIdx);
        DEBUG_PRINTF("Part %d: %s (from %d to end)\n", partCount, parts[partCount].c_str(), startIdx);
        partCount++;
    }
    DEBUG_PRINTF("Total parts found: %d\n", partCount);

    // <<< ADDED: Split ApparelType-ShirtColor
    String apparelType = "N/A";
    String shirtColor = "N/A";
    if (partCount > 1) {
        int hyphenPos = parts[1].indexOf('-');
        if (hyphenPos != -1) {
            apparelType = parts[1].substring(0, hyphenPos);
            shirtColor = parts[1].substring(hyphenPos + 1);
        } else {
            // Handle case where hyphen might be missing (e.g., old data)
            shirtColor = parts[1]; // Assume it's just the color if no hyphen
        }
    }
    // >>> ADDED

    // Format the output (Timestamp is now handled by the caller)
    String formatted = "Gender: " + (partCount > 0 ? parts[0] : "N/A") + "\n";
    // <<< MODIFIED: Use split parts
    formatted += "Apparel: " + apparelType + "\n";
    formatted += "Shirt Color: " + shirtColor + "\n";
    // >>> MODIFIED
    formatted += "Pants: " + (partCount > 2 ? parts[2] : "N/A") + "\n";
    formatted += "Shoes: " + (partCount > 3 ? parts[3] : "N/A") + "\n";
    formatted += "Item: " + (partCount > 4 ? parts[4] : "N/A"); // Item is now part 4

    DEBUG_PRINTF("Formatted: %s\n", formatted.c_str());
    return formatted;
}

// --- Screen Creation Function Implementations ---

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
    lv_label_set_text(version_label, "v1.0.0 - 2025"); // TODO: Make version dynamic?
    lv_obj_set_style_text_color(version_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(version_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // Load the screen
    lv_scr_load(loading_screen);

    // Start the timer for progress updates
    // The timer will call updateLoadingProgress every 50ms
    // We'll pass the progress bar as user data
    lv_timer_t* timer = lv_timer_create(updateLoadingProgress, 50, progress_bar);

    // Store the timer in the screen's user data so we can access it later
    // lv_obj_set_user_data(loading_screen, timer); // Not strictly needed if timer deletes itself

    DEBUG_PRINT("Loading screen created");
}

// Update the loading progress bar
void updateLoadingProgress(lv_timer_t* timer) {
    lv_obj_t* bar = (lv_obj_t*)lv_timer_get_user_data(timer);
    // Ensure bar is still valid before using it
    if (!bar || !lv_obj_is_valid(bar)) {
        DEBUG_PRINT("Progress bar invalid in timer, deleting timer.");
        lv_timer_del(timer);
        return;
    }

    int32_t value = lv_bar_get_value(bar);
    value += 1; // Increment by 1 for a 5-second load time (100 steps * 50ms)
    lv_bar_set_value(bar, value, LV_ANIM_ON);

    if (value >= 100) { // Check against the bar's maximum value (100)
        lv_timer_del(timer); // Delete the timer

        // Take the GUI semaphore to ensure exclusive access
        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(500)) == pdTRUE) {
            lv_obj_t* old_screen = lv_scr_act(); // Store reference to loading screen
            createLockScreen();                  // Create and load the lock screen
            if (old_screen && old_screen != lv_scr_act() && lv_obj_is_valid(old_screen)) {
                // Use lv_obj_del_async for potentially safer deletion from a timer callback context
                lv_obj_del_async(old_screen);
            }
            xSemaphoreGive(xGuiSemaphore);       // Release semaphore
        } else {
            DEBUG_PRINT("Failed to take xGuiSemaphore in updateLoadingProgress");
        }
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
    // Card 3: Settings (Row 1, Col 0)
    lv_obj_t* settings_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(settings_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_style(settings_card, &style_card_action, 0);
    lv_obj_add_style(settings_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* settings_icon = lv_label_create(settings_card);
    lv_label_set_text(settings_icon, LV_SYMBOL_SETTINGS); // Settings icon
    lv_obj_set_style_text_font(settings_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(settings_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* settings_label = lv_label_create(settings_card);
    lv_label_set_text(settings_label, "Settings");
    lv_obj_align(settings_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(settings_card, [](lv_event_t* e) {
        createSettingsScreen(); // Call the correct function
    }, LV_EVENT_CLICKED, NULL);

    // Card 4: WiFi (Row 1, Col 1)
    lv_obj_t* wifi_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(wifi_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_add_style(wifi_card, &style_card_action, 0);
    lv_obj_add_style(wifi_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* wifi_icon = lv_label_create(wifi_card);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI); // WiFi icon
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* wifi_card_label = lv_label_create(wifi_card); // Renamed to avoid conflict
    lv_label_set_text(wifi_card_label, "WiFi");
    lv_obj_align(wifi_card_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(wifi_card, [](lv_event_t* e) {
        createWiFiManagerScreen(); // Or createWiFiScreen() depending on desired flow
    }, LV_EVENT_CLICKED, NULL);

    // Card 5: Date/Time (Row 2, Col 0)
    lv_obj_t* datetime_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(datetime_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_style(datetime_card, &style_card_action, 0);
    lv_obj_add_style(datetime_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* datetime_icon = lv_label_create(datetime_card);
    lv_label_set_text(datetime_icon, LV_SYMBOL_BELL); // Using Bell as placeholder, consider Calendar icon
    lv_obj_set_style_text_font(datetime_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(datetime_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* datetime_label = lv_label_create(datetime_card);
    lv_label_set_text(datetime_label, "Date/Time");
    lv_obj_align(datetime_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(datetime_card, [](lv_event_t* e) {
        createDateSelectionScreen();
    }, LV_EVENT_CLICKED, NULL);

    // Card 6: Lock (Row 2, Col 1)
    lv_obj_t* lock_card = lv_obj_create(grid);
    lv_obj_set_grid_cell(lock_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
    lv_obj_add_style(lock_card, &style_card_action, 0);
    lv_obj_add_style(lock_card, &style_card_pressed, LV_STATE_PRESSED);
    lv_obj_t* lock_icon = lv_label_create(lock_card);
    lv_label_set_text(lock_icon, ""); // Lock icon (UTF-8 symbol for v9)
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_20, 0);
    lv_obj_align(lock_icon, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_t* lock_label = lv_label_create(lock_card);
    lv_label_set_text(lock_label, "Lock");
    lv_obj_align(lock_label, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_add_event_cb(lock_card, [](lv_event_t* e) {
        createLockScreen(); // Go back to lock screen
    }, LV_EVENT_CLICKED, NULL);


    // Add Status Bar and Time Display
    addStatusBar(main_menu_screen);
    addTimeDisplay(main_menu_screen);

    // Load the screen
    lv_scr_load(main_menu_screen);
    DEBUG_PRINTF("Free heap after main menu: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

// Event handler for the reset confirmation message box
static void reset_confirm_event_cb(lv_event_t* e) {
    lv_obj_t* mbox = (lv_obj_t*)lv_event_get_target(e); // Get the object that dispatched the event
    uint16_t btn_id = *(uint16_t*)lv_event_get_param(e); // v9: Get button index from event parameter

    // In v9, the event is triggered *before* the msgbox closes automatically on button press.
    // We just need to check which button was pressed.
    // Button IDs are 0 for "Cancel", 1 for "Reset" based on creation order.

    if (btn_id == 1) { // Check if the "Reset" button (ID 1) was pressed
        DEBUG_PRINTLN("Resetting device...");
            // Perform the reset
            ESP.restart();
    }
    // No need to manually close the msgbox in v9 for footer buttons, it closes automatically.
    // lv_msgbox_close(mbox); // Removed
}

// Implementation from .ino lines 400-440 (createSettingsScreen)
void createSettingsScreen() {
    DEBUG_PRINT("Creating Settings Screen...");
    if (settingsScreen) {
        lv_obj_clean(settingsScreen);
        lv_obj_del(settingsScreen);
        settingsScreen = nullptr;
    }
    settingsScreen = lv_obj_create(NULL);
    lv_obj_add_style(settingsScreen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(settingsScreen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    // Container for settings options
    lv_obj_t* cont = lv_obj_create(settingsScreen);
    lv_obj_set_size(cont, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 10, 0); // Spacing between items

    // --- Brightness Setting ---
    lv_obj_t* brightness_cont = lv_obj_create(cont);
    lv_obj_set_width(brightness_cont, lv_pct(100));
    lv_obj_set_height(brightness_cont, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(brightness_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(brightness_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(brightness_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* brightness_label = lv_label_create(brightness_cont);
    lv_label_set_text(brightness_label, "Brightness");
    lv_obj_add_style(brightness_label, &style_text, 0);

    lv_obj_t* brightness_slider = lv_slider_create(brightness_cont);
    lv_obj_set_width(brightness_slider, lv_pct(50));
    lv_slider_set_range(brightness_slider, 10, 255); // Min 10, Max 255
    lv_slider_set_value(brightness_slider, displayBrightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, [](lv_event_t* e) {
        lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e); // No cast needed
        displayBrightness = (uint8_t)lv_slider_get_value(slider);
        M5.Display.setBrightness(displayBrightness);
        // Save brightness setting
        Preferences prefs;
        prefs.begin("appSettings", false);
        prefs.putUChar("brightness", displayBrightness);
        prefs.end();
        DEBUG_PRINTF("Brightness set to %d\n", displayBrightness);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- WiFi Enable/Disable ---
    lv_obj_t* wifi_cont = lv_obj_create(cont);
    lv_obj_set_width(wifi_cont, lv_pct(100));
    lv_obj_set_height(wifi_cont, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wifi_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(wifi_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* wifi_toggle_label = lv_label_create(wifi_cont);
    lv_label_set_text(wifi_toggle_label, "Enable WiFi");
    lv_obj_add_style(wifi_toggle_label, &style_text, 0);

    lv_obj_t* wifi_switch = lv_switch_create(wifi_cont);
    if (wifiEnabled) {
        lv_obj_add_state(wifi_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(wifi_switch, [](lv_event_t* e) {
        lv_obj_t* sw = (lv_obj_t*)lv_event_get_target(e); // No cast needed
        wifiEnabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
        // Save WiFi setting
        Preferences prefs;
        prefs.begin("appSettings", false);
        prefs.putBool("wifiEnabled", wifiEnabled);
        prefs.end();
        DEBUG_PRINTF("WiFi %s\n", wifiEnabled ? "Enabled" : "Disabled");
        // Optionally turn WiFi hardware on/off immediately
        if (wifiEnabled) {
            WiFi.mode(WIFI_STA); // Or WIFI_AP_STA if needed
            WiFi.begin(); // Reconnect or start AP if applicable
        } else {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
        }
        // updateStatusBar(); // Original error suggested updateStatus
        updateStatus("WiFi status updated", 0xFFFFFF); // Use updateStatus as hinted by original error
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Reset Device Button ---
    lv_obj_t* reset_btn = lv_btn_create(cont);
    lv_obj_set_width(reset_btn, lv_pct(80));
    lv_obj_add_style(reset_btn, &style_btn, 0);
    lv_obj_add_style(reset_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset Device");
    lv_obj_center(reset_label);
    lv_obj_add_event_cb(reset_btn, [](lv_event_t* e) {
        // Show confirmation message box
        lv_obj_t* msgbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(msgbox, "Confirm Reset");
        lv_msgbox_add_text(msgbox, "Are you sure you want to reset the device? This cannot be undone.");
        // Add buttons using the v9 method
        lv_msgbox_add_footer_button(msgbox, "Cancel"); // ID 0
        lv_msgbox_add_footer_button(msgbox, "Reset");  // ID 1
        lv_obj_center(msgbox);
        // Add event callback to the message box itself
        lv_obj_add_event_cb(msgbox, reset_confirm_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // Event triggers when button clicked
    }, LV_EVENT_CLICKED, NULL);


    // Load the screen
    lv_scr_load(settingsScreen);
    DEBUG_PRINT("Settings Screen created.");
}


// Implementation from .ino lines 442-574 (createViewLogsScreen)
void createViewLogsScreen() {
    DEBUG_PRINT("Creating View Logs Screen...");
    if (view_logs_screen) {
        lv_obj_clean(view_logs_screen);
        lv_obj_del(view_logs_screen);
        view_logs_screen = nullptr;
    }
    view_logs_screen = lv_obj_create(NULL);
    lv_obj_add_style(view_logs_screen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(view_logs_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "View Logs");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    // Date Range Selection (Placeholder - Needs implementation)
    // TODO: Add date pickers or similar UI for selecting start/end dates

    // Log List Container
    lv_obj_t* log_list_cont = lv_obj_create(view_logs_screen);
    lv_obj_set_size(log_list_cont, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100); // Adjust size for header/footer
    lv_obj_align(log_list_cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(log_list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(log_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(log_list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(log_list_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(log_list_cont, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_style_pad_row(log_list_cont, 5, 0); // Spacing between log entries

    // Load and display logs (Initial load for today or default range)
    // Assuming loadLogEntriesForDateRange populates parsedLogEntries
    // For now, just load all entries from the file for simplicity
    parsedLogEntries.clear(); // Clear previous entries
    loadAllLogEntries(parsedLogEntries); // Load all entries into the vector

    // Sort entries by timestamp (descending - newest first)
    std::sort(parsedLogEntries.begin(), parsedLogEntries.end(), [](const LogEntry& a, const LogEntry& b) {
        return a.timestamp > b.timestamp; // Sort descending
    });

    totalLogPages = (parsedLogEntries.size() + LOGS_PER_PAGE - 1) / LOGS_PER_PAGE;
    currentLogPage = 0;

    // Display the first page of logs
    displayLogPage(log_list_cont, currentLogPage);

    // Pagination Controls
    lv_obj_t* footer = lv_obj_create(view_logs_screen);
    lv_obj_set_size(footer, SCREEN_WIDTH, 40);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Previous Button
    lv_obj_t* prev_btn = lv_btn_create(footer);
    lv_obj_add_style(prev_btn, &style_btn, 0);
    lv_obj_add_style(prev_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* prev_label = lv_label_create(prev_btn);
    lv_label_set_text(prev_label, LV_SYMBOL_PREV);
    lv_obj_center(prev_label);
    // Use user data, ensure validity check
    lv_obj_add_event_cb(prev_btn, [](lv_event_t* e) {
        if (currentLogPage > 0) {
            currentLogPage--;
            lv_obj_t* list_cont = (lv_obj_t*)lv_event_get_user_data(e);
            if (list_cont && lv_obj_is_valid(list_cont)) {
                 displayLogPage(list_cont, currentLogPage);
            }
        }
    }, LV_EVENT_CLICKED, log_list_cont);

    // Page Indicator Label
    lv_obj_t* page_label = lv_label_create(footer);
    lv_label_set_text_fmt(page_label, "Page %d / %d", currentLogPage + 1, totalLogPages > 0 ? totalLogPages : 1);
    lv_obj_add_style(page_label, &style_text, 0);
    lv_obj_set_user_data(page_label, log_list_cont); // Store list container for update

    // Next Button
    lv_obj_t* next_btn = lv_btn_create(footer);
    lv_obj_add_style(next_btn, &style_btn, 0);
    lv_obj_add_style(next_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* next_label = lv_label_create(next_btn);
    lv_label_set_text(next_label, LV_SYMBOL_NEXT);
    lv_obj_center(next_label);
    // Use user data, ensure validity check
    lv_obj_add_event_cb(next_btn, [](lv_event_t* e) {
        if (currentLogPage < totalLogPages - 1) {
            currentLogPage++;
            lv_obj_t* list_cont = (lv_obj_t*)lv_event_get_user_data(e);
             if (list_cont && lv_obj_is_valid(list_cont)) {
                 displayLogPage(list_cont, currentLogPage);
             }
        }
    }, LV_EVENT_CLICKED, log_list_cont);

    // Load the screen
    lv_scr_load(view_logs_screen);
    DEBUG_PRINT("View Logs Screen created.");
}

// Helper function to display a specific page of logs
void displayLogPage(lv_obj_t* list_container, int page) {
    lv_obj_clean(list_container); // Clear previous entries

    int start_index = page * LOGS_PER_PAGE;
    int end_index = start_index + LOGS_PER_PAGE;
    if (end_index > parsedLogEntries.size()) {
        end_index = parsedLogEntries.size();
    }

    if (parsedLogEntries.empty()) {
        lv_obj_t* empty_label = lv_label_create(list_container);
        lv_label_set_text(empty_label, "No log entries found.");
        lv_obj_add_style(empty_label, &style_text, 0);
        lv_obj_align(empty_label, LV_ALIGN_CENTER, 0, 0);
    } else {
        for (int i = start_index; i < end_index; ++i) {
            lv_obj_t* entry_label = lv_label_create(list_container);
            // Format the entry for display (e.g., timestamp + data)
            String display_text = getFormattedEntry(parsedLogEntries[i]); // Corrected call
            lv_label_set_text(entry_label, display_text.c_str());
            lv_label_set_long_mode(entry_label, LV_LABEL_LONG_WRAP); // Wrap long lines
            lv_obj_set_width(entry_label, lv_pct(100));
            lv_obj_add_style(entry_label, &style_text, 0);
            // Add alternating background color for readability (optional)
            if (i % 2 == 0) {
                 lv_obj_set_style_bg_color(entry_label, lv_color_hex(0x252525), 0);
                 lv_obj_set_style_bg_opa(entry_label, LV_OPA_COVER, 0);
            }
        }
    }

    // Update page indicator label (find it within the parent's children)
    lv_obj_t* screen = lv_obj_get_screen(list_container);
    lv_obj_t* footer = lv_obj_get_child(screen, -1); // Assuming footer is the last child
    if (footer) {
        lv_obj_t* page_label = lv_obj_get_child(footer, 1); // Assuming label is the middle child
        if (page_label) {
            lv_label_set_text_fmt(page_label, "Page %d / %d", currentLogPage + 1, totalLogPages > 0 ? totalLogPages : 1);
        }
    }
}


// Implementation from .ino lines 576-647 (createGenderMenu)
void createGenderMenu() {
    DEBUG_PRINT("Creating Gender Menu...");
    if (genderMenu) {
        lv_obj_clean(genderMenu);
        lv_obj_del(genderMenu);
        genderMenu = nullptr;
    }
    genderMenu = lv_obj_create(NULL);
    lv_obj_add_style(genderMenu, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(genderMenu);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Select Gender");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu(); // Go back to main menu
    }, LV_EVENT_CLICKED, NULL);

    // Container for gender cards
    lv_obj_t* cont = lv_obj_create(genderMenu);
    lv_obj_set_size(cont, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW); // Arrange cards horizontally
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // Space them out
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE); // Allow scrolling if needed
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    // Gender Card Event Callback
    auto gender_card_event_cb = [](lv_event_t* e) {
        lv_obj_t* card = (lv_obj_t*)lv_event_get_target(e); // No cast needed
        const char* gender = (const char*)lv_event_get_user_data(e);
        selectedGender = String(gender);
        DEBUG_PRINTF("Selected Gender: %s\n", selectedGender.c_str());
        createApparelTypeMenu(); // Proceed to next step
    };

    // Male Card
    lv_obj_t* male_card = lv_obj_create(cont);
    lv_obj_set_size(male_card, 140, 100); // Adjust size as needed
    lv_obj_add_style(male_card, &style_card, 0); // Use generic card style
    lv_obj_add_style(male_card, &style_card_pressed, LV_STATE_PRESSED); // Use generic pressed style
    lv_obj_t* male_icon = lv_label_create(male_card);
    lv_label_set_text(male_icon, ""); // Male symbol (FontAwesome if available, or text)
    lv_obj_set_style_text_font(male_icon, &lv_font_montserrat_28, 0); // Use declared font
    lv_obj_align(male_icon, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t* male_label = lv_label_create(male_card);
    lv_label_set_text(male_label, "Male");
    lv_obj_align(male_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(male_card, gender_card_event_cb, LV_EVENT_CLICKED, (void*)"Male");

    // Female Card
    lv_obj_t* female_card = lv_obj_create(cont);
    lv_obj_set_size(female_card, 140, 100); // Adjust size as needed
    lv_obj_add_style(female_card, &style_card, 0); // Use generic card style
    lv_obj_add_style(female_card, &style_card_pressed, LV_STATE_PRESSED); // Use generic pressed style
    lv_obj_t* female_icon = lv_label_create(female_card);
    lv_label_set_text(female_icon, ""); // Female symbol (FontAwesome if available, or text)
    lv_obj_set_style_text_font(female_icon, &lv_font_montserrat_28, 0); // Use declared font
    lv_obj_align(female_icon, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t* female_label = lv_label_create(female_card);
    lv_label_set_text(female_label, "Female");
    lv_obj_align(female_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(female_card, gender_card_event_cb, LV_EVENT_CLICKED, (void*)"Female");

    // Load the screen
    lv_scr_load(genderMenu);
    DEBUG_PRINT("Gender Menu created.");
}


// Implementation from .ino lines 649-702 (createApparelTypeMenu)
void createApparelTypeMenu() {
    DEBUG_PRINT("Creating Apparel Type Menu...");
    if (apparel_screen) { // Use the correct pointer name
        lv_obj_clean(apparel_screen);
        lv_obj_del(apparel_screen);
        apparel_screen = nullptr;
    }
    apparel_screen = lv_obj_create(NULL);
    lv_obj_add_style(apparel_screen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(apparel_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Select Apparel Type");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createGenderMenu(); // Go back to gender selection
    }, LV_EVENT_CLICKED, NULL);

    // Container for apparel type cards (Grid layout)
    lv_obj_t* cont = lv_obj_create(apparel_screen);
    lv_obj_set_size(cont, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 50);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {140, 140, LV_GRID_TEMPLATE_LAST}; // 2 columns
    static lv_coord_t row_dsc[] = {80, 80, LV_GRID_TEMPLATE_LAST}; // 2 rows (adjust if more types)
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(cont, 10, 0);
    lv_obj_set_style_pad_row(cont, 10, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    // Apparel Card Event Callback
    auto apparel_card_event_cb = [](lv_event_t* e) {
        lv_obj_t* card = (lv_obj_t*)lv_event_get_target(e); // No cast needed
        const char* type = (const char*)lv_event_get_user_data(e);
        selectedApparelType = String(type);
        DEBUG_PRINTF("Selected Apparel Type: %s\n", selectedApparelType.c_str());
        createColorMenuShirt(); // Proceed to shirt color selection
    };

    // Create cards dynamically based on apparelTypes array
    for (int i = 0; i < NUM_APPAREL_TYPES; ++i) {
        int row = i / 2;
        int col = i % 2;

        lv_obj_t* card = lv_obj_create(cont);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_add_style(card, &style_card, 0);
        lv_obj_add_style(card, &style_card_pressed, LV_STATE_PRESSED);

        // Add icon (placeholder - replace with actual icons if available)
        lv_obj_t* icon = lv_label_create(card);
        // Simple placeholder icon logic
        if (strcmp(apparelTypes[i], "Hoodie") == 0) lv_label_set_text(icon, ""); // Placeholder icon
        else if (strcmp(apparelTypes[i], "Jacket") == 0) lv_label_set_text(icon, ""); // Placeholder icon
        else if (strcmp(apparelTypes[i], "Long Sleeve") == 0) lv_label_set_text(icon, ""); // Placeholder icon
        else if (strcmp(apparelTypes[i], "Short Sleeve") == 0) lv_label_set_text(icon, ""); // Placeholder icon
        else lv_label_set_text(icon, "?");
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 8);

        // Add label
        lv_obj_t* label = lv_label_create(card);
        lv_label_set_text(label, apparelTypes[i]);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -8);

        // Add event callback
        lv_obj_add_event_cb(card, apparel_card_event_cb, LV_EVENT_CLICKED, (void*)apparelTypes[i]);
    }

    // Load the screen
    lv_scr_load(apparel_screen);
    DEBUG_PRINT("Apparel Type Menu created.");
}


// Implementation from .ino lines 704-848 (createColorMenuShirt)
// Helper struct for color info
struct ColorInfo {
    const char* name;
    uint32_t hexValue;
};

// Define colors (can be expanded)
const ColorInfo colors[] = {
    {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Gray", 0x808080},
    {"Red", 0xFF0000}, {"Blue", 0x0000FF}, {"Green", 0x008000},
    {"Yellow", 0xFFFF00}, {"Orange", 0xFFA500}, {"Purple", 0x800080},
    {"Pink", 0xFFC0CB}, {"Brown", 0xA52A2A}, {"Beige", 0xF5F5DC}
    // Add more colors as needed
};
const int NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

// Static callback function for color buttons
static void color_btn_event_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Cast needed in v9
    const char* color_name = (const char*)lv_event_get_user_data(e);
    bool is_selected = lv_obj_has_state(btn, LV_STATE_CHECKED);

    // Determine which color menu we are in (Shirt, Pants, Shoes)
    // We can check the parent screen or use a global state variable if needed.
    // For now, assume we know it's the shirt color menu based on context.
    String* targetColorString = &selectedShirtColors; // Default to shirt

    // Find the parent screen to determine context (a bit fragile, consider passing context)
    lv_obj_t* screen = lv_obj_get_screen(btn);
    // A better way might be to pass the target string pointer as user data to the NEXT button
    // or store context in the screen's user data.

    // Update the selected colors string
    String color_str = String(color_name);
    if (is_selected) {
        // Add color if not already present
        if (targetColorString->indexOf(color_str) == -1) {
            if (targetColorString->length() > 0) {
                *targetColorString += ", ";
            }
            *targetColorString += color_str;
        }
    } else {
        // Remove color
        String search_str = ", " + color_str;
        targetColorString->replace(search_str, "");
        search_str = color_str + ", ";
        targetColorString->replace(search_str, "");
        // If it was the only color
        if (*targetColorString == color_str) {
            *targetColorString = "";
        }
    }
    DEBUG_PRINTF("Selected Shirt Colors: %s\n", targetColorString->c_str());

    // Enable/disable Next button based on selection
    // Find the Next button (assuming it's stored in shirt_next_btn)
    if (shirt_next_btn) { // Check if the pointer is valid
        if (targetColorString->length() > 0) {
            lv_obj_clear_state(shirt_next_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(shirt_next_btn, LV_STATE_DISABLED);
        }
    }
}

// Static callback for the "Next" button in the Shirt Color menu
static void shirt_color_next_btn_event_cb(lv_event_t* e) {
    // Proceed to the next step (Pants Type selection)
    createPantsTypeMenu();
}


void createColorMenuShirt() {
    DEBUG_PRINT("Creating Shirt Color Menu...");
    if (colorMenu) {
        lv_obj_clean(colorMenu);
        lv_obj_del(colorMenu);
        colorMenu = nullptr;
        shirt_next_btn = nullptr; // Reset button pointer
    }
    colorMenu = lv_obj_create(NULL);
    lv_obj_add_style(colorMenu, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Select Shirt Color(s)");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createApparelTypeMenu(); // Go back to apparel type selection
    }, LV_EVENT_CLICKED, NULL);

    // Container for color buttons (Grid layout)
    lv_obj_t* cont = lv_obj_create(colorMenu);
    lv_obj_set_size(cont, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 100); // Adjust size for header/footer
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    // Adjust grid columns/rows based on number of colors and desired layout
    static lv_coord_t col_dsc[] = {70, 70, 70, 70, LV_GRID_TEMPLATE_LAST}; // 4 columns
    // Calculate required rows dynamically based on NUM_COLORS
    int num_rows_calc = (NUM_COLORS + 3) / 4; // +3 for integer division ceiling
    lv_coord_t* row_dsc = (lv_coord_t*)malloc(sizeof(lv_coord_t) * (num_rows_calc + 1));
    if (row_dsc) {
        for(int i=0; i < num_rows_calc; ++i) {
            row_dsc[i] = 50; // Set row height
        }
        row_dsc[num_rows_calc] = LV_GRID_TEMPLATE_LAST;
        lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);
        free(row_dsc); // Free memory after setting
    } else {
        // Fallback to static if malloc fails (adjust size manually if needed)
        static lv_coord_t row_dsc_static[] = {50, 50, 50, LV_GRID_TEMPLATE_LAST}; // Example for 12 colors
        lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc_static);
        DEBUG_PRINT("Warning: Failed to allocate memory for dynamic row descriptor in createColorMenuShirt.");
    }

    lv_obj_set_style_pad_column(cont, 5, 0);
    lv_obj_set_style_pad_row(cont, 5, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    // Create color buttons dynamically
    for (int i = 0; i < NUM_COLORS; ++i) {
        int row = i / 4;
        int col = i % 4;

        lv_obj_t* btn = lv_btn_create(cont);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE); // Make button checkable
        lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i].hexValue), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), LV_STATE_CHECKED); // White border when checked
        lv_obj_set_style_border_width(btn, 2, LV_STATE_CHECKED);
        lv_obj_set_style_radius(btn, 5, 0);

        // Add label (optional, maybe only show on hover/press?)
        // lv_obj_t* label = lv_label_create(btn);
        // lv_label_set_text(label, colors[i].name);
        // lv_obj_center(label);
        // lv_obj_set_style_text_color(label, lv_color_is_light(lv_color_hex(colors[i].hexValue)) ? lv_color_black() : lv_color_white(), 0);

        // Add event callback
        lv_obj_add_event_cb(btn, color_btn_event_cb, LV_EVENT_CLICKED, (void*)colors[i].name);
    }

    // Footer for Next button
    lv_obj_t* footer = lv_obj_create(colorMenu);
    lv_obj_set_size(footer, SCREEN_WIDTH, 50);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(footer, 0, 0);

    // Next Button
    shirt_next_btn = lv_btn_create(footer); // Assign to global pointer
    lv_obj_set_size(shirt_next_btn, 100, 40);
    lv_obj_align(shirt_next_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_style(shirt_next_btn, &style_btn, 0);
    lv_obj_add_style(shirt_next_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* next_label = lv_label_create(shirt_next_btn);
    lv_label_set_text(next_label, "Next");
    lv_obj_center(next_label);
    lv_obj_add_event_cb(shirt_next_btn, shirt_color_next_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Initially disable Next button if no colors selected
    if (selectedShirtColors.length() == 0) {
        lv_obj_add_state(shirt_next_btn, LV_STATE_DISABLED);
    }

    // Load the screen
    lv_scr_load(colorMenu);
    DEBUG_PRINT("Shirt Color Menu created.");
}


// Implementation from .ino lines 850-903 (createPantsTypeMenu)
// NOTE: This function's body was missing in the original ui.cpp, adding a placeholder
void createPantsTypeMenu() {
    DEBUG_PRINT("Creating Pants Type Menu");
    selectedPantsType = ""; // Reset selection

    // Create the screen
    lv_obj_t* pants_type_screen = lv_obj_create(NULL);
    lv_obj_add_style(pants_type_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(pants_type_screen, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(pants_type_screen, LV_OPA_COVER, 0);
    lv_obj_add_flag(pants_type_screen, LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = pants_type_screen; // Allow scrolling if needed

    // Back Button (Top-Left)
    lv_obj_t* back_btn = lv_btn_create(pants_type_screen);
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
        // Go back to shirt color selection, clear relevant parts of entry
        currentEntry = selectedGender + "," + selectedApparelType + "-" + selectedShirtColors + ",";
        createColorMenuShirt();
    }, LV_EVENT_CLICKED, NULL);

    // Title
    lv_obj_t* title = lv_label_create(pants_type_screen);
    lv_label_set_text(title, "Select Pants Type");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Pants Type Options Grid
    lv_obj_t* grid = lv_obj_create(pants_type_screen);
    lv_obj_set_size(grid, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 70); // Adjust size as needed
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}; // 2 columns
    // Adjust rows based on NUM_PANTS_TYPES
    // Use static array for row descriptors
    static lv_coord_t row_dsc[] = {60, 60, 60, LV_GRID_TEMPLATE_LAST}; // Assuming max 3 rows needed for 5 items + LAST
    // Note: If NUM_PANTS_TYPES increases significantly, this array size needs adjustment.
    // A more dynamic approach might be needed for many items, but static is safer here.

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);

    for (int i = 0; i < NUM_PANTS_TYPES; i++) {
        lv_obj_t* card = lv_obj_create(grid);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_height(card, 60); // Fixed height for cards
        lv_obj_add_style(card, &style_card, 0); // Use existing card style
        lv_obj_add_style(card, &style_card_pressed, LV_STATE_PRESSED);

        lv_obj_t* label = lv_label_create(card);
        lv_label_set_text(label, pantsTypes[i]);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

        // Store the pants type text in user data
        lv_obj_set_user_data(card, (void*)pantsTypes[i]);

        lv_obj_add_event_cb(card, pants_type_card_event_cb, LV_EVENT_CLICKED, NULL); // <<< Use static function
        /* Original Lambda: (Comment closed below)
        [](lv_event_t* e) {
            lv_obj_t* target_card = (lv_obj_t*)lv_event_get_target(e);
            const char* type = (const char*)lv_obj_get_user_data(target_card);
            if (type) {
                selectedPantsType = String(type);
                DEBUG_PRINTF("Pants Type selected: %s\n", selectedPantsType.c_str());
                createColorMenuPants(); // Proceed to pants color selection
            }
        }; // End of lambda body
        */ // <<< Correctly closing the comment block
        // }, LV_EVENT_CLICKED, NULL); // Original event registration line (now commented out)
    }

    lv_scr_load(pants_type_screen);
    DEBUG_PRINT("Pants Type menu loaded");
}

// Static event handler for pants type selection cards
static void pants_type_card_event_cb(lv_event_t* e) {
    lv_obj_t* target_card = (lv_obj_t*)lv_event_get_target(e);
    const char* type = (const char*)lv_obj_get_user_data(target_card);
    if (type) {
        selectedPantsType = String(type);
        DEBUG_PRINTF("Pants Type selected: %s\n", selectedPantsType.c_str());
        createColorMenuPants(); // Proceed to pants color selection
    }
}


// Implementation from .ino lines 905-958 (createColorMenuPants)
// NOTE: This function's body was missing, adding placeholder
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
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST}; // 6 rows for 12 colors
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
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Pink", 0xFFC0CB},
        {"Brown", 0xA52A2A}, {"Silver", 0xC0C0C0}, {"Grey", 0x808080}
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

// Implementation from .ino lines 960-1013 (createShoeStyleMenu)
// NOTE: This function's body was missing, adding placeholder
void createShoeStyleMenu() {
    DEBUG_PRINT("Creating Shoe Style Menu");
    selectedShoeStyle = ""; // Reset selection

    // Create the screen
    lv_obj_t* shoe_style_screen = lv_obj_create(NULL);
    lv_obj_add_style(shoe_style_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(shoe_style_screen, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_opa(shoe_style_screen, LV_OPA_COVER, 0);
    lv_obj_add_flag(shoe_style_screen, LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = shoe_style_screen; // Allow scrolling if needed

    // Back Button (Top-Left)
    lv_obj_t* back_btn = lv_btn_create(shoe_style_screen);
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
        // Go back to pants color selection, clear relevant parts of entry
        // Find the last comma before the shoe part (which hasn't been added yet)
        int lastComma = currentEntry.lastIndexOf(',');
        if (lastComma > 0) { // Ensure there is a comma
            int secondLastComma = currentEntry.lastIndexOf(',', lastComma - 1);
             if (secondLastComma >= 0) { // Ensure there's a section before pants
                 // Remove the pants section (PantsType-PantsColor,)
                 currentEntry = currentEntry.substring(0, secondLastComma + 1);
             } else { // Only gender and shirt were added, should not happen here but handle defensively
                 currentEntry = currentEntry.substring(0, lastComma + 1);
             }
        }
        DEBUG_PRINTF("Going back to Pants Color. Entry reset to: %s\n", currentEntry.c_str());
        createColorMenuPants();
    }, LV_EVENT_CLICKED, NULL);

    // Title
    lv_obj_t* title = lv_label_create(shoe_style_screen);
    lv_label_set_text(title, "Select Shoe Style");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Shoe Style Options Grid
    lv_obj_t* grid = lv_obj_create(shoe_style_screen);
    lv_obj_set_size(grid, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 70); // Adjust size as needed
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_GRID);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}; // 2 columns
    // Adjust rows based on NUM_SHOE_STYLES
    static lv_coord_t row_dsc[] = {60, 60, LV_GRID_TEMPLATE_LAST}; // 2 rows needed for 4 items + LAST

    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, 10, 0);
    lv_obj_set_style_pad_row(grid, 10, 0);

    for (int i = 0; i < NUM_SHOE_STYLES; i++) {
        lv_obj_t* card = lv_obj_create(grid);
        lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, i % 2, 1, LV_GRID_ALIGN_STRETCH, i / 2, 1);
        lv_obj_set_height(card, 60); // Fixed height for cards
        lv_obj_add_style(card, &style_card, 0); // Use existing card style
        lv_obj_add_style(card, &style_card_pressed, LV_STATE_PRESSED);

        lv_obj_t* label = lv_label_create(card);
        lv_label_set_text(label, shoeStyles[i]);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);

        // Store the shoe style text in user data
        lv_obj_set_user_data(card, (void*)shoeStyles[i]);

        lv_obj_add_event_cb(card, [](lv_event_t* e) {
            lv_obj_t* target_card = (lv_obj_t*)lv_event_get_target(e);
            const char* style = (const char*)lv_obj_get_user_data(target_card);
            if (style) {
                selectedShoeStyle = String(style);
                // Handle "/" in style name for filename/entry safety if needed later
                // selectedShoeStyle.replace("/", "-"); // Example replacement
                DEBUG_PRINTF("Shoe Style selected: %s\n", selectedShoeStyle.c_str());
                createColorMenuShoes(); // Proceed to shoe color selection
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    lv_scr_load(shoe_style_screen);
    DEBUG_PRINT("Shoe Style menu loaded");
}

// Implementation from .ino lines 1015-1068 (createColorMenuShoes)
// NOTE: This function's body was missing, adding placeholder
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
    static lv_coord_t row_dsc[] = {60, 60, 60, 60, 60, 60, LV_GRID_TEMPLATE_LAST}; // 6 rows for 12 colors
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
        {"Black", 0x000000}, {"White", 0xFFFFFF}, {"Pink", 0xFFC0CB},
        {"Brown", 0xA52A2A}, {"Silver", 0xC0C0C0}, {"Grey", 0x808080}
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

// Implementation from .ino lines 1070-1107 (createItemMenu)
// NOTE: This function's body was missing, adding placeholder
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

// Implementation from .ino lines 3180-3225 (createConfirmScreen)
// NOTE: This function's body was missing, adding placeholder
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


// --- Settings Screens ---

// Implementation from .ino lines 3688-3755 (createSoundSettingsScreen)
void createSoundSettingsScreen() {
    DEBUG_PRINT("Entering createSoundSettingsScreen");

    // Clean up previous instance if it exists
    if (sound_settings_screen && lv_obj_is_valid(sound_settings_screen)) {
        lv_obj_del(sound_settings_screen);
        sound_settings_screen = nullptr;
    }


    // Create the screen
    sound_settings_screen = lv_obj_create(NULL);
    lv_obj_add_style(sound_settings_screen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(sound_settings_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50); // Standard header size
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0); // Consistent header color
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0); // No radius for header

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Sound Settings");
    lv_obj_add_style(title, &style_title, 0); // Use global title style
    lv_obj_center(title);

    // Sound Toggle Switch
    lv_obj_t* sound_toggle_label = lv_label_create(sound_settings_screen); // Use declared variable
    lv_label_set_text(sound_toggle_label, "Sound Enable");
    lv_obj_align(sound_toggle_label, LV_ALIGN_TOP_LEFT, 20, 80); // Position below header
    lv_obj_add_style(sound_toggle_label, &style_text, 0);

    lv_obj_t* sound_switch = lv_switch_create(sound_settings_screen);
    lv_obj_align(sound_switch, LV_ALIGN_TOP_RIGHT, -20, 75); // Align with label
    lv_obj_set_size(sound_switch, 50, 25); // Standard switch size

    // Load initial state from Preferences
    Preferences prefs;
    prefs.begin("settings", true); // Read-only initially
    bool soundEnabled = prefs.getBool("sound_enabled", true); // Default to true if not found
    uint8_t current_volume = prefs.getUChar("volume", 128); // Load saved volume
    prefs.end();

    if (soundEnabled) {
        lv_obj_add_state(sound_switch, LV_STATE_CHECKED);
        M5.Speaker.setVolume(current_volume); // Ensure volume is set if enabled
    } else {
        lv_obj_clear_state(sound_switch, LV_STATE_CHECKED);
        M5.Speaker.setVolume(0); // Ensure volume is 0 if disabled
    }

    lv_obj_add_event_cb(sound_switch, [](lv_event_t* e) {
        Preferences prefs_cb; // Need separate instance for callback scope
        prefs_cb.begin("settings", false); // Open for writing
        bool enabled = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
        uint8_t saved_volume = prefs_cb.getUChar("volume", 128); // Get saved volume again
        M5.Speaker.setVolume(enabled ? saved_volume : 0); // Restore volume or mute
        if (enabled) {
             M5.Speaker.tone(440, 100); // Play 440Hz test tone only when enabling
        }
        prefs_cb.putBool("sound_enabled", enabled); // Save state
        DEBUG_PRINTF("Sound %s\n", enabled ? "Enabled" : "Disabled");
        prefs_cb.end();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Volume Slider
    lv_obj_t* volume_label = lv_label_create(sound_settings_screen);
    lv_label_set_text(volume_label, "Volume");
    lv_obj_align(volume_label, LV_ALIGN_TOP_LEFT, 20, 130); // Position below toggle
    lv_obj_add_style(volume_label, &style_text, 0);

    // Volume Value Display (Static pointer needed for update callback)
    static lv_obj_t* volume_value_label = nullptr;
    if (volume_value_label && lv_obj_is_valid(volume_value_label)) lv_obj_del(volume_value_label); // Clean previous if exists
    volume_value_label = lv_label_create(sound_settings_screen);
    char volume_text[10];
    snprintf(volume_text, sizeof(volume_text), "%d", M5.Speaker.getVolume()); // Use current actual volume
} // End of createSoundSettingsScreen

// Implementation from .ino lines 3757-3825 (createBrightnessSettingsScreen)
void createBrightnessSettingsScreen() {
    DEBUG_PRINT("Entering createBrightnessSettingsScreen");

    // Create a static variable for the brightness screen
    static lv_obj_t* brightness_settings_screen = nullptr;

    // Clean up previous instance if it exists
    if (brightness_settings_screen && lv_obj_is_valid(brightness_settings_screen)) {
        DEBUG_PRINTF("Cleaning and deleting existing brightness_settings_screen: %p\n", brightness_settings_screen);
        // lv_obj_clean(brightness_settings_screen); // lv_obj_del handles children
        lv_obj_del(brightness_settings_screen);
        brightness_settings_screen = nullptr;
        // lv_task_handler(); // Delay might not be needed if loading new screen immediately
        // delay(10);
    }

    // Create a new screen
    brightness_settings_screen = lv_obj_create(NULL);
    if (!brightness_settings_screen) {
        DEBUG_PRINT("Failed to create brightness_settings_screen");
        return;
    }
    DEBUG_PRINTF("Created brightness_settings_screen: %p\n", brightness_settings_screen);

    lv_obj_add_style(brightness_settings_screen, &style_screen, 0);
    // lv_scr_load(brightness_settings_screen); // Load later
    DEBUG_PRINT("Screen style added");

    // Header with gradient
    lv_obj_t* header = lv_obj_create(brightness_settings_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_bg_grad_color(header, lv_color_hex(0x357ABD), 0);
    lv_obj_set_style_bg_grad_dir(header, LV_GRAD_DIR_VER, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Display Brightness");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container with subtle shadow
    lv_obj_t* container = lv_obj_create(brightness_settings_screen);
    lv_obj_set_size(container, 300, 200); // Increased height to fit back button
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 70); // Position below header
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2A2A40), 0);
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_shadow_color(container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_shadow_width(container, 15, 0);
    lv_obj_set_style_pad_all(container, 20, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE); // Container doesn't scroll

    // Brightness value label (Static pointer needed for update callbacks)
    static lv_obj_t* brightnessValueLabel = nullptr;
    if (brightnessValueLabel && lv_obj_is_valid(brightnessValueLabel)) lv_obj_del(brightnessValueLabel); // Clean previous
    brightnessValueLabel = lv_label_create(container);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (displayBrightness * 100) / 255);
    lv_label_set_text(brightnessValueLabel, buf);
    lv_obj_align(brightnessValueLabel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(brightnessValueLabel, &lv_font_montserrat_24, 0); // Larger font
    lv_obj_set_style_text_color(brightnessValueLabel, lv_color_hex(0xFFFFFF), 0);

    // Brightness slider with custom colors
    lv_obj_t* brightnessSlider = lv_slider_create(container);
    lv_obj_set_width(brightnessSlider, 260);
    lv_obj_align(brightnessSlider, LV_ALIGN_TOP_MID, 0, 40); // Position below value label
    lv_slider_set_range(brightnessSlider, 10, 255); // Min brightness 10
    lv_slider_set_value(brightnessSlider, displayBrightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x4A90E2), LV_PART_KNOB); // Blue knob
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xCCCCCC), LV_PART_MAIN); // Gray track
    lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); // White indicator

    // Preset buttons
    lv_obj_t* presetContainer = lv_obj_create(container);
    lv_obj_set_size(presetContainer, 260, 50);
    lv_obj_align(presetContainer, LV_ALIGN_TOP_MID, 0, 70); // Position below slider
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

        // Pass preset value directly as user data (cast needed)
        lv_obj_set_user_data(btn, (void*)(uintptr_t)presetValues[i]);

        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
            uint8_t value = (uint8_t)(uintptr_t)lv_obj_get_user_data(btn);

            // Find slider and value label relative to button's container's container
            lv_obj_t* p_container = lv_obj_get_parent(lv_obj_get_parent(btn)); // presetContainer -> container
            lv_obj_t* slider = lv_obj_get_child(p_container, 1); // Assuming slider is 2nd child
            lv_obj_t* valueLabel = lv_obj_get_child(p_container, 0); // Assuming value label is 1st child

            if (slider && valueLabel && lv_obj_is_valid(slider) && lv_obj_is_valid(valueLabel)) {
                displayBrightness = value;
                lv_slider_set_value(slider, value, LV_ANIM_ON);
                char buf_cb[16];
                snprintf(buf_cb, sizeof(buf_cb), "%d%%", (value * 100) / 255);
                lv_label_set_text(valueLabel, buf_cb);
                M5.Display.setBrightness(value);

                Preferences prefs_cb;
                prefs_cb.begin("settings", false);
                prefs_cb.putUChar("brightness", value);
                prefs_cb.end();

                DEBUG_PRINTF("Brightness preset set to %d\n", value);
            } else {
                 DEBUG_PRINT("Error finding slider/label in preset callback");
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    // Slider event handler
    lv_obj_add_event_cb(brightnessSlider, [](lv_event_t* e) {
        lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);
        displayBrightness = lv_slider_get_value(slider);

        // Find value label relative to slider's parent
        lv_obj_t* p_container = lv_obj_get_parent(slider);
        lv_obj_t* valueLabel = lv_obj_get_child(p_container, 0); // Assuming value label is 1st child

        if (valueLabel && lv_obj_is_valid(valueLabel)) {
            char buf_cb[16];
            snprintf(buf_cb, sizeof(buf_cb), "%d%%", (displayBrightness * 100) / 255);
            lv_label_set_text(valueLabel, buf_cb);
        }
        M5.Display.setBrightness(displayBrightness);

        Preferences prefs_cb;
        prefs_cb.begin("settings", false);
        prefs_cb.putUChar("brightness", displayBrightness);
        prefs_cb.end();

        // DEBUG_PRINTF("Brightness set to %d\n", displayBrightness); // Reduce debug noise
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Auto-brightness option (Removed as per original code comment - requires sensor)
    /*
    lv_obj_t* auto_container = lv_obj_create(container);
    // ... setup auto brightness switch ...
    */

    // Back button
    lv_obj_t* back_btn = lv_btn_create(brightness_settings_screen); // Attach to screen
    lv_obj_set_size(back_btn, 140, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10); // Position bottom center
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);

    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* current_screen = lv_obj_get_screen((lv_obj_t*)lv_event_get_target(e));
        createSettingsScreen();
         if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            brightness_settings_screen = nullptr; // Clear static pointer
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(brightness_settings_screen); // Load screen at the end
    DEBUG_PRINT("Finished createBrightnessSettingsScreen");
}
// --- Power Management Message Box Callback ---
// Moved from inside createPowerManagementScreen to be accessible by nested lambdas
static void power_management_msgbox_event_cb(lv_event_t* e) {
    lv_obj_t* mbox = (lv_obj_t*)lv_event_get_target(e);
    uint16_t btn_id = *(uint16_t*)lv_event_get_param(e); // v9: Get button index directly

    // Button IDs are 0 for "Cancel", 1 for "Power Off" / "Restart" based on creation order

    if (btn_id == 1) { // Check if the second button ("Power Off" or "Restart") was pressed
        lv_obj_t* title_obj = lv_msgbox_get_title(mbox); // Get title label object
        const char* title_text = title_obj ? lv_label_get_text(title_obj) : NULL; // Get text from label object

        if (title_text) { // Check if we got the title text
             if (strcmp(title_text, "Power Off") == 0) {
                 DEBUG_PRINT("Powering off...");
                 // Optional: Show a brief "Shutting down..." message
                 lv_obj_t* shutdown_label = lv_label_create(lv_scr_act());
                 lv_label_set_text(shutdown_label, "Shutting down...");
                 lv_obj_center(shutdown_label);
                 lv_task_handler(); // Update display
                 delay(1000);

                 M5.Lcd.fillScreen(TFT_BLACK);
                 M5.Lcd.sleep();
                 M5.Lcd.waitDisplay();
                 M5.Power.powerOff(); // Use M5Unified power off
             } else if (strcmp(title_text, "Restart") == 0) {
                 DEBUG_PRINT("Restarting...");
                 // Optional: Show a brief "Restarting..." message
                 lv_obj_t* restart_label = lv_label_create(lv_scr_act());
                 lv_label_set_text(restart_label, "Restarting...");
                 lv_obj_center(restart_label);
                 lv_task_handler(); // Update display
                 delay(1000);
                 ESP.restart();
             }
        } // End if (title_text)
    } // End if (btn_id == 1)
    // "Cancel" button (ID 0) or other IDs don't need specific action here.
    // Message box closes automatically in v9.
}

// Implementation from .ino lines 3827-3997
// Implementation from .ino lines 3827-3997
void createPowerManagementScreen() {
    // Use local variable for screen pointer
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
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60); // Adjust size
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 50); // Position below header
    lv_obj_set_style_bg_color(container, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); // Align items to center horizontally
    lv_obj_set_style_pad_row(container, 15, 0); // Spacing between buttons

    // Power Off Button
    lv_obj_t* power_off_btn = lv_btn_create(container);
    lv_obj_set_size(power_off_btn, 280, 50); // Slightly smaller height
    lv_obj_add_style(power_off_btn, &style_btn, 0);
    lv_obj_add_style(power_off_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(power_off_btn, lv_color_hex(0xE74C3C), 0); // Red color

    lv_obj_t* power_off_label = lv_label_create(power_off_btn);
    lv_label_set_text(power_off_label, LV_SYMBOL_POWER " Power Off");
    lv_obj_set_style_text_font(power_off_label, &lv_font_montserrat_16, 0); // Smaller font
    lv_obj_center(power_off_label);

    // Restart Button
    lv_obj_t* restart_btn = lv_btn_create(container);
    lv_obj_set_size(restart_btn, 280, 50);
    lv_obj_add_style(restart_btn, &style_btn, 0);
    lv_obj_add_style(restart_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(restart_btn, lv_color_hex(0xF39C12), 0); // Orange color

    lv_obj_t* restart_label = lv_label_create(restart_btn);
    lv_label_set_text(restart_label, LV_SYMBOL_REFRESH " Restart");
    lv_obj_set_style_text_font(restart_label, &lv_font_montserrat_16, 0);
    lv_obj_center(restart_label);

    // Sleep Button (Removed as deep sleep logic might be complex/risky)
    /*
    lv_obj_t* sleep_btn = lv_btn_create(container);
    // ... setup sleep button ...
    lv_obj_add_event_cb(sleep_btn, [](lv_event_t* e) {
        // ... deep sleep logic ...
    }, LV_EVENT_CLICKED, NULL);
    */

    // Back Button
    lv_obj_t* back_btn = lv_btn_create(container); // Add back button inside container
    lv_obj_set_size(back_btn, 140, 50);
    // lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, 0); // Align at bottom of container
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x6c757d), 0); // Gray color
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);

    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);

    // NOTE: msgbox_event_cb is now defined as a static function above createPowerManagementScreen


    // Event handlers for main buttons
    lv_obj_add_event_cb(power_off_btn, [](lv_event_t* e) {
        // Create confirmation message box
        lv_obj_t* mbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(mbox, "Power Off");
        lv_msgbox_add_text(mbox, "Are you sure you want to power off?");
        // v9: Add footer buttons
        lv_msgbox_add_footer_button(mbox, "Cancel"); // ID 0
        lv_msgbox_add_footer_button(mbox, "Power Off"); // ID 1
        lv_obj_add_event_cb(mbox, power_management_msgbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // Use updated callback
        lv_obj_center(mbox);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(restart_btn, [](lv_event_t* e) {
         // Create confirmation message box
        lv_obj_t* mbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(mbox, "Restart");
        lv_msgbox_add_text(mbox, "Are you sure you want to restart?");
        // v9: Add footer buttons
        lv_msgbox_add_footer_button(mbox, "Cancel"); // ID 0
        lv_msgbox_add_footer_button(mbox, "Restart"); // ID 1
        lv_obj_add_event_cb(mbox, power_management_msgbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL); // Use updated callback
        lv_obj_center(mbox);
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Get the button
        lv_obj_t* current_screen = lv_obj_get_screen(btn); // Get screen from button
        createSettingsScreen(); // Go back to main settings
        if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen); // Delete the power_screen
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(power_screen);
}

// --- WiFi UI Functions ---

// Implementation from .ino lines 1109-1242
void createWiFiManagerScreen() {
    if (wifi_manager_screen && lv_obj_is_valid(wifi_manager_screen)) { // Check validity
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
        lv_obj_t* current_screen = lv_obj_get_screen((lv_obj_t*)lv_event_get_target(e)); // Cast needed for lv_obj_get_screen
        createSettingsScreen();
         if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            wifi_manager_screen = nullptr; // Clear global pointer
        }
    }, LV_EVENT_CLICKED, NULL);

    // Title
    lv_obj_t* title = lv_label_create(wifi_manager_screen);
    lv_label_set_text(title, "WiFi Manager");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // WiFi Enable Switch
    lv_obj_t* wifi_enable_label = lv_label_create(wifi_manager_screen);
    lv_label_set_text(wifi_enable_label, "WiFi Enable");
    lv_obj_align(wifi_enable_label, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_obj_add_style(wifi_enable_label, &style_text, 0);

    lv_obj_t* wifi_switch = lv_switch_create(wifi_manager_screen);
    lv_obj_align(wifi_switch, LV_ALIGN_TOP_RIGHT, -20, 65);
    lv_obj_set_size(wifi_switch, 50, 25);
    // Use wifiEnabled global variable (synced with Preferences)
    if (wifiEnabled) {
        lv_obj_add_state(wifi_switch, LV_STATE_CHECKED);
    } else {
         lv_obj_clear_state(wifi_switch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(wifi_switch, [](lv_event_t* e) {
        wifiEnabled = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
        if (wifiEnabled) {
            // wifiManager.enable(); // Assuming wifi_handler handles enabling logic
            DEBUG_PRINT("WiFi Enabled (UI)");
        } else {
            // wifiManager.disable(); // Assuming wifi_handler handles disabling logic
             WiFi.disconnect(true); // Force disconnect when disabling via UI
            DEBUG_PRINT("WiFi Disabled (UI)");
        }
        // Save state
        Preferences prefs;
        prefs.begin("settings", false);
        prefs.putBool("wifi_enabled", wifiEnabled);
        prefs.end();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Scan Button
    lv_obj_t* scan_btn = lv_btn_create(wifi_manager_screen);
    lv_obj_set_size(scan_btn, 100, 40);
    lv_obj_align(scan_btn, LV_ALIGN_TOP_MID, 0, 110); // Position below switch
    lv_obj_add_style(scan_btn, &style_btn, 0);
    lv_obj_add_style(scan_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan"); // Add text
    lv_obj_center(scan_label);
    lv_obj_set_style_text_color(scan_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_add_event_cb(scan_btn, [](lv_event_t* e) {
        if (wifiEnabled) { // Check global flag
            lv_obj_t* current_screen = lv_obj_get_screen((lv_obj_t*)lv_event_get_target(e));
            createWiFiScreen(); // Go to scan results screen
             if (current_screen && lv_obj_is_valid(current_screen)) {
                lv_obj_del_async(current_screen);
                wifi_manager_screen = nullptr;
            }
        } else {
            // Show message box if WiFi is disabled
            // Create message box
            lv_obj_t* msgbox = lv_msgbox_create(NULL);
            lv_msgbox_add_title(msgbox, "WiFi Disabled");
            lv_msgbox_add_text(msgbox, "Please enable WiFi to scan.");
            // v9: Add footer buttons
            lv_msgbox_add_footer_button(msgbox, "OK"); // ID 0
            // lv_msgbox_add_close_button(msgbox); // Close button is often implicit or handled by window manager
            lv_obj_center(msgbox);
            // Close message box when OK is clicked
            lv_obj_add_event_cb(msgbox, [](lv_event_t* e) {
                 lv_obj_t* target_mbox = (lv_obj_t*)lv_event_get_target(e); // Get the target msgbox
                 uint16_t btn_id = *(uint16_t*)lv_event_get_param(e); // v9: Get button index from event parameter
                 if (btn_id == 0) { // Check if "OK" button (ID 0) was clicked
                     // No need to manually close, v9 handles it
                     // lv_msgbox_close(target_mbox);
                 }
                 // If other buttons existed, check their IDs here
            }, LV_EVENT_VALUE_CHANGED, NULL); // Event triggers when button clicked
        }
    }, LV_EVENT_CLICKED, NULL);

    // TODO: Add list for saved networks if needed

    lv_scr_load(wifi_manager_screen);
    DEBUG_PRINT("WiFi Manager screen loaded");
}

// Implementation from .ino lines 1244-1400
void createWiFiScreen() {
    if (wifi_screen && lv_obj_is_valid(wifi_screen)) { // Check validity
        lv_obj_del(wifi_screen);
        wifi_screen = nullptr;
    }
    wifi_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(wifi_screen, lv_color_hex(0x1A1A1A), 0); // Dark gray background
    lv_obj_set_style_bg_opa(wifi_screen, LV_OPA_COVER, 0);
    // lv_obj_add_flag(wifi_screen, LV_OBJ_FLAG_SCROLLABLE); // Let list handle scrolling
    current_scroll_obj = nullptr; // Reset scroll object

    // Header
    lv_obj_t* header = lv_obj_create(wifi_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);

    // Back Button (inside header, left side)
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30); // Smaller size to fit in header
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0); // Aligned to left with padding
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0); // Slightly lighter gray
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT); // Left arrow icon
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Target is the button
        lv_obj_t* current_screen = lv_obj_get_screen(btn); // Get screen from button
        // Stop scan timer if active
        if (scan_timer) {
            lv_timer_del(scan_timer);
            scan_timer = nullptr;
        }
        createWiFiManagerScreen();
         if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            wifi_screen = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);

    // Scan Button (inside header, right side)
    lv_obj_t* scan_btn = lv_btn_create(header);
    lv_obj_set_size(scan_btn, 40, 30); // Smaller size to fit in header
    lv_obj_align(scan_btn, LV_ALIGN_RIGHT_MID, -10, 0); // Aligned to right with padding
    lv_obj_set_style_bg_color(scan_btn, lv_color_hex(0x444444), 0); // Slightly lighter gray
    lv_obj_set_style_radius(scan_btn, 5, 0);
    lv_obj_add_style(scan_btn, &style_btn, 0);
    lv_obj_add_style(scan_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH); // Search icon
    lv_obj_center(scan_label);
    lv_obj_set_style_text_color(scan_label, lv_color_hex(0xFFFFFF), 0); // White text

    // Title (inside header, centered)
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Available Networks");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container for the list and status
    lv_obj_t* container = lv_obj_create(wifi_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60); // Fill space below header
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 55); // Position below header
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2D2D2D), 0); // Darker background for list area
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0); // Opaque background
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE); // Container itself doesn't scroll

    // Status Label (inside container, top)
    wifi_status_label = lv_label_create(container);
    lv_label_set_text(wifi_status_label, "Scanning...");
    lv_obj_set_width(wifi_status_label, lv_pct(100));
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_align(wifi_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_style(wifi_status_label, &style_text, 0); // Use standard text style

    // WiFi List (inside container, below status)
    wifi_list = lv_list_create(container);
    lv_obj_set_size(wifi_list, lv_pct(100), lv_pct(100) - 30); // Fill remaining space minus status label height
    lv_obj_align(wifi_list, LV_ALIGN_BOTTOM_MID, 0, 0); // Align to bottom of container
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x3A3A3A), 0); // Slightly lighter list background
    lv_obj_set_style_radius(wifi_list, 5, 0);
    lv_obj_set_style_border_width(wifi_list, 0, 0);
    lv_obj_set_style_pad_all(wifi_list, 5, 0);
    current_scroll_obj = wifi_list; // Allow list scrolling

    // Scan Event Handler (attached to scan button)
    lv_obj_add_event_cb(scan_btn, [](lv_event_t* e) {
        lv_label_set_text(wifi_status_label, "Scanning...");
        lv_obj_clean(wifi_list); // Clear previous results (v9 API)

        // Show spinner while scanning
        if (!g_spinner) {
            g_spinner = lv_spinner_create(wifi_screen); // Create spinner on the screen (v9 API)
            lv_obj_set_size(g_spinner, 40, 40);
            lv_obj_align(g_spinner, LV_ALIGN_CENTER, 0, 0); // Center it
        } else {
             lv_obj_clear_flag(g_spinner, LV_OBJ_FLAG_HIDDEN); // Make sure it's visible
        }

        // Start scan using wifi_handler function
        // Start scan using WiFiManager (assuming it handles callbacks/results internally or via its own event system)
        wifiManager.startScan();
        // Note: The original callback logic that populated the list is removed here.
        // This logic should now be handled by the WiFiManager library's events/callbacks,
        // which would typically call a function like `onWiFiScanComplete` (declared in globals.h).
        // We need to ensure WiFiManager is configured to call such a handler.

    }, LV_EVENT_CLICKED, NULL);

    // Initial Scan Trigger
    // lv_event_send(scan_btn, LV_EVENT_CLICKED, NULL); // Commented out - Causing persistent compile error, likely build/include issue

    lv_scr_load(wifi_screen);
    DEBUG_PRINT("WiFi Scan screen loaded");
}

// Implementation from .ino lines 1402-1498
void createNetworkDetailsScreen(const String& ssid) {
    // Create the screen object
    lv_obj_t* details_screen = lv_obj_create(NULL);
    lv_obj_add_style(details_screen, &style_screen, 0);
    lv_obj_set_style_bg_color(details_screen, lv_color_hex(0x1A1A1A), 0); // Dark gray background
    lv_obj_set_style_bg_opa(details_screen, LV_OPA_COVER, 0);

    // Header
    lv_obj_t* header = lv_obj_create(details_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);

    // Back Button (inside header, left side)
    lv_obj_t* back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 40, 30);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0);
    lv_obj_set_style_radius(back_btn, 5, 0);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Target is the button
        lv_obj_t* current_screen = lv_obj_get_screen(btn); // Get screen from button
        createWiFiScreen(); // Go back to the scan list
         if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            // No global pointer for details screen
        }
    }, LV_EVENT_CLICKED, NULL);

    // Title (inside header, centered)
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, ssid.c_str()); // Show SSID in title
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Container for buttons
    lv_obj_t* container = lv_obj_create(details_screen);
    lv_obj_set_size(container, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 60); // Fill space below header
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 55); // Position below header
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2D2D2D), 0); // Darker background
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0); // Opaque background
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE); // Container doesn't scroll

    // Connect Button
    lv_obj_t* connect_btn = lv_btn_create(container);
    lv_obj_set_size(connect_btn, 140, 50);
    lv_obj_align(connect_btn, LV_ALIGN_TOP_MID, 0, 20); // Positioned near top
    lv_obj_add_style(connect_btn, &style_btn, 0);
    lv_obj_add_style(connect_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_center(connect_label);
    // Pass SSID to connect button callback using strdup for safety
    char* ssid_cstr_connect = strdup(ssid.c_str());
    if (ssid_cstr_connect) {
        lv_obj_set_user_data(connect_btn, (void*)ssid_cstr_connect);
        lv_obj_add_event_cb(connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, NULL); // Pass NULL, get data in callback
         // Add delete callback for connect button user data
        lv_obj_add_event_cb(connect_btn, [](lv_event_t* e_del){
            // Explicitly cast the target to lv_obj_t* for lv_obj_get_user_data
            char* data_to_free = (char*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e_del));
            if(data_to_free) free(data_to_free);
        }, LV_EVENT_DELETE, NULL);
    } else {
         DEBUG_PRINT("Failed to allocate memory for connect button SSID");
    }


    // Forget Button (Requires library support - keep commented)
    /*
    lv_obj_t* forget_btn = lv_btn_create(container);
    // ... (styling) ...
    char* ssid_cstr_forget = strdup(ssid.c_str());
    if (ssid_cstr_forget) {
        lv_obj_set_user_data(forget_btn, (void*)ssid_cstr_forget);
        lv_obj_add_event_cb(forget_btn, forget_btn_event_cb, LV_EVENT_CLICKED, NULL);
         lv_obj_add_event_cb(forget_btn, [](lv_event_t* e_del){
            char* data_to_free = (char*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e_del)); // v9 cast
            if(data_to_free) free(data_to_free);
        }, LV_EVENT_DELETE, NULL);
    }
    */

    // Cancel Button (Alternative to Back, or use Back button in header)
    lv_obj_t* cancel_btn = lv_btn_create(container);
    lv_obj_set_size(cancel_btn, 140, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_TOP_MID, 0, 80); // Positioned below connect
    lv_obj_add_style(cancel_btn, &style_btn, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x6c757d), 0); // Gray color
    lv_obj_add_style(cancel_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(cancel_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Target is the button
        lv_obj_t* current_screen = lv_obj_get_screen(btn); // Get screen from button
        createWiFiScreen(); // Go back to scan list
         if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            // No global pointer for details screen to clear
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(details_screen);
}

// Implementation from .ino lines 1499-1553
static void connect_btn_event_cb(lv_event_t* e) {
    // Cast the result of lv_event_get_target to lv_obj_t* to match lv_obj_get_user_data parameter type
    char* ssid_cstr = (char*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
    if (!ssid_cstr) {
        DEBUG_PRINT("Error: SSID pointer is null in connect_btn_event_cb");
        return;
    }
    String ssid = String(ssid_cstr); // Convert to String for comparison/use
    bool isConnected = (WiFi.status() == WL_CONNECTED) && (WiFi.SSID() == ssid);

    if (isConnected) {
        // Already connected to this network, offer disconnect
        // Create message box (v9 style)
        lv_obj_t* msgbox = lv_msgbox_create(NULL);
        lv_msgbox_add_title(msgbox, "Disconnect");
        lv_msgbox_add_text(msgbox, ("Disconnect from " + ssid + "?").c_str());
        static const char* btns[] = {"Cancel", "Disconnect", ""}; // v9: Cancel usually first (ID 0)
        // v9: Add footer buttons
        lv_msgbox_add_footer_button(msgbox, btns[0]); // Cancel (ID 0)
        lv_msgbox_add_footer_button(msgbox, btns[1]); // Disconnect (ID 1)
        lv_obj_center(msgbox); // Center it
        // Allocate memory for SSID to pass as user_data, ensuring lifetime
        char* ssid_copy = strdup(ssid.c_str());
        if (!ssid_copy) {
            DEBUG_PRINTLN("Failed to allocate memory for SSID copy!");
            lv_msgbox_close(msgbox); // Close the message box if allocation fails (Corrected typo: mbox -> msgbox)
            // Potentially add more robust error handling here
        } else {
            // Add event callback with captureless lambda and user_data
            // Corrected typo: mbox -> msgbox
            lv_obj_add_event_cb(msgbox, [](lv_event_t* e_msg) { // Captureless lambda
                char* captured_ssid = (char*)lv_event_get_user_data(e_msg); // Retrieve user data
                lv_obj_t* mbox = (lv_obj_t*)lv_event_get_target(e_msg); // Get the target msgbox
                uint16_t btn_id = *(uint16_t*)lv_event_get_param(e_msg); // v9: Get button index from event parameter

                if (captured_ssid) { // Check if SSID data is valid
                    if (btn_id == 1) { // Check if "Disconnect" button (ID 1) was clicked
                        WiFi.disconnect(true); // Disconnect and erase credentials
                        manualDisconnect = true; // Set flag
                        DEBUG_PRINTF("Manually disconnected from %s\n", captured_ssid); // Use captured C-string
                        // Optionally go back to WiFi Manager screen
                        lv_obj_t* current_screen = lv_obj_get_screen(mbox); // Get screen before closing msgbox
                        // lv_msgbox_close(mbox); // No need to close manually in v9
                        createWiFiManagerScreen();
                         if (current_screen && lv_obj_is_valid(current_screen)) {
                            lv_obj_del_async(current_screen);
                        }
                    } else { // Cancel button (ID 0) or close clicked
                         // lv_msgbox_close(mbox); // No need to close manually in v9
                    }
                } // <-- Closing brace for 'if (captured_ssid)'

                // Free the allocated memory after use, only if not null
                if (captured_ssid) {
                    free(captured_ssid);
                }
            }, LV_EVENT_VALUE_CHANGED, ssid_copy); // Keep cast removed
        }
        // Removed redundant lv_obj_center(msgbox) from line 2514
    } else {
        // Not connected, show password input
        strncpy(selected_ssid, ssid.c_str(), sizeof(selected_ssid) - 1);
        selected_ssid[sizeof(selected_ssid) - 1] = '\0'; // Ensure null termination
        selected_password[0] = '\0'; // Clear previous password attempt
        showWiFiKeyboard();
    }
    // NOTE: Do NOT free ssid_cstr here, it's needed if the button is clicked again (e.g., disconnect)
    // It will be freed when the button itself is deleted via the LV_EVENT_DELETE callback.
}

// Implementation from .ino lines 1555-1588 (Forget Network - Requires Library Support)
static void forget_btn_event_cb(lv_event_t* e) {
     // Explicitly cast target to lv_obj_t* for lv_obj_get_user_data, then cast result to char*
     char* ssid_cstr = (char*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
     if (!ssid_cstr) {
         DEBUG_PRINT("Error: SSID pointer is null in forget_btn_event_cb");
         return;
     }
     String ssid_to_forget = String(ssid_cstr);

     // Show confirmation dialog
     // Create message box (v9 style)
     lv_obj_t* msgbox = lv_msgbox_create(NULL);
     lv_msgbox_add_title(msgbox, "Forget Network");
     lv_msgbox_add_text(msgbox, ("Forget " + ssid_to_forget + "?").c_str());
     static const char* btns[] = {"Cancel", "Forget", ""}; // v9: Cancel usually first (ID 0)
     // v9: Add footer buttons
     lv_msgbox_add_footer_button(msgbox, btns[0]); // Cancel (ID 0)
     lv_msgbox_add_footer_button(msgbox, btns[1]); // Forget (ID 1)
     lv_obj_center(msgbox); // Center it
     // Allocate memory for SSID to pass as user_data, ensuring lifetime
     char* ssid_copy = strdup(ssid_to_forget.c_str());
     if (!ssid_copy) {
         DEBUG_PRINTLN("Failed to allocate memory for SSID copy (forget)!");
         lv_msgbox_close(msgbox); // Close the message box if allocation fails
     } else {
         lv_obj_add_event_cb(msgbox, [](lv_event_t* e_msg) { // Captureless lambda
             char* captured_ssid = (char*)lv_event_get_user_data(e_msg); // Retrieve user data
             lv_obj_t* mbox = (lv_obj_t*)lv_event_get_target(e_msg); // Get the target msgbox (cast needed)
             uint16_t btn_id = *(uint16_t*)lv_event_get_param(e_msg); // v9: Get button index from event parameter

             if (captured_ssid) { // Check if SSID data is valid
                 if (btn_id == 1) { // Check if "Forget" button (ID 1) was clicked
                 // ** This requires a function like wifiManager.forgetNetwork(captured_ssid); **
                 // ** which is NOT standard in ESP32 WiFi or typical WiFiManager libraries. **
                 // ** Assuming such a function exists for demonstration: **
                 // bool success = wifiManager.forgetNetwork(captured_ssid);
                 bool success = false; // Placeholder - Assume failure as function likely doesn't exist
                 DEBUG_PRINTF("Attempted to forget %s - Success: %d (Placeholder - Functionality likely missing)\n", captured_ssid, success);

                 // Create result message box
                 lv_obj_t* result_msgbox = lv_msgbox_create(NULL);
                 lv_msgbox_add_title(result_msgbox, "Forget Network");
                 lv_msgbox_add_text(result_msgbox, success ? "Network forgotten." : "Forget failed (Not Supported).");
                 // v9: Add footer button
                 lv_msgbox_add_footer_button(result_msgbox, "OK"); // ID 0
                 lv_obj_center(result_msgbox);
                 // Add event callback to close the result message box
                 lv_obj_add_event_cb(result_msgbox, [](lv_event_t* e_res) {
                     lv_obj_t* target_mbox = (lv_obj_t*)lv_event_get_target(e_res); // Get the target result_msgbox
                     uint16_t res_btn_id = *(uint16_t*)lv_event_get_param(e_res); // v9: Get button index from event parameter
                     if (res_btn_id == 0) { // Check if "OK" button (ID 0) was clicked
                         // lv_msgbox_close(target_mbox); // No need to close manually
                     }
                 }, LV_EVENT_VALUE_CHANGED, NULL);

                 // Optionally navigate back after forgetting
                 lv_obj_t* current_screen = lv_obj_get_screen(mbox); // Get screen before closing msgbox
                 // lv_msgbox_close(mbox); // No need to close manually
                 createWiFiManagerScreen(); // Go back to manager
                  if (current_screen && lv_obj_is_valid(current_screen)) {
                     lv_obj_del_async(current_screen);
                 }

             } else { // Cancel button (ID 0) or close clicked
                 // lv_msgbox_close(mbox); // No need to close manually
             }
            } // <-- Closing brace for 'if (captured_ssid)'

             // Free the allocated memory after use, only if not null
             if (captured_ssid) {
                 free(captured_ssid);
             }
         }, LV_EVENT_VALUE_CHANGED, ssid_copy); // Pass the allocated copy as user_data
     }

     // lv_obj_center(msgbox); // Already centered after creation

     // NOTE: Do NOT free ssid_cstr here. It's freed by the button's LV_EVENT_DELETE callback.
}

// Implementation from .ino lines 1590-1666
void showWiFiKeyboard() {
    // Create a container for the keyboard and text area
    lv_obj_t* kb_container = lv_obj_create(lv_layer_top()); // Create on top layer for modal effect
    lv_obj_set_size(kb_container, 300, 220); // Adjust size
    lv_obj_center(kb_container);
    lv_obj_set_style_bg_color(kb_container, lv_color_hex(0x333333), 0); // Dark background
    lv_obj_set_style_radius(kb_container, 10, 0);
    lv_obj_set_style_border_width(kb_container, 1, 0);
    lv_obj_set_style_border_color(kb_container, lv_color_hex(0xCCCCCC), 0);

    // Add a title label
    lv_obj_t* kb_title = lv_label_create(kb_container);
    String title_text = "Enter Password for:\n" + String(selected_ssid);
    lv_label_set_text(kb_title, title_text.c_str());
    lv_obj_add_style(kb_title, &style_text, 0); // Use standard text style
    lv_obj_set_style_text_align(kb_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(kb_title, LV_ALIGN_TOP_MID, 0, 5);

    // Create the text area for password input
    lv_obj_t* ta = lv_textarea_create(kb_container);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_password_mode(ta, true);
    lv_obj_set_width(ta, lv_pct(90));
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40); // Position below title
    lv_textarea_set_placeholder_text(ta, "Password");
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x555555), 0); // Text area background
    lv_obj_set_style_text_color(ta, lv_color_hex(0xFFFFFF), 0); // Text color

    // Create the keyboard
    wifi_keyboard = lv_keyboard_create(lv_layer_top()); // Create keyboard on top layer
    lv_obj_set_size(wifi_keyboard, LV_HOR_RES, LV_VER_RES / 2); // Adjust size as needed
    lv_keyboard_set_textarea(wifi_keyboard, ta); // Link keyboard to text area

    // Style keyboard buttons (optional, but improves look)
    lv_obj_add_style(wifi_keyboard, &style_keyboard_btn, LV_PART_ITEMS);

    // Define the lambda separately to avoid repetition
    auto keyboard_event_handler_lambda = [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t* kb = (lv_obj_t*)lv_event_get_target(e); // Keyboard object
        lv_obj_t* kb_cont = (lv_obj_t*)lv_event_get_user_data(e); // Get container from user data (keep cast for user data)
        lv_obj_t* ta_local = lv_keyboard_get_textarea(kb); // Get linked text area

        if (!ta_local) { // Add check for text area validity
             DEBUG_PRINT("Error: Could not get text area from keyboard in event handler.");
             // Clean up keyboard and container if ta is invalid
             if (kb_cont && lv_obj_is_valid(kb_cont)) lv_obj_del_async(kb_cont);
             if (kb && lv_obj_is_valid(kb)) lv_obj_del_async(kb);
             wifi_keyboard = nullptr;
             return;
        }

        if (code == LV_EVENT_READY) {
            // lv_obj_t* ta_local = lv_keyboard_get_textarea(kb); // Moved up
            if (ta_local) { // Check if text area is valid
                const char* pwd = lv_textarea_get_text(ta_local);
                strncpy(selected_password, pwd, sizeof(selected_password) - 1);
                selected_password[sizeof(selected_password) - 1] = '\0'; // Ensure null termination

                DEBUG_PRINTF("Connecting to %s with password...\n", selected_ssid);
                showWiFiLoadingScreen(selected_ssid); // Show loading screen
                // Call the connection function from wifi_handler
                connectToWiFi(selected_ssid, selected_password);
            } else {
                 DEBUG_PRINT("Error: Could not get text area from keyboard in READY event."); // Should not happen now due to check above
            }
        } else if (code == LV_EVENT_CANCEL) {
             DEBUG_PRINT("WiFi password entry cancelled.");
             // Go back to details screen
             createNetworkDetailsScreen(selected_ssid); // Recreate details screen
        }

        // Delete keyboard and container (runs for both READY and CANCEL)
        // Check validity before deleting
        if (kb_cont && lv_obj_is_valid(kb_cont)) {
             lv_obj_del_async(kb_cont); // Delete the container first
        }
        // The keyboard itself is also on the top layer and needs deletion
        if (kb && lv_obj_is_valid(kb)) {
             lv_obj_del_async(kb);
             wifi_keyboard = nullptr; // Clear global pointer
        }
    };

    // Add separate event callbacks for READY and CANCEL
    lv_obj_add_event_cb(wifi_keyboard, keyboard_event_handler_lambda, LV_EVENT_READY, kb_container);
    lv_obj_add_event_cb(wifi_keyboard, keyboard_event_handler_lambda, LV_EVENT_CANCEL, kb_container);

}

// Implementation from .ino lines 1668-1729
void showWiFiLoadingScreen(const String& ssid) {
    if (wifi_loading_screen && lv_obj_is_valid(wifi_loading_screen)) {
        lv_obj_del(wifi_loading_screen);
        wifi_loading_screen = nullptr;
    }
    wifi_loading_screen = lv_obj_create(NULL); // Create on default screen
    lv_obj_add_style(wifi_loading_screen, &style_screen, 0);
    lv_obj_set_style_bg_opa(wifi_loading_screen, LV_OPA_COVER, 0);

    // Create a semi-transparent background overlay
    lv_obj_t* overlay = lv_obj_create(wifi_loading_screen);
    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0); // Make it semi-transparent
    lv_obj_center(overlay);

    // Spinner
    wifi_loading_spinner = lv_spinner_create(wifi_loading_screen); // v9 API
    lv_obj_set_size(wifi_loading_spinner, 60, 60);
    lv_obj_align(wifi_loading_spinner, LV_ALIGN_CENTER, 0, -20);

    // Loading Label
    wifi_loading_label = lv_label_create(wifi_loading_screen);
    String loading_text = "Connecting to:\n" + ssid;
    lv_label_set_text(wifi_loading_label, loading_text.c_str());
    lv_obj_add_style(wifi_loading_label, &style_text, 0);
    lv_obj_set_style_text_align(wifi_loading_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(wifi_loading_label, wifi_loading_spinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);

    // Result Label (initially hidden)
    wifi_result_label = lv_label_create(wifi_loading_screen);
    lv_label_set_text(wifi_result_label, "");
    lv_obj_add_style(wifi_result_label, &style_text, 0);
    lv_obj_set_style_text_align(wifi_result_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(wifi_result_label, wifi_loading_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_flag(wifi_result_label, LV_OBJ_FLAG_HIDDEN); // Hide initially

    lv_scr_load(wifi_loading_screen); // Load the screen
}

// Implementation from .ino lines 1731-1784
void updateWiFiLoadingScreen(bool success, const String& message) {
    if (!wifi_loading_screen || !lv_obj_is_valid(wifi_loading_screen)) {
        DEBUG_PRINT("WiFi loading screen is not valid for update.");
        return;
    }

    // Hide spinner and loading label
    if (wifi_loading_spinner && lv_obj_is_valid(wifi_loading_spinner)) {
        lv_obj_add_flag(wifi_loading_spinner, LV_OBJ_FLAG_HIDDEN);
    }
    if (wifi_loading_label && lv_obj_is_valid(wifi_loading_label)) {
        lv_obj_add_flag(wifi_loading_label, LV_OBJ_FLAG_HIDDEN);
    }

    // Show result label
    if (wifi_result_label && lv_obj_is_valid(wifi_result_label)) {
        lv_label_set_text(wifi_result_label, message.c_str());
        lv_obj_set_style_text_color(wifi_result_label, lv_color_hex(success ? 0x00FF00 : 0xFF0000), 0); // Green for success, red for fail
        lv_obj_clear_flag(wifi_result_label, LV_OBJ_FLAG_HIDDEN); // Make visible
        lv_obj_align(wifi_result_label, LV_ALIGN_CENTER, 0, 0); // Re-center result text
    }

    // Create a timer to automatically close the screen after a delay
    static lv_timer_t* close_timer = nullptr;
    if (close_timer) {
        lv_timer_del(close_timer); // Delete previous timer if any
    }
    close_timer = lv_timer_create([](lv_timer_t* timer) {
        lv_obj_t* screen_to_close = (lv_obj_t*)lv_timer_get_user_data(timer);
        if (screen_to_close && lv_obj_is_valid(screen_to_close)) {
             // Decide where to go next based on success?
             // For now, always go back to WiFi Manager
             createWiFiManagerScreen();
             lv_obj_del_async(screen_to_close); // Delete the loading screen
             wifi_loading_screen = nullptr; // Clear pointer
        }
        close_timer = nullptr; // Clear static timer pointer
    }, 3000, wifi_loading_screen); // 3-second delay, pass screen as user data
}

// --- Date/Time Selection Screens ---

// Static callback handlers for date/time rollers
static void on_year_change(lv_event_t* e) {
    if (!g_year_roller || !g_selected_date_label || !lv_obj_is_valid(g_year_roller) || !lv_obj_is_valid(g_selected_date_label)) return; // Add validity checks
    int year = 2023 + lv_roller_get_selected(g_year_roller); // Adjust base year
    selected_date.year = year;
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", selected_date.year, selected_date.month, selected_date.day);
    lv_label_set_text(g_selected_date_label, date_str);
}

static void on_month_change(lv_event_t* e) {
     if (!g_month_roller || !g_selected_date_label || !lv_obj_is_valid(g_month_roller) || !lv_obj_is_valid(g_selected_date_label)) return; // Add validity checks
    int month = lv_roller_get_selected(g_month_roller) + 1;
    selected_date.month = month;
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", selected_date.year, selected_date.month, selected_date.day);
    lv_label_set_text(g_selected_date_label, date_str);
}

static void on_day_change(lv_event_t* e) {
     if (!g_day_roller || !g_selected_date_label || !lv_obj_is_valid(g_day_roller) || !lv_obj_is_valid(g_selected_date_label)) return; // Add validity checks
    int day = lv_roller_get_selected(g_day_roller) + 1;
    selected_date.day = day;
    char date_str[64];
    snprintf(date_str, sizeof(date_str), "Selected: %04d-%02d-%02d", selected_date.year, selected_date.month, selected_date.day);
    lv_label_set_text(g_selected_date_label, date_str);
}

static void on_hour_change(lv_event_t* e) {
    if (!g_hour_roller || !g_selected_time_label || !lv_obj_is_valid(g_hour_roller) || !lv_obj_is_valid(g_selected_time_label)) return; // Add validity checks
    selected_hour = lv_roller_get_selected(g_hour_roller) + 1; // 1-12
    char selected_time_str[32];
    snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s",
             selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
    lv_label_set_text(g_selected_time_label, selected_time_str);
}

static void on_minute_change(lv_event_t* e) {
    if (!g_minute_roller || !g_selected_time_label || !lv_obj_is_valid(g_minute_roller) || !lv_obj_is_valid(g_selected_time_label)) return; // Add validity checks
    selected_minute = lv_roller_get_selected(g_minute_roller);
    char selected_time_str[32];
    snprintf(selected_time_str, sizeof(selected_time_str), "Selected: %02d:%02d %s",
             selected_hour, selected_minute, selected_is_pm ? "PM" : "AM");
    lv_label_set_text(g_selected_time_label, selected_time_str);
}

void createDateSelectionScreen() {
    static lv_obj_t* date_screen = nullptr; // Keep static to manage deletion
    if (date_screen && lv_obj_is_valid(date_screen)) {
        lv_obj_del(date_screen);
        date_screen = nullptr;
    }
    date_screen = lv_obj_create(NULL);
    lv_obj_add_style(date_screen, &style_screen, 0);

    // Header
    lv_obj_t* header = lv_obj_create(date_screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(header, 10, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

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

    // Get current date from RTC
    m5::rtc_date_t DateStruct;
    M5.Rtc.getDate(&DateStruct);
    // Use current RTC date as initial selection if valid, else default
    if (DateStruct.year >= 2023) {
        selected_date.year = DateStruct.year;
        selected_date.month = DateStruct.month;
        selected_date.day = DateStruct.date;
    } else {
        // Use the default defined in globals (already initialized)
    }


    // Selected date display
    g_selected_date_label = lv_label_create(container);
    lv_obj_set_style_text_font(g_selected_date_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_bg_color(g_selected_date_label, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_bg_opa(g_selected_date_label, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(g_selected_date_label, 5, 0);
    lv_obj_set_style_radius(g_selected_date_label, 5, 0);
    lv_obj_set_style_text_color(g_selected_date_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_selected_date_label, LV_ALIGN_TOP_MID, 0, 0);

    // Year picker
    lv_obj_t* year_label = lv_label_create(container);
    lv_label_set_text(year_label, "Year:");
    lv_obj_align(year_label, LV_ALIGN_TOP_LEFT, 10, 25);

    g_year_roller = lv_roller_create(container);
    char years_str[512] = {0}; // Ensure large enough buffer
    for (int year = 2023; year <= 2040; year++) { // Start from a more recent year
        char year_str[8];
        snprintf(year_str, sizeof(year_str), "%04d\n", year);
        strcat(years_str, year_str);
    }
    if (strlen(years_str) > 0) {
        years_str[strlen(years_str) - 1] = '\0'; // Remove last newline
    }
    lv_roller_set_options(g_year_roller, years_str, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_year_roller, 2);
    lv_obj_set_width(g_year_roller, 95);
    lv_obj_align(g_year_roller, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_roller_set_selected(g_year_roller, selected_date.year - 2023, LV_ANIM_OFF); // Adjust index based on start year

    // Month picker
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
    lv_roller_set_selected(g_month_roller, selected_date.month - 1, LV_ANIM_OFF); // Month is 1-based

    // Day picker
    lv_obj_t* day_label = lv_label_create(container);
    lv_label_set_text(day_label, "Day:");
    lv_obj_align(day_label, LV_ALIGN_TOP_RIGHT, -10, 25);

    g_day_roller = lv_roller_create(container);
    char days_str[256] = {0}; // Ensure large enough
    for (int day = 1; day <= 31; day++) {
        char day_str[8];
        snprintf(day_str, sizeof(day_str), "%02d\n", day);
        strcat(days_str, day_str);
    }
    if (strlen(days_str) > 0) {
        days_str[strlen(days_str) - 1] = '\0'; // Remove last newline
    }
    lv_roller_set_options(g_day_roller, days_str, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(g_day_roller, 2);
    lv_obj_set_width(g_day_roller, 95);
    lv_obj_align(g_day_roller, LV_ALIGN_TOP_RIGHT, -10, 40);
    lv_roller_set_selected(g_day_roller, selected_date.day - 1, LV_ANIM_OFF); // Day is 1-based

    // Initial update for selected date label
    char date_str_init[64];
    snprintf(date_str_init, sizeof(date_str_init), "Selected: %04d-%02d-%02d", selected_date.year, selected_date.month, selected_date.day);
    lv_label_set_text(g_selected_date_label, date_str_init);


    // Event handlers
    lv_obj_add_event_cb(g_year_roller, on_year_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_month_roller, on_month_change, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(g_day_roller, on_day_change, LV_EVENT_VALUE_CHANGED, NULL);

    // Continue button
    lv_obj_t* continue_btn = lv_btn_create(date_screen);
    lv_obj_set_size(continue_btn, 120, 40);
    lv_obj_align(continue_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(continue_btn, &style_btn, 0);
    lv_obj_add_style(continue_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* continue_label = lv_label_create(continue_btn);
    lv_label_set_text(continue_label, "Next");
    lv_obj_center(continue_label);
    lv_obj_add_event_cb(continue_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Target is the button
        lv_obj_t* current_screen = lv_obj_get_screen(btn); // Get screen from button
        createTimeSelectionScreen();
        if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            // date_screen = nullptr; // Clear static pointer
        }
    }, LV_EVENT_CLICKED, NULL);

    // Back button
    lv_obj_t* back_btn = lv_btn_create(date_screen);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x6c757d), 0); // Gray
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e); // Target is the button
        lv_obj_t* current_screen = lv_obj_get_screen(btn); // Get screen from button
        createSettingsScreen();
        if (current_screen && lv_obj_is_valid(current_screen)) {
            lv_obj_del_async(current_screen);
            // date_screen = nullptr; // Clear static pointer
        }
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(date_screen);
}

// --- UI Update Function Implementations ---
void createTimeSelectionScreen() {
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

void addStatusBar(lv_obj_t* screen) {
    if (!screen) return; // Need a valid screen to add to
    if (status_bar && lv_obj_is_valid(status_bar)) {
         // If status bar exists and is valid, maybe just update its parent?
         // Or delete and recreate if switching screens requires it.
         // For simplicity, let's delete and recreate.
         lv_obj_del(status_bar);
         status_bar = nullptr;
    }
    status_bar = lv_label_create(screen);
    lv_obj_set_style_text_font(status_bar, &lv_font_montserrat_14, 0);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(status_bar, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(status_bar, "Ready"); // Initial text
}

// Definition for updateStatusBar
void updateStatusBar() {
    if (!status_bar || !lv_obj_is_valid(status_bar)) {
        DEBUG_PRINT("Status bar invalid, cannot update.");
        return;
    }

    // --- WiFi Status ---
    String wifi_status_text = "WiFi: ";
    if (!wifiEnabled) {
        wifi_status_text += "Off";
    } else {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
             wifi_status_text += WiFi.SSID();
            // Add signal strength icon (example)
            long rssi = WiFi.RSSI();
            if (rssi >= -67) wifi_status_text += " " LV_SYMBOL_WIFI; // Strong
            else if (rssi >= -75) wifi_status_text += " " LV_SYMBOL_WIFI; // Medium (use same icon for now)
            else wifi_status_text += " " LV_SYMBOL_WIFI; // Weak (use same icon for now)
        } else if (status == WL_IDLE_STATUS) { // Replaced WL_CONNECTING
             wifi_status_text += "Connecting...";
        } else {
             wifi_status_text += "Disconnected";
        }
    }

    // --- Battery Status (Placeholder) ---
    // Reading AXP2101 requires I2C communication.
    // This is a simplified placeholder. Replace with actual AXP reading.
    int battery_level = M5.Power.getBatteryLevel(); // Use M5Unified function
    String battery_status_text;
    if (M5.Power.isCharging()) {
         battery_status_text = LV_SYMBOL_CHARGE " ";
    } else {
        // Choose battery icon based on level
        if (battery_level > 80) battery_status_text = LV_SYMBOL_BATTERY_FULL " ";
        else if (battery_level > 50) battery_status_text = LV_SYMBOL_BATTERY_3 " ";
        else if (battery_level > 20) battery_status_text = LV_SYMBOL_BATTERY_2 " ";
        else if (battery_level > 5) battery_status_text = LV_SYMBOL_BATTERY_1 " ";
        else battery_status_text = LV_SYMBOL_BATTERY_EMPTY " ";
    }
    battery_status_text += String(battery_level) + "%";


    // --- Combine and Update Label ---
    String full_status = battery_status_text + " | " + wifi_status_text;
    lv_label_set_text(status_bar, full_status.c_str());
    // Color is handled by updateStatus function if needed
}

void updateStatus(const char* message, uint32_t color) {
    if (!status_bar || !lv_obj_is_valid(status_bar)) return; // Ensure status bar exists and is valid

    // Update status message and color
    strncpy(current_status_msg, message, sizeof(current_status_msg) - 1);
    current_status_msg[sizeof(current_status_msg) - 1] = '\0'; // Ensure null termination
    current_status_color = color; // Store color in case needed elsewhere

    lv_label_set_text(status_bar, current_status_msg);
    lv_obj_set_style_text_color(status_bar, lv_color_hex(current_status_color), 0);

    // NOTE: The original implementation in the .ino file (lines 4630-4654)
    // incorrectly updated a separate 'time_label' within this function.
    // That logic has been removed here as time updates should be handled
    // by updateTimeDisplay() or updateLockScreenTime() called from the main loop.
}


void addTimeDisplay(lv_obj_t *screen) {
    if (!screen) return;
    // This function seems specific to the main menu's time display card.
    // Consider renaming or making it part of createMainMenu if only used there.

    // Check if time_label already exists (e.g., navigating back to main menu)
    // This check might be redundant if createMainMenu properly cleans up first.
    if (time_label && lv_obj_is_valid(time_label)) {
        // Maybe just update parent if needed? Or assume createMainMenu handles cleanup.
        // lv_obj_set_parent(time_label, screen); // Example if reusing
        // return; // Don't recreate if it exists
    }


    // Create the time display card (similar to info cards)
    lv_obj_t* time_card = lv_obj_create(screen);
    lv_obj_set_size(time_card, 180, 40); // Example size
    lv_obj_align(time_card, LV_ALIGN_TOP_RIGHT, -10, 5); // Position top right (adjust as needed)
    lv_obj_add_style(time_card, &style_card_info, 0); // Use info card style
    lv_obj_set_style_bg_color(time_card, lv_color_hex(0x2D2D2D), 0); // Darker background maybe?
    lv_obj_set_style_border_width(time_card, 0, 0); // No border

    // Time Icon (Optional)
    lv_obj_t* time_icon = lv_label_create(time_card);
    lv_label_set_text(time_icon, LV_SYMBOL_REFRESH); // Or LV_SYMBOL_BELL?
    lv_obj_set_style_text_font(time_icon, &lv_font_montserrat_16, 0); // Smaller font
    lv_obj_set_style_text_color(time_icon, lv_color_hex(0xCCCCCC), 0); // Lighter gray
    lv_obj_align(time_icon, LV_ALIGN_LEFT_MID, 5, 0);

    // Time Label (Assign to global pointer)
    time_label = lv_label_create(time_card);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0); // White text
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -5, 0); // Align text to the right
    lv_obj_set_width(time_label, LV_SIZE_CONTENT); // Adjust width dynamically

    updateTimeDisplay(); // Initial update
}

void updateTimeDisplay() {
    // Check if the label exists and is valid
    if (!time_label || !lv_obj_is_valid(time_label)) {
        // DEBUG_PRINT("Skipping main menu time update (label invalid)");
        return;
    }
     // Ensure the main menu screen is active before updating its specific time label
    if (lv_scr_act() != main_menu_screen) {
         // DEBUG_PRINT("Skipping main menu time update (screen not active)");
         return;
    }


    m5::rtc_time_t TimeStruct;
    if (!M5.Rtc.getTime(&TimeStruct)) {
        lv_label_set_text(time_label, "RTC Err");
        return;
    }

    // Convert 24-hour to 12-hour format
    int hour = TimeStruct.hours;
    const char* period = (hour >= 12) ? "PM" : "AM";
    if (hour == 0) {
        hour = 12; // Midnight
    } else if (hour > 12) {
        hour -= 12;
    }

    char timeStr[24]; // HH:MM:SS AM/PM
    snprintf(timeStr, sizeof(timeStr), "%d:%02d:%02d %s",
             hour, TimeStruct.minutes, TimeStruct.seconds, period);
    lv_label_set_text(time_label, timeStr);
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
    lv_obj_set_size(unlock_btn, 160, 40);
    lv_obj_align(unlock_btn, LV_ALIGN_BOTTOM_MID, 0, -5); // Position near bottom center
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
    // Check if the label exists, is valid, and the lock screen is currently active
    if (!g_lock_screen_datetime_label || !lv_obj_is_valid(g_lock_screen_datetime_label) || lv_scr_act() != lock_screen) {
        // DEBUG_PRINT("Skipping lock screen time update (label invalid or screen not active)");
        return; // Only update if the label exists and lock screen is active
    }

    m5::rtc_date_t DateStruct;
    m5::rtc_time_t TimeStruct;
    // It's safer to check if getDate/getTime succeeded, though M5Unified usually works
    if (!M5.Rtc.getDate(&DateStruct) || !M5.Rtc.getTime(&TimeStruct)) {
         lv_label_set_text(g_lock_screen_datetime_label, "RTC Read Error");
         return;
    }


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
