# System Patterns: Loss Prevention Log System

## Architecture Overview
The Loss Prevention Log System is built on an event-driven architecture with a modular design. It utilizes the M5Stack CoreS3 hardware and follows these key architectural patterns:

1. **Model-View-Controller (MVC)**: The system separates data management, user interface, and control logic.
2. **Event-Driven Programming**: UI interactions and system events trigger specific actions and updates.
3. **State Machine**: Particularly for WiFi connection management (state managed across main thread and background task).
4. **Singleton**: Used for global instances like WiFiManager.
5. **Concurrency**: Utilizes FreeRTOS tasks to offload blocking operations (e.g., WiFi) from the main UI thread.

## Key Components

### 1. Main Application (`Loss_Prevention_Log.ino`)
- Serves as the entry point and main controller.
- Manages the overall application flow and UI rendering.
- Initializes hardware components and sets up the system.

### 2. WiFi Management (`WiFiManager.cpp` & `WiFiManager.h`)
- Implements a custom WiFi connection management system using a dedicated FreeRTOS background task pinned to Core 0.
- Offloads potentially blocking operations (scan, connect, status monitoring) from the main UI thread (Core 1).
- Uses FreeRTOS queues (`_commandQueue`, `_resultQueue`) for safe communication between the main thread and the background task.
- Employs a state machine (`WiFiState`) managed cooperatively between the main thread (receiving results) and the background task (performing operations).
- Provides callbacks (`StatusCallback`, `ScanCallback`) triggered from the main thread context upon receiving results from the background task.

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
3. **Command Pattern**: Utilized in handling user interactions and executing corresponding actions. Also used implicitly via the command queue for the WiFi task.
4. **Singleton**: Applied to WiFiManager and other global utility classes.
5. **State Pattern**: Implemented in the WiFi connection management state machine.
6. **Producer-Consumer**: Used via FreeRTOS queues for communication between the main thread (producer of commands, consumer of results) and the WiFi task (consumer of commands, producer of results).
7. **Active Object** (Conceptual): The WiFi background task acts like an active object, encapsulating its own thread of control for managing WiFi operations.

## Data Flow
1. User input captured through touch interface or physical buttons.
2. Input processed by event handlers in the main application (`Loss_Prevention_Log.ino` on Core 1).
3. **WiFi Operations:**
    - Main application sends commands (Scan, Connect, Disconnect, Enable/Disable) to `WiFiManager`.
    - `WiFiManager` places commands onto a FreeRTOS command queue.
    - A dedicated WiFi background task (on Core 0) reads commands from the queue.
    - The WiFi task performs blocking/long-running WiFi operations (`WiFi.scanNetworks`, `WiFi.begin`, status checks).
    - The WiFi task places results (status updates, scan results) onto a FreeRTOS result queue.
    - `WiFiManager::update()` (called in the main loop on Core 1) reads results from the queue.
    - `WiFiManager` updates its internal state and triggers callbacks.
4. Data logged to SD card.
5. UI updated via callbacks to reflect changes and provide feedback (on Core 1).

## Error Handling
- Comprehensive error checking and handling throughout the system.
- Use of try-catch blocks and error codes for robust operation.
- Fallback mechanisms (e.g., RTC when NTP fails) for critical functionalities.

## Optimization Strategies
- Memory management optimizations to work within ESP32-S3 constraints.
- Use of LVGL's memory-efficient UI rendering.
- Efficient power management for extended battery life.
- **Concurrency**: Offloading WiFi operations to a separate task ensures UI responsiveness.

## Extensibility
- Modular design allows for easy addition of new features.
- Clear separation of concerns facilitates maintenance and updates.
- Use of callback mechanisms and event-driven architecture supports future enhancements.

This system architecture provides a robust foundation for the Loss Prevention Log System, ensuring reliability, performance, and ease of future development.
