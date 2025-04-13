# Loss Prevention Log System – High-Level Process Flowchart

This flowchart provides a high-level overview of the main logic and process flow between the core modules of the Loss Prevention Log System.

```mermaid
flowchart TD
    A[System Boot / Power On]
    B[Initialize Hardware (M5Unified, RTC, SD)]
    C[Initialize Modules (UI, WiFiHandler, SDLogger, TimeUtils)]
    D[Load Preferences & Saved Networks]
    E[Attempt Auto WiFi Connect]
    F[Main Loop]
    G[User Interacts with UI]
    H[Incident Logging Flow]
    I[Save Log Entry to SD Card]
    J[Send Webhook (if WiFi)]
    K[View Logs]
    L[Settings Adjustments]
    M[WiFi Scan/Connect/Forget]
    N[Manual NTP Sync]
    O[RTC/Time Update]
    P[Power Management (Sleep/Restart/Off)]

    A --> B
    B --> C
    C --> D
    D --> E
    E --> F
    F --> G
    G --> H
    H --> I
    I --> J
    G --> K
    G --> L
    L --> M
    L --> N
    N --> O
    G --> P

    %% Additional relationships
    E -- WiFi Connected --> N
    N -- Updates --> O
    M -- Updates --> E
    L -- Adjusts --> F
    P -- Triggers --> A
```

**How to view:**  
Open this file in VS Code and use the Markdown preview (with your Mermaid extension enabled) to see the rendered flowchart.
