# Technical Context: Loss Prevention Log System (Updated)

## Platform
- **Hardware**: M5Stack CoreS3
- **Microcontroller**: ESP32-S3

## Development Environment
- **Framework**: Arduino framework for ESP32
- **Build System**: PlatformIO

## Primary Technologies

### 1. M5Stack CoreS3
- 2" IPS LCD touchscreen (320x240 resolution)
- Built-in battery management (AXP2101)
- RTC (Real-Time Clock)
- SD card slot
- Wi-Fi connectivity
- Power management via AXP2101 and AW9523 chips

### 2. ESP32-S3 Microcontroller
- Dual-core processor
- Wi-Fi and Bluetooth capabilities
- Low power consumption features (including deep sleep)

### 3. LVGL (Light and Versatile Graphics Library)
- **Version**: ^9.2.2 (as per `platformio.ini`)
- Used for creating the graphical user interface.
- LVGL timer handler (`lv_timer_handler`) runs in a dedicated FreeRTOS task (`lvgl_task`) pinned to a specific core (likely Core 1) for responsiveness.

## Libraries and Dependencies (`platformio.ini`)

1.  **m5stack/M5CoreS3**: ^1.0.1
2.  **m5stack/M5Unified**: ^0.2.5 (Unified library for M5Stack hardware abstraction)
3.  **m5stack/M5GFX**: ^0.2.6 (Graphics library, used by M5Unified/LVGL integration)
4.  **lvgl/lvgl**: ^9.2.2 (Core graphics library)
5.  **fbiego/ESP32Time**: ^2.0.6 (Time management)
6.  **adafruit/Adafruit BusIO**: ^1.17.0 (Likely a dependency for other libraries)
7.  **ESP32 Arduino Core Libraries**: Standard libraries for WiFi, Preferences, SPI, etc.

## Key Modules (`src/` and `lib/`)

1.  **Main Application (`src/Loss_Prevention_Log.ino`)**: Entry point, setup, main loop, orchestrates module initialization and updates.
2.  **UI (`src/ui.h`, `src/ui.cpp`)**: Implements all LVGL screens, styles, widgets, and navigation logic. Handles user interactions for WiFi management (connect/disconnect/forget saved networks) within `createWiFiManagerScreen`.
3.  **WiFi Manager (`lib/WiFiManager/WiFiManager.h`, `lib/WiFiManager/WiFiManager.cpp`)**: Handles WiFi state (connect, disconnect, forget, scan), saves/loads networks using `Preferences`. Operates within the main application loop via its `update()` method (single-threaded design).
4.  **WiFi Handler (`src/wifi_handler.h`, `src/wifi_handler.cpp`)**: Contains callbacks (`onWiFiStatus`, `onWiFiScanComplete`) used by `WiFiManager` and potentially other WiFi-related utility functions (e.g., `connectToWiFi`, `sendWebhook`).
5.  **SD Logger (`src/sd_logger.h`, `src/sd_logger.cpp`)**: Manages SD card initialization and log file read/write operations via SPI.
6.  **Time Utilities (`src/time_utils.h`, `src/time_utils.cpp`)**: Handles RTC interaction, system time setting, NTP synchronization, and timestamp formatting.
7.  **Globals (`src/globals.h`)**: Defines shared constants, global variables (declared `extern`), and potentially forward declarations.

## Hardware Interfaces

1.  **Display**: 320x240 IPS LCD touchscreen, managed via M5Unified/M5GFX and LVGL.
2.  **SD Card**: Connected via SPI interface (Pins defined in `sd_logger.cpp` or `globals.h`).
3.  **Power/RTC**: Managed via I2C using AXP2101 chip.
4.  **Touch/Wake**: Touch input via FT6336 (I2C), wake-up from deep sleep via AW9523 (I2C) interrupt connected to GPIO 21.
5.  **Speaker**: Integrated speaker controlled via M5Unified API.

## Technical Constraints

1.  **Memory Limitations**: ESP32-S3 requires efficient memory usage, especially with LVGL. PSRAM is enabled (`-DBOARD_HAS_PSRAM`).
2.  **Power Management**: Battery-powered operation necessitates efficient code and use of sleep modes (deep sleep implemented).
3.  **Single-Threaded WiFi**: Current `WiFiManager` runs in the main loop; complex or blocking WiFi operations could potentially impact UI responsiveness if not handled carefully within the `update()` cycle or callbacks.
4.  **Storage**: SD card speed and capacity limitations.

## Development Workflow

1.  Code development in PlatformIO.
2.  Compilation and flashing via PlatformIO.
3.  Debugging via Serial Monitor (`monitor_speed = 115200`) with ESP32 exception decoding. `CORE_DEBUG_LEVEL=5` enabled for verbose logging.
