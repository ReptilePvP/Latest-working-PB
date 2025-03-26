# Loss Prevention Log System Documentation

## Project Overview

The Loss Prevention Log system is an IoT device built on the M5Stack CoreS3 platform with M5Stack Dual Button & Key unit for input. It serves as a digital logging system for retail loss prevention, allowing staff to quickly document suspicious activities or incidents with standardized information including gender, clothing colors, and items of interest. This system replaces traditional paper-based logging with a streamlined, digital approach, improving efficiency and data consistency.

## Hardware Components

### Primary Hardware
- **M5Stack CoreS3**: The main computing and display unit featuring:
  - ESP32-S3 microcontroller
  - 2" IPS LCD touchscreen (320x240 resolution)
  - Built-in battery management
  - RTC (Real-Time Clock)
  - SD card slot for data storage
  - Wi-Fi connectivity
  - Power management via AXP2101 and AW9523 chips

### Input Peripherals
- **M5Stack Dual Button & Key unit**: Provides physical button inputs for navigation and interaction
  - Used for scrolling through menus and confirming selections
  - Complements the touchscreen interface
  - I2C interface (address 0x58) for communication with the CoreS3.

## Software Architecture

### Core Components

1. **Main Application (`Loss_Prevention_Log.ino`)**
   - Handles the overall application flow and user interface
   - Manages the LVGL graphics library for UI rendering
   - Implements the loss prevention logging workflow
   - Manages file system operations for log storage
   - Initializes all hardware components and sets up the system.
   - Processes user input from the touchscreen and physical buttons.

2. **WiFi Management (`WiFiManager.cpp` & `WiFiManager.h`)**
   - Custom WiFi connection management system
   - Handles network scanning, connection, and reconnection
   - Stores and prioritizes saved networks
   - Provides callbacks for connection status updates
   - Includes visual feedback for connection attempts with loading screen
   - Implements a state machine for robust WiFi connection management.

3. **Screen Transitions (`screen_transition.h`)**
   - Provides smooth and consistent transitions between screens
   - Implements multiple transition types:
     - Fade transitions for subtle screen changes
     - Slide transitions (left/right/up/down) for directional navigation
     - Zoom transitions for emphasis on new screens
     - Over transitions for overlay screens
   - Configurable animation duration and easing functions
   - Optimized for M5Stack CoreS3 performance

4. **Card-Style UI Components**
   - Modern card-based interface design
   - Consistent styling across all screens
   - Enhanced button styles with pressed state animations
   - Improved visual hierarchy with cards for content grouping
   - Better touch target sizing for improved usability
   - Responsive layout adapting to screen orientation

5. **User Interface**
   - Built with LVGL (Light and Versatile Graphics Library) (8.4.0)
   - Features multiple screens for different functions:
     - Main menu
     - Gender selection
     - Color selection (shirt, pants, shoes)
     - Item selection
     - Confirmation screen
     - WiFi configuration with loading screen
     - Log viewing
     - Settings screens (sound, brightness)
     - Power management screen with power off, restart, and sleep options
   - Optimized scrollable lists with non-scrollable headers and individual items
   - Visual feedback for WiFi connection attempts
   - Smooth transitions between screens for improved user experience

6. **Data Storage**
   - Uses SD card for log file storage
   - Implements file operations with error handling
   - Stores logs in a text-based format with timestamps
   - Uses a simple text file (`log.txt`) for storing log entries.

7. **Time Synchronization**
   - NTP (Network Time Protocol) for internet time synchronization
   - Fallback to RTC when internet is unavailable
   - Time display in 12-hour format with AM/PM

8. **Power Management**
   - Dedicated power management screen accessible from main menu
   - Three power options:
     - Power Off: Safely powers down the device with 3-second countdown
     - Restart: Restarts the device with 3-second countdown
     - Sleep Mode: Puts device into deep sleep until touched
   - Utilizes M5Stack CoreS3's power management capabilities
   - Implements hardware-level power control via AXP2101 chip for power operations
   - Uses AW9523 chip for touch wake-up functionality in sleep mode
   - Touch wake-up functionality using GPIO21 as external wake-up source

