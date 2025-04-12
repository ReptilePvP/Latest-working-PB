# Active Context: Loss Prevention Log System (Updated 2025-04-10 12:33)

## Current Focus
- **Memory Bank Update**: Updating Memory Bank files (`systemPatterns.md`, `progress.md`) to reflect the recent change in WiFi connection logic.

## Recent Changes (This Session)
- **WiFi Connection Logic Updated**:
    - Modified `connectToBestNetwork()` in `lib/WiFiManager/WiFiManager.cpp` to only attempt connections to saved networks that are also present in the latest scan results (`_scanResults`). This prevents attempts to connect to saved networks that are currently out of range.
- Read all core Memory Bank files (as part of WiFi task).
- **NTP Time Synchronization Implemented (Previous Session)**:
    - Added `syncTimeWithNTP()` and `getLastSyncStatus()` to `src/time_utils.cpp` and `.h`.
    - Modified `src/wifi_handler.cpp` to automatically call `syncTimeWithNTP()` upon successful WiFi connection. Added `isWiFiConnected()` helper.
    - Added a manual "Sync Now" button and status label to the Date & Time settings screen in `src/ui.cpp`.
    - Added an initial NTP sync attempt in `setup()` within `src/Loss_Prevention_Log.ino` if WiFi connects automatically.
- Read source documentation files: `README.md`, `src/PROJECT_COMPONENT_OVERVIEW.md`, `src/PROJECT_DOCUMENTATION.md` (during previous documentation sync).
- Updated `memory-bank/projectbrief.md` (log filename, incident flow, NTP status, auto-brightness, webhook) (during previous documentation sync).
- Updated `memory-bank/productContext.md` (incident flow, log filename) (during previous documentation sync).
- Updated `memory-bank/systemPatterns.md` (WiFi structure, SPI switching pattern, wake mechanism, NTP status) (during previous documentation sync).
- Updated `memory-bank/techContext.md` (WiFi structure, SPI switching, NTP status, wake mechanism) (during previous documentation sync).
- Updated `memory-bank/progress.md` (log filename, incident flow, NTP status, auto-brightness, webhook, known issues, next steps) (during previous documentation sync).
- Updated this file (`memory-bank/activeContext.md`) (during previous documentation sync and current session).

## Next Steps
1.  **Update Memory Bank**: Update `systemPatterns.md` and `progress.md` to reflect the new WiFi connection logic in `connectToBestNetwork()`.
2.  **Consider MCP Update**: After Memory Bank is updated, evaluate if MCP server update is needed for this WiFi change.

## Open Questions / Considerations
- Is the current single-threaded WiFi approach sufficient, or should refactoring to a background task be prioritized? (Remains open)
- How should the timezone for NTP be configured (currently hardcoded to UTC)?

## Notes
- The core Memory Bank files need final updates for the NTP feature before updating MCP servers.
