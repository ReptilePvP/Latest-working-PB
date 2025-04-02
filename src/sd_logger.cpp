#include "sd_logger.h"
#include "globals.h"
#include "wifi_handler.h" // Needed for sendWebhook
#include "time_utils.h"   // Needed for getTimestamp
#include "ui.h"           // Needed for updateStatus

// --- SD Card Specific Globals ---
// Define the SPI instance for SD Card. Assuming HSPI is correct for CoreS3 SD slot.
// If using VSPI, change accordingly.
SPIClass SPI_SD(HSPI);
bool fileSystemInitialized = false;

// --- SD Card Initialization ---
// Implementation from .ino lines 882-903
void initFileSystem() {
    if (fileSystemInitialized) return;
    DEBUG_PRINT("Initializing SD card...");
    SPI.end(); delay(200); // Release default SPI bus
    // Initialize SD SPI pins
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1); // Use -1 for CS if handled manually
    pinMode(SD_SPI_CS_PIN, OUTPUT);
    digitalWrite(SD_SPI_CS_PIN, HIGH); // Ensure CS is high initially
    delay(200);

    bool sdInitialized = false;
    // Attempt SD initialization multiple times
    for (int i = 0; i < 3 && !sdInitialized; i++) {
        // Use the custom SPI instance and specify frequency
        sdInitialized = SD.begin(SD_SPI_CS_PIN, SPI_SD, 2000000); // 2 MHz frequency
        if (!sdInitialized) {
            DEBUG_PRINTF("SD Init attempt %d failed\n", i + 1);
            delay(100);
        }
    }

    if (!sdInitialized) {
        DEBUG_PRINT("All SD card initialization attempts failed");
        // Show error message box (requires LVGL to be initialized)
        if (lv_is_initialized()) { // Check if LVGL is ready
             lv_obj_t* msgbox = lv_msgbox_create(NULL);
             if (msgbox) { // Check if creation succeeded
                 lv_obj_set_size(msgbox, 280, 150);
                 lv_obj_center(msgbox);
                 lv_msgbox_add_title(msgbox, "Storage Error");
                 lv_msgbox_add_text(msgbox, "SD card initialization failed.");
                 lv_obj_t* close_btn = lv_msgbox_add_footer_button(msgbox, "OK");
                 if (close_btn) { // Check button creation
                     lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
                         lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
                         // Get the message box object by going up two parent levels (button -> footer -> msgbox)
                         lv_obj_t* mbox = lv_obj_get_parent(lv_obj_get_parent(btn));
                         if(mbox) lv_msgbox_close(mbox); // Use lv_msgbox_close for msgboxes
                     }, LV_EVENT_CLICKED, NULL);
                 }
             } else {
                 DEBUG_PRINT("Failed to create msgbox object!");
             }
        } // Closing brace for if (lv_is_initialized())
        // Restore default SPI bus for display
        SPI_SD.end();
        SPI.begin();
        pinMode(TFT_DC, OUTPUT);
        digitalWrite(TFT_DC, HIGH);
        return;
    }
    DEBUG_PRINT("SD card initialized successfully");
    fileSystemInitialized = true;
    // Restore default SPI bus for display
    SPI_SD.end();
    SPI.begin();
    pinMode(TFT_DC, OUTPUT);
    digitalWrite(TFT_DC, HIGH);
}

// --- Log Operations ---