9. **Network Connectivity**
   - WiFi connection for time synchronization
   - Optional webhook functionality for remote logging
   - Enhanced WiFi scanning with proper state management
   - Visual connection status feedback

## Functional Workflow

1. **Startup Sequence**
   - Initialize hardware components (display, touch, SD card, buttons)
   - Set up LVGL UI framework
   - Load saved WiFi networks and attempt connection
   - Synchronize time with NTP if connected, otherwise use RTC.
   - Display main menu

2. **Logging Process**
   - Select gender (Male/Female)
   - Select clothing colors (shirt, pants, shoes)
   - Select items of interest
   - Confirm and save entry
   - Entry is timestamped and saved to SD card
   - Optional: Send entry to webhook if configured

3. **Log Management**
   - View saved logs with pagination
   - Logs are displayed with timestamps
   - Sorted by recency (newest first)
   - Non-scrollable headers for better navigation
   - Log entries are displayed in reverse chronological order.

4. **Settings Management**
   - WiFi configuration with visual connection feedback
   - Sound settings (on/off, volume)
   - Brightness control (low, medium, high, custom)
   - Time settings (automatic via NTP or manual)
   - Power management options:
     - Power Off with countdown
     - Restart with countdown
     - Sleep Mode with touch wake-up

5. **WiFi Management**
   - Scan for available networks
   - Connect to selected networks with loading screen feedback
   - Manage saved networks with scrollable list
   - Prioritize networks for automatic connection
   - Network scanning and connection are managed by a state machine.

## UI Enhancements

### Card-Style UI
- Modern card-based interface design for content organization
- Elevated appearance with subtle shadows and rounded corners
- Consistent styling across all screens for visual coherence
- Improved touch targets for better usability
- Visual feedback on interaction (pressed states, animations)
- Optimized for the M5Stack CoreS3 display size and resolution
- Uses custom styles defined in `initStyles()`

### Screen Transitions
- Smooth animated transitions between screens
- Multiple transition types for different navigation contexts:
  - Slide Left/Right: For forward/backward navigation
  - Slide Up/Down: For hierarchical navigation
  - Fade: For subtle screen changes
  - Zoom In/Out: For emphasis on new screens or returning to previous screens
  - Over: For overlaying screens.
- Consistent navigation patterns:
  - Forward navigation uses slide left or zoom in
  - Backward navigation uses slide right or zoom out
  - Settings screens use slide up transitions
  - Confirmation screens use fade transitions
- Configurable animation duration (default: 300ms)
- Performance optimizations for smooth animations

### Scrollable Lists
- Implemented in the WiFi manager screen for saved networks list and the log viewing screen.
- Individual network items are non-scrollable for better user experience
- Proper padding between items for visual separation
- Uses `lv_obj_set_scroll_dir()` and `lv_obj_clear_flag()` for proper scrolling behavior.

### Non-Scrollable Headers
- Headers in various screens (logs, settings, etc.) are set as non-scrollable
- Improves user experience by keeping titles visible during scrolling
- Implemented using `lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE)`

### WiFi Connection Feedback
- Loading screen with spinner during connection attempts
- Visual feedback for successful or failed connections
- Automatic return to WiFi manager after successful connection
- Manual return option for failed connections
- Clear status messages displayed on the loading screen.

## Key Files and Their Functions

### Main Application Files
- **`Loss_Prevention_Log.ino`**: Main application entry point and core functionality
- **`WiFiManager.cpp`**: Implementation of WiFi management functionality
- **`WiFiManager.h`**: Header file defining the WiFiManager class and related structures
- **`screen_transition.h`**: Implementation of screen transition effects and animations

### Documentation Files
- **`README.md`**: Overview and quick-start documentation
- **`PROJECT_DOCUMENTATION.md`**: This detailed technical documentation
- **`References.md`**: Reference materials and function listings
- **`Card Style UI.md`**: Implementation details for the card-style UI
- **`SCREEN TRANSITIONS IMPLEMENTATION GUIDE.md`**: Guide for implementing screen transitions
- **`Back button.md`**: Code snippet for creating a back button.

### Data Files
- **`log.txt`**: Main log file stored on the SD card
- **`wifi_config`**: Saved in Preferences storage for WiFi credentials
- **`settings`**: Saved in Preferences storage for system settings.

