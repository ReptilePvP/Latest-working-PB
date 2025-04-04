# Project Component Overview

This document provides an explanation of the different source code files and configuration components within the Loss Prevention Log project.

## `src/Loss_Prevention_Log.ino` - The Orchestrator

Think of this `.ino` file as the main conductor of your project. It's the standard entry point for Arduino sketches. Its primary responsibilities are:

1.  **Initialization (`setup()`):**
    *   Starts the basic hardware components (Serial, M5Stack core features).
    *   Initializes core software libraries (LVGL, Preferences, WiFiManager).
    *   Calls initialization functions from other modules (`initFileSystem`, `initStyles`, `setSystemTimeFromRTC`).
    *   Creates a dedicated background task (`lvgl_task`) for LVGL updates.
2.  **Main Execution Loop (`loop()`):**
    *   Runs repeatedly after `setup()`.
    *   Calls `M5.update()` for hardware events.
    *   Calls `wifiManager.update()` for background WiFi tasks.
    *   Periodically updates the time display on the UI, using a semaphore (`xGuiSemaphore`) for safe access.
    *   Includes a small `delay()` for cooperative multitasking.
3.  **Includes:** Includes necessary headers (`.h`) from other modules.
4.  **Global Objects:** Defines the actual `wifiManager` object.

In essence, `Loss_Prevention_Log.ino` sets everything up and then runs a loop that keeps essential system components updated, triggers UI updates, and relies on other modules for specific functionalities.

## `src/globals.h` - The Central Hub

This header file acts as a central hub for definitions and declarations shared across multiple `.cpp` files.

1.  **Include Guards:** Prevents multiple inclusions.
2.  **Includes:** Includes standard Arduino/ESP32 libraries, C++ libraries, and crucial project headers like `m5gfx_lvgl.hpp` (which includes `lvgl.h`).
3.  **Debug Macros:** Provides `DEBUG_PRINT`, `DEBUG_PRINTF`, etc., for conditional serial output controlled by `DEBUG_ENABLED`.
4.  **Constants (`#define`):** Defines fixed values (Pins, I2C Addresses, Screen Dimensions, WiFi Settings, Task Parameters, etc.).
5.  **`extern` Declarations:** Declares global variables, objects (like `SPI_SD`, `xGuiSemaphore`, `wifiManager`), UI element pointers (`main_menu_screen`, etc.), and styles (`style_btn`, etc.) that are *defined* in other `.cpp` files. This allows modules to share access to the same instances.
6.  **Data Structures (`struct`):** Defines `WifiNetwork` and `LogEntry`.
7.  **Function Prototypes:** Declares functions defined in various `.cpp` files, allowing them to be called across modules.

`globals.h` acts as the "glue," providing common includes, constants, shared variable/object declarations, and function declarations needed for inter-module communication.

## `src/ui.h` / `src/ui.cpp` - UI Management (LVGL)

This module manages the User Interface using the LVGL library.