// Implementation from .ino lines 430-459
bool appendToLog(const String& entry) {
    if (!fileSystemInitialized) {
        DEBUG_PRINT("File system not initialized, attempting to initialize...");
        initFileSystem(); // Try to initialize again
        if (!fileSystemInitialized) {
            DEBUG_PRINT("File system still not initialized, cannot save entry");
            updateStatus("SD Error: Cannot Save", 0xFF0000); // Update UI status
            return false;
        }
    }

    // Manage SPI bus access
    releaseSPIBus(); // Ensure default SPI is ended
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1);
    pinMode(SD_SPI_CS_PIN, OUTPUT); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(100); // Ensure CS is high, then low for operation

    File file = SD.open(LOG_FILENAME, FILE_APPEND);
    if (!file) {
        DEBUG_PRINT("Failed to open log file for writing");
        updateStatus("SD Error: Open Failed", 0xFF0000);
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); // Restore default SPI
        return false;
    }

    String timestamp = getTimestamp(); // Needs getTimestamp() from time_utils
    String formattedEntry = timestamp + ": " + entry;
    size_t bytesWritten = file.println(formattedEntry);
    file.close();

    // Restore default SPI bus
    SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);

    if (bytesWritten == 0) {
        DEBUG_PRINT("Failed to write to log file (0 bytes written)");
         updateStatus("SD Error: Write Failed", 0xFF0000);
        return false;
    }

    DEBUG_PRINTF("Entry saved to file (%d bytes): %s\n", bytesWritten, formattedEntry.c_str());
    return true;
}

// Implementation from .ino lines 462-471
void saveEntry(const String& entry) {
    Serial.println("New Entry:"); Serial.println(entry); // Keep basic Serial log
    bool saved = appendToLog(entry);

    // Update UI status and potentially send webhook
    if (wifiManager.isConnected()) { // Needs wifiManager from globals.h
        sendWebhook(entry); // Needs sendWebhook from wifi_handler
        // Status depends on both save and send success (sendWebhook should ideally return status)
        // For now, assume sendWebhook handles its own status update or logs errors
        updateStatus(saved ? "Entry Saved & Sent" : "Error Saving Entry", saved ? 0x00FF00 : 0xFF0000);
    } else {
        updateStatus(saved ? "Offline: Entry Saved" : "Error: Save Failed", saved ? 0xFFFF00 : 0xFF0000);
    }
}

// Implementation from .ino lines 373-405
time_t parseTimestamp(const String& entry) {
    struct tm timeinfo = {0};
    int day, year, hour, minute, second;
    char monthStr[4];
    char ampmStr[3] = {0}; // Buffer for AM/PM

    // Updated sscanf to parse 12-hour format (%I) and AM/PM (%2s)
    // Example format: "18-Mar-2025 02:30:00 PM: ..."
    if (sscanf(entry.c_str(), "%d-%3s-%d %d:%d:%d %2s", &day, monthStr, &year, &hour, &minute, &second, ampmStr) == 7) {
        timeinfo.tm_mday = day;
        timeinfo.tm_year = year - 1900; // tm_year is years since 1900
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = second;
        timeinfo.tm_isdst = -1; // Let mktime determine DST

        // Convert 12-hour format with AM/PM to 24-hour format
        if (strcmp(ampmStr, "PM") == 0 && hour != 12) {
            timeinfo.tm_hour = hour + 12;
        } else if (strcmp(ampmStr, "AM") == 0 && hour == 12) { // Handle midnight (12 AM)
            timeinfo.tm_hour = 0;
        } else {
            timeinfo.tm_hour = hour; // Handles 1 AM to 11 AM and 12 PM
        }

        // Convert month abbreviation to month number (0-11)
        static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        for (int i = 0; i < 12; i++) {
            if (strncmp(monthStr, months[i], 3) == 0) {
                timeinfo.tm_mon = i; // tm_mon is 0-11
                break;
            }
        }
        // Convert struct tm to time_t
        return mktime(&timeinfo);
    }
    DEBUG_PRINTF("Failed to parse timestamp from: %s\n", entry.c_str());
    return 0; // Return 0 (epoch start) if parsing fails
}

