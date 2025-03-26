#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <vector>
#include <Preferences.h>
#include <functional>

/**
 * Represents the possible states of the WiFi connection
 */
enum class WiFiState {
    WIFI_DISABLED,      // WiFi is turned off
    WIFI_DISCONNECTED,  // WiFi is on but not connected to any network
    WIFI_SCANNING,      // Currently performing a network scan
    WIFI_CONNECTING,    // Attempting to connect to a network
    WIFI_CONNECTED,     // Successfully connected to a network
    WIFI_CONNECTION_FAILED  // Connection attempt failed
};

/**
 * Holds information about a WiFi network
 */
struct NetworkInfo {
    String ssid;
    String password;
    int8_t rssi;
    uint8_t encryptionType;
    bool saved;
    bool connected;
    int priority; // Higher value = higher priority
};

class WiFiManager {
public:
    using StatusCallback = std::function<void(WiFiState, const String&)>;
    using ScanCallback = std::function<void(const std::vector<NetworkInfo>&)>;

    WiFiManager();
    ~WiFiManager();

    /**
     * Initialize the WiFi manager
     * Loads saved networks and attempts to connect if enabled
     */
    void begin();

    /**
     * Update the WiFi state machine
     * Must be called regularly from the main loop
     */
    void update();

    // Configuration Methods
    /**
     * Set callback for WiFi status updates
     * @param cb Callback function receiving state and status message
     */
    void setStatusCallback(StatusCallback cb);

    /**
     * Set callback for WiFi scan results
     * @param cb Callback function receiving vector of NetworkInfo
     */
    void setScanCallback(ScanCallback cb);

    /**
     * Enable or disable WiFi
     * When disabled, WiFi hardware is turned off
     * @param enabled true to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * @return true if WiFi is enabled
     */
    bool isEnabled() const;

    /**
     * @return true if WiFiManager is initialized
     */
    bool isInitialized() const;

    // Connection Methods
    /**
     * Initiates an asynchronous connection attempt to a WiFi network
     * @param ssid Network SSID to connect to
     * @param password Network password
     * @param save If true, network will be saved for future connections
     * @param priority Connection priority (higher = more preferred)
     * @return true if connection attempt was initiated, false if preconditions fail
     */
    bool connect(const String& ssid, const String& password, bool save = true, int priority = 0);

    /**
     * Attempts to connect to the highest priority saved network
     * Connection result will be reported via status callback
     * @return true if connection attempt was initiated, false if no networks available
     */
    bool connectToBestNetwork();

    /**
     * Disconnects from the current network
     * @param manual If true, indicates user-initiated disconnect
     */
    void disconnect(bool manual = true);

    /**
     * @return true if currently connected to a network
     */
    bool isConnected() const;

    /**
     * @return SSID of currently connected network, or empty string if not connected
     */
    String getCurrentSSID() const;

    /**
     * @return Current connection signal strength in dBm, or 0 if not connected
     */
    int getRSSI() const;

    /**
     * @return Current IP address as string, or empty string if not connected
     */
    String getIPAddress() const;

    // Scanning Methods
    /**
     * Start an asynchronous network scan
     * @return true if scan was started, false if preconditions fail
     */
    bool startScan();

    /**
     * @return true if a scan is in progress
     */
    bool isScanning() const;

    /**
     * @return vector of NetworkInfo from last scan
     */
    std::vector<NetworkInfo> getScanResults() const;

    // Network Management Methods
    /**
     * Add or update a network in the saved networks list
     * @param ssid Network SSID
     * @param password Network password
     * @param priority Connection priority (higher = more preferred)
     * @return true if network was added/updated successfully
     */
    bool addNetwork(const String& ssid, const String& password, int priority = 0);

    /**
     * Remove a network from the saved networks list
     * @param ssid Network SSID to remove
     * @return true if network was found and removed
     */
    bool removeNetwork(const String& ssid);

    /**
     * Update the priority of a saved network
     * @param ssid Network SSID
     * @param priority New priority value (higher = more preferred)
     * @return true if network was found and priority was updated
     */
    bool setNetworkPriority(const String& ssid, int priority);

    /**
     * @return Vector of all saved networks, sorted by priority
     */
    std::vector<NetworkInfo> getSavedNetworks() const;

    /**
     * Save current network list to persistent storage
     */
    void saveNetworks();

    /**
     * Load saved networks from persistent storage
     */
    void loadSavedNetworks();

    // State Methods
    /**
     * @return Current WiFi state
     */
    WiFiState getState() const;

    /**
     * @return Human-readable string representing current WiFi state
     */
    String getStateString() const;

private:
    WiFiState _state;
    bool _enabled;
    bool _manualDisconnect;
    bool _initialized;
    bool _scanInProgress;
    unsigned long _lastConnectionAttempt;
    unsigned long _scanStartTime;
    int _connectionAttempts;
    String _connectingSSID;
    String _connectingPassword;
    std::vector<NetworkInfo> _savedNetworks;
    std::vector<NetworkInfo> _scanResults;
    Preferences _preferences;
    StatusCallback _statusCallback;
    ScanCallback _scanCallback;

    static const unsigned long CONNECTION_TIMEOUT = 10000; // 10s
    static const unsigned long SCAN_TIMEOUT = 8000;       // 8s
    static const unsigned long RECONNECT_INTERVAL = 30000; // 30s between retries
    static const int MAX_CONNECTION_ATTEMPTS = 3;
    static const int MAX_SAVED_NETWORKS = 5;

    void updateState();
    void notifyStatus(const String& message);
    void sortNetworksByPriority(std::vector<NetworkInfo>& networks);
    int findNetwork(const String& ssid, const std::vector<NetworkInfo>& networks) const;
};
#endif
