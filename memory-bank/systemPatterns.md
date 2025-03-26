# System Patterns: Loss Prevention Log System

## Architecture Overview
The Loss Prevention Log System is built on an event-driven architecture with a modular design. It utilizes the M5Stack CoreS3 hardware and follows these key architectural patterns:

1. **Model-View-Controller (MVC)**: The system separates data management, user interface, and control logic.
2. **Event-Driven Programming**: UI interactions and system events trigger specific actions and updates.
3. **State Machine**: Particularly for WiFi connection management.
4. **Singleton**: Used for global instances like WiFiManager.

## Key Components

### 1. Main Application (`Loss_Prevention_Log.ino`)
- Serves as the entry point and main controller.
- Manages the overall application flow and UI rendering.
- Initializes hardware components and sets up the system.

### 2. WiFi Management (`WiFiManager.cpp` & `WiFiManager.h`)
- Implements a custom WiFi connection management system.
- Uses a state machine for robust connection handling.
- Provides callbacks for connection status updates.

### 3. User Interface
- Built with LVGL (Light and Versatile Graphics Library).
- Implements a card-style UI with consistent styling across screens.
- Uses screen transitions for smooth navigation.

### 4. Data Storage
- Utilizes SD card for log file storage.
- Implements file operations with error handling.

### 5. Time Synchronization
- Uses NTP for internet time synchronization.
- Falls back to RTC when internet is unavailable.

### 6. Power Management
- Implements hardware-level power control via AXP2101 chip.
- Uses AW9523 chip for touch wake-up functionality in sleep mode.

## Design Patterns

1. **Observer Pattern**: Used for WiFi status updates and event handling in the UI.
2. **Factory Method**: Employed in creating different types of UI elements and screens.
3. **Command Pattern**: Utilized in handling user interactions and executing corresponding actions.
4. **Singleton**: Applied to WiFiManager and other global utility classes.
5. **State Pattern**: Implemented in the WiFi connection management state machine.

## Data Flow
1. User input captured through touch interface or physical buttons.
2. Input processed by event handlers in the main application.
3. Data logged to SD card and optionally sent to a remote server via webhook.
4. UI updated to reflect changes and provide feedback.

## Error Handling
- Comprehensive error checking and handling throughout the system.
- Use of try-catch blocks and error codes for robust operation.
- Fallback mechanisms (e.g., RTC when NTP fails) for critical functionalities.

## Optimization Strategies
- Memory management optimizations to work within ESP32-S3 constraints.
- Use of LVGL's memory-efficient UI rendering.
- Efficient power management for extended battery life.

## Extensibility
- Modular design allows for easy addition of new features.
- Clear separation of concerns facilitates maintenance and updates.
- Use of callback mechanisms and event-driven architecture supports future enhancements.

This system architecture provides a robust foundation for the Loss Prevention Log System, ensuring reliability, performance, and ease of future development.