// Implementation from .ino lines 3141-3191, adapted for LogEntry input
String getFormattedEntry(const LogEntry& entry) {
    // entry.text contains the full log line, e.g., "18-Mar-2025 02:30:00 PM: Male,Hoodie-Red+Blue,..."
    // We need to extract the data part after the timestamp and ": "
    int dataStartPos = entry.text.indexOf(": ");
    String entryData = "";
    if (dataStartPos != -1) {
        entryData = entry.text.substring(dataStartPos + 2); // Get the part after ": "
    } else {
        DEBUG_PRINTF("Warning: Could not find ': ' in log entry text: %s\n", entry.text.c_str());
        entryData = entry.text; // Fallback to using the whole line if format is unexpected
    }

    String parts[5]; // Expecting 5 parts: Gender, Apparel, Pants, Shoes, Item
    int partCount = 0;
    int startIdx = 0;

    DEBUG_PRINTF("Parsing entryData for formatting: %s\n", entryData.c_str());

    // Parse the extracted data string
    for (int i = 0; i < entryData.length() && partCount < 5; i++) {
        if (entryData.charAt(i) == ',') {
            parts[partCount] = entryData.substring(startIdx, i);
            startIdx = i + 1;
            partCount++;
        }
    }
    // Get the last part (Item)
    if (partCount < 5 && startIdx < entryData.length()) { // Check startIdx to avoid empty substring
        parts[partCount] = entryData.substring(startIdx);
        partCount++;
    }

     // Format the timestamp from entry.timestamp
    char timeStr[20]; // Buffer for "YYYY-MM-DD HH:MM:SS"
    struct tm timeinfo;
    localtime_r(&entry.timestamp, &timeinfo); // Convert time_t to struct tm
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // Split Type-Color parts
    String apparelType = "N/A", shirtColor = "N/A";
    String pantsType = "N/A", pantsColor = "N/A";
    String shoeStyle = "N/A", shoeColor = "N/A";

    if (partCount > 1 && parts[1].length() > 0) { // Apparel-ShirtColor
        int hyphenPos = parts[1].indexOf('-');
        if (hyphenPos != -1) {
            apparelType = parts[1].substring(0, hyphenPos);
            shirtColor = parts[1].substring(hyphenPos + 1);
        } else { shirtColor = parts[1]; } // Fallback if no hyphen
    }
    if (partCount > 2 && parts[2].length() > 0) { // PantsType-PantsColor
        int hyphenPos = parts[2].indexOf('-');
        if (hyphenPos != -1) {
            pantsType = parts[2].substring(0, hyphenPos);
            pantsColor = parts[2].substring(hyphenPos + 1);
        } else { pantsColor = parts[2]; } // Fallback
    }
     if (partCount > 3 && parts[3].length() > 0) { // ShoeStyle-ShoeColor
        int hyphenPos = parts[3].indexOf('-');
        if (hyphenPos != -1) {
            shoeStyle = parts[3].substring(0, hyphenPos);
            shoeColor = parts[3].substring(hyphenPos + 1);
        } else { shoeColor = parts[3]; } // Fallback
    }

    // Format the output string
    String formatted = "Gender: " + (partCount > 0 ? parts[0] : "N/A") + "\n";
    formatted += "Apparel: " + apparelType + " (" + shirtColor + ")\n";
    formatted += "Pants: " + pantsType + " (" + pantsColor + ")\n";
    formatted += "Shoes: " + shoeStyle + " (" + shoeColor + ")\n";
    formatted += "Item: " + (partCount > 4 ? parts[4] : "N/A");

    // Prepend the formatted timestamp
    String finalFormatted = String(timeStr) + "\n" + formatted;

    DEBUG_PRINTF("Formatted Entry: %s\n", finalFormatted.c_str());
    return finalFormatted;
}


