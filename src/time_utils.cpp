#include "time_utils.h"
#include "globals.h"
#include <sys/time.h> // For settimeofday
#include <WiFi.h>     // For WiFi status check
#include <time.h>     // For time functions like configTime, getLocalTime

// --- NTP Configuration ---
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
// TODO: Make timezone configurable. Using UTC for now.
const long gmtOffset_sec = 0; // UTC offset in seconds
const int daylightOffset_sec = 0; // Daylight saving offset in seconds

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

// Implementation from .ino lines 3999-4031
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

    DEBUG_PRINT("Attempting NTP time synchronization...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    struct tm timeinfo;
    // Try to get time for up to 10 seconds
    if (!getLocalTime(&timeinfo, 10000)) {
        DEBUG_PRINT("NTP Sync failed: Could not obtain time from server.");
        lastSyncStatus = "Failed (Server Error)";
        return false;
    }

    DEBUG_PRINTF("NTP Sync successful: %s", asctime(&timeinfo));

    // Time obtained, now set the RTC
    m5::rtc_date_t date_to_set;
    date_to_set.year = timeinfo.tm_year + 1900;
    date_to_set.month = timeinfo.tm_mon + 1;
    date_to_set.date = timeinfo.tm_mday;
    date_to_set.weekDay = timeinfo.tm_wday; // tm_wday: 0=Sun, 6=Sat (matches M5 RTC)

    m5::rtc_time_t time_to_set;
    time_to_set.hours = timeinfo.tm_hour;
    time_to_set.minutes = timeinfo.tm_min;
    time_to_set.seconds = timeinfo.tm_sec;

    DEBUG_PRINT("Setting RTC from NTP time...");
    M5.Rtc.setDate(&date_to_set); // Returns void
    M5.Rtc.setTime(&time_to_set); // Returns void
    DEBUG_PRINT("RTC set calls completed (assuming success).");

    // Since we can't check return value, assume success if NTP fetch worked.
    // Format the successful sync time for the status
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    lastSyncStatus = "Success: " + String(buffer);

    // Ensure system time is also explicitly set (though getLocalTime might do it)
    time_t t = mktime(&timeinfo);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    DEBUG_PRINT("System time updated from NTP.");

    return true;
}

// --- Get Last Sync Status ---
String getLastSyncStatus() {
    return lastSyncStatus;
}
