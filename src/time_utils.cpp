#include "time_utils.h"
#include "globals.h"
#include <sys/time.h> // For settimeofday
#include <WiFi.h>     // For WiFi status check
#include <time.h>     // For time functions like configTime, getLocalTime, time_t, mktime, localtime_r

// --- NTP Configuration ---
// const char* ntpServer1 = "pool.ntp.org";
// const char* ntpServer2 = "time.nist.gov";
const char* ntpServer1 = "0.pool.ntp.org"; // User requested servers
const char* ntpServer2 = "1.pool.ntp.org";
const char* ntpServer3 = "2.pool.ntp.org";
// Timezone handling will be done manually by fetching UTC and applying offset
const long utcOffsetSeconds = -4 * 3600; // EDT is UTC-4. Adjust if/when standard time starts. TODO: Make this dynamic or configurable

// --- Static variable for last sync status ---
static String lastSyncStatus = "Never";

// --- Time Functions ---

// Implementation from .ino lines 408-427
String getTimestamp() {
    m5::rtc_date_t DateStruct;
    m5::rtc_time_t TimeStruct;
    // Check if RTC read is successful
    if (!M5.Rtc.getDate(&DateStruct) || !M5.Rtc.getTime(&TimeStruct)) {
        DEBUG_PRINT("Failed to read RTC for timestamp");
        return "RTC Error";
    }

    struct tm timeinfo = {0};
    timeinfo.tm_year = DateStruct.year - 1900;
    timeinfo.tm_mon = DateStruct.month - 1;
    timeinfo.tm_mday = DateStruct.date;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    timeinfo.tm_isdst = -1; // Let mktime determine DST

    // Basic check for valid year before formatting
    if (timeinfo.tm_year > (2023 - 1900)) { // Check if year is reasonably recent
        char buffer[25]; // dd-Mon-YYYY HH:MM:SS AM/PM
        strftime(buffer, sizeof(buffer), "%d-%b-%Y %I:%M:%S %p", &timeinfo); // Use %I for 12-hour, %p for AM/PM
        return String(buffer);
    }
    DEBUG_PRINT("RTC time appears invalid, returning 'NoTime'");
    return "NoTime";
}

// Implementation from .ino lines 249-275
void setSystemTimeFromRTC() {
    m5::rtc_date_t DateStruct;
    m5::rtc_time_t TimeStruct;
    if (!M5.Rtc.getDate(&DateStruct) || !M5.Rtc.getTime(&TimeStruct)) {
         DEBUG_PRINT("Failed to read RTC for setting system time");
         return;
    }

    struct tm timeinfo = {0};
    timeinfo.tm_year = DateStruct.year - 1900;
    timeinfo.tm_mon = DateStruct.month - 1;
    timeinfo.tm_mday = DateStruct.date;
    timeinfo.tm_hour = TimeStruct.hours;
    timeinfo.tm_min = TimeStruct.minutes;
    timeinfo.tm_sec = TimeStruct.seconds;
    timeinfo.tm_isdst = -1; // Let mktime determine DST if possible

    time_t t = mktime(&timeinfo);
    if (t == -1) {
        DEBUG_PRINT("mktime failed to convert RTC time");
        return;
    }
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    if (settimeofday(&tv, NULL) != 0) {
         DEBUG_PRINT("settimeofday failed");
         return;
    }
    DEBUG_PRINT("System time set from RTC");

    // Optional: Verify the time set
    struct tm timeinfo_check;
    if (getLocalTime(&timeinfo_check)) {
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %I:%M:%S %p", &timeinfo_check);
        DEBUG_PRINTF("Current local time: %s", timeStr);
    } else {
        DEBUG_PRINT("Failed to get local time after setting");
    }
}

