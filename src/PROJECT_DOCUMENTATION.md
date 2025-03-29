# Loss Prevention Log System Documentation

## Project Overview

The Loss Prevention Log system is an IoT device built on the M5Stack CoreS3 platform. It serves as a digital logging system for retail loss prevention, allowing staff to quickly document suspicious activities or incidents with standardized information including gender, clothing colors, and items of interest via a touch interface. This system replaces traditional paper-based logging with a streamlined, digital approach, improving efficiency and data consistency.

## Hardware Components

### Primary Hardware
- **M5Stack CoreS3**: The main computing and display unit featuring:
  - ESP32-S3 microcontroller
  - 2" IPS LCD touchscreen (320x240 resolution)
  - Built-in battery management
  - RTC (Real-Time Clock)
  - SD card slot for data storage
  - Wi-Fi connectivity
  - Power management via AXP2101 chip

### Supporting Hardware/Chips
- **AW9523 I/O Expander**: Used primarily for:
    - Interfacing with the touch panel's interrupt line.
    - Enabling wake-from-sleep functionality via its interrupt output connected to the CoreS3's GPIO21.
- **SD Card**: Required for storing log files (FAT32 format recommended).

*(Note: While the M5Stack Dual Button unit uses an AW9523, this application primarily utilizes the chip for touch/wake functions, not the physical buttons for input.)*

## Software Architecture

### Core Components

1.  **Main Application (`Loss_Prevention_Log.ino`)**
    - Handles the overall application flow and user interface.
    - Manages the LVGL graphics library (v9.x) for UI rendering.
    - Implements the loss prevention logging workflow.
    - Manages file system operations for log storage on the SD card.
    - Initializes all hardware components and sets up the system.
    - Processes user input from the touchscreen.

2.  **WiFi Management (`lib/WiFiManager/`)**
    - Custom WiFi connection management system running in a background task.
    - Handles network scanning, connection attempts, and reconnection logic.
    - Stores and manages saved networks (SSID/password).
    - Provides callbacks for connection status updates to the main application.
    - Includes visual feedback for connection attempts (loading screen).
    - Implements a state machine for robust WiFi connection management.

3.  **Card-Style UI Components**
    - Modern card-based interface design for menus and information display.
    - Consistent styling across different screens.
    - Enhanced button styles with pressed state feedback.
    - Visual hierarchy using cards for content grouping.
    - Touch targets sized for usability.

4.  **User Interface**
    - Built with LVGL (Light and Versatile Graphics Library) (v9.x).
    - Features multiple screens for different functions:
        - Loading Screen
        - Lock Screen (with time display)
        - Main menu (card-based)
        - Gender selection
        - Color selection (shirt, pants, shoes) - multi-select capable
        - Item selection (scrollable list)
        - Confirmation screen
        - WiFi configuration (scan results, saved networks, password entry)
        - Log viewing (tabbed by day, scrollable entries, details popup)
        - Settings screens (WiFi, Sound, Display, Date/Time, Power)
    - Visual feedback for WiFi connection attempts.
    - Status bar for messages.

5.  **Data Storage**
    - Uses SD card for log file storage.
    - Implements file operations (append, read, reset) with error handling.
    - Stores logs in a text-based format (`/loss_prevention_log.txt`) with timestamps.
    - Uses Preferences library for storing WiFi credentials and system settings (volume, brightness, etc.).

6.  **Time Management**
    - Uses the M5Stack CoreS3's built-in RTC (Real-Time Clock).
    - Allows manual setting of date and time via dedicated settings screens.
    - System time is initialized from RTC on startup.
    - Timestamps for logs are generated based on the current RTC time.
    *(Note: Automatic NTP synchronization is not currently implemented.)*

7.  **Power Management**
    - Dedicated power management screen accessible from the settings menu.
    - Three power options:
        - Power Off: Safely powers down the device using AXP2101 control (with countdown).
        - Restart: Restarts the device using `ESP.restart()` (with countdown).
        - Sleep Mode: Puts the device into deep sleep; wakes on touchscreen press (via AW9523 interrupt -> GPIO21).
    - Utilizes M5Stack CoreS3's power management capabilities (AXP2101).

8.  **Network Connectivity**
    - WiFi connection managed by the `WiFiManager` library.
    - Optional webhook functionality (`sendWebhook` function) to send log entries to a remote server (e.g., Zapier) when connected.
    - Visual connection status feedback on relevant screens.

## Functional Workflow

