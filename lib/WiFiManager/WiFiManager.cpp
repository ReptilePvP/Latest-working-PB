#include "WiFiManager.h"
#include "esp_log.h" // For ESP logging

static const char *TAG = "WiFiManager";

WiFiManager::WiFiManager(bool enabled)
    : _state(WiFiState::WIFI_DISABLED)
    , _enabled(enabled)
    , _manualDisconnect(false)
    , _initialized(false)
    , _scanInProgress(false)
    , _lastConnectionAttempt(0)
    , _scanStartTime(0)
    , _connectionAttempts(0)
    , _lastReconnectAttempt(0)
    , _wifiTaskHandle(NULL)
    , _commandQueue(NULL)
    , _resultQueue(NULL) {
    _preferences.begin("wifi_networks", false);
    ESP_LOGI(TAG, "WiFiManager constructed with enabled=%d", _enabled);
}

WiFiManager::~WiFiManager() {
    if (_initialized) {
        ESP_LOGI(TAG, "Shutting down WiFi Manager...");
        if (_commandQueue) {
            WiFiCommand cmd = {WiFiCommandType::CMD_TERMINATE};
            if (xQueueSend(_commandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Failed to send terminate command");
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (_commandQueue) {
            vQueueDelete(_commandQueue);
            _commandQueue = nullptr;
        }
        if (_resultQueue) {
            vQueueDelete(_resultQueue);
            _resultQueue = nullptr;
        }

        if (_enabled) {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            ESP_LOGI(TAG, "WiFi turned off during shutdown");
        }

        saveNetworks();
        _preferences.end();
        _wifiTaskHandle = nullptr;
        _initialized = false;
        ESP_LOGI(TAG, "WiFi Manager shutdown complete.");
    }
}

void WiFiManager::begin() {
    if (_initialized) {
        ESP_LOGW(TAG, "WiFiManager already initialized, skipping begin()");
        return;
    }

    _commandQueue = xQueueCreate(WIFI_COMMAND_QUEUE_SIZE, sizeof(WiFiCommand));
    _resultQueue = xQueueCreate(WIFI_RESULT_QUEUE_SIZE, sizeof(WiFiResult));
    if (!_commandQueue || !_resultQueue) {
        ESP_LOGE(TAG, "Failed to create queues");
        if (_commandQueue) vQueueDelete(_commandQueue);
        if (_resultQueue) vQueueDelete(_resultQueue);
        _commandQueue = nullptr;
        _resultQueue = nullptr;
        return;
    }

    loadSavedNetworks();
    _state = WiFiState::WIFI_DISCONNECTED;
    _scanInProgress = false;
    ESP_LOGI(TAG, "WiFiManager initializing with %d saved networks", _savedNetworks.size());

    WiFiCommand cmd;
    cmd.type = _enabled ? WiFiCommandType::CMD_ENABLE : WiFiCommandType::CMD_DISABLE;
    if (xQueueSend(_commandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send initial command to task");
        vQueueDelete(_commandQueue);
        vQueueDelete(_resultQueue);
        _commandQueue = nullptr;
        _resultQueue = nullptr;
        return;
    }

    BaseType_t taskCreated = xTaskCreatePinnedToCore(&wifiTaskLoop, "WiFiTask", WIFI_TASK_STACK_SIZE, this, WIFI_TASK_PRIORITY, &_wifiTaskHandle, 1);
    if (taskCreated != pdPASS) {
        ESP_LOGE(TAG, "Failed to create WiFi task");
        vQueueDelete(_commandQueue);
        vQueueDelete(_resultQueue);
        _commandQueue = nullptr;
        _resultQueue = nullptr;
        return;
    }

    _initialized = true;
    notifyStatus(_enabled ? "WiFi enabling..." : "WiFi disabled");
    ESP_LOGI(TAG, "WiFiManager initialized successfully");
}

void WiFiManager::update() {
    if (!_initialized || !_resultQueue) return;

    static unsigned long lastScanStart = 0;
    const unsigned long SCAN_TIMEOUT = 15000;

    WiFiResult result;
    if (xQueueReceive(_resultQueue, &result, 0) == pdPASS) {
        ESP_LOGD(TAG, "Received result: Type %d, State %d, Msg=%s", 
                 (int)result.type, (int)result.newState, result.message.c_str());

        _state = result.newState;
        _scanInProgress = (result.newState == WiFiState::WIFI_SCANNING || 
                           result.newState == WiFiState::WIFI_SCAN_REQUESTED);

        if (result.type == WiFiResultType::RESULT_STATUS_UPDATE) {
            if (_statusCallback) {
                _statusCallback(_state, result.message);
            }
        } else if (result.type == WiFiResultType::RESULT_SCAN_COMPLETE) {
            _scanResults = result.scanResults;
            _scanInProgress = false; // Force reset
            ESP_LOGI(TAG, "Scan completed with %d networks, resetting _scanInProgress", _scanResults.size());
            if (_scanCallback) {
                _scanCallback(_scanResults);
            }
            if (_statusCallback) {
                _statusCallback(_state, result.message);
            }
            lastScanStart = 0;
        }
    } else if (_scanInProgress) {
        ESP_LOGD(TAG, "No result yet, scan in progress, state=%d", (int)_state);
    }

    if (_scanInProgress && lastScanStart > 0 && (millis() - lastScanStart > SCAN_TIMEOUT)) {
        ESP_LOGW(TAG, "Scan timed out after %lu ms, forcing reset", SCAN_TIMEOUT);
        _scanInProgress = false;
        _state = WiFiState::WIFI_DISCONNECTED;
        notifyStatus("Scan timed out");
        if (_scanCallback) {
            _scanCallback(_scanResults);
        }
        lastScanStart = 0;
    }

    if (_scanInProgress && lastScanStart == 0) {
        lastScanStart = millis();
    }
}

void WiFiManager::setEnabled(bool enabled) {
    if (!_initialized || !_commandQueue) return;
    if (_enabled == enabled) return;

    _enabled = enabled;
    _preferences.putBool("enabled", enabled);

    WiFiCommand cmd;
    cmd.type = enabled ? WiFiCommandType::CMD_ENABLE : WiFiCommandType::CMD_DISABLE;

    if (xQueueSend(_commandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send %s command to queue", enabled ? "ENABLE" : "DISABLE");
    } else {
        ESP_LOGI(TAG, "Sent %s command to WiFi task", enabled ? "ENABLE" : "DISABLE");
        _state = enabled ? WiFiState::WIFI_DISCONNECTED : WiFiState::WIFI_DISABLED;
        notifyStatus(enabled ? "WiFi enabling..." : "WiFi disabling...");
    }
}

bool WiFiManager::connect(const String& ssid, const String& password, bool save, int priority) {
    if (!_initialized || !_commandQueue || !_enabled) return false;

    WiFiCommand cmd;
    cmd.type = WiFiCommandType::CMD_CONNECT;
    cmd.ssid = ssid;
    cmd.password = password;
    cmd.save = save;
    cmd.priority = priority;

    if (save) {
        addNetwork(ssid, password, priority);
    }

    if (xQueueSend(_commandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send CONNECT command for %s", ssid.c_str());
        return false;
    } else {
        ESP_LOGI(TAG, "Sent CONNECT command for %s", ssid.c_str());
        _state = WiFiState::WIFI_CONNECT_REQUESTED;
        _connectingSSID = ssid;
        notifyStatus("Connection requested for " + ssid);
        return true;
    }
}

bool WiFiManager::connectToBestNetwork() {
    if (!_initialized || !_commandQueue || !_enabled || _savedNetworks.empty()) return false;

    sortNetworksByPriority(_savedNetworks);
    const auto& bestNetwork = _savedNetworks[0];

    return connect(bestNetwork.ssid, bestNetwork.password, false);
}

void WiFiManager::disconnect(bool manual) {
    if (!_initialized || !_commandQueue || !_enabled) return;

    WiFiCommand cmd;
    cmd.type = WiFiCommandType::CMD_DISCONNECT;
    cmd.manualDisconnect = manual;

    if (xQueueSend(_commandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send DISCONNECT command");
    } else {
        ESP_LOGI(TAG, "Sent DISCONNECT command (manual: %s)", manual ? "true" : "false");
        _state = WiFiState::WIFI_DISCONNECTED;
        _manualDisconnect = manual;
        notifyStatus("Disconnect requested");
    }
}

bool WiFiManager::startScan() {
    if (!_initialized || !_commandQueue || !_enabled) {
        ESP_LOGW(TAG, "Cannot start scan: not initialized or not enabled");
        return false;
    }
    if (_scanInProgress) {
        ESP_LOGW(TAG, "Scan already in progress");
        return false;
    }

    WiFiCommand cmd;
    cmd.type = WiFiCommandType::CMD_START_SCAN;

    if (xQueueSend(_commandQueue, &cmd, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send START_SCAN command");
        return false;
    } else {
        ESP_LOGI(TAG, "Sent START_SCAN command");
        _state = WiFiState::WIFI_SCAN_REQUESTED;
        _scanInProgress = true;
        notifyStatus("Scan requested");
        return true;
    }
}

bool WiFiManager::isEnabled() const {
    return _enabled;
}

bool WiFiManager::isInitialized() const {
    return _initialized;
}

bool WiFiManager::isConnected() const {
    return _state == WiFiState::WIFI_CONNECTED;
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

bool WiFiManager::isScanning() const {
    return _scanInProgress;
}

std::vector<NetworkInfo> WiFiManager::getScanResults() const {
    return _scanResults;
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
        case WiFiState::WIFI_SCAN_REQUESTED: return "Scan Requested";
        case WiFiState::WIFI_CONNECT_REQUESTED: return "Connect Requested";
        default: return "Unknown";
    }
}

bool WiFiManager::addNetwork(const String& ssid, const String& password, int priority) {
    if (!_initialized) return false;
    int existingIndex = findNetwork(ssid, _savedNetworks);
    bool changed = false;
    if (existingIndex >= 0) {
        if (_savedNetworks[existingIndex].password != password || _savedNetworks[existingIndex].priority != priority) {
            _savedNetworks[existingIndex].password = password;
            _savedNetworks[existingIndex].priority = priority;
            changed = true;
        }
    } else if (_savedNetworks.size() < MAX_SAVED_NETWORKS) {
        NetworkInfo network;
        network.ssid = ssid;
        network.password = password;
        network.priority = priority;
        network.saved = true;
        network.connected = false;
        _savedNetworks.push_back(network);
        changed = true;
    } else {
        sortNetworksByPriority(_savedNetworks);
        if (priority < _savedNetworks.back().priority) {
            ESP_LOGI(TAG, "Replacing network %s (priority %d) with %s (priority %d)",
                     _savedNetworks.back().ssid.c_str(), _savedNetworks.back().priority,
                     ssid.c_str(), priority);
            _savedNetworks.back() = {ssid, password, 0, WIFI_AUTH_OPEN, true, false, priority};
            changed = true;
        } else {
            ESP_LOGW(TAG, "Max saved networks reached (%d), and new network priority (%d) not higher than lowest (%d)",
                     MAX_SAVED_NETWORKS, priority, _savedNetworks.back().priority);
            return false;
        }
    }

    if (changed) {
        sortNetworksByPriority(_savedNetworks);
        saveNetworks();
        ESP_LOGI(TAG, "Network %s %s/updated.", ssid.c_str(), (existingIndex >= 0) ? "updated" : "added");
    }
    return changed;
}

void WiFiManager::addNetwork(const NetworkInfo& network) {
    if (!_initialized) return;
    addNetwork(network.ssid, network.password, network.priority);
}

bool WiFiManager::removeNetwork(const String& ssid) {
    if (!_initialized) return false;
    int index = findNetwork(ssid, _savedNetworks);
    if (index < 0) return false;

    bool wasConnected = isConnected() && (ssid == getCurrentSSID());
    _savedNetworks.erase(_savedNetworks.begin() + index);
    saveNetworks();
    ESP_LOGI(TAG, "Removed network %s.", ssid.c_str());

    if (wasConnected) {
        disconnect(false);
    }
    return true;
}

bool WiFiManager::setNetworkPriority(const String& ssid, int priority) {
    if (!_initialized) return false;
    int index = findNetwork(ssid, _savedNetworks);
    if (index < 0) return false;

    if (_savedNetworks[index].priority != priority) {
        _savedNetworks[index].priority = priority;
        sortNetworksByPriority(_savedNetworks);
        saveNetworks();
        ESP_LOGI(TAG, "Updated priority for network %s to %d.", ssid.c_str(), priority);
    }
    return true;
}

std::vector<NetworkInfo> WiFiManager::getSavedNetworks() const {
    return _savedNetworks;
}

void WiFiManager::saveNetworks() {
    if (!_initialized) return;
    ESP_LOGD(TAG, "Saving %d networks to Preferences...", _savedNetworks.size());
    size_t previousCount = _preferences.getUInt("net_count", 0);
    for (size_t i = 0; i < previousCount; ++i) {
        String ssidKey = "ssid_" + String(i);
        String pwdKey = "pwd_" + String(i);
        String priKey = "pri_" + String(i);
        if (_preferences.isKey(ssidKey.c_str())) _preferences.remove(ssidKey.c_str());
        if (_preferences.isKey(pwdKey.c_str())) _preferences.remove(pwdKey.c_str());
        if (_preferences.isKey(priKey.c_str())) _preferences.remove(priKey.c_str());
    }

    _preferences.putUInt("net_count", _savedNetworks.size());
    for (size_t i = 0; i < _savedNetworks.size(); ++i) {
        _preferences.putString(("ssid_" + String(i)).c_str(), _savedNetworks[i].ssid);
        _preferences.putString(("pwd_" + String(i)).c_str(), _savedNetworks[i].password);
        _preferences.putInt(("pri_" + String(i)).c_str(), _savedNetworks[i].priority);
    }
    ESP_LOGD(TAG, "Networks saved.");
}

void WiFiManager::loadSavedNetworks() {
    if (!_initialized) {
        ESP_LOGD(TAG, "Loading networks from Preferences...");
        _savedNetworks.clear();
        size_t networkCount = _preferences.getUInt("net_count", 0);
        networkCount = std::min(networkCount, (size_t)MAX_SAVED_NETWORKS);

        for (size_t i = 0; i < networkCount; ++i) {
            String ssid = _preferences.getString(("ssid_" + String(i)).c_str(), "");
            if (ssid.isEmpty()) continue;

            String password = _preferences.getString(("pwd_" + String(i)).c_str(), "");
            int priority = _preferences.getInt(("pri_" + String(i)).c_str(), 0);

            NetworkInfo network;
            network.ssid = ssid;
            network.password = password;
            network.priority = priority;
            network.saved = true;
            network.connected = false;
            _savedNetworks.push_back(network);
            ESP_LOGD(TAG, "Loaded network: %s (Priority: %d)", ssid.c_str(), priority);
        }
        sortNetworksByPriority(_savedNetworks);
        ESP_LOGD(TAG, "Networks loaded.");
    } else {
        ESP_LOGW(TAG, "loadSavedNetworks called after initialization, ignoring.");
    }
}

void WiFiManager::setStatusCallback(StatusCallback cb) {
    _statusCallback = cb;
    ESP_LOGI(TAG, "Status callback set");
}

void WiFiManager::setScanCallback(ScanCallback cb) {
    _scanCallback = cb;
    ESP_LOGI(TAG, "Scan callback set");
}

void WiFiManager::notifyStatus(const String& message) {
    if (_statusCallback) {
        _statusCallback(_state, message);
    }
    ESP_LOGD(TAG, "Notify Status: State=%d, Msg=%s", (int)_state, message.c_str());
}

void WiFiManager::sortNetworksByPriority(std::vector<NetworkInfo>& networks) {
    std::sort(networks.begin(), networks.end(),
        [](const NetworkInfo& a, const NetworkInfo& b) {
            return a.priority < b.priority; // Lower number = higher priority
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

void WiFiManager::wifiTaskLoop(void* parameter) {
    WiFiManager* instance = static_cast<WiFiManager*>(parameter);
    WiFiCommand cmd;
    WiFiResult result;
    bool taskEnabled = false;
    bool taskManualDisconnect = false;
    unsigned long lastReconnectAttemptTime = 0;
    String currentConnectingSSID = "";

    ESP_LOGI(TAG, "WiFi Task Started on Core %d", xPortGetCoreID());

    if (xQueueReceive(instance->_commandQueue, &cmd, pdMS_TO_TICKS(1000)) == pdPASS) {
        if (cmd.type == WiFiCommandType::CMD_ENABLE) {
            taskEnabled = true;
            WiFi.mode(WIFI_STA);
            ESP_LOGI(TAG, "Task received initial ENABLE command");
            if (!instance->_savedNetworks.empty()) {
                instance->sortNetworksByPriority(instance->_savedNetworks);
                currentConnectingSSID = instance->_savedNetworks[0].ssid;
                ESP_LOGI(TAG, "Task: Attempting initial connection to %s", currentConnectingSSID.c_str());
                WiFi.begin(instance->_savedNetworks[0].ssid.c_str(), instance->_savedNetworks[0].password.c_str());
                result.newState = WiFiState::WIFI_CONNECTING;
                result.message = "Connecting to " + currentConnectingSSID;
                result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                xQueueSend(instance->_resultQueue, &result, 0);
                lastReconnectAttemptTime = millis();
            } else {
                result.newState = WiFiState::WIFI_DISCONNECTED;
                result.message = "WiFi enabled, no saved networks";
                result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                xQueueSend(instance->_resultQueue, &result, 0);
            }
        } else if (cmd.type == WiFiCommandType::CMD_DISABLE) {
            taskEnabled = false;
            WiFi.mode(WIFI_OFF);
            ESP_LOGI(TAG, "Task received initial DISABLE command");
            result.newState = WiFiState::WIFI_DISABLED;
            result.message = "WiFi disabled";
            result.type = WiFiResultType::RESULT_STATUS_UPDATE;
            xQueueSend(instance->_resultQueue, &result, 0);
        }
    } else {
        ESP_LOGW(TAG, "WiFi Task did not receive initial command within 1s. Assuming disabled.");
        taskEnabled = false;
        WiFi.mode(WIFI_OFF);
        result.newState = WiFiState::WIFI_DISABLED;
        result.message = "WiFi task started, disabled by default";
        result.type = WiFiResultType::RESULT_STATUS_UPDATE;
        xQueueSend(instance->_resultQueue, &result, 0);
    }

    while (true) {
        // ESP_LOGD(TAG, "Task loop iteration"); // Commented out noisy log

        WiFiState currentState = WiFi.status() == WL_CONNECTED ? WiFiState::WIFI_CONNECTED : WiFiState::WIFI_DISCONNECTED;
        if (!taskEnabled) currentState = WiFiState::WIFI_DISABLED;

        if (xQueueReceive(instance->_commandQueue, &cmd, pdMS_TO_TICKS(100)) == pdPASS) {
            ESP_LOGD(TAG, "Task received command: %d", (int)cmd.type);

            if (cmd.type == WiFiCommandType::CMD_TERMINATE) {
                ESP_LOGI(TAG, "WiFi Task received TERMINATE command. Exiting.");
                break;
            }

            if (cmd.type == WiFiCommandType::CMD_ENABLE) {
                if (!taskEnabled) {
                    taskEnabled = true;
                    taskManualDisconnect = false;
                    WiFi.mode(WIFI_STA);
                    ESP_LOGI(TAG, "Task: Enabling WiFi");
                    if (!instance->_savedNetworks.empty()) {
                        instance->sortNetworksByPriority(instance->_savedNetworks);
                        currentConnectingSSID = instance->_savedNetworks[0].ssid;
                        ESP_LOGI(TAG, "Task: Attempting connection to %s", currentConnectingSSID.c_str());
                        WiFi.begin(instance->_savedNetworks[0].ssid.c_str(), instance->_savedNetworks[0].password.c_str());
                        result.newState = WiFiState::WIFI_CONNECTING;
                        result.message = "Connecting to " + currentConnectingSSID;
                        lastReconnectAttemptTime = millis();
                    } else {
                        result.newState = WiFiState::WIFI_DISCONNECTED;
                        result.message = "WiFi enabled, no saved networks";
                    }
                    result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                    if (xQueueSend(instance->_resultQueue, &result, 0) != pdPASS) {
                        ESP_LOGE(TAG, "Task: Failed to send ENABLE status to result queue");
                    }
                }
            } else if (cmd.type == WiFiCommandType::CMD_DISABLE) {
                if (taskEnabled) {
                    taskEnabled = false;
                    WiFi.disconnect(true);
                    WiFi.mode(WIFI_OFF);
                    ESP_LOGI(TAG, "Task: Disabling WiFi");
                    result.newState = WiFiState::WIFI_DISABLED;
                    result.message = "WiFi disabled";
                    result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                    if (xQueueSend(instance->_resultQueue, &result, 0) != pdPASS) {
                        ESP_LOGE(TAG, "Task: Failed to send DISABLE status to result queue");
                    }
                }
            } else if (taskEnabled) {
                if (cmd.type == WiFiCommandType::CMD_START_SCAN) {
                    ESP_LOGI(TAG, "Task: Received CMD_START_SCAN");
                    if (currentState != WiFiState::WIFI_SCANNING) {
                        ESP_LOGI(TAG, "Task: Starting WiFi scan");
                        result.newState = WiFiState::WIFI_SCANNING;
                        result.message = "Scanning for networks...";
                        result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                        if (xQueueSend(instance->_resultQueue, &result, 0) != pdPASS) {
                            ESP_LOGE(TAG, "Task: Failed to send SCANNING status to result queue");
                        }

                        unsigned long scanStart = millis();
                        ESP_LOGI(TAG, "Task: Calling WiFi.scanNetworks(false, false)... @ %lu ms", scanStart);
                        int scanResultCount = WiFi.scanNetworks(false, false);
                        unsigned long scanEnd = millis();
                        ESP_LOGI(TAG, "Task: Scan finished in %lu ms, result code: %d @ %lu ms", scanEnd - scanStart, scanResultCount, scanEnd);

                        result.scanResults.clear();
                        if (scanResultCount > 0) {
                            ESP_LOGI(TAG, "Task: Processing %d found networks...", scanResultCount);
                            for (int i = 0; i < scanResultCount; i++) {
                                NetworkInfo network;
                                network.ssid = WiFi.SSID(i);
                                network.rssi = WiFi.RSSI(i);
                                network.encryptionType = WiFi.encryptionType(i);
                                network.saved = (instance->findNetwork(network.ssid, instance->_savedNetworks) >= 0);
                                network.connected = (WiFi.status() == WL_CONNECTED) && (network.ssid == WiFi.SSID());
                                result.scanResults.push_back(network);
                                ESP_LOGD(TAG, "Found network: %s, RSSI: %d", network.ssid.c_str(), network.rssi);
                            }
                            std::sort(result.scanResults.begin(), result.scanResults.end(),
                                      [](const NetworkInfo& a, const NetworkInfo& b) { return a.rssi > b.rssi; });
                        } else if (scanResultCount == 0) {
                            ESP_LOGI(TAG, "Task: No networks found");
                        } else {
                            ESP_LOGE(TAG, "Task: Scan failed with error code %d", scanResultCount);
                        }
                        WiFi.scanDelete();

                        result.type = WiFiResultType::RESULT_SCAN_COMPLETE;
                        result.newState = (WiFi.status() == WL_CONNECTED) ? WiFiState::WIFI_CONNECTED : WiFiState::WIFI_DISCONNECTED;
                        result.message = (scanResultCount >= 0) ? "Scan complete" : "Scan failed";
                        ESP_LOGI(TAG, "Task: Sending SCAN_COMPLETE result. State: %d, Msg: %s", (int)result.newState, result.message.c_str());
                        if (xQueueSend(instance->_resultQueue, &result, 0) != pdPASS) {
                            ESP_LOGE(TAG, "Task: Failed to send SCAN_COMPLETE to result queue");
                        } else {
                            ESP_LOGI(TAG, "Task: Sent SCAN_COMPLETE to result queue");
                        }
                    } else {
                        ESP_LOGW(TAG, "Task: Scan already in progress, ignoring command");
                    }
                } else if (cmd.type == WiFiCommandType::CMD_CONNECT) {
                    ESP_LOGI(TAG, "Task: Received connect command for %s", cmd.ssid.c_str());
                    taskManualDisconnect = false;
                    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == cmd.ssid) {
                        ESP_LOGI(TAG, "Task: Already connected to %s", cmd.ssid.c_str());
                        result.newState = WiFiState::WIFI_CONNECTED;
                        result.message = "Already connected to " + cmd.ssid;
                        result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                        xQueueSend(instance->_resultQueue, &result, 0);
                    } else {
                        if (WiFi.status() == WL_CONNECTED) {
                            WiFi.disconnect();
                            vTaskDelay(pdMS_TO_TICKS(100));
                        }
                        currentConnectingSSID = cmd.ssid;
                        WiFi.begin(cmd.ssid.c_str(), cmd.password.c_str());
                        ESP_LOGI(TAG, "Task: Attempting connection to %s", cmd.ssid.c_str());
                        result.newState = WiFiState::WIFI_CONNECTING;
                        result.message = "Connecting to " + cmd.ssid;
                        result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                        xQueueSend(instance->_resultQueue, &result, 0);
                        lastReconnectAttemptTime = millis();
                    }
                } else if (cmd.type == WiFiCommandType::CMD_DISCONNECT) {
                    if (WiFi.status() == WL_CONNECTED) {
                        WiFi.disconnect(true);
                        ESP_LOGI(TAG, "Task: Disconnected (Manual: %s)", cmd.manualDisconnect ? "true" : "false");
                        result.newState = WiFiState::WIFI_DISCONNECTED;
                        result.message = cmd.manualDisconnect ? "Disconnected by user" : "Disconnected";
                        result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                        xQueueSend(instance->_resultQueue, &result, 0);
                    }
                    taskManualDisconnect = cmd.manualDisconnect;
                    currentConnectingSSID = "";
                }
            }
        }

        if (taskEnabled) {
            WiFiState reportedState = WiFiState::WIFI_DISABLED;
            String reportedMessage = "";
            bool shouldReport = false;

            if (instance->_state == WiFiState::WIFI_CONNECTING || instance->_state == WiFiState::WIFI_CONNECT_REQUESTED) {
                if (WiFi.status() == WL_CONNECTED) {
                    ESP_LOGI(TAG, "Task: Connection successful to %s", WiFi.SSID().c_str());
                    reportedState = WiFiState::WIFI_CONNECTED;
                    instance->_state = reportedState; // Update state immediately
                    reportedMessage = "Connected to " + WiFi.SSID();
                    shouldReport = true;
                    currentConnectingSSID = "";
                } else if (millis() - lastReconnectAttemptTime > CONNECTION_TIMEOUT) {
                    ESP_LOGW(TAG, "Task: Connection timed out for %s", currentConnectingSSID.c_str());
                    WiFi.disconnect();
                    reportedState = WiFiState::WIFI_CONNECTION_FAILED;
                    reportedMessage = "Connection failed for " + currentConnectingSSID;
                    shouldReport = true;
                    currentConnectingSSID = "";
                    lastReconnectAttemptTime = millis();
                }
            }

            if (instance->_state == WiFiState::WIFI_CONNECTED && WiFi.status() != WL_CONNECTED && !taskManualDisconnect) {
                ESP_LOGW(TAG, "Task: Connection lost");
                reportedState = WiFiState::WIFI_DISCONNECTED;
                reportedMessage = "Connection lost";
                shouldReport = true;
                lastReconnectAttemptTime = millis();
                currentConnectingSSID = "";
            }

            if ((instance->_state == WiFiState::WIFI_DISCONNECTED || instance->_state == WiFiState::WIFI_CONNECTION_FAILED) &&
                !taskManualDisconnect && !instance->_savedNetworks.empty() &&
                millis() - lastReconnectAttemptTime > RECONNECT_INTERVAL) {
                ESP_LOGI(TAG, "Task: Attempting automatic reconnect...");
                instance->sortNetworksByPriority(instance->_savedNetworks);
                currentConnectingSSID = instance->_savedNetworks[0].ssid;
                WiFi.begin(instance->_savedNetworks[0].ssid.c_str(), instance->_savedNetworks[0].password.c_str());
                reportedState = WiFiState::WIFI_CONNECTING;
                reportedMessage = "Reconnecting to " + currentConnectingSSID;
                shouldReport = true;
                lastReconnectAttemptTime = millis();
            }

            if (shouldReport) {
                result.newState = reportedState;
                result.message = reportedMessage;
                result.type = WiFiResultType::RESULT_STATUS_UPDATE;
                if (xQueueSend(instance->_resultQueue, &result, 0) != pdPASS) {
                    ESP_LOGE(TAG, "Task: Failed to send status update to result queue");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (taskEnabled) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        ESP_LOGI(TAG, "Task: WiFi turned off during cleanup");
    }
    ESP_LOGI(TAG, "WiFi Task exiting.");
    vTaskDelete(NULL);
}