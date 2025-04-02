#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include "globals.h" // Include common definitions

// --- SD Card Initialization ---
void initFileSystem();

// --- Log Operations ---
bool appendToLog(const String& entry);
void saveEntry(const String& entry); // Coordinates logging and webhook
time_t parseTimestamp(const String& entry); // Parses timestamp from a log line
String getFormattedEntry(const LogEntry& entry); // Formats the LogEntry for display (Changed parameter type)

// Function to load entries within a specific date range (replaces listSavedEntries)
// void loadLogEntriesForDateRange(time_t start, time_t end); // Keep commented if loadAllLogEntries is used
void loadAllLogEntries(std::vector<LogEntry>& entries); // Add prototype for the function used in ui.cpp
bool resetLogFile(); // Resets (deletes) the log file

#endif // SD_LOGGER_H