// Function to load entries within a specific date range
void loadLogEntriesForDateRange(time_t start, time_t end) {
    DEBUG_PRINTF("Loading log entries from %ld to %ld\n", start, end);
    parsedLogEntries.clear(); // Clear previous entries

    if (!fileSystemInitialized) {
        DEBUG_PRINT("File system not initialized - skipping loadLogEntriesForDateRange");
        // Optionally show an error message on the UI
        return;
    }

    // Manage SPI bus access carefully
    releaseSPIBus(); // Ensure default SPI is ended
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1); // Use -1 for CS
    pinMode(SD_SPI_CS_PIN, OUTPUT); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(50); // Ensure CS is high

    File log_file = SD.open(LOG_FILENAME, FILE_READ);
    if (!log_file) {
        DEBUG_PRINT("Failed to open log file for reading");
        updateStatus("SD Error: Read Fail", 0xFF0000);
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); // Restore default SPI
        return;
    }

    DEBUG_PRINT("Reading log file for date range...");
    while (log_file.available()) {
        String line = log_file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0 && !line.startsWith("#")) {
            time_t timestamp = parseTimestamp(line);
            // Filter by date range (inclusive)
            if (timestamp >= start && timestamp <= end) {
                parsedLogEntries.push_back({line, timestamp});
            }
        }
    }
    log_file.close();
    DEBUG_PRINTF("Finished reading log file. Found %d entries in range.\n", parsedLogEntries.size());

    SPI_SD.end(); // End SD SPI
    delay(50);
    SPI.begin(); // Reinitialize default SPI
    pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); // Re-init TFT DC pin

    // Sort entries after loading (ascending by time)
    std::sort(parsedLogEntries.begin(), parsedLogEntries.end(),
              [](const LogEntry& a, const LogEntry& b) {
                  return a.timestamp < b.timestamp;
              });
}

// New function to load all log entries
void loadAllLogEntries(std::vector<LogEntry>& entries) {
    DEBUG_PRINT("Loading all log entries...");
    entries.clear(); // Clear previous entries

    if (!fileSystemInitialized) {
        DEBUG_PRINT("File system not initialized - skipping loadAllLogEntries");
        return;
    }

    // Manage SPI bus access carefully
    releaseSPIBus(); // Ensure default SPI is ended
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1); // Use -1 for CS
    pinMode(SD_SPI_CS_PIN, OUTPUT); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(50); // Ensure CS is high

    File log_file = SD.open(LOG_FILENAME, FILE_READ);
    if (!log_file) {
        DEBUG_PRINT("Failed to open log file for reading");
        updateStatus("SD Error: Read Fail", 0xFF0000);
        SPI_SD.end(); SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); // Restore default SPI
        return;
    }

    DEBUG_PRINT("Reading entire log file...");
    while (log_file.available()) {
        String line = log_file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0 && !line.startsWith("#")) {
            time_t timestamp = parseTimestamp(line);
            // Store the full line as the text part of LogEntry
            entries.push_back({line, timestamp});
        }
    }
    log_file.close();
    DEBUG_PRINTF("Finished reading log file. Found %d total entries.\n", entries.size());

    SPI_SD.end(); // End SD SPI
    delay(50);
    SPI.begin(); // Reinitialize default SPI
    pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH); // Re-init TFT DC pin

    // Sorting is done in createViewLogsScreen after loading
}


// --- Reset Log File ---
// Implementation based on original .ino lines 813-825
bool resetLogFile() {
    DEBUG_PRINT("Resetting log file...");

    if (!fileSystemInitialized) {
        DEBUG_PRINT("File system not initialized, attempting to initialize...");
        initFileSystem(); // Try to initialize first
        if (!fileSystemInitialized) {
            DEBUG_PRINT("File system still not initialized, cannot reset log");
            updateStatus("SD Error: Reset Failed", 0xFF0000);
            return false;
        }
    }

    // Manage SPI bus access
    releaseSPIBus(); // Ensure default SPI is ended
    SPI_SD.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, -1);
    pinMode(SD_SPI_CS_PIN, OUTPUT); digitalWrite(SD_SPI_CS_PIN, HIGH); delay(50);

    bool reset_success = SD.remove(LOG_FILENAME); // Attempt to remove the file

    // Restore default SPI bus
    SPI_SD.end(); delay(50);
    SPI.begin(); pinMode(TFT_DC, OUTPUT); digitalWrite(TFT_DC, HIGH);

    if (reset_success) {
        DEBUG_PRINT("Log file reset successfully.");
        // Re-initialize to potentially recreate the file if needed by appendToLog
        // Note: initFileSystem() handles SPI switching internally now
        initFileSystem();
    } else {
        DEBUG_PRINT("Failed to reset log file.");
        updateStatus("SD Error: Reset Failed", 0xFF0000);
    }

    return reset_success;
}
