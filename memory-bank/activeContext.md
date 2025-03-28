# Active Context: Loss Prevention Log System

## Current Focus
- Completing the refactoring of `WiFiManager` to use a background task for improved UI responsiveness and user experience during Wi-Fi operations (scan, connect, disconnect).

## Recent Changes
- Read all core memory bank files.
- Analyzed existing `WiFiManager.h` and `WiFiManager.cpp`.
- Planned refactoring strategy to use a background FreeRTOS task and queues.
- Modified `WiFiManager.h`:
    - Added FreeRTOS includes (`task.h`, `queue.h`).
    - Added handles for task and queues (`_wifiTaskHandle`, `_commandQueue`, `_resultQueue`).
    - Defined `WiFiCommand` and `WiFiResult` structs for inter-task communication.
    - Added new states (`WIFI_SCAN_REQUESTED`, `WIFI_CONNECT_REQUESTED`).
    - Declared static `wifiTaskLoop` function.
- Rewrote `WiFiManager.cpp`:
    - Implemented `begin()` to create queues and the background task pinned to Core 0.
    - Modified `update()` to process results from the `_resultQueue` and trigger callbacks in the main thread context.
    - Changed `setEnabled()`, `connect()`, `disconnect()`, `startScan()` to send commands to the `_commandQueue` instead of performing actions directly.
    - Implemented the `wifiTaskLoop` static function to:
        - Wait for commands from `_commandQueue`.
        - Perform actual Wi-Fi operations (scan, connect, disconnect, status checks) using `WiFi` library calls.
        - Handle connection logic, timeouts, and automatic reconnections.
        - Send results (status updates, scan results) back to the main thread via `_resultQueue`.
    - Added ESP logging (`esp_log.h`) for better debugging.
    - Fixed a syntax error (missing semicolon after `std::sort` lambda) in `wifiTaskLoop`.

## Next Steps
1. Update `systemPatterns.md` to reflect the new background task and queue architecture.
2. Update `progress.md` to note the completion of the `WiFiManager` refactoring.
3. Review `Loss_Prevention_Log.ino` to ensure it correctly interacts with the refactored `WiFiManager` (handling new states, callbacks).
4. Make necessary adjustments to `Loss_Prevention_Log.ino` for UI feedback during requested/pending states.
5. Request compilation and testing.

## Active Decisions and Considerations
- Moving Wi-Fi operations to Core 0 ensures the main loop/UI on Core 1 remains responsive.
- Using queues provides a standard FreeRTOS mechanism for safe inter-task communication.
- Synchronous `WiFi.scanNetworks()` is used *within* the background task, which is acceptable as it doesn't block the main UI thread.
- Network management methods (`addNetwork`, `removeNetwork`, `saveNetworks`, `loadSavedNetworks`) still run in the main thread context for simplicity, assuming they are fast enough. `loadSavedNetworks` is only called during `begin` before the task starts. `saveNetworks` might be called when adding/removing, which could potentially block briefly, but this is less frequent than scan/connect.
- Increased connection/scan timeouts slightly in the header.

## Open Questions
- How does the UI in `Loss_Prevention_Log.ino` currently handle Wi-Fi status updates and trigger actions? (Requires reading the file)
- Will the increased stack size (`WIFI_TASK_STACK_SIZE`) be sufficient? (Requires testing)

## Notes
- This is a significant architectural change for `WiFiManager`. Thorough testing is crucial.
- Error handling within the task (e.g., queue send/receive failures) is basic and could be enhanced if needed.
