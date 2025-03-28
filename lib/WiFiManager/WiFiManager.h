#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h> // Core Arduino functions (String, millis, etc.)
#include <WiFi.h>
#include <Preferences.h>
#include <vector>
#include <functional>
#include <algorithm> // For std::sort, std::min
#include <cstdint>   // For int8_t
#include <cstddef>   // For NULL/nullptr (though nullptr is preferred)
#include "freertos/FreeRTOS.h" // Core FreeRTOS
#include "freertos/task.h"     // Task functions (vTaskDelay, vTaskDelete, TaskHandle_t)
#include "freertos/queue.h"    // Queue functions (xQueueCreate, xQueueSend, xQueueReceive, QueueHandle_t)
#include "esp_log.h"     // ESP logging framework (ESP_LOGI, ESP_LOGW, etc.)

// Define constants
#define CONNECTION_TIMEOUT 10000  // 10 seconds for connection timeout
#define RECONNECT_INTERVAL 30000  // 30 seconds for reconnection attempts
#define WIFI_COMMAND_QUEUE_SIZE 10
#define WIFI_RESULT_QUEUE_SIZE 10
#define WIFI_TASK_STACK_SIZE 8192 // Increased from 4096
#define WIFI_TASK_PRIORITY 5      // Task priority
#define MAX_SAVED_NETWORKS 10     // Maximum number of saved networks
#define WIFI_CONNECT_TIMEOUT_MS 15000 // Connection attempt timeout
#define WIFI_MAX_CONNECT_ATTEMPTS 3   // Max connection retries
#define WIFI_SCAN_TIMEOUT_MS 20000    // Scan timeout
#define WIFI_TASK_DELAY_MS 100      // Delay in the WiFi task loop
#define WIFI_RECONNECT_INTERVAL_MS 30000 // Interval between auto-reconnect attempts

// Enum for WiFi states
enum class WiFiState {
    WIFI_DISABLED,          // WiFi is off
    WIFI_DISCONNECTED,      // WiFi on, not connected
    WIFI_CONNECTING,        // Attempting to connect
    WIFI_CONNECTED,         // Successfully connected
    WIFI_CONNECTION_FAILED, // Connection attempt failed
    WIFI_SCAN_REQUESTED,    // Scan requested but not yet started
    WIFI_SCANNING,          // Actively scanning for networks
    WIFI_CONNECT_REQUESTED  // Connect requested but not yet started
};

// Enum for command types sent to the task
enum class WiFiCommandType {
    CMD_ENABLE,       // Enable WiFi
    CMD_DISABLE,      // Disable WiFi
    CMD_START_SCAN,   // Start a network scan
    CMD_CONNECT,      // Connect to a specific network
    CMD_DISCONNECT,   // Disconnect from current network
    CMD_TERMINATE     // Terminate the task
};

// Enum for result types sent back from the task
enum class WiFiResultType {
    RESULT_STATUS_UPDATE, // General state update (e.g., connecting, disconnected)
    RESULT_SCAN_COMPLETE  // Scan completed with results
};

// Structure for network information
struct NetworkInfo {
    String ssid;
    String password;        // Only used for saved networks
    int32_t rssi;           // Signal strength
    wifi_auth_mode_t encryptionType;
    bool saved;             // Is this network in saved list?
    bool connected;         // Are we currently connected to it?
    int priority = 0;       // Higher number = lower priority (changed from uint8_t to int for consistency)
};

// Structure for commands sent to the task
struct WiFiCommand {
    WiFiCommandType type;
    String ssid;            // Used for CMD_CONNECT
    String password;        // Used for CMD_CONNECT
    bool manualDisconnect;  // Used for CMD_DISCONNECT
    bool enable;            // Used for CMD_ENABLE/CMD_DISABLE
    bool save;              // Used for CMD_CONNECT (save to preferences)
    int priority;           // Used for CMD_CONNECT (priority of network)
};

