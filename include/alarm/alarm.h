/**
 * @file alarm.h
 * @brief Alarm module - multiple alarms with absolute time triggers
 *
 * Supports up to MAX_ALARMS (10) alarms, each with:
 * - Absolute time (hour:minute)
 * - Custom message
 * - Repeat mode (one-time, daily, or specific weekdays)
 * - Enable/disable toggle
 */

#ifndef ALARM_H
#define ALARM_H

#include <windows.h>
#include "../config.h"

/**
 * @brief Check all alarms and trigger notifications
 * @param hwnd Window handle for notifications
 *
 * @details Called by the independent wall-clock alarm timer.
 *          Checks enabled alarms and catches up occurrences delayed by up to five minutes.
 */
void CheckAlarmTriggers(HWND hwnd);

/**
 * @brief Initialize the scheduler's last-check timestamp.
 */
void AlarmScheduler_Initialize(void);

/**
 * @brief Check for missed alarms after system resume.
 */
void CheckMissedAlarms(HWND hwnd);

/**
 * @brief Parse a comma-separated weekday list. An empty string means every day.
 */
BOOL ParseAlarmDays(const char* days, unsigned* dayMask);

/**
 * @brief Check whether an alarm is scheduled for today's local date.
 */
BOOL ShouldTriggerToday(const AlarmEntry* alarm);

/**
 * @brief Check if an alarm is scheduled on a supplied local calendar date.
 */
BOOL ShouldTriggerOnDate(const AlarmEntry* alarm, const SYSTEMTIME* date);

/**
 * @brief Find the next future occurrence of an alarm.
 * @param now Current local time
 * @param occurrence Receives the next local date and time
 * @param minutesUntil Receives the ceiling number of minutes remaining
 */
BOOL GetNextAlarmOccurrence(const AlarmEntry* alarm, const SYSTEMTIME* now,
                            SYSTEMTIME* occurrence, int* minutesUntil);

/** Return code from AddAlarm when persistence fails. */
#define ALARM_ADD_SAVE_FAILED (-2)

/**
 * @brief Add new alarm to configuration
 * @param hour Hour (0-23)
 * @param minute Minute (0-59)
 * @param message Alarm message (UTF-8)
 * @param recurring TRUE for repeat, FALSE for one-time
 * @param days Comma-separated days "0,1,2,3,4,5,6" or empty for daily
 * @return Index of new alarm, -1 for invalid/full input, or ALARM_ADD_SAVE_FAILED
 */
int AddAlarm(int hour, int minute, const char* message, BOOL recurring, const char* days);

/**
 * @brief Remove alarm from configuration
 * @param index Alarm index to remove
 * @return TRUE on success, FALSE on invalid index or persistence failure
 */
BOOL RemoveAlarm(int index);

/**
 * @brief Update existing alarm
 * @param index Alarm index
 * @param hour New hour
 * @param minute New minute
 * @param message New message
 * @param recurring New recurring flag
 * @param days New days string
 * @param enabled New enabled flag
 * @return TRUE on success, FALSE on invalid input or persistence failure
 */
BOOL UpdateAlarm(int index, int hour, int minute, const char* message, BOOL recurring, const char* days, BOOL enabled);

/**
 * @brief Toggle alarm enabled state
 * @param index Alarm index
 * @param enabledAfterToggle Optional output for the new enabled state
 * @return TRUE on success, FALSE on invalid index or persistence failure
 */
BOOL ToggleAlarm(int index, BOOL* enabledAfterToggle);

/**
 * @brief Clear all alarms
 * @return TRUE on success, FALSE on persistence failure
 */
BOOL ClearAllAlarms(void);

/**
 * @brief Save the complete alarm configuration as one atomic INI update
 * @return TRUE on success, FALSE on invalid state or persistence failure
 */
BOOL SaveAlarmConfig(void);

/**
 * @brief Load alarm configuration from INI
 *
 * @details Called during ReadConfig initialization and after external config changes.
 */
void LoadAlarmConfig(void);

/**
 * @brief Parse alarm time string
 * @param timeStr Time string "HH:MM" or "HH:MM:SS"
 * @param hour Output hour
 * @param minute Output minute
 * @return TRUE on success, FALSE on invalid format
 */
BOOL ParseAlarmTime(const char* timeStr, int* hour, int* minute);

/**
 * @brief Format alarm time to string
 * @param hour Hour
 * @param minute Minute
 * @param buffer Output buffer
 * @param bufferSize Buffer size
 */
void FormatAlarmTime(int hour, int minute, char* buffer, size_t bufferSize);

#endif /* ALARM_H */
