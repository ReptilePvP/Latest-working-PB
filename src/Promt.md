Project Prompt: Loss Prevention Log System Enhancement

1. Introduction

The Loss Prevention Log system is an IoT device built on the M5Stack CoreS3 platform, designed to streamline the logging of suspicious activities in retail environments. The system features a card-style UI, smooth screen transitions, and integrates hardware components like the M5Stack Dual Button & Key Unit for user input. The current system, built with LVGL 8.4.0, stores logs on an SD card and offers WiFi connectivity for time synchronization.

2. Project Goals

The primary goal is to enhance the existing Loss Prevention Log system to improve its efficiency, maintainability, and scalability. This involves:

Optimizing for Field Use: Enhancing the user experience for on-the-field staff, ensuring quick and easy data entry.
Improving Code Efficiency: Optimizing the program to reduce memory leaks and improve overall performance.
Enhancing Code Modularity: Restructuring the codebase to be more modular and well-organized, facilitating easier management and feature additions.
Upgrading LVGL Library: Migrating from LVGL 8.4.0 to the latest stable version (9.2.2), adapting to API changes, and leveraging new features.
Comprehensive Documentation: Ensuring all code is thoroughly documented for future maintainability and feature development.
Complete UI Redesign: Implementing a modern, sleek, and unique user interface design.
3. Key Responsibilities

The new programmer will be responsible for:

Code Refactoring:
Analyzing the existing system architecture and identifying areas for improvement in terms of efficiency, memory usage, and modularity.
Implementing a modular architecture that separates concerns and promotes code reuse.
Ensuring all code adheres to modern C++ standards and best practices.
Ensuring the refactored code is fully compatible with the Arduino IDE.
UI/UX Optimization:
Completely redesigning the user interface with a modern, sleek, and unique design, incorporating the following principles:
Minimalism: Focus on simplicity and clarity, removing unnecessary elements and reducing visual clutter.
Intuitive Navigation: Design a clear and easy-to-use navigation system that allows users to quickly access the information they need.
Consistent Design Language: Maintain a consistent look and feel throughout the entire UI, using a unified color palette, typography, and iconography.
Accessibility: Ensure the UI is accessible to users with disabilities, following accessibility guidelines and best practices.
Touch-Friendly Design: Optimize the UI for touch input, with large, easily tappable buttons and controls.
Visual Hierarchy: Use visual cues such as size, color, and contrast to guide the user's attention and highlight important information.
Modern Aesthetics: Incorporate modern design trends such as flat design, material design, or neumorphism to create a visually appealing and up-to-date UI.
Consider using a bottom navigation bar for quick access to main features.
Implement a dark mode option for improved usability in low-light conditions.
Optimizing touch input and button navigation for quick data entry.
Ensuring the UI is responsive and adapts well to different screen orientations.
LVGL Library Upgrade:
Upgrading the LVGL library from version 8.4.0 to 9.2.2.
Adapting the existing code to the new LVGL API, considering the API differences between the versions.
Leveraging new features in LVGL 9.2.2 to enhance the UI and improve performance.
Memory Leak Detection and Prevention:
Identifying and fixing existing memory leaks in the codebase.
Implementing robust memory management practices to prevent future leaks.
Using memory profiling tools to monitor memory usage and identify potential issues.
Documentation:
Creating comprehensive documentation for all code, including function descriptions, class structures, and usage examples.
Maintaining up-to-date documentation as the codebase evolves.
Using a documentation generator (e.g., Doxygen) to automate the documentation process.
Testing:
Writing unit tests to ensure the functionality of individual components.
Conducting integration tests to verify the interaction between different parts of the system.
Performing user acceptance testing to ensure the system meets the needs of the end-users.
4. Technical Requirements

