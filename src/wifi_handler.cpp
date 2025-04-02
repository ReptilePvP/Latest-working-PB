#include "wifi_handler.h"
#include <M5Unified.h>      // Ensure base types are known (might be optional here)
#include "m5gfx_lvgl.hpp"   // Ensure LVGL types like lv_obj_t are known before globals.h
#include "globals.h"
#include "ui.h" // For updateStatus, showWiFiKeyboard, createWiFiScreen etc.
#include "time_utils.h" // Needed for getTimestamp
#include <WiFiClientSecure.h> // Needed for HTTPS webhook
#include <cstdlib> // For free
#include <cstring> // For strdup, strncpy etc.

// --- WiFi Event Callbacks ---

// Implementation from .ino lines 2922-2944
void onWiFiStatus(WiFiState state, const String& message) {
    DEBUG_PRINTF("WiFi Status: %s - %s\n", wifiManager.getStateString().c_str(), message.c_str());

    // Update status bar if visible and valid
    if (status_bar && lv_obj_is_valid(status_bar)) {
        // Use lv_label_set_text_fmt to avoid potential buffer overflows if message is long
        // lv_label_set_text(status_bar, message.c_str()); // Original
        lv_label_set_text_fmt(status_bar, "WiFi: %s", message.c_str()); // Add context
        lv_obj_set_style_text_color(status_bar,
            state == WiFiState::WIFI_CONNECTED ? lv_color_hex(0x00FF00) : // Green
            state == WiFiState::WIFI_CONNECTING ? lv_color_hex(0xFFFF00) : // Yellow
            lv_color_hex(0xFF0000), 0); // Red
    }

    // Update loading screen if active
    if (wifi_loading_screen != nullptr && lv_obj_is_valid(wifi_loading_screen) &&
        lv_scr_act() == wifi_loading_screen) {
        if (state == WiFiState::WIFI_CONNECTED) {
            updateWiFiLoadingScreen(true, "WiFi Connected!");
        } else if (state == WiFiState::WIFI_DISCONNECTED &&
                   (message.indexOf("failed") >= 0 || message.indexOf("Failed") >= 0)) {
            updateWiFiLoadingScreen(false, "Connection Failed!");
        }
    }
}

