# System Patterns: Loss Prevention Log System (Updated)

## Architecture Overview
The Loss Prevention Log System is built on an event-driven architecture with a modular design, utilizing the M5Stack CoreS3 hardware. Key architectural patterns include:

1.  **Modular Design**: Code is separated into distinct modules (UI, WiFi, SD Logging, Time) with clear responsibilities, primarily managed through header (`.h`) and implementation (`.cpp`) files in `src/` and `lib/`.
2.  **Event-Driven Programming**: UI interactions (LVGL events) and system events (like WiFi status changes via callbacks) trigger specific actions and updates.
3.  **Concurrency (Limited)**:
    *   **LVGL Task**: A dedicated FreeRTOS task (`lvgl_task`) handles the LVGL timer handler (`lv_timer_handler`), ensuring UI rendering doesn't block the main loop excessively. This task likely runs on Core 1.
    *   **Main Loop**: The primary `loop()` function runs on the other core (likely Core 0), handling `M5.update()`, `wifiManager.update()`, and other periodic checks.
    *   **No Background WiFi Task**: The current `WiFiManager` implementation does *not* use a dedicated background task for WiFi operations. Scanning and connection logic are managed within the `WiFiManager::update()` method called from the main loop.
4.  **State Machine**: Used implicitly within `WiFiManager` to track connection states (`WiFiState` enum: DISABLED, DISCONNECTED, CONNECTING, CONNECTED, SCANNING).

## Key Components & Responsibilities

1.  **Main Application (`src/Loss_Prevention_Log.ino`)**:
    *   Entry point (`setup()`, `loop()`).
    *   Initializes hardware (M5Unified, Power, Speaker, RTC) and software modules (LVGL, WiFiManager, SDLogger, TimeUtils).
    *   Creates the `lvgl_task`.
    *   Main loop handles `M5.update()`, `wifiManager.update()`, and periodic UI time updates.
2.  **UI Module (`src/ui.h`, `src/ui.cpp`)**:
    *   Manages all LVGL screen creation, styling, and event handling for user interactions.
    *   Interacts with other modules to display data (logs, WiFi status) and trigger actions (save entry, connect WiFi, disconnect WiFi, forget WiFi).
    *   Includes specific screens like `createWiFiManagerScreen` which now handles saved network actions (connect, forget, disconnect).
3.  **WiFi Manager (`lib/WiFiManager/WiFiManager.h`, `lib/WiFiManager/WiFiManager.cpp`)**:
    *   Manages WiFi connection lifecycle (scan, connect, disconnect, forget, status monitoring).
    *   Handles saving/loading known networks via `Preferences`.
    *   Provides callbacks (`StatusCallback`, `ScanCallback`) to notify the main application/UI of state changes or scan results.
    *   **Operates synchronously within the `update()` method called from the main loop.**
4.  **WiFi Handler (`src/wifi_handler.h`, `src/wifi_handler.cpp`)**:
    *   Implements the callback functions (`onWiFiStatus`, `onWiFiScanComplete`) used by `WiFiManager`. These callbacks likely update UI elements or global state.
    *   Contains helper functions like `connectToWiFi` (called from UI) and `sendWebhook`.
5.  **SD Logger (`src/sd_logger.h`, `src/sd_logger.cpp`)**:
    *   Handles all interactions with the SD card via SPI.
    *   Provides functions for initializing the filesystem, saving log entries, and loading/parsing log entries.
6.  **Time Utilities (`src/time_utils.h`, `src/time_utils.cpp`)**:
    *   Manages RTC communication.
    *   Handles system time synchronization (from RTC and potentially NTP via `WiFiManager`).
    *   Provides timestamp formatting functions.
7.  **LVGL Task (`lvgl_task` in `Loss_Prevention_Log.ino`)**:
    *   Dedicated FreeRTOS task responsible for calling `lv_timer_handler()` periodically.
    *   Uses a semaphore (`xGuiSemaphore`) to protect access to LVGL functions from the main loop or other tasks if necessary (though current interaction seems limited).

## Design Patterns Employed

1.  **Callback**: Extensively used for handling asynchronous events like WiFi status changes (`StatusCallback`, `ScanCallback`) and LVGL widget interactions (`lv_event_cb_t`).
2.  **Singleton (Conceptual)**: Global instances like `wifiManager` (defined in `.ino`) act as single points of access for their respective functionalities.
3.  **State Pattern**: Implemented within `WiFiManager` to manage the different WiFi connection states.
4.  **Observer (Implicit)**: The callback mechanism acts as a form of the Observer pattern, where the UI or main application observes changes in the `WiFiManager` state.
5.  **Modular Programming**: Code is broken down into logical modules (`.h`/`.cpp` pairs).

## Data Flow Example (WiFi Scan)

1.  User presses the "Scan" button in the UI (`createWiFiManagerScreen` or `createWiFiScreen`).
2.  The button's LVGL event callback is triggered (running in `lvgl_task` context).
3.  The callback calls `wifiManager.startScan()`.
4.  `wifiManager.startScan()` sets the state to `WIFI_SCANNING`, sets `_scanInProgress = true`, and initiates an asynchronous scan (`WiFi.scanNetworks(true)`).
5.  The main `loop()` continues executing.
6.  Periodically, `loop()` calls `wifiManager.update()`.
7.  Inside `wifiManager.update()`, `updateState()` checks `WiFi.scanComplete()`.
8.  When the scan finishes (`scanComplete() >= 0`), `updateState()` processes the results, populates `_scanResults`, sets `_scanInProgress = false`, and calls the registered `_scanCallback` (`onWiFiScanComplete` in `wifi_handler.cpp`).
9.  `onWiFiScanComplete` (running in the main loop's context) receives the results and updates the UI list (`wifi_list` in `ui.cpp`).

## Error Handling

-   Error checking appears present in SD card operations and log parsing.
-   `WiFiManager` includes connection attempt limits and timeouts.
-   UI provides feedback messages (e.g., "WiFi Disabled", connection status).
-   Debug logging (`DEBUG_PRINT`, `DEBUG_PRINTF`) is used throughout via `globals.h`.

## Optimization Strategies

-   **LVGL Task**: Offloads LVGL rendering updates from the main loop.
-   **Memory**: Use of `Preferences` for settings, careful string handling (though `String` class is used). PSRAM enabled.
-   **Power**: ESP32 WiFi sleep mode enabled by default. Deep sleep implemented for low-power states.

This architecture provides a functional structure but relies heavily on the main loop for processing WiFi state changes. The separation of LVGL rendering into its own task helps maintain UI responsiveness.