// Structure for results sent back from the task
struct WiFiResult {
    WiFiResultType type;
    WiFiState newState;
    String message;
    std::vector<NetworkInfo> scanResults; // Used for RESULT_SCAN_COMPLETE
};

class WiFiManager {
public:
    // Constructor with optional enabled parameter
    WiFiManager(bool enabled = true);
    // Destructor
    ~WiFiManager();

    // Public methods
    void begin();                          // Initialize the manager and start the task
    bool startScan();                      // Request a network scan
    bool isScanning() const;               // Check if a scan is in progress
    bool isEnabled() const;                // Check if WiFi is enabled
    bool isInitialized() const;            // Check if manager is initialized
    bool isConnected() const;              // Check if connected to a network
    void setEnabled(bool enabled);         // Enable or disable WiFi
    WiFiState getState() const;            // Get current WiFi state
    String getStateString() const;         // Get state as string
    String getCurrentSSID() const;         // Get current SSID if connected
    int getRSSI() const;                   // Get signal strength if connected
    String getIPAddress() const;           // Get IP address if connected
    bool connect(const String& ssid, const String& password, bool save = false, int priority = 0); // Connect to a network
    bool connectToBestNetwork();           // Connect to best saved network
    void disconnect(bool manual = false);  // Disconnect from current network
    std::vector<NetworkInfo> getScanResults() const; // Get latest scan results

    // Callback setters
    using StatusCallback = std::function<void(WiFiState, const String&)>;
    using ScanCallback = std::function<void(const std::vector<NetworkInfo>&)>;
    void setStatusCallback(StatusCallback cb);
    void setScanCallback(ScanCallback cb);

    // Network management
    bool addNetwork(const String& ssid, const String& password, int priority = 0); // Add a network with priority
    void addNetwork(const NetworkInfo& network); // Add a network object
    bool removeNetwork(const String& ssid);      // Remove a network from saved list
    bool setNetworkPriority(const String& ssid, int priority); // Update network priority
    std::vector<NetworkInfo> getSavedNetworks() const; // Get list of saved networks

    // Main update function to process task results
    void update();

private:
    // Private members
    WiFiState _state;              // Current WiFi state
    bool _enabled;                 // Is WiFi enabled?
    bool _manualDisconnect;        // Was disconnection manual?
    bool _initialized;             // Has begin() been called successfully?
    bool _scanInProgress;          // Is a scan currently in progress?
    unsigned long _lastConnectionAttempt; // Last connection attempt time
    unsigned long _scanStartTime;  // Time when last scan started
    int _connectionAttempts;       // Number of connection attempts
    unsigned long _lastReconnectAttempt; // Last reconnect attempt time
    String _connectingSSID;        // SSID currently being connected to
    TaskHandle_t _wifiTaskHandle;  // Handle for the WiFi task
    QueueHandle_t _commandQueue;   // Queue for sending commands to task
    QueueHandle_t _resultQueue;    // Queue for receiving results from task
    Preferences _preferences;      // Storage for saved networks
    std::vector<NetworkInfo> _savedNetworks; // List of saved networks
    std::vector<NetworkInfo> _scanResults;   // Latest scan results
    StatusCallback _statusCallback; // Callback for status updates
    ScanCallback _scanCallback;     // Callback for scan results

    // Private helper methods
    void loadSavedNetworks();      // Load saved networks from Preferences
    void saveNetworks();           // Save networks to Preferences
    void notifyStatus(const String& message); // Notify status via callback
    int findNetwork(const String& ssid, const std::vector<NetworkInfo>& networks) const; // Find network index
    void sortNetworksByPriority(std::vector<NetworkInfo>& networks); // Sort networks by priority
    String getPasswordForSSID(const String& ssid); // Helper function to get password for a given SSID

    // Static task function
    static void wifiTaskLoop(void* parameter);
};

#endif // WIFI_MANAGER_H