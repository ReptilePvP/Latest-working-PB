// Core Arduino/M5Stack Includes
#include <Arduino.h>
#include <M5Unified.h>

// Global Definitions and Forward Declarations
#include "globals.h"

// Module Headers
#include "ui.h"
#include "sd_logger.h"
#include "wifi_handler.h"
#include "time_utils.h"

// --- Global Object Definitions ---
// Define objects declared as extern in globals.h
WiFiManager wifiManager;
// SPIClass SPI_SD is defined in sd_logger.cpp

// --- LVGL Task ---
// Task to handle LVGL background tasks
void lvgl_task(void *pvParameters) {
    (void) pvParameters; // Unused parameter

    while (1) {
        // Let the GUI do its work
        if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE) {
             lv_timer_handler();
             xSemaphoreGive(xGuiSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(LV_TICK_PERIOD_MS)); // Wait a short period
    }
}

// --- SPI Bus Management ---
// Releases the default SPI bus (used by TFT)
void releaseSPIBus() {
    // May need more specific pin manipulation depending on how M5Unified handles SPI bus switching
    SPI.end();
    delay(50); // Small delay to allow bus release
    DEBUG_PRINT("Default SPI Bus Released");
}


// --- Main Setup ---
void setup() {
    Serial.begin(115200);
    DEBUG_PRINT("Starting Loss Prevention Log (Modular)...");

    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.begin(); // Initialize power management
    DEBUG_PRINT("M5Unified Initialized.");

    // Initialize LVGL and M5GFX integration
    lv_init();
    m5gfx_lvgl_init();
    if (!lv_is_initialized()) {
         DEBUG_PRINT("LVGL Init Failed!");
         // Handle initialization failure (e.g., display error, halt)
         while(1) delay(1000);
    }
    DEBUG_PRINT("LVGL Initialized.");

    // Create the LVGL task
     xTaskCreatePinnedToCore(
        lvgl_task,          /* Task function. */
        "LVGL",             /* name of task. */
        LVGL_STACK_SIZE,    /* Stack size of task */
        NULL,               /* parameter of the task */
        LVGL_TASK_PRIORITY, /* priority of the task */
        NULL,               /* Task handle to keep track of created task */
        LVGL_TASK_CORE);    /* pin task to core */
     DEBUG_PRINT("LVGL Task Created.");


    // Initialize File System (SD Card)
    initFileSystem(); // Function is now in sd_logger.cpp

    // Initialize UI Styles
    initStyles(); // Function is now in ui.cpp

    // Show Loading Screen
    createLoadingScreen(); // Function is now in ui.cpp

    // Initialize Speaker
    M5.Speaker.begin();
    DEBUG_PRINT("Speaker initialized");

    // Load saved settings (Volume, Brightness, etc.)
    Preferences prefs;
    prefs.begin("settings", false);
    uint8_t saved_volume = prefs.getUChar("volume", 128);
    bool sound_enabled = prefs.getBool("sound_enabled", true);
    M5.Speaker.setVolume(sound_enabled ? saved_volume : 0);
    displayBrightness = prefs.getUChar("brightness", 128); // Load global brightness
    M5.Display.setBrightness(displayBrightness);
    wifiEnabled = prefs.getBool("wifiEnabled", true); // Load WiFi enabled state
    prefs.end();
    DEBUG_PRINTF("Settings loaded (Volume: %d, Sound: %s, Brightness: %d, WiFi: %s)\n",
                 saved_volume, sound_enabled ? "On" : "Off", displayBrightness, wifiEnabled ? "On" : "Off");


    // Initialize RTC and System Time
    m5::rtc_date_t DateStruct;
    M5.Rtc.getDate(&DateStruct);
    if (DateStruct.year < 2023) { // Check if RTC time is valid/set
        DEBUG_PRINT("RTC time seems invalid, setting default.");
        DateStruct.year = 2024; DateStruct.month = 1; DateStruct.date = 1; DateStruct.weekDay = 1; // Example default
        M5.Rtc.setDate(&DateStruct);
        m5::rtc_time_t TimeStruct;
        TimeStruct.hours = 12; TimeStruct.minutes = 0; TimeStruct.seconds = 0;
        M5.Rtc.setTime(&TimeStruct);
    }
    setSystemTimeFromRTC(); // Function is now in time_utils.cpp

    // Initialize WiFi Manager
    // Callbacks are now defined in wifi_handler.cpp
    wifiManager.setStatusCallback(onWiFiStatus);
    wifiManager.setScanCallback(onWiFiScanComplete);
    wifiManager.begin(); // Starts WiFiManager background task
    DEBUG_PRINT("WiFi Manager initialized.");

    DEBUG_PRINT("Setup complete!");
}

// --- Main Loop ---
void loop() {
    M5.update(); // Update button states, etc.

    // Let WiFiManager handle its background tasks and callbacks
    wifiManager.update();

    // LVGL Timer Handler - Needs to run frequently in the main loop
    // Semaphore protection is handled within lvgl_task now.
    lv_timer_handler(); // Removed: Handled by lvgl_task

    // --- Time Update Section ---
    // Periodically update time displays on relevant screens
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 1000) { // Update every second
        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) { // Protect LVGL access
            if (lv_scr_act() == main_menu_screen) {
                updateTimeDisplay(); // Update main menu time display (ui.cpp)
            } else if (lv_scr_act() == lock_screen) {
                updateLockScreenTime(); // Update lock screen time display (ui.cpp)
            }
            // Add checks for other screens needing time updates if necessary
            xSemaphoreGive(xGuiSemaphore);
        }
        lastTimeUpdate = millis();
    }

    // --- Other Periodic Tasks ---
    // Example: Battery level check (could be moved to a timer if preferred)
    // static unsigned long lastBatteryCheck = 0;
    // if (millis() - lastBatteryCheck > 30000) { // Check every 30 seconds
    //     // Code to read battery and update UI elements if necessary
    //     // Remember semaphore protection if updating LVGL objects
    //     lastBatteryCheck = millis();
    // }


    // Small delay for cooperative multitasking
    delay(5);
}
