# Changelog

All notable changes to the Loss Prevention Log project will be documented in this file.

## [Unreleased] - 2025-04-02
### Changed
- Refactored codebase into a modular structure for improved organization and maintainability:
  - Separated UI logic into `src/ui.cpp` / `src/ui.h`.
  - Separated SD card logging logic into `src/sd_logger.cpp` / `src/sd_logger.h`.
  - Separated WiFi handling logic into `src/wifi_handler.cpp` / `src/wifi_handler.h`.
  - Separated time utility functions into `src/time_utils.cpp` / `src/time_utils.h`.
  - Introduced `src/globals.h` for shared definitions.
  - Main application logic (`.ino`) now focuses on initialization and coordination between modules.

## [1.0.0] - 2025-03-18
### Initial Release

#### Core Features
- User interface built using LVGL library with modern, responsive design
- Main menu with card-style navigation for primary functions
- Comprehensive loss prevention logging system with structured data entry

#### Entry System
- Multi-step entry process with:
  - Gender selection (Male/Female)
  - Shirt color selection (multiple colors supported)
  - Pants color selection (multiple colors supported)
  - Shoes color selection (multiple colors supported)
  - Item category selection

#### Data Management
- SD card-based storage system
- Organized log file structure
- Date/time stamping of entries
- Log entry viewing with:
  - 3-day view organization
  - Categorized display
  - Detailed entry viewing
  - Log clearing capability

#### Connectivity
- WiFi management system with:
  - Network scanning
  - Saved network management
  - Auto-reconnect functionality
  - Signal strength monitoring
  - Network status display

#### Settings & Configuration
- Sound settings with:
  - Volume control
  - Sound enable/disable
  - Sound feedback for interactions
- Display settings with:
  - Brightness control
  - Auto-brightness option
  - Preset brightness levels
- Date & Time settings with:
  - RTC synchronization
  - Date selection interface
  - Time selection interface
- Power management with:
  - Power off functionality
  - Restart option
  - Sleep mode
  - Battery monitoring

#### UI/UX Features
- Touch-based interface
- Swipe navigation support
- Scrolling lists
- Status bar with:
  - Battery level indicator
  - WiFi status
  - Current time display
- Loading animations
- Error handling with user feedback
- Confirmation dialogs for critical actions

#### System Features
- Real-time clock (RTC) integration
- External data logging capability
- SPI bus management for peripherals
- I2C device management
- Power management system
- Memory optimization