*   **`ui.h` (The UI Module's Public Interface):**
    *   Includes `globals.h`.
    *   Declares the public functions implemented in `ui.cpp`:
        *   `initStyles()`: Sets up LVGL styles.
        *   Screen Creation Functions (`createMainMenu`, `createSettingsScreen`, etc.): Functions to build each specific UI screen.
        *   UI Update Functions (`addStatusBar`, `updateTimeDisplay`, etc.): Functions to modify existing UI elements.
*   **`ui.cpp` (The UI Implementation):**
    *   Includes `ui.h`, `globals.h`, and headers from other modules it interacts with (`time_utils.h`, `sd_logger.h`, `wifi_handler.h`).
    *   *Defines* the global variables and UI element pointers declared `extern` in `globals.h`.
    *   *Implements* `initStyles()` using LVGL style functions.
    *   *Implements* the `create...` screen functions, using LVGL widgets (`lv_obj_create`, `lv_btn_create`, etc.) to build layouts. Assigns created screen objects to the global pointers.
    *   Adds **event callbacks** (`lv_obj_add_event_cb`) to buttons and other interactive elements. These callbacks handle user input and trigger actions (e.g., loading new screens, updating state variables, calling functions in other modules like `saveEntry` or `startWifiScan`).
    *   *Implements* UI update functions.

This module is the engine room of the UI, defining its look, state, and behavior.

## `src/sd_logger.h` / `src/sd_logger.cpp` - SD Card Interaction

This module handles all interactions with the SD card for logging.

*   **`sd_logger.h` (SD Card & Logging Interface):**
    *   Includes `globals.h`.
    *   Declares functions for:
        *   `initFileSystem()`: Initializes the SD card hardware and library.
        *   `appendToLog()` / `saveEntry()`: Writes entries to the log file (saveEntry also coordinates webhook).
        *   `parseTimestamp()` / `getFormattedEntry()`: Parses and formats log entries/timestamps.
        *   `loadAllLogEntries()`: Reads the log file into memory.
        *   `resetLogFile()`: Deletes the log file.
*   **`sd_logger.cpp` (SD Card Interaction Logic):**
    *   Includes necessary headers.
    *   *Defines* SD-specific globals (`SPIClass SPI_SD(HSPI);`, `fileSystemInitialized`).
    *   *Implements* `initFileSystem()`, including the crucial **SPI bus switching** logic (`SPI.end()`, `SPI_SD.begin()`, `SD.begin()`, `SPI_SD.end()`, `SPI.begin()`) to avoid conflicts between the SD card and the display. Handles initialization errors.
    *   *Implements* `appendToLog()`, performing the same SPI bus switching before/after opening the file (`SD.open(LOG_FILENAME, FILE_APPEND)`), writing (`file.println()`), and closing. Handles file operation errors and updates UI status.
    *   *Implements* `saveEntry()`, calling `appendToLog()` and then `sendWebhook()` if WiFi is connected. Updates UI status.
    *   *Implements* `parseTimestamp()` using `sscanf` to extract time from log lines.
    *   *Implements* `getFormattedEntry()` to create user-readable strings from `LogEntry` structs.
    *   *Implements* `loadAllLogEntries()`, performing SPI switching, reading the file line by line, parsing timestamps, and populating a vector of `LogEntry` structs.
    *   *Implements* `resetLogFile()`, performing SPI switching and calling `SD.remove()`.

This module encapsulates SD card access, especially the complex SPI bus management.

## `src/wifi_handler.h` / `src/wifi_handler.cpp` - WiFi and Network Management

This module manages WiFi connectivity and network operations, acting as an interface to the custom `WiFiManager` class.

*   **`wifi_handler.h` (WiFi and Network Interface):**
    *   Includes `globals.h`.
    *   Declares **callback functions** (`onWiFiStatus`, `onWiFiScanComplete`) to be registered with the `WiFiManager` class.
    *   Declares `sendWebhook()` to send log data externally.
    *   Declares `connectToWiFi()` to initiate a connection attempt.
*   **`wifi_handler.cpp` (WiFi Logic and Network Communication):**
    *   Includes necessary headers.
    *   *Implements* `onWiFiStatus()`: Updates the UI status bar based on the state reported by `WiFiManager`. Updates the WiFi loading screen if active.
    *   *Implements* `onWiFiScanComplete()`: Takes the GUI semaphore, updates the WiFi list UI with scan results provided by `WiFiManager`, handles memory allocation (`strdup`/`free`) for SSIDs stored in button user data, and adds click handlers to list items to trigger password entry (`showWiFiKeyboard`). Releases the semaphore.
    *   *Implements* `sendWebhook()`: Checks WiFi status, parses the log entry, constructs a JSON payload, and sends it via HTTPS POST to a hardcoded Zapier URL using `HTTPClient` and `WiFiClientSecure`. Includes basic error handling and status updates. Uses `client.setInsecure()`.
    *   *Implements* `connectToWiFi()`: Delegates the connection attempt to `wifiManager.connect(ssid, password)`. Deletes the WiFi keyboard UI element.

This module connects the application logic and UI to the underlying `WiFiManager` class and handles external communication.

## `src/time_utils.h` / `src/time_utils.cpp` - Time Management

This module handles time functions, primarily interacting with the M5Stack's Real-Time Clock (RTC).

*   **`time_utils.h` (Time Management Interface):**
    *   Includes `globals.h`.
    *   Declares functions:
        *   `getTimestamp()`: Gets current RTC time as a formatted string.
        *   `setSystemTimeFromRTC()`: Synchronizes the ESP32 system clock from the RTC.
        *   `save_time_to_rtc()`: Saves user-selected time (from global variables) to the RTC.
*   **`time_utils.cpp` (Time Management Logic):**
    *   Includes necessary headers (`sys/time.h` for `settimeofday`).
    *   *Implements* `getTimestamp()`: Reads date/time from `M5.Rtc`, converts to `struct tm`, formats using `strftime` ("DD-Mon-YYYY HH:MM:SS AM/PM").
    *   *Implements* `setSystemTimeFromRTC()`: Reads from `M5.Rtc`, converts to `struct tm`, converts to `time_t` using `mktime`, sets system clock using `settimeofday`.
    *   *Implements* `save_time_to_rtc()`: Reads selected date/time from global UI state variables, converts 12-hr format to 24-hr, calculates the day of the week (using Zeller's congruence), writes date/time to RTC using `M5.Rtc.setDate()` and `M5.Rtc.setTime()`, and then calls `setSystemTimeFromRTC()` to update the system clock immediately.

This module centralizes RTC interaction and time conversions/formatting.

## `platformio.ini` - Project Build Configuration

This file configures the PlatformIO build system.

*   Specifies the target platform (`espressif32`), board (`m5stack-cores3`), and framework (`arduino`).
*   Sets Serial Monitor options (`monitor_speed`, `monitor_filters`).
*   Lists **library dependencies** (`lib_deps`) like `M5CoreS3`, `M5Unified`, `M5GFX`, `lvgl`, etc., which PlatformIO automatically manages.
*   Defines **build flags** (`build_flags`) passed to the compiler, controlling C++ standard (`-std=c++11`), enabling PSRAM (`-DBOARD_HAS_PSRAM`), configuring LVGL includes/settings (`-DLV...`), defining screen dimensions, setting core debug level, and adding the `./include` directory to the include path.

It dictates how the project is compiled and linked for the specific hardware.

## `lib/m5gfx_lvgl/` - M5GFX/LVGL Bridge Library

This custom library connects M5Stack's graphics driver (`M5GFX`) to the LVGL UI library.

*   **`m5gfx_lvgl.hpp` (Interface):**
    *   Includes `lvgl.h`, `M5Unified.h`, `M5GFX.h`.
    *   Declares `extern SemaphoreHandle_t xGuiSemaphore;` making the GUI semaphore globally accessible.
    *   Declares the initialization function `m5gfx_lvgl_init()`.
*   **`m5gfx_lvgl.cpp` (Implementation):**
    *   *Defines* the `xGuiSemaphore` variable.
    *   *Implements* `m5gfx_lvgl_flush()`: The LVGL display flush callback. Uses `M5.Display.pushImageDMA()` to efficiently draw LVGL's rendered buffer onto the screen.
    *   *Implements* `m5gfx_lvgl_read()`: The LVGL input device read callback. Reads touch coordinates using `M5.Touch.getDetail()` and reports state/position to LVGL.
    *   *Implements* `my_tick_function()`: Provides a millisecond time source for LVGL.
    *   *Implements* `m5gfx_lvgl_init()`:
        *   Sets the LVGL tick function.
        *   Allocates display buffer(s) (currently one static buffer for partial render mode).
        *   Creates and configures the LVGL display driver, registering the `m5gfx_lvgl_flush` callback.
        *   Creates and configures the LVGL input driver, registering the `m5gfx_lvgl_read` callback.
        *   **Creates the `xGuiSemaphore` mutex** using `xSemaphoreCreateMutex()`.

This library provides the essential low-level glue for LVGL to work on the M5Stack display.

## `lib/WiFiManager/` - Custom WiFi Management Library

This custom library provides a class (`WiFiManager`) to handle WiFi connectivity, scanning, and network persistence.

*   **`WiFiManager.h` (Class Interface):**
    *   Defines the `WiFiState` enum and `NetworkInfo` struct.
    *   Defines the `WiFiManager` class public interface:
        *   Constructor/Destructor.
        *   `begin()`, `update()` methods.
        *   Configuration methods (`setStatusCallback`, `setScanCallback`, `setEnabled`).
        *   Connection methods (`connect`, `connectToBestNetwork`, `disconnect`, `isConnected`, etc.).
        *   Scanning methods (`startScan`, `isScanning`, `getScanResults`).
        *   Network Management methods (`addNetwork`, `removeNetwork`, `getSavedNetworks`, `loadSavedNetworks`, `saveNetworks`).
        *   State getters (`getState`, `getStateString`).
    *   Declares private members for internal state, buffers, callbacks, and constants.
*   **`WiFiManager.cpp` (Class Implementation):**
    *   *Implements* all the public methods declared in the header.
    *   `begin()` initializes WiFi, loads networks from Preferences.
    *   `update()` calls `updateState()`.
    *   `updateState()` contains the core state machine logic: handles scan completion (populating results, calling scan callback), connection timeouts/retries, automatic reconnection attempts after intervals when disconnected.
    *   Connection methods call underlying `WiFi` library functions (`WiFi.begin`, `WiFi.disconnect`).
    *   Scanning methods use asynchronous scanning (`WiFi.scanNetworks(true)`).
    *   Network management methods use the `Preferences` library to save/load SSIDs, passwords, and priorities under keys like "ssid0", "pass0", etc.
    *   `sortNetworksByPriority` sorts networks for connection attempts and before saving/removing overflow.
    *   `notifyStatus` calls the registered status callback function.

This class encapsulates WiFi logic, making it easier to manage connections and saved networks within the main application.