Programming Language: C++ (Arduino)
Hardware Platform: M5Stack CoreS3
UI Library: LVGL (version 9.2.2)
Development Environment: Arduino IDE
Libraries:
WiFiManager.h: Custom WiFi management library
HTTPClient.h: For HTTP requests (if webhook integration is used)
Wire.h: For I2C communication (M5Stack Dual Button & Key Unit)
M5Unified.h: Unified library for M5Stack devices
M5GFX.h: Graphics library for M5Stack
lvgl.h (version 9.2.2): Light and Versatile Graphics Library
WiFi.h: For WiFi connectivity
Preferences.h: For storing system settings
SPI.h: For SPI communication (SD card)
SD.h: For SD card access
time.h: For time management
ESP32Time.h: For time synchronization
5. Important Definitions and Configurations

The following definitions and configurations must be used for the device:

// SD Card pins for M5Stack CoreS3
#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN   4
#define TFT_DC 35

// Screen dimensions for CoreS3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 20];  // Increased buffer size

// Display and input drivers
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// LVGL semaphore for thread safety 
SemaphoreHandle_t xGuiSemaphore;
#define LV_TICK_PERIOD_MS 10

// Touch tracking 
static bool touch_active = false;
static int16_t touch_start_x = 0;
static int16_t touch_start_y = 0;
static int16_t touch_last_x = 0;
static int16_t touch_last_y = 0;
static unsigned long touch_start_time = 0;
static bool was_touching = false;  // Add this line
const int TOUCH_SWIPE_THRESHOLD = 30;  // Pixels for a swipe
const int TOUCH_MAX_SWIPE_TIME = 700;  // Max time (ms) for a swipe

// LVGL tick task 
static void lvgl_tick_task(void *arg) {
    (void)arg;
    // Use lv_tick_get instead of lv_tick_inc which is not available in this LVGL version
    static uint32_t last_tick = 0;
    uint32_t current_tick = millis();
    if (current_tick - last_tick > LV_TICK_PERIOD_MS) {
        last_tick = current_tick;
        lv_task_handler(); // Process LVGL tasks
    }
}

// Display flush function
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5.Display.startWrite();
    M5.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5.Display.pushPixels((uint16_t *)color_p, w * h); // Replace pushColors with pushPixels
    M5.Display.endWrite();
    lv_disp_flush_ready(disp);
}