## Important Functions

### UI Management
- `createMainMenu()`: Creates the main application menu
- `createGenderMenu()`: Creates the gender selection screen
- `createColorMenuShirt()`, `createColorMenuPants()`, `createColorMenuShoes()`: Color selection screens
- `createItemMenu()`: Creates the item selection screen
- `createConfirmScreen()`: Creates the confirmation screen
- `createViewLogsScreen()`: Creates the log viewing screen with non-scrollable header
- `createSettingsScreen()`: Creates the settings menu
- `createWiFiManagerScreen()`: Creates the WiFi manager screen with scrollable network list
- `showWiFiLoadingScreen()`: Shows loading screen during WiFi connection attempts
- `updateWiFiLoadingScreen()`: Updates the WiFi loading screen based on connection results
- `createBrightnessSettingsScreen()`: Creates the brightness settings screen.
- `createSoundSettingsScreen()`: Creates the sound settings screen.
- `createPowerManagementScreen()`: Creates the power management screen with power off, restart, and sleep options

### Screen Transitions
- `load_screen_with_animation()`: Main function for loading screens with transitions
- `transition_fade_anim()`: Handles fade transition animations
- `transition_slide_anim()`: Handles slide transition animations
- `transition_zoom_anim()`: Handles zoom transition animations
- `transition_over_anim()`: Handles overlay transition animations
- `transition_anim_ready_cb()`: Callback function when animations complete

### WiFi Management
- `WiFiManager::connect()`: Connects to a specific WiFi network
- `WiFiManager::startScan()`: Initiates a WiFi network scan
- `WiFiManager::update()`: Updates the WiFi state machine to process events and state changes
- `WiFiManager::addNetwork()`: Adds a network to saved networks
- `scanNetworks()`: Scans for available WiFi networks
- `connectToSavedNetworks()`: Attempts to connect to previously saved networks
- `onWiFiStatus()`: Callback for WiFi connection status updates
- `loadSavedNetworks()`: Loads saved WiFi credentials.
- `saveNetworks()`: Saves WiFi credentials.

### Data Management
- `appendToLog()`: Appends a new entry to the log file
- `getFormattedEntry()`: Formats the entry with all selected attributes
- `getTimestamp()`: Gets the current timestamp for log entries
- `listSavedEntries()`: Lists all saved log entries
- `saveEntry()`: Saves the current entry to storage
- `sendWebhook()`: Sends entry data to remote server.
- `parseTimestamp()`: Parses timestamp from log entry.

### System Functions
- `initFileSystem()`: Initializes the SD card file system
- `syncTimeWithNTP()`: Synchronizes time with NTP servers
- `updateBatteryIndicator()`: Updates the battery level indicator
- `updateWifiIndicator()`: Updates the WiFi connection indicator
- `loadSettings()`: Loads saved settings from Preferences.
- `saveSettings()`: Saves settings to Preferences.
- `updateBrightness()`: Updates display brightness.
- `toggleSound()`: Toggles sound on/off.
- `adjustVolume()`: Adjusts system volume.
- `toggleAutoBrightness()`: Toggles auto-brightness feature.
- `releaseSPIBus()`: Releases SPI bus for other operations.
- `setSystemTimeFromRTC()`: Sets system time from M5Stack RTC.
- `println_log()`: Helper for logging to Serial and Display.
- `printf_log()`: Formatted logging to Serial and Display.
- `DEBUG_PRINT()`: Debug logging macro.
- `DEBUG_PRINTF()`: Formatted debug logging macro.

### Touch and Input Handling
- `my_disp_flush()`: Display driver flush callback.
- `my_touchpad_read()`: Touch input driver callback.
- `handleSwipeLeft()`: Handles left swipe gesture.
- `handleSwipeVertical()`: Handles vertical swipe gesture.

## Hardware Interfaces

### Display
- 320x240 pixel IPS LCD touchscreen
- Managed through LVGL library
- Touch input handled by `my_touchpad_read()` function
- Uses `my_disp_flush()` for display driver flush.

### SD Card
- Connected via SPI interface
- Pins defined in the main sketch:
  - SCK: 36
  - MISO: 35
  - MOSI: 37
  - CS: 4
