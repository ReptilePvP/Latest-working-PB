# Loss Prevention Log System for M5Stack CoreS3

## Overview
This project implements a loss prevention logging system for retail environments using the M5Stack CoreS3 hardware. The application allows store employees to quickly log incidents with details including gender, clothing colors (shirt, pants, shoes), and items of interest using a touch-based interface. Logs are stored locally on an SD card and can be optionally sent via webhook.

## Key Features
- Intuitive touch-based user interface (LVGL v9) with card-style design
- Incident logging workflow: Gender -> Shirt Color(s) -> Pants Color(s) -> Shoes Color(s) -> Item
- Multi-color selection for clothing items
- Local log storage on SD card (`/loss_prevention_log.txt`) with timestamps
- Log viewing screen with entries sorted by date (last 3 days) and time
- WiFi connectivity management using a custom `WiFiManager` library:
    - Network scanning and connection
    - Saved network management (connect, forget)
    - Status indicators
- Settings Menu:
    - WiFi configuration
    - Sound settings (enable/disable, volume)
    - Display brightness control (slider, presets, auto-brightness toggle)
    - Date and Time configuration (manual setting via RTC)
- Power Management Screen:
    - Power Off (via AXP2101)
    - Restart
    - Sleep Mode (with touch wake-up via AW9523/GPIO21)
- Optional webhook integration (e.g., Zapier) to send log entries over WiFi
- Status bar for system messages
- Loading and Lock screens

## Hardware Requirements
- M5Stack CoreS3
- SD Card (FAT32 formatted) for log storage

*(Note: The M5Stack Dual Button unit's AW9523 chip is used for touch wake-up functionality, but the buttons themselves are not currently utilized for primary input in this application.)*

## Documentation
Detailed technical documentation is available in:
- `src/PROJECT_DOCUMENTATION.md`: Comprehensive technical guide covering architecture, workflow, UI, hardware interfaces, and troubleshooting.

*(Other documentation files previously mentioned like `src/README.md` or `src/References.md` are consolidated into `PROJECT_DOCUMENTATION.md` or removed.)*

## Development
This project is developed using PlatformIO with the Arduino framework for ESP32.

## Dependencies
Key libraries used (see `platformio.ini` for specific versions):
- M5CoreS3 / M5Unified / M5GFX (M5Stack hardware libraries)
- lvgl (v9.x - Graphics library)
- ESP32Time (Time functions)
- WiFiManager (Custom library in `lib/`)
- Standard ESP32/Arduino libraries: WiFi, HTTPClient, Preferences, SD, Wire, SPI

## License
Proprietary - All rights reserved
