# 1 "C:\\Users\\nickd\\AppData\\Local\\Temp\\tmpy4lcen8c"
#include <Arduino.h>
# 1 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"

#include <Arduino.h>
#include <M5Unified.h>


#include "globals.h"


#include "ui.h"
#include "sd_logger.h"
#include "wifi_handler.h"
#include "time_utils.h"



WiFiManager wifiManager;
void lvgl_task(void *pvParameters);
void releaseSPIBus();
void setup();
void loop();
#line 21 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"
void lvgl_task(void *pvParameters) {
    (void) pvParameters;

    while (1) {

        if (xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE) {
             lv_timer_handler();
             xSemaphoreGive(xGuiSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(LV_TICK_PERIOD_MS));
    }
}



void releaseSPIBus() {

    SPI.end();
    delay(50);
    DEBUG_PRINT("Default SPI Bus Released");
}



void setup() {
    Serial.begin(115200);
    DEBUG_PRINT("Starting Loss Prevention Log (Modular)...");

    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Power.begin();
    DEBUG_PRINT("M5Unified Initialized.");


    lv_init();
    m5gfx_lvgl_init();
    if (!lv_is_initialized()) {
         DEBUG_PRINT("LVGL Init Failed!");

         while(1) delay(1000);
    }
    DEBUG_PRINT("LVGL Initialized.");


     xTaskCreatePinnedToCore(
        lvgl_task,
        "LVGL",
        LVGL_STACK_SIZE,
        NULL,
        LVGL_TASK_PRIORITY,
        NULL,
        LVGL_TASK_CORE);
     DEBUG_PRINT("LVGL Task Created.");



    initFileSystem();


    initStyles();


    createLoadingScreen();


    M5.Speaker.begin();
    DEBUG_PRINT("Speaker initialized");


    Preferences prefs;
    prefs.begin("settings", false);
    uint8_t saved_volume = prefs.getUChar("volume", 128);
    bool sound_enabled = prefs.getBool("sound_enabled", true);
    M5.Speaker.setVolume(sound_enabled ? saved_volume : 0);
    displayBrightness = prefs.getUChar("brightness", 128);
    M5.Display.setBrightness(displayBrightness);
    wifiEnabled = prefs.getBool("wifiEnabled", true);
    prefs.end();
    DEBUG_PRINTF("Settings loaded (Volume: %d, Sound: %s, Brightness: %d, WiFi: %s)\n",
                 saved_volume, sound_enabled ? "On" : "Off", displayBrightness, wifiEnabled ? "On" : "Off");



    m5::rtc_date_t DateStruct;
    M5.Rtc.getDate(&DateStruct);
    if (DateStruct.year < 2023) {
        DEBUG_PRINT("RTC time seems invalid, setting default.");
        DateStruct.year = 2024; DateStruct.month = 1; DateStruct.date = 1; DateStruct.weekDay = 1;
        M5.Rtc.setDate(&DateStruct);
        m5::rtc_time_t TimeStruct;
        TimeStruct.hours = 12; TimeStruct.minutes = 0; TimeStruct.seconds = 0;
        M5.Rtc.setTime(&TimeStruct);
    }
    setSystemTimeFromRTC();



    WiFi.setAutoReconnect(false);
    DEBUG_PRINT("WiFi Auto-Reconnect Disabled.");



    wifiManager.setStatusCallback(onWiFiStatus);
    wifiManager.setScanCallback(onWiFiScanComplete);
    wifiManager.begin();
    DEBUG_PRINT("WiFi Manager initialized.");

    DEBUG_PRINT("Setup complete!");
}


void loop() {
    M5.update();


    wifiManager.update();



    lv_timer_handler();



    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 1000) {
        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (lv_scr_act() == main_menu_screen) {
                updateTimeDisplay();
            } else if (lv_scr_act() == lock_screen) {
                updateLockScreenTime();
            }

            xSemaphoreGive(xGuiSemaphore);
        }
        lastTimeUpdate = millis();
    }
# 169 "C:/Users/nickd/Documents/PlatformIO/Projects/250313-072142-m5stack-cores3/src/Loss_Prevention_Log.ino"
    delay(5);
}