- Uses `initFileSystem()` to initialize the SD card.

### Button Interface
- M5Stack Dual Button & Key unit connected via I2C
- I2C address: 0x58 (AW9523 chip)
- Used for navigation and selection
- Provides physical buttons for user input.

## Development References

### M5Stack CoreS3 Resources
- M5Stack CoreS3 Official Documentation
- M5Stack CoreS3 Pinout Diagram
- M5Stack CoreS3 GitHub Repository

### M5Stack Dual Button & Key Unit Resources
- Dual Button Unit Documentation
- Key Unit Documentation

### Libraries
- M5Unified: Unified library for M5Stack devices
- M5GFX: Graphics library for M5Stack
- LVGL: Light and Versatile Graphics Library
- ESP32Time: Time management library for ESP32

## Future Development Considerations

### Potential Enhancements
1. **Cloud Integration**
   - Implement full cloud synchronization of logs for backup and remote access.
   - Add user authentication for secure access to cloud-based logs.
   - Explore integration with existing security systems via APIs.
   - Implement two-way synchronization to allow remote log management.

2. **Advanced UI Features**
   - Add photo capture capability using an ESP32-CAM module or similar.
   - Implement search functionality for logs, allowing users to quickly find specific entries.
   - Add filtering options for log viewing (e.g., by date, gender, item).
   - Implement multiple UI themes for customization.
   - Add customizable menus to allow users to tailor the interface.
   - Enhance the visualization of log data with charts or graphs.

3. **Hardware Expansion**
   - Support for additional M5Stack modules (e.g., RFID reader, barcode scanner).
   - Barcode scanner integration for quick item identification.
   - RFID reader support for tracking tagged items.
   - Voice notes using I2S microphone for detailed incident descriptions.

4. **Performance Optimizations**
   - Battery life improvements through more aggressive power management.
   - Memory usage optimization to reduce RAM consumption.
   - Faster UI rendering by optimizing LVGL configurations and code.
   - Optimize SD card access for faster log saving and retrieval.

5. **Security Enhancements**
    - Encrypted log storage to protect sensitive data.
    - User authentication to prevent unauthorized access.
    - Role-based access control to limit access to certain features.

6. **Advanced Data Analysis**
    - Implement basic data analysis features to identify trends in loss prevention data.
    - Generate reports based on the collected data.

### Known Limitations
1. Limited battery life when WiFi is enabled due to power consumption.
2. SD card operations can be slow for large log files, especially with frequent writes.
3. Touch interface may require calibration in some environments or with certain screen protectors.
4. The ESP32-S3 has limited RAM, which can restrict the complexity of the UI and the amount of data that can be processed.

## Troubleshooting Guide

## Troubleshooting Guide

### Common Issues
1. **WiFi Connection Problems**
   - **Problem**: Unable to connect to WiFi networks.
   - **Solution**:
     - Check that WiFi credentials (SSID and password) are entered correctly.
     - Ensure `WiFiManager::update()` is called in the main loop for proper state management.
     - Verify the WiFi network is within range and the signal strength is adequate.
     - Check that the network is using 2.4GHz, as the ESP32-S3 does not support 5GHz.
     - Try resetting saved networks if persistent issues occur (delete `wifi_config` in Preferences).
   - **Technical**: The WiFi connection process is managed by the `WiFiManager` class, which includes timeout handling and reconnection logic.

2. **SD Card Issues**
   - **Problem**: Unable to read or write to the SD card.
   - **Solution**:
     - Ensure the SD card is properly inserted into the slot.
     - Verify the SD card is formatted as FAT32.
     - Try a different SD card to rule out a faulty card.
     - Check for physical damage to the SD card or the SD card slot.
     - Ensure the correct SPI pins are used for the SD card interface (SCK: 36, MISO: 35, MOSI: 37, CS: 4).
     - Check that `initFileSystem()` is called during the startup sequence.
     - If the issue persists, try reformatting the SD card.
   - **Technical**: The `initFileSystem()` function initializes the SD card, and file operations are handled using standard file I/O functions. Errors during file operations are logged to the serial monitor.

