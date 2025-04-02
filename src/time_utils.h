#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "globals.h" // Include common definitions

// --- Time Functions ---
String getTimestamp(); // Gets current RTC time as formatted string
void setSystemTimeFromRTC(); // Sets the ESP32 system time from the RTC
void save_time_to_rtc(); // Saves the selected date/time (from globals) to RTC

#endif // TIME_UTILS_H