// Note: This function relies on global variables selected_date, selected_hour, etc.
// which are defined in ui.cpp (declared extern in globals.h)
void save_time_to_rtc() {
    DEBUG_PRINTF("Saving time: %04d-%02d-%02d %02d:%02d:00 %s\n",
                 selected_date.year, selected_date.month, selected_date.day,
                 selected_hour, selected_minute, selected_is_pm ? "PM" : "AM"); // <<< ADDED SEMICOLON HERE

    m5::rtc_date_t DateStruct;
    DateStruct.year = selected_date.year;
    DateStruct.month = selected_date.month;
    DateStruct.date = selected_date.day;

    // Calculate weekday using Zeller's Congruence (or similar method)
    // Zeller's: h = (q + floor(13*(m+1)/5) + K + floor(K/4) + floor(J/4) - 2*J) mod 7
    // where q=day, m=month (3=Mar,.. 14=Feb), K=year%100, J=floor(year/100)
    // Weekday: 0=Sat, 1=Sun, ... 6=Fri. M5 RTC uses 0=Sun, 1=Mon,... 6=Sat
    int q = selected_date.day;
    int m = selected_date.month;
    int y = selected_date.year;
    if (m < 3) { // Adjust month/year for Zeller's formula
        m += 12;
        y--;
    }
    int K = y % 100;
    int J = y / 100;
    int h = (q + (13 * (m + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7; // Adjusted Zeller for 0=Sat
    DateStruct.weekDay = (h + 1) % 7; // Convert Zeller's result (0=Sat) to M5's (0=Sun)

    DEBUG_PRINT("Attempting to set RTC Date...");
    M5.Rtc.setDate(&DateStruct);
    DEBUG_PRINT("RTC Date set call completed.");

    // Read back date immediately to verify
    m5::rtc_date_t read_date;
    if (M5.Rtc.getDate(&read_date)) {
        DEBUG_PRINTF("RTC Date read back: %04d-%02d-%02d (Weekday: %d)\n", read_date.year, read_date.month, read_date.date, read_date.weekDay);
    } else {
        DEBUG_PRINT("Failed to read back RTC Date after setting!");
    }


    m5::rtc_time_t TimeStruct;
    // Convert 12-hour format with AM/PM to 24-hour format for RTC
    int hour_24 = selected_hour;
    if (selected_is_pm && selected_hour != 12) hour_24 += 12; // 1 PM to 11 PM
    else if (!selected_is_pm && selected_hour == 12) hour_24 = 0; // 12 AM (Midnight)
    // 12 PM (Noon) is hour=12, selected_is_pm=1 -> hour_24 = 12 (correct)
    // 1 AM to 11 AM is hour=1..11, selected_is_pm=0 -> hour_24 = 1..11 (correct)

    TimeStruct.hours = hour_24;
    TimeStruct.minutes = selected_minute;
    TimeStruct.seconds = 0; // Set seconds to 0 when saving

    DEBUG_PRINT("Attempting to set RTC Time...");
    M5.Rtc.setTime(&TimeStruct);
    DEBUG_PRINT("RTC Time set call completed.");

    // Read back time immediately to verify
    m5::rtc_time_t read_time;
    if (M5.Rtc.getTime(&read_time)) {
        DEBUG_PRINTF("RTC Time read back: %02d:%02d:%02d\n", read_time.hours, read_time.minutes, read_time.seconds);
    } else {
        DEBUG_PRINT("Failed to read back RTC Time after setting!");
    }

    // After setting RTC, update the system time immediately
    // We proceed even if the read-back check shows issues, as the set might still have worked partially
    // or the read failed. The primary goal is to set the system time based on user input.
    setSystemTimeFromRTC();
    DEBUG_PRINT("System time update attempted after RTC set.");

}

// --- NTP Sync Function ---
bool syncTimeWithNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINT("NTP Sync failed: WiFi not connected.");
        lastSyncStatus = "Failed (No WiFi)";
         return false;
    }

    DEBUG_PRINT("Attempting NTP time synchronization (Fetching UTC)...");
    // configTzTime(timeZonePOSIX, ntpServer1, ntpServer2); // Use POSIX timezone string
    configTime(0, 0, ntpServer1, ntpServer2, ntpServer3); // Fetch UTC time, provide all 3 servers
    delay(500); // Add a small delay after config

    struct tm timeinfo_utc; // Store fetched UTC time components
    // Try to get time for up to 10 seconds
    if (!getLocalTime(&timeinfo_utc, 10000)) { // Get UTC time components
        DEBUG_PRINT("NTP Sync failed: Could not obtain UTC time from server.");
        lastSyncStatus = "Failed (Server Error)";
        return false;
    }

    DEBUG_PRINTF("NTP Sync successful (UTC): %s", asctime(&timeinfo_utc));

    // --- Manually calculate local time ---
    // Convert UTC struct tm to time_t (epoch seconds)
    // Note: mktime usually expects local time components. Using it on UTC components might be inaccurate
    // if the underlying system tries to apply timezone rules. A safer way is needed if available (like mkgmtime).
    // Assuming getLocalTime after configTime(0,0,...) correctly populates timeinfo_utc with UTC components
    // and mktime can convert it back to UTC epoch correctly in this context.
    time_t utc_epoch = mktime(&timeinfo_utc);
    if (utc_epoch == -1) {
        DEBUG_PRINT("Failed to convert UTC tm struct to time_t epoch.");
        lastSyncStatus = "Failed (Time Conversion)";
        return false;
    }
    DEBUG_PRINTF("UTC Epoch: %ld\n", utc_epoch);

    // Apply manual offset for local time (EDT = UTC-4)
    time_t local_epoch = utc_epoch + utcOffsetSeconds; // Add the negative offset
    DEBUG_PRINTF("Calculated Local Epoch: %ld (Offset: %ld)\n", local_epoch, utcOffsetSeconds);

    // Convert local epoch time back to struct tm for setting RTC
    struct tm timeinfo_local;
    localtime_r(&local_epoch, &timeinfo_local); // Convert epoch to local time components
    DEBUG_PRINTF("Calculated Local Time: %s", asctime(&timeinfo_local));

    // Time calculated, now set the RTC using local components
    m5::rtc_date_t date_to_set;
    date_to_set.year = timeinfo_local.tm_year + 1900;
    date_to_set.month = timeinfo_local.tm_mon + 1;
    date_to_set.date = timeinfo_local.tm_mday;
    date_to_set.weekDay = timeinfo_local.tm_wday; // tm_wday: 0=Sun, 6=Sat (matches M5 RTC)

    m5::rtc_time_t time_to_set;
    time_to_set.hours = timeinfo_local.tm_hour;
    time_to_set.minutes = timeinfo_local.tm_min;
    time_to_set.seconds = timeinfo_local.tm_sec;

    DEBUG_PRINTF("Attempting to set RTC Date (Local): %04d-%02d-%02d (Weekday: %d)\n", date_to_set.year, date_to_set.month, date_to_set.date, date_to_set.weekDay);
    M5.Rtc.setDate(&date_to_set); // Returns void
    DEBUG_PRINTF("Attempting to set RTC Time (Local): %02d:%02d:%02d\n", time_to_set.hours, time_to_set.minutes, time_to_set.seconds);
    M5.Rtc.setTime(&time_to_set); // Returns void
    DEBUG_PRINT("RTC set calls completed.");

    // --- ADDED: Verify RTC after setting ---
    m5::rtc_date_t read_date;
    m5::rtc_time_t read_time;
    bool date_ok = M5.Rtc.getDate(&read_date);
    bool time_ok = M5.Rtc.getTime(&read_time);
    if (date_ok && time_ok) {
        DEBUG_PRINTF("RTC Read Back after NTP set: %04d-%02d-%02d %02d:%02d:%02d\n",
                     read_date.year, read_date.month, read_date.date,
                     read_time.hours, read_time.minutes, read_time.seconds);
        // Optional: Compare read_date/read_time with date_to_set/time_to_set for stricter verification
    } else {
        DEBUG_PRINT("Failed to read back RTC time after NTP set!");
    }
    // --- END ADDED ---

    // Format the successful sync time for the status (using calculated local time)
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo_local);
    lastSyncStatus = "Success: " + String(buffer);

    // Ensure system time is also explicitly set using the calculated local epoch
    struct timeval tv = { .tv_sec = local_epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    DEBUG_PRINT("System time updated from calculated local time.");

    return true;
}

// --- Get Last Sync Status ---
String getLastSyncStatus() {
    return lastSyncStatus;
}
