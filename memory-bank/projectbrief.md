# Project Brief: Loss Prevention Log System (Updated)

## Overview
The Loss Prevention Log System is a digital logging tool for retail environments, running on M5Stack CoreS3 hardware. It aims to replace paper-based methods, providing an efficient way for employees to record theft incidents via a touch interface.

## Goals
- Improve efficiency and accuracy in logging theft incidents.
- Provide a user-friendly touch interface for data entry.
- Ensure reliable local storage of log data.
- Enable basic device management features (time sync, power).

## Scope
- **Hardware**: M5Stack CoreS3.
- **Interface**: Touch-based GUI built with LVGL v9.
- **Storage**: Local storage on SD card.
- **Connectivity**: WiFi for time synchronization (NTP) and potentially future features (e.g., webhooks).
- **Functionality**: Incident logging (gender, apparel, color, item), log viewing, device settings (WiFi, sound, brightness, date/time, power).

## Key Features (Implemented)
- **UI**:
    - Loading and Lock screens.
    - Main menu with card-style navigation.
    - Multi-step incident entry flow (Gender -> Apparel Type -> Shirt Color -> Pants Type -> Pants Color -> Shoe Style -> Shoe Color -> Item -> Confirmation).
    - Log viewing screen with entries grouped by day (last 3 days).
    - Settings menu for WiFi, Sound, Display (Brightness), Date & Time, Power Management.
    - WiFi management screen (scan, connect via password entry, view saved networks, **connect to saved**, **disconnect**, **forget saved**).
- **Logging**:
    - Saves formatted log entries with timestamps to SD card (`log.csv`).
    - Parses and displays logs.
- **Connectivity**:
    - WiFi scanning and connection (WPA/WPA2).
    - Saves known networks using Preferences.
    - Automatic connection attempts to best/saved network.
    - Time synchronization via NTP when connected.
    - Basic webhook functionality (`sendWebhook`).
- **Device Management**:
    - RTC for timekeeping (fallback when offline).
    - Manual Date & Time setting via UI.
    - Sound enable/disable and volume control.
    - Display brightness control.
    - Power options: Restart, Power Off, Deep Sleep (wake via touch).
    - Persistent settings storage using `Preferences`.

## Hardware Requirements
- M5Stack CoreS3
- SD Card for storage