1.  **Startup Sequence**
    - Initialize hardware components (display, touch, SD card, power management).
    - Set up LVGL UI framework.
    - Initialize file system (SD card).
    - Initialize RTC and set system time.
    - Initialize WiFiManager (loads saved networks, starts background task).
    - Display loading screen, then lock screen.

2.  **Logging Process (After Unlock)**
    - Navigate from Main Menu -> New Entry.
    - Select gender (Male/Female).
    - Select shirt color(s).
    - Select pants color(s).
    - Select shoes color(s).
    - Select item(s) of interest from a list.
    - Review entry on confirmation screen.
    - Confirm and save entry.
    - Entry is timestamped and appended to `/loss_prevention_log.txt` on the SD card.
    - If WiFi is connected, attempt to send entry to the configured webhook URL.

3.  **Log Management**
    - Access via Logs card on the main menu.
    - View saved logs from the last 3 days, organized in tabs by date.
    - Entries within each day are sorted chronologically.
    - Tap an entry to view formatted details in a popup.
    - Option to reset (delete and recreate) the log file with confirmation.

4.  **Settings Management**
    - Access via Settings card on the main menu.
    - **WiFi:** Access WiFi Manager screen to view saved networks, scan for new networks, connect, or forget networks.
    - **Sound:** Enable/disable sound, adjust volume with slider and test tone.
    - **Display:** Adjust brightness with slider or presets (Low/Medium/High), toggle auto-brightness.
    - **Date & Time:** Manually set the date (Year/Month/Day) and time (Hour/Minute/AM-PM) using rollers.
    - **Power:** Access Power Management screen (Power Off, Restart, Sleep).

5.  **WiFi Management (Detailed)**
    - Access via Settings -> WiFi Settings.
    - View list of saved networks and connection status.
    - Tap a saved network to view details (SSID, Status, Signal Strength) and options (Connect/Disconnect, Forget).
    - Initiate a scan for available networks (navigates to scan results screen).
    - Select a scanned network to enter the password via an on-screen keyboard.
    - Connection attempts show a loading screen with status updates.

## UI Enhancements

### Card-Style UI
- Modern card-based interface design for content organization.
- Elevated appearance with subtle shadows and rounded corners.
- Consistent styling across all screens for visual coherence.
- Improved touch targets for better usability.
- Visual feedback on interaction (pressed states).
- Optimized for the M5Stack CoreS3 display size and resolution.
- Uses custom styles defined in `initStyles()`.

### Scrollable Lists
- Implemented in the WiFi manager screen, item selection, and log viewing screens.
- Uses standard LVGL list or container scrolling capabilities.
- Momentum scrolling enabled where appropriate.

### Non-Scrollable Headers
- Headers in various screens (logs, settings, etc.) remain fixed while content scrolls.
- Improves user experience by keeping titles visible.
- Implemented using separate header objects outside the scrollable content area.

### WiFi Connection Feedback
- Loading screen with spinner during connection attempts.
- Visual feedback for successful or failed connections.
- Automatic return to the WiFi manager screen after a successful connection or timeout.
- Clear status messages displayed on the loading screen.

## Key Files and Their Functions

### Main Application Files
- **`src/Loss_Prevention_Log.ino`**: Main application entry point, UI screen creation, event handling, core logic.
- **`lib/WiFiManager/WiFiManager.cpp`**: Implementation of WiFi management functionality (state machine, scanning, connection).
- **`lib/WiFiManager/WiFiManager.h`**: Header file defining the WiFiManager class and related structures.

### Configuration Files
- **`platformio.ini`**: Project build configuration, library dependencies.
- **`include/lv_conf.h`**: LVGL library configuration.

### Documentation Files
- **`README.md`**: Top-level overview and quick-start documentation.
- **`src/PROJECT_DOCUMENTATION.md`**: This detailed technical documentation.
- **`src/Back button.md`**: Code snippet example for creating a back button.

### Data Files (Runtime)
- **`/loss_prevention_log.txt`**: Main log file stored on the SD card.
- **Preferences Storage**: Used internally by the `Preferences` library to store:
    - `wifi_config` namespace: Saved WiFi networks and count.
    - `settings` namespace: Volume, brightness, sound enabled status, auto-brightness status.

## Important Functions (Illustrative List)