// Implementation from .ino lines 2946-3046
void onWiFiScanComplete(const std::vector<NetworkInfo>& results) {
    // Attempt to take the semaphore
    if (xSemaphoreTake(xGuiSemaphore, (TickType_t)100 / portTICK_PERIOD_MS) == pdTRUE) {
        DEBUG_PRINTF("onWiFiScanComplete: Got semaphore. Processing %d results.\n", results.size());

        if (wifi_screen && lv_obj_is_valid(wifi_screen) && wifi_list && lv_obj_is_valid(wifi_list)) {
            // Remove spinner
            if (g_spinner != nullptr && lv_obj_is_valid(g_spinner)) {
                 lv_obj_del(g_spinner);
                 g_spinner = nullptr;
            } else if (g_spinner != nullptr) { g_spinner = nullptr; } // Clear pointer if object was already deleted

            DEBUG_PRINT("Cleaning wifi_list");
            lv_obj_clean(wifi_list); // Clear previous list items

            if (results.empty()) {
                if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text(wifi_status_label, "No networks found");
                }
                DEBUG_PRINT("No networks found.");
            } else {
                 if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text(wifi_status_label, "Populating list..."); // Indicate start
                 }
                 DEBUG_PRINTF("Populating list with %d networks...\n", results.size());

                 int count = 0;
                 const int BATCH_SIZE = 5; // Process N items before yielding slightly

                 for (const auto& net : results) {
                    if (net.ssid.isEmpty()) continue;

                    String displayText = net.ssid;
                    if (net.encryptionType != WIFI_AUTH_OPEN) {
                        displayText += " *"; // Indicate secured network
                    }
                    // DEBUG_PRINTF("  Adding: %s\n", displayText.c_str());

                    lv_obj_t* btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, displayText.c_str());

                    if (btn) {
                        // lv_obj_add_style(btn, &style_btn, 0); // Use default list button style or customize

                        // IMPORTANT: Need to allocate memory for SSID to store in user data
                        // Use strdup and free it in the callback
                        char* ssid_copy = strdup(net.ssid.c_str());
                        if (ssid_copy) {
                            lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                                char* stored_ssid = (char*)lv_event_get_user_data(e);
                                if (stored_ssid) {
                                    strncpy(selected_ssid, stored_ssid, sizeof(selected_ssid) - 1);
                                    selected_ssid[sizeof(selected_ssid) - 1] = '\0'; // Ensure null termination
                                    DEBUG_PRINTF("Selected network: %s\n", selected_ssid);

                                    // Need to handle screen transitions carefully
                                    lv_obj_t* current_screen = lv_obj_get_screen((lv_obj_t*)lv_event_get_target(e));
                                    showWiFiKeyboard(); // Show password entry screen

                                    // Clean up the stored SSID copy
                                    free(stored_ssid);
                                    lv_obj_set_user_data((lv_obj_t*)lv_event_get_target(e), NULL); // Clear user data pointer

                                    // Delete the scan list screen after transitioning
                                    if (current_screen && lv_obj_is_valid(current_screen)) {
                                        lv_obj_del_async(current_screen);
                                        // wifi_screen = nullptr; // Cannot assign to global in captureless lambda, pointer managed elsewhere
                                    }
                                }
                            }, LV_EVENT_CLICKED, (void*)ssid_copy); // Pass the copy
                        } else {
                             DEBUG_PRINT("Failed to allocate memory for SSID copy");
                             lv_obj_del(btn); // Clean up the button if allocation failed
                             continue;
                        }
                        count++;

                        // --- ADDED: Incremental Processing ---
                        if (count % BATCH_SIZE == 0) {
                            DEBUG_PRINTF("Processed batch of %d items, calling lv_task_handler...\n", BATCH_SIZE);
                            lv_task_handler(); // Allow LVGL to process UI updates for this batch
                            // Optional small delay if still having issues, but task_handler might be enough
                            // vTaskDelay(pdMS_TO_TICKS(5));
                        }
                        // --- END ADDED ---

                    } else {
                         DEBUG_PRINTF("Failed to create list button for: %s\n", displayText.c_str());
                    }
                 } // End of for loop

                 DEBUG_PRINTF("Finished adding %d networks to the list widget.\n", count);

                 // Call handler one last time after the loop finishes for any remaining items
                 DEBUG_PRINT("Calling final lv_task_handler() after loop.");
                 lv_task_handler();
                 DEBUG_PRINT("Finished final lv_task_handler() call.");

                 // Update status label after populating and processing
                 if (wifi_status_label && lv_obj_is_valid(wifi_status_label)) {
                    lv_label_set_text_fmt(wifi_status_label, "Scan complete (%d found)", results.size());
                 }
            }
        } else {
             DEBUG_PRINT("onWiFiScanComplete: wifi_screen or wifi_list no longer valid after taking semaphore.");
        }

        xSemaphoreGive(xGuiSemaphore);
        DEBUG_PRINT("onWiFiScanComplete: Released semaphore.");

    } else {
        DEBUG_PRINT("onWiFiScanComplete: Failed to get GUI semaphore. Skipping UI update.");
    }
}

// --- Webhook ---