// Touchpad read function
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    lgfx::touch_point_t tp[1];
    static bool was_touching = false;
    static int16_t touch_start_x = 0;
    static int16_t touch_start_y = 0;
    static int16_t touch_last_x = 0;
    static int16_t touch_last_y = 0;
    static unsigned long touch_start_time = 0;

    M5.update();
    
    // Get touch points using the CoreS3 User Demo approach
    int nums = M5.Display.getTouchRaw(tp, 1);
    if (nums) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = tp[0].x;
        data->point.y = tp[0].y;
        
        if (!was_touching) {
            touch_start_x = tp[0].x;
            touch_start_y = tp[0].y;
            touch_start_time = millis();
            was_touching = true;
        }
        touch_last_x = tp[0].x;
        touch_last_y = tp[0].y;
    } else if (was_touching) {
        data->state = LV_INDEV_STATE_REL;
        was_touching = false;

        int dx = touch_last_x - touch_start_x;
        int dy = touch_last_y - touch_start_y;
        int abs_dx = abs(dx);
        int abs_dy = abs(dy);
        unsigned long duration = millis() - touch_start_time;

        if (duration < TOUCH_MAX_SWIPE_TIME) {
            if (abs_dx > TOUCH_SWIPE_THRESHOLD && abs_dx > abs_dy) {
                if (dx < 0) handleSwipeLeft();
            } else if (abs_dy > TOUCH_SWIPE_THRESHOLD && abs_dy > abs_dx) {
                handleSwipeVertical(dy < 0 ? -SCROLL_AMOUNT : SCROLL_AMOUNT);
            }
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
6. Detailed System Overview

The Loss Prevention Log system is structured around several key components:

User Interface (UI): The UI is built using the LVGL library and provides a card-style interface for user interaction. It consists of multiple screens, including a main menu, gender selection, color selection (shirt, pants, shoes), item selection, confirmation screen, WiFi settings screen, view logs screen, settings menu, brightness settings screen, sound settings screen, and power management screen. The UI utilizes custom styles for cards, buttons, and text, and incorporates smooth screen transitions for a seamless user experience.
Data Management: The system stores log data on an SD card in a structured format with timestamps. It also includes WiFi connectivity for time synchronization via NTP. The getFormattedEntry() function formats the log entry with all selected attributes, and the appendToLog() function appends the entry to the log file on the SD card. The listSavedEntries() function retrieves and displays the saved log entries.
WiFi Management: The WiFiManager class handles WiFi connectivity, network scanning, connection management, and saved network prioritization. It uses a state machine to manage the WiFi connection process and provides visual feedback to the user through a loading screen. The loadSavedNetworks() and saveNetworks() functions manage the storage of WiFi credentials in the Preferences library.
Screen Transitions: The system uses custom screen transitions implemented in the screen_transition.h file. These transitions provide smooth animations between screens, enhancing the user experience. The load_screen_with_animation() function loads a new screen with the specified transition type and duration.
Touch Input: The system uses the M5Stack CoreS3's touchscreen for user input. The my_touchpad_read() function reads the touch input and provides the coordinates to the LVGL library. The system also implements swipe gestures for navigation.
Power Management: The system includes a power management screen with options to power off, restart, or enter sleep mode. The power off and restart functions use a 3-second countdown to prevent accidental activation. The sleep mode utilizes the M5Stack CoreS3's deep sleep functionality with touch wake-up.
7. Key Algorithms and Techniques

The following algorithms and techniques are used in the system:

State Machine: The WiFiManager class uses a state machine to manage the WiFi connection process. This ensures robust and reliable WiFi connectivity.
Screen Transitions: The screen transitions are implemented using LVGL's animation system. This provides smooth and visually appealing transitions between screens.
Swipe Gestures: The system implements swipe gestures for navigation. This allows users to quickly navigate through the UI using touch input.
Data Formatting: The getFormattedEntry() function formats the log entry with all selected attributes. This ensures that the log data is stored in a consistent and structured format.
8. Deliverables

Refactored and optimized codebase with improved modularity and efficiency, fully compatible with the Arduino IDE.
Upgraded LVGL library from version 8.4.0 to 9.2.2.
Completely redesigned user interface with a modern, sleek, and unique design.
Comprehensive documentation for all code, including function descriptions, class structures, and usage examples.
Unit tests and integration tests to ensure code quality and functionality.
A detailed report outlining the changes made, the optimizations implemented, and the results of testing.
9. Existing Code Structure

The project consists of the following key files:

src/Loss_Prevention_Log.ino: Main application file.
src/WiFiManager.cpp and src/WiFiManager.h: Custom WiFi management library.
lib/ScreenManager.h: Manages screen transitions and navigation.
lib/screen_transition.h: Implements screen transition effects and animations.
src/lv_conf.h: LVGL configuration file.
10. Key Considerations

Arduino IDE Compatibility: Ensure all code is compatible with the Arduino IDE.
LVGL API Changes: Be aware of the API differences between LVGL 8.4.0 and 9.2.2. Refer to the LVGL documentation for migration guidelines.
Memory Management: Pay close attention to memory management to avoid leaks and ensure efficient resource utilization.
User Experience: Prioritize the user experience for on-the-field staff, making the system as intuitive and efficient as possible.
Documentation: Maintain comprehensive and up-to-date documentation for all code.
Use the provided definitions and configurations for the device.
11. Success Metrics

Improved code efficiency and reduced memory usage.
Successful migration to LVGL 9.2.2 with full functionality.
Implementation of a modern, sleek, and unique user interface design.
Comprehensive and up-to-date documentation.
Positive user feedback on the improved UI/UX.