3. **Touchscreen Unresponsive or Inaccurate**
   - **Problem**: Touch input is not detected or is detected in the wrong location.
   - **Solution**:
     - Ensure the touchscreen is clean and free of debris.
     - Avoid using excessive force on the touchscreen.
     - Check for any physical damage to the touchscreen.
     - Verify that the `my_touchpad_read()` function is correctly implemented and called.
     - Check for any interference from external electromagnetic sources.
   - **Technical**: Touch input is handled by the `my_touchpad_read()` function, which is called by the LVGL library.

4. **System Freezes or Crashes**
   - **Problem**: The system becomes unresponsive or restarts unexpectedly.
   - **Solution**:
     - Check for memory leaks or excessive memory usage.
     - Review the code for any infinite loops or blocking operations.
     - Ensure that all hardware components are properly initialized.
     - Check for any conflicts between different libraries or components.
     - Reduce the complexity of the UI or the amount of data being processed.
     - Monitor the serial output for any error messages or stack traces.
   - **Technical**: System stability is dependent on proper memory management and error handling. The ESP32-S3 has limited RAM, so memory leaks or inefficient code can lead to crashes.

5. **Button Input Not Detected**
   - **Problem**: The M5Stack Dual Button & Key unit buttons are not responding.
   - **Solution**:
     - Verify that the unit is properly connected to the CoreS3.
     - Ensure the I2C address (0x58) is correct.
     - Check for any physical damage to the button unit or the I2C connection.
     - Verify that the AW9523 chip is correctly initialized.
     - Check for any I2C bus conflicts with other devices.
   - **Technical**: The button unit communicates via I2C, and input is handled by reading the button states from the AW9523 chip.

6. **Time Synchronization Issues**
    - **Problem**: The system time is incorrect or not updating.
    - **Solution**:
        - If using NTP, ensure a stable WiFi connection is established.
        - Verify that the NTP server is reachable.
        - Check the time zone settings if applicable.
        - If using RTC, ensure the RTC battery is not depleted.
        - Check that `syncTimeWithNTP()` is called after a successful WiFi connection.
        - If using RTC only, ensure `setSystemTimeFromRTC()` is called at startup.
    - **Technical**: Time synchronization is handled by the `syncTimeWithNTP()` function, which uses the `sntp` library. The RTC is used as a fallback when WiFi is unavailable.

7. **Power Management Problems**
    - **Problem**: Device does not power off, restart, or enter sleep mode correctly.
    - **Solution**:
        - Ensure the AXP2101 chip is correctly initialized.
        - Check for any conflicts with other power-related settings.
        - Verify that the `createPowerManagementScreen()` is correctly implemented.
        - Check that the correct functions are called for power off, restart, and sleep.
        - Ensure that touch wake-up is properly configured using GPIO21.
    - **Technical**: Power management is handled by the AXP2101 and AW9523 chips. The `powerOff()`, `restart()`, and `sleep()` functions control the power states.

8. **Log File Corruption**
    - **Problem**: Log file (`log.txt`) is corrupted or unreadable.
    - **Solution**:
        - Check for errors during SD card operations.
        - Avoid abruptly removing power from the device while writing to the SD card.
        - If the file is corrupted, try deleting it and allowing the system to create a new one.
        - Implement more robust error handling during file operations.
        - Consider using a more robust file system if data integrity is critical.
    - **Technical**: Log entries are appended to `log.txt` using standard file I/O functions. File corruption can occur if power is lost during a write operation.

### General Debugging Tips

-   **Serial Monitor**: Use the serial monitor to view debug messages, error logs, and system status.
-   **Debug Macros**: Utilize the `DEBUG_PRINT()` and `DEBUG_PRINTF()` macros for conditional debugging output.
-   **Code Review**: Carefully review the code for any logical errors or potential issues.
-   **Simplify**: If encountering complex issues, try simplifying the code to isolate the problem.
-   **Divide and Conquer**: Break down complex problems into smaller, more manageable parts.
- **Check Connections**: Make sure all connections are secure.
- **Check Power**: Make sure the device is getting enough power.
- **Check for Updates**: Make sure all libraries are up to date.
- **Check for Compatibility**: Make sure all libraries are compatible with each other.