// Implementation from .ino lines 3345-3428
void sendWebhook(const String& entry) {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINT("WiFi not connected, skipping webhook");
        return;
    }

    HTTPClient http;
    // TODO: Make webhook URL configurable (e.g., via Preferences or a settings screen)
    const char* zapierUrl = "https://hooks.zapier.com/hooks/catch/21957602/2qk3799/"; // Replace with your Zapier URL
    Serial.println("Starting HTTP client...");
    DEBUG_PRINTF("Webhook URL: %s\n", zapierUrl);
    http.setReuse(false); // Consider setting to true if sending frequently to the same host
    // Add timeout
    http.setTimeout(10000); // 10 seconds timeout

    // Use WiFiClientSecure for HTTPS
    WiFiClientSecure client;
    // Allow insecure connections - Zapier hook might not need strict cert validation,
    // but for production, proper certificate handling is recommended.
    client.setInsecure(); // Use this carefully!

    if (!http.begin(client, zapierUrl)) { // Use secure client
        DEBUG_PRINT("Failed to begin HTTPS client");
        return;
    }
    http.addHeader("Content-Type", "application/json");

    // Structure payload as key-value pairs for Zapier
    String timestamp = getTimestamp(); // Needs getTimestamp() from time_utils
    String jsonPayload = "{";
    jsonPayload += "\"timestamp\":\"" + timestamp + "\",";

    // Parse entry string: Gender,Apparel-ShirtColor,Pants-PantsColor,Shoe-ShoeColor,Item
    int firstComma = entry.indexOf(',');
    int secondComma = (firstComma != -1) ? entry.indexOf(',', firstComma + 1) : -1;
    int thirdComma = (secondComma != -1) ? entry.indexOf(',', secondComma + 1) : -1;
    int fourthComma = (thirdComma != -1) ? entry.indexOf(',', thirdComma + 1) : -1;

    String gender = (firstComma != -1) ? entry.substring(0, firstComma) : "N/A";
    String apparelShirt = (firstComma != -1 && secondComma != -1) ? entry.substring(firstComma + 1, secondComma) : "N/A";
    String pants = (secondComma != -1 && thirdComma != -1) ? entry.substring(secondComma + 1, thirdComma) : "N/A";
    String shoes = (thirdComma != -1 && fourthComma != -1) ? entry.substring(thirdComma + 1, fourthComma) : "N/A";
    String item = (fourthComma != -1) ? entry.substring(fourthComma + 1) : "N/A"; // Item is the last part

    // Split apparelShirt into type and color
    String apparelType = "N/A";
    String shirtColor = "N/A";
    int hyphenPosApparel = apparelShirt.indexOf('-');
    if (hyphenPosApparel != -1) {
        apparelType = apparelShirt.substring(0, hyphenPosApparel);
        shirtColor = apparelShirt.substring(hyphenPosApparel + 1);
    } else if (apparelShirt != "N/A") {
        shirtColor = apparelShirt; // Assume it's just color if no hyphen
    }

    // Split pants into type and color
    String pantsType = "N/A";
    String pantsColor = "N/A";
    int hyphenPosPants = pants.indexOf('-');
    if (hyphenPosPants != -1) {
        pantsType = pants.substring(0, hyphenPosPants);
        pantsColor = pants.substring(hyphenPosPants + 1);
    } else if (pants != "N/A") {
        pantsColor = pants; // Assume it's just color if no hyphen
    }

     // Split shoes into style and color
    String shoeStyle = "N/A";
    String shoeColor = "N/A";
    int hyphenPosShoes = shoes.indexOf('-');
    if (hyphenPosShoes != -1) {
        shoeStyle = shoes.substring(0, hyphenPosShoes);
        shoeColor = shoes.substring(hyphenPosShoes + 1);
    } else if (shoes != "N/A") {
        shoeColor = shoes; // Assume it's just color if no hyphen
    }


    jsonPayload += "\"gender\":\"" + gender + "\",";
    jsonPayload += "\"apparelType\":\"" + apparelType + "\",";
    jsonPayload += "\"shirtColor\":\"" + shirtColor + "\","; // Key consistency
    jsonPayload += "\"pantsType\":\"" + pantsType + "\",";
    jsonPayload += "\"pantsColor\":\"" + pantsColor + "\",";
    jsonPayload += "\"shoeStyle\":\"" + shoeStyle + "\","; // Key consistency
    jsonPayload += "\"shoeColor\":\"" + shoeColor + "\","; // Key consistency
    jsonPayload += "\"item\":\"" + item + "\"";

    jsonPayload += "}";

    DEBUG_PRINTF("Sending webhook payload: %s\n", jsonPayload.c_str());
    int httpCode = http.POST(jsonPayload);
    DEBUG_PRINTF("HTTP response code: %d\n", httpCode);

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_ACCEPTED) { // Check for success codes
            DEBUG_PRINT("Webhook sent successfully");
            // updateStatus("Webhook Sent", 0x00FF00); // Status updated in saveEntry
        } else {
            String response = http.getString();
            DEBUG_PRINTF("Webhook failed, response: %s\n", response.c_str());
            updateStatus("Webhook Failed", 0xFF0000); // Update status on failure
        }
    } else {
        DEBUG_PRINTF("HTTP POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        updateStatus("Webhook Failed", 0xFF0000); // Update status on failure
    }
    http.end();
}

// --- WiFi Connection ---

// Implementation based on original .ino lines 1100-1107, adapted for arguments
void connectToWiFi(const char* ssid, const char* password) {
    if (!ssid || !password || strlen(ssid) == 0) {
        DEBUG_PRINT("connectToWiFi: Invalid SSID or password provided.");
        // Optionally update UI status here
        return;
    }
    DEBUG_PRINTF("Attempting connection to SSID: %s\n", ssid);

    // Use the WiFiManager instance to handle the connection attempt
    // WiFiManager should ideally handle status updates via its callback (onWiFiStatus)
    wifiManager.connect(ssid, password);

    // Note: The original code deleted the wifi_keyboard here.
    // This might be better handled in the UI code (e.g., showWiFiKeyboard)
    // when the connection attempt starts or finishes, depending on the desired flow.
    // If connect() is blocking, deleting it here might be okay, but if it's async,
    // the keyboard might be needed until the connection result is known.
    // For now, we replicate the original logic's location.
    if (wifi_keyboard && lv_obj_is_valid(wifi_keyboard)) {
        lv_obj_del_async(wifi_keyboard); // Use async delete
        wifi_keyboard = nullptr;
    }
}
