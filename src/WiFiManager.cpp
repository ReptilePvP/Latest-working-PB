#include "WiFiManager.h"

WiFiManager::WiFiManager()
    : _state(WiFiState::WIFI_DISABLED)
    , _enabled(false)
    , _manualDisconnect(false)
    , _initialized(false)
    , _scanInProgress(false)
    , _lastConnectionAttempt(0)
    , _scanStartTime(0)
    , _connectionAttempts(0) {
}

WiFiManager::~WiFiManager() {
    if (_initialized) {
        saveNetworks();
        _preferences.end();
    }
}

void WiFiManager::begin() {
    if (_initialized) return;

    _preferences.begin("wifi", false);
    _enabled = _preferences.getBool("enabled", true);
    
    WiFi.mode(WIFI_STA);
    _state = _enabled ? WiFiState::WIFI_DISCONNECTED : WiFiState::WIFI_DISABLED;
    
    loadSavedNetworks();
    _initialized = true;
    
    if (_enabled) {
        connectToBestNetwork();
    }
    
    notifyStatus("WiFi Manager initialized");
}

void WiFiManager::update() {
    if (!_initialized || !_enabled) return;

    // Handle scanning state
    if (_state == WiFiState::WIFI_SCANNING) {
        int8_t scanResult = WiFi.scanComplete(); // Check scan status

        // Check if scan finished (successfully or with 0 results)
        if (scanResult >= 0) {
            _scanResults.clear(); // Clear previous results regardless

            // If networks were found, process them
            if (scanResult > 0) {
                for (int i = 0; i < scanResult; i++) {
                    NetworkInfo network;
                    network.ssid = WiFi.SSID(i);
                    network.rssi = WiFi.RSSI(i);
                    network.encryptionType = WiFi.encryptionType(i);
                    network.saved = (findNetwork(network.ssid, _savedNetworks) >= 0);
                    // Check if this network is the one we are currently connected to
                    network.connected = isConnected() && (network.ssid == WiFi.SSID());
                    _scanResults.push_back(network);
                }
                // Sort by signal strength (strongest first)
                std::sort(_scanResults.begin(), _scanResults.end(),
                    [](const NetworkInfo& a, const NetworkInfo& b) {
                        return a.rssi > b.rssi;
                    });
            }

            // Scan is no longer in progress
            _scanInProgress = false;
            // Update state based on current connection status
            _state = isConnected() ? WiFiState::WIFI_CONNECTED : WiFiState::WIFI_DISCONNECTED;
            // Notify about scan completion
            notifyStatus(scanResult > 0 ? "Scan complete" : "Scan complete (0 networks found)");
            // Trigger the callback with the results (potentially empty)
            if (_scanCallback) {
                 _scanCallback(_scanResults);
            }
            // IMPORTANT: Free scan results memory
            WiFi.scanDelete();
        }
        // Check if scan failed OR if it's still running but has timed out
        else if (scanResult == WIFI_SCAN_FAILED || (scanResult == WIFI_SCAN_RUNNING && millis() - _scanStartTime > SCAN_TIMEOUT)) {
            // Scan is no longer in progress
            _scanInProgress = false;
            // Update state based on current connection status
            _state = isConnected() ? WiFiState::WIFI_CONNECTED : WiFiState::WIFI_DISCONNECTED;
            // Notify about scan failure or timeout
            notifyStatus(scanResult == WIFI_SCAN_FAILED ? "Scan failed" : "Scan timed out");
            // Trigger the callback with an empty vector to signal failure/timeout
            if (_scanCallback) {
                 _scanCallback(std::vector<NetworkInfo>());
            }
             // IMPORTANT: Free scan results memory even on failure/timeout
            WiFi.scanDelete();
        }
        // If scanResult == WIFI_SCAN_RUNNING and not timed out, do nothing and wait for the next update() call
    }

    // Handle connecting state
    else if (_state == WiFiState::WIFI_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = WiFiState::WIFI_CONNECTED;
            _connectionAttempts = 0; // Reset attempts on success

            // Mark the network as connected in our saved list
            int networkIndex = findNetwork(_connectingSSID, _savedNetworks);
            if (networkIndex >= 0) {
                // Reset all others first
                for(auto& net : _savedNetworks) { net.connected = false; }
                _savedNetworks[networkIndex].connected = true;
            }

            notifyStatus("Connected to " + WiFi.SSID());
        }
        // Check for various failure conditions or timeout
        else if (WiFi.status() == WL_CONNECT_FAILED ||
                 WiFi.status() == WL_NO_SSID_AVAIL ||
                 WiFi.status() == WL_IDLE_STATUS || // Can sometimes indicate failure too
                 millis() - _lastConnectionAttempt > CONNECTION_TIMEOUT) {

            _connectionAttempts++;
            if (_connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
                _state = WiFiState::WIFI_CONNECTION_FAILED;
                notifyStatus("Connection failed to " + _connectingSSID);
                 // Optionally: Try the next saved network automatically here
                 // connectToBestNetwork(); // Be careful of potential loops
            } else {
                // Retry connection
                notifyStatus("Retrying connection to " + _connectingSSID + " (" + String(_connectionAttempts) + ")");
                _lastConnectionAttempt = millis(); // Update attempt time before retrying
                WiFi.begin(_connectingSSID.c_str(), _connectingPassword.c_str());
            }
        }
    }

    // Handle connected state - check if connection dropped
    else if (_state == WiFiState::WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
        _state = WiFiState::WIFI_DISCONNECTED;
        notifyStatus("Connection lost");
         // Mark the network as disconnected in our saved list
        int networkIndex = findNetwork(_connectingSSID, _savedNetworks); // Use last connected SSID
        if (networkIndex >= 0) {
            _savedNetworks[networkIndex].connected = false;
        }
        _connectingSSID = ""; // Clear the SSID we thought we were connected to
        _lastConnectionAttempt = millis(); // Start the timer for reconnection attempts
    }

    // Handle disconnected/failed state - attempt auto-reconnect if not manually disconnected
    else if ((_state == WiFiState::WIFI_DISCONNECTED || _state == WiFiState::WIFI_CONNECTION_FAILED) &&
             !_manualDisconnect &&
             millis() - _lastConnectionAttempt > RECONNECT_INTERVAL) {
        notifyStatus("Attempting auto-reconnect...");
        connectToBestNetwork(); // Try connecting to the highest priority network
        _lastConnectionAttempt = millis(); // Reset timer after attempting reconnect
    }
}

void WiFiManager::setStatusCallback(StatusCallback cb) {
    _statusCallback = cb;
}

void WiFiManager::setScanCallback(ScanCallback cb) {
    _scanCallback = cb;
}

void WiFiManager::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    
    _enabled = enabled;
    _preferences.putBool("enabled", enabled);
    
    if (!enabled) {
        disconnect(false);
        _state = WiFiState::WIFI_DISABLED;
        notifyStatus("WiFi disabled");
        WiFi.mode(WIFI_OFF);
    } else {
        WiFi.mode(WIFI_STA);
        _state = WiFiState::WIFI_DISCONNECTED;
        notifyStatus("WiFi enabled");
        connectToBestNetwork();
    }
}

bool WiFiManager::isEnabled() const {
    return _enabled;
}

bool WiFiManager::isInitialized() const {
    return _initialized;
}

bool WiFiManager::connect(const String& ssid, const String& password, bool save, int priority) {
    if (!_enabled) return false;
    
    if (_state == WiFiState::WIFI_CONNECTING || 
        (_state == WiFiState::WIFI_CONNECTED && ssid == WiFi.SSID())) {
        return true;
    }
    
    if (_state == WiFiState::WIFI_CONNECTED) {
        disconnect(false);
    }
    
    _state = WiFiState::WIFI_CONNECTING;
    _connectingSSID = ssid;
    _connectingPassword = password;
    _connectionAttempts = 0;
    _lastConnectionAttempt = millis();
    
    if (save) {
        addNetwork(ssid, password, priority);
    }
    
    WiFi.begin(ssid.c_str(), password.c_str());
    notifyStatus("Connecting to " + ssid);
    return true;
}

bool WiFiManager::connectToBestNetwork() {
    if (!_enabled || _savedNetworks.empty()) return false;
    
    sortNetworksByPriority(_savedNetworks);
    
    for (const auto& network : _savedNetworks) {
        if (connect(network.ssid, network.password, false)) {
            return true;
        }
    }
    
    return false;
}

void WiFiManager::disconnect(bool manual) {
    _manualDisconnect = manual;
    WiFi.disconnect();
    _state = WiFiState::WIFI_DISCONNECTED;
    _connectingSSID = "";
    _connectingPassword = "";
    notifyStatus(manual ? "Disconnected by user" : "Disconnected");
}

bool WiFiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getCurrentSSID() const {
    return isConnected() ? WiFi.SSID() : "";
}

int WiFiManager::getRSSI() const {
    return isConnected() ? WiFi.RSSI() : 0;
}

String WiFiManager::getIPAddress() const {
    return isConnected() ? WiFi.localIP().toString() : "";
}

bool WiFiManager::startScan() {
    if (!_enabled || _scanInProgress) return false;
    
    _state = WiFiState::WIFI_SCANNING;
    _scanInProgress = true;
    _scanStartTime = millis();
    _scanResults.clear();
    
    if (WiFi.scanNetworks(true, false) == WIFI_SCAN_FAILED) {
        _scanInProgress = false;
        _state = isConnected() ? WiFiState::WIFI_CONNECTED : WiFiState::WIFI_DISCONNECTED;
        notifyStatus("Failed to start scan");
        return false;
    }
    
    notifyStatus("Starting WiFi scan");
    return true;
}

bool WiFiManager::isScanning() const {
    return _scanInProgress;
}