### UI Management
- `createLoadingScreen()`, `createLockScreen()`, `createMainMenu()`
- `createGenderMenu()`, `createColorMenuShirt()`, `createColorMenuPants()`, `createColorMenuShoes()`, `createItemMenu()`
- `createConfirmScreen()`, `createViewLogsScreen()`
- `createSettingsScreen()`, `createWiFiManagerScreen()`, `createNetworkDetailsScreen()`, `createWiFiScreen()`
- `createSoundSettingsScreen()`, `createBrightnessSettingsScreen()`, `createPowerManagementScreen()`
- `createDateSelectionScreen()`, `createTimeSelectionScreen()`
- `showWiFiLoadingScreen()`, `updateWiFiLoadingScreen()`
- `addStatusBar()`, `updateStatus()`

### WiFi Management (via `wifiManager` object)
- `wifiManager.begin()`: Initializes the WiFi manager.
- `wifiManager.update()`: Processes WiFi events (called in `loop()`).
- `wifiManager.startScan()`: Initiates a WiFi network scan.
- `wifiManager.connect()`: Attempts to connect to a specified network.
- `wifiManager.disconnect()`: Disconnects from the current network.
- `wifiManager.addNetwork()`: Saves a new network credential.
- `wifiManager.removeNetwork()`: Forgets a saved network.
- `wifiManager.getSavedNetworks()`: Retrieves the list of saved networks.
- `onWiFiStatus()`: Callback function to handle status updates from WiFiManager.
- `onWiFiScanComplete()`: Callback function to handle scan results from WiFiManager.

### Data Management
- `appendToLog()`: Appends a new entry to the log file on the SD card.
- `getFormattedEntry()`: Formats the raw log data for display.
- `getTimestamp()`: Gets the current timestamp from RTC for log entries.
- `listSavedEntries()`: Reads and parses log entries from the SD card (used by `createViewLogsScreen`).
- `saveEntry()`: Coordinates saving the entry locally and potentially sending the webhook.
- `sendWebhook()`: Sends entry data to the configured remote server URL.
- `parseTimestamp()`: Parses timestamp from a log entry string.

### System Functions
- `setup()`: Arduino setup function, initializes hardware and software components.
- `loop()`: Arduino main loop, handles M5Stack updates and LVGL tasks.
- `initFileSystem()`: Initializes the SD card file system.
- `initStyles()`: Initializes custom LVGL styles.
- `setSystemTimeFromRTC()`: Sets the ESP32 system time based on the M5Stack RTC.
- Power management functions within `createPowerManagementScreen` event handlers (interact with M5.Power, AXP2101, ESP-IDF sleep functions).
- Brightness/Volume functions within settings screens (interact with M5.Display, M5.Speaker, Preferences).
- `DEBUG_PRINT()`, `DEBUG_PRINTF()`: Debug logging macros.

### Touch and Input Handling
- LVGL's internal touch handling uses `m5gfx_lvgl` driver which interfaces with the touch controller.
- Swipe gestures (`handleSwipeLeft`, `handleSwipeVertical`) are implemented using LVGL event detection on screen objects.

## Hardware Interfaces

### Display & Touch
- 320x240 pixel IPS LCD touchscreen.
- Managed through LVGL library via the `m5gfx_lvgl` driver.
- Touch input handled by the driver, events processed by LVGL.

### SD Card
- Connected via SPI interface.
- Pins defined in the main sketch (`SD_SPI_SCK_PIN`, etc.).
- Uses a dedicated `SPIClass` instance (`SPI_SD`) to avoid conflicts with the display's SPI bus.
- Managed using the standard Arduino `SD` library.

### Power Management (AXP2101)
- Communicates via I2C.
- Used for battery level monitoring, power control (power off).
- Managed via `M5.Power` functions in the M5Unified library.

### I/O Expander (AW9523)
- Communicates via I2C (address 0x58).
- INT line connected to CoreS3 GPIO21.
- Used to detect touch panel interrupt for wake-from-sleep.

## Development References

### M5Stack CoreS3 Resources
- M5Stack CoreS3 Official Documentation
- M5Stack CoreS3 Pinout Diagram
- M5Stack GitHub Repository (for M5Unified, M5GFX)

