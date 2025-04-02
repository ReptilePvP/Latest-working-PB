#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include "globals.h" // Include common definitions

// --- WiFi Event Callbacks (Used by WiFiManager) ---
void onWiFiStatus(WiFiState state, const String& message);
void onWiFiScanComplete(const std::vector<NetworkInfo>& results);

// --- Webhook ---
void sendWebhook(const String& entry);

// --- WiFi Connection ---
void connectToWiFi(const char* ssid, const char* password);

// --- Helper Functions (If any needed specifically for WiFi handling) ---
#endif // WIFI_HANDLER_H