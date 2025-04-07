# Testing Plan: WiFi Management Features (Saved Networks)

This plan outlines the test cases for the "Connect", "Disconnect", and "Forget" actions available for saved networks in the WiFi Manager screen (`createWiFiManagerScreen`).

## Test Environment
- M5Stack CoreS3 device running the Loss Prevention Log firmware.
- At least two known WiFi networks (Network A, Network B) saved in Preferences. One should be valid/in-range, and ideally one invalid/out-of-range for testing failure cases.
- Access to view device UI and Serial Monitor output (for state verification).

## Test Cases

### 1. Connect (Valid Saved Network)
- **Precondition:** Device is disconnected. Network A is saved and valid/in-range.
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Select Network A from the saved networks list.
    3. Choose the "Connect" action from the menu.
- **Expected Result:**
    - UI displays "Connecting..." status.
    - UI updates to "Connected" status for Network A.
    - `WiFiManager` state transitions to `WIFI_CONNECTED`.
    - Device obtains a valid IP address (verify via Serial Monitor or UI if displayed).
    - Time synchronizes via NTP (if enabled and successful).
    - WiFi status icon in the header updates to 'connected'.

### 2. Connect (Invalid/Out-of-Range Saved Network)
- **Precondition:** Device is disconnected. Network B is saved but invalid (e.g., wrong password) or out of range.
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Select Network B from the saved networks list.
    3. Choose the "Connect" action.
- **Expected Result:**
    - UI displays "Connecting..." status.
    - Connection attempt eventually fails.
    - UI updates to "Disconnected" status (or shows a failure message).
    - `WiFiManager` state transitions back to `WIFI_DISCONNECTED`.
    - WiFi status icon updates to 'disconnected'.

### 3. Disconnect (While Connected)
- **Precondition:** Device is connected to Network A (as per Test Case 1).
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Press the "Disconnect" button.
- **Expected Result:**
    - UI displays "Disconnecting..." status (optional).
    - UI updates to "Disconnected" status.
    - `WiFiManager` state transitions to `WIFI_DISCONNECTED`.
    - Device loses IP address.
    - WiFi status icon updates to 'disconnected'.

### 4. Forget (While Disconnected)
- **Precondition:** Device is disconnected. Network A and Network B are saved.
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Select Network B from the saved networks list.
    3. Choose the "Forget" action.
    4. Confirm the action in the prompt.
- **Expected Result:**
    - Network B is removed from the saved networks list in the UI.
    - Network B is removed from Preferences (verify by restarting the device and checking the list again).
    - Device remains disconnected.

### 5. Forget (While Connected to the *Same* Network)
- **Precondition:** Device is connected to Network A. Network A is saved.
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Select Network A from the saved networks list.
    3. Choose the "Forget" action.
    4. Confirm the action.
- **Expected Result:**
    - Device disconnects from Network A.
    - UI updates to "Disconnected" status.
    - Network A is removed from the saved networks list in the UI.
    - Network A is removed from Preferences.
    - WiFi status icon updates to 'disconnected'.

### 6. Forget (While Connected to a *Different* Network)
- **Precondition:** Device is connected to Network A. Network B is also saved.
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Select Network B from the saved networks list.
    3. Choose the "Forget" action for Network B.
    4. Confirm the action.
- **Expected Result:**
    - Network B is removed from the saved networks list in the UI.
    - Network B is removed from Preferences.
    - Device remains connected to Network A.
    - UI status still shows "Connected" to Network A.
    - WiFi status icon remains 'connected'.

### 7. Cancel Action
- **Precondition:** Device state can be connected or disconnected. Network A is saved.
- **Steps:**
    1. Navigate to the WiFi Manager screen.
    2. Select Network A from the saved networks list.
    3. Choose either "Connect" or "Forget".
    4. Select "Cancel" from the confirmation prompt (if applicable for the action).
- **Expected Result:**
    - No change in the device's WiFi connection state.
    - No change in the list of saved networks in the UI or Preferences.