std::vector<NetworkInfo> WiFiManager::getScanResults() const {
    return _scanResults;
}

bool WiFiManager::addNetwork(const String& ssid, const String& password, int priority) {
    // Check if network already exists
    int existingIndex = findNetwork(ssid, _savedNetworks);
    if (existingIndex >= 0) {
        _savedNetworks[existingIndex].password = password;
        _savedNetworks[existingIndex].priority = priority;
    } else if (_savedNetworks.size() < MAX_SAVED_NETWORKS) {
        NetworkInfo network;
        network.ssid = ssid;
        network.password = password;
        network.priority = priority;
        network.saved = true;
        network.connected = (ssid == WiFi.SSID());
        _savedNetworks.push_back(network);
    } else {
        // Find lowest priority network
        int lowestPriority = priority;
        size_t lowestIndex = _savedNetworks.size();
        
        for (size_t i = 0; i < _savedNetworks.size(); i++) {
            if (_savedNetworks[i].priority < lowestPriority) {
                lowestPriority = _savedNetworks[i].priority;
                lowestIndex = i;
            }
        }
        
        if (lowestIndex < _savedNetworks.size()) {
            _savedNetworks[lowestIndex] = {
                ssid, password, 0, 0, true, 
                (ssid == WiFi.SSID()), priority
            };
        } else {
            return false;
        }
    }
    
    sortNetworksByPriority(_savedNetworks);
    saveNetworks();
    return true;
}

bool WiFiManager::removeNetwork(const String& ssid) {
    int index = findNetwork(ssid, _savedNetworks);
    if (index < 0) return false;
    
    _savedNetworks.erase(_savedNetworks.begin() + index);
    saveNetworks();
    return true;
}

bool WiFiManager::setNetworkPriority(const String& ssid, int priority) {
    int index = findNetwork(ssid, _savedNetworks);
    if (index < 0) return false;
    
    _savedNetworks[index].priority = priority;
    sortNetworksByPriority(_savedNetworks);
    saveNetworks();
    return true;
}

std::vector<NetworkInfo> WiFiManager::getSavedNetworks() const {
    return _savedNetworks;
}

void WiFiManager::saveNetworks() {
    if (!_initialized) return;
    
    _preferences.remove("networks"); // Clear existing networks
    
    String networksStr;
    for (const auto& network : _savedNetworks) {
        networksStr += network.ssid + "\n" + 
                      network.password + "\n" + 
                      String(network.priority) + "\n";
    }
    
    _preferences.putString("networks", networksStr);
}

void WiFiManager::loadSavedNetworks() {
    _savedNetworks.clear();
    
    String networksStr = _preferences.getString("networks", "");
    if (networksStr.isEmpty()) return;
    
    int pos = 0;
    while (pos < networksStr.length()) {
        int ssidEnd = networksStr.indexOf('\n', pos);
        if (ssidEnd < 0) break;
        String ssid = networksStr.substring(pos, ssidEnd);
        
        pos = ssidEnd + 1;
        int passwordEnd = networksStr.indexOf('\n', pos);
        if (passwordEnd < 0) break;
        String password = networksStr.substring(pos, passwordEnd);
        
        pos = passwordEnd + 1;
        int priorityEnd = networksStr.indexOf('\n', pos);
        if (priorityEnd < 0) priorityEnd = networksStr.length();
        int priority = networksStr.substring(pos, priorityEnd).toInt();
        
        NetworkInfo network;
        network.ssid = ssid;
        network.password = password;
        network.priority = priority;
        network.saved = true;
        network.connected = (ssid == WiFi.SSID());
        _savedNetworks.push_back(network);
        
        pos = priorityEnd + 1;
    }
    
    sortNetworksByPriority(_savedNetworks);
}

WiFiState WiFiManager::getState() const {
    return _state;
}

String WiFiManager::getStateString() const {
    switch (_state) {
        case WiFiState::WIFI_DISABLED: return "Disabled";
        case WiFiState::WIFI_DISCONNECTED: return "Disconnected";
        case WiFiState::WIFI_SCANNING: return "Scanning";
        case WiFiState::WIFI_CONNECTING: return "Connecting";
        case WiFiState::WIFI_CONNECTED: return "Connected";
        case WiFiState::WIFI_CONNECTION_FAILED: return "Connection Failed";
        default: return "Unknown";
    }
}

void WiFiManager::notifyStatus(const String& message) {
    if (_statusCallback) {
        _statusCallback(_state, message);
    }
}

void WiFiManager::sortNetworksByPriority(std::vector<NetworkInfo>& networks) {
    std::sort(networks.begin(), networks.end(),
        [](const NetworkInfo& a, const NetworkInfo& b) {
            return a.priority > b.priority;
        });
}

int WiFiManager::findNetwork(const String& ssid, const std::vector<NetworkInfo>& networks) const {
    for (size_t i = 0; i < networks.size(); i++) {
        if (networks[i].ssid == ssid) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
