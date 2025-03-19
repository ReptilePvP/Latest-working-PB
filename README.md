# Loss Prevention Log System for M5Stack CoreS3

## Overview
This project implements a loss prevention logging system for retail environments using the M5Stack CoreS3 hardware. The application allows store employees to quickly log theft incidents with details including gender, clothing colors, and stolen items.

## Key Features
- Intuitive touch-based user interface with card-style design
- Gender and item selection
- Color selection for clothing (shirts, pants, shoes)
- Local storage on SD card
- WiFi connectivity for time synchronization
- Formatted entry logging with timestamps
- Power management settings (power off, restart, sleep mode)
- Date and time configuration
- WiFi network management with visual feedback

## Hardware Requirements
- M5Stack CoreS3
- M5Stack Dual Button & Key Unit
- SD Card for storage

## Documentation
Detailed documentation is available in the following files:
- `src/README.md`: Comprehensive project guide with installation instructions
- `src/PROJECT_DOCUMENTATION.md`: Detailed technical documentation
- `src/References.md`: Reference materials and function listings

## Development
This project is developed using PlatformIO with the Arduino framework for ESP32.

## Dependencies
- M5Unified (0.2.4+)
- M5GFX (0.2.6+)
- LVGL (8.4.0)
- ESP32Time (2.0.6+)
- Other standard ESP32 Arduino libraries

## License
Proprietary - All rights reserved