### Libraries
- LVGL Documentation (v9): [https://docs.lvgl.io/9.0/](https://docs.lvgl.io/9.0/)
- PlatformIO Documentation: [https://docs.platformio.org/](https://docs.platformio.org/)
- ESP-IDF Documentation (for sleep functions, etc.): [https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)

## Future Development Considerations

### Potential Enhancements
1.  **Cloud Integration**: Implement robust cloud sync for logs (MQTT, REST API).
2.  **Advanced UI**: Add log search/filtering, themes, photo capture (requires camera).
3.  **Hardware Expansion**: Support for barcode/RFID scanners.
4.  **Performance**: Optimize memory usage, improve SD card access speed.
5.  **Security**: Encrypt logs on SD card, add user PIN/authentication for access.
6.  **NTP Sync**: Implement optional automatic time synchronization over WiFi.

### Known Limitations
1.  WiFi usage significantly impacts battery life.
2.  SD card write speed can be a bottleneck if logging is extremely frequent.
3.  ESP32-S3 RAM limitations may restrict future UI complexity.

## Troubleshooting Guide

### Common Issues
1.  **WiFi Connection Problems**
    - **Problem**: Unable to connect to WiFi networks.
    - **Solution**: Check credentials, signal strength, ensure network is 2.4GHz. Use WiFi Manager screen to forget/re-add network. Check serial monitor for `WiFiManager` debug messages.
    - **Technical**: Managed by `WiFiManager` state machine. Failures often logged to Serial.

2.  **SD Card Issues**
    - **Problem**: "SD card initialization failed" or unable to save/read logs.
    - **Solution**: Ensure card is inserted correctly, formatted as FAT32. Try a different card. Check serial monitor for `initFileSystem` errors. Ensure `SPI_SD` pins are correct.
    - **Technical**: Uses dedicated SPI bus (`SPI_SD`). `initFileSystem` attempts initialization multiple times. Errors logged to Serial. Log file is `/loss_prevention_log.txt`.

3.  **Touchscreen Unresponsive or Inaccurate**
    - **Problem**: Touch input not detected or misaligned.
    - **Solution**: Clean screen. Restart device. Check for physical damage.
    - **Technical**: Handled by `m5gfx_lvgl` driver and LVGL. Calibration is usually handled by the driver.

4.  **System Freezes or Crashes**
    - **Problem**: Device unresponsive or restarts unexpectedly.
    - **Solution**: Monitor serial output for error messages or stack traces (requires `esp32_exception_decoder` filter in PlatformIO). Check for potential memory issues (use `heap_caps_get_free_size`). Simplify code temporarily to isolate the issue. Ensure tasks (like WiFiManager, LVGL) are running correctly.
    - **Technical**: Often related to memory allocation errors, stack overflows, or blocking operations in event handlers or the main loop.

5.  **Time Incorrect**
    - **Problem**: Displayed time is wrong or doesn't update.
    - **Solution**: Use Settings -> Date & Time to set the correct time manually. Ensure RTC battery is functional (if time resets after power loss).
    - **Technical**: Relies on the M5Stack's RTC. `setSystemTimeFromRTC` updates ESP32 time from RTC.

6.  **Power Management Problems**
    - **Problem**: Device doesn't power off, restart, or sleep/wake correctly.
    - **Solution**: Check serial logs during power operations. Ensure AXP2101/AW9523 are communicating (no I2C errors). Verify GPIO21 is configured for wake-up.
    - **Technical**: Power off uses AXP2101 registers. Restart uses `ESP.restart()`. Sleep uses `esp_deep_sleep_start` with EXT0 wake-up on GPIO21 triggered by AW9523.

7.  **Log File Corruption**
    - **Problem**: Log file (`/loss_prevention_log.txt`) is unreadable or contains garbage data.
    - **Solution**: Use the "Reset" button in the Log Viewing screen (with caution, data loss). Avoid abrupt power removal during operation. Ensure proper SD card handling (unmount before removal if possible, though not programmatically simple here).
    - **Technical**: Logs are appended as text. Corruption usually occurs due to power loss during writes or SD card physical issues.

### General Debugging Tips
-   **Serial Monitor**: Essential for viewing `DEBUG_PRINT` messages and system errors (set `monitor_speed = 115200`). Use `monitor_filters = esp32_exception_decoder` in `platformio.ini`.
-   **Debug Macros**: Enable/disable `DEBUG_ENABLED` flag in `Loss_Prevention_Log.ino` to control verbose output.
-   **Isolate Issues**: Comment out sections of code (e.g., WiFi, specific UI elements) to narrow down problems.
-   **Check Connections**: Ensure SD card is seated properly.
-   **Check Power**: Use a reliable power source.
-   **Update Libraries**: Ensure M5Stack libraries and LVGL are reasonably up-to-date via PlatformIO.
