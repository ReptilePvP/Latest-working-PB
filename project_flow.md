# Loss Prevention Log System – Project Sequence Diagram

This diagram provides a high-level overview of the main operational flows in the Loss Prevention Log System.

```mermaid
sequenceDiagram
    participant User
    participant UI (ui.cpp)
    participant MainLoop (Loss_Prevention_Log.ino)
    participant WiFiHandler (wifi_handler.cpp)
    participant SDLogger (sd_logger.cpp)
    participant TimeUtils (time_utils.cpp)
    participant M5Hardware (M5Unified, RTC, Power, SD)
    %% WiFiManager is now integrated into wifi_handler.cpp

    %% Initialization Sequence %%
    Note over MainLoop, M5Hardware: System Boot / setup()
    MainLoop->>M5Hardware: Initialize M5Unified (Display, Power, Speaker)
    MainLoop->>M5Hardware: Initialize RTC
    MainLoop->>SDLogger: initFileSystem()
    SDLogger->>M5Hardware: SPI Bus Switch & SD.begin()
    M5Hardware-->>SDLogger: SD OK
    SDLogger->>M5Hardware: SPI Bus Restore
    SDLogger-->>MainLoop: Filesystem OK
    MainLoop->>TimeUtils: setSystemTimeFromRTC()
    TimeUtils->>M5Hardware: Read RTC
    M5Hardware-->>TimeUtils: RTC Time
    TimeUtils-->>MainLoop: System Time Set
    MainLoop->>WiFiHandler: Initialize (registers callbacks with WiFiManager)
    WiFiHandler->>WiFiManager: Register StatusCallback (onWiFiStatus)
    WiFiHandler->>WiFiManager: Register ScanCallback (onWiFiScanComplete)
    WiFiManager-->>WiFiHandler: Callbacks Registered
    WiFiHandler-->>MainLoop: WiFi Handler Initialized
    MainLoop->>WiFiManager: init() / loadNetworks()
    WiFiManager->>M5Hardware: Read Preferences
    M5Hardware-->>WiFiManager: Saved Networks
    WiFiManager-->>MainLoop: WiFi Manager Initialized
    MainLoop->>UI: Initialize LVGL & Create Screens
    UI-->>MainLoop: UI Initialized
    MainLoop->>WiFiManager: connectToBestNetwork() (Attempt auto-connect)
    WiFiManager->>MainLoop: Starts Connection Attempt (if applicable)
    Note over MainLoop, M5Hardware: setup() complete, enters loop()

    %% Main Loop Operation (Simplified) %%
    loop Periodic Updates
        MainLoop->>M5Hardware: M5.update() (Handles button presses, etc.)
        MainLoop->>WiFiManager: update() (Checks WiFi state, scan status)
        Note over WiFiManager: If connection succeeds during update()...
        WiFiManager->>WiFiHandler: Calls onWiFiStatus(CONNECTED) callback
        WiFiHandler->>TimeUtils: syncTimeWithNTP()
        TimeUtils->>M5Hardware: Configure NTP & Get Time
        M5Hardware-->>TimeUtils: NTP Time Received
        TimeUtils->>M5Hardware: Update RTC
        M5Hardware-->>TimeUtils: RTC Updated
        TimeUtils-->>WiFiHandler: Sync Complete
        WiFiHandler->>UI: Update WiFi/NTP Status Display
        MainLoop->>UI: lv_timer_handler() (via lvgl_task)
    end

    %% Incident Logging Sequence %%
    User->>UI: Navigate to 'New Entry'
    User->>UI: Input Gender, Colors, Item details
    User->>UI: Press 'Save' button
    UI->>SDLogger: appendToLog(entryData)
    SDLogger->>TimeUtils: getTimestamp()
    TimeUtils-->>SDLogger: Current Timestamp
    SDLogger->>M5Hardware: SPI Bus Switch & Open Log File
    SDLogger->>M5Hardware: Write Formatted Entry to SD Card
    M5Hardware-->>SDLogger: Write OK
    SDLogger->>M5Hardware: SPI Bus Restore
    SDLogger-->>UI: Log Saved Successfully
    UI->>User: Show Confirmation Message
    Note over UI, WiFiHandler: If WiFi connected, UI might trigger...
    UI->>WiFiHandler: sendWebhook(entryData) (Optional)
    WiFiHandler->>M5Hardware: Send HTTP POST Request
    M5Hardware-->>WiFiHandler: Webhook Sent (or Failed)
    WiFiHandler-->>UI: Webhook Status

```

**How to view:**  
Open this file in VS Code and use the Markdown preview (with your Mermaid extension enabled) to see the rendered sequence diagram.
