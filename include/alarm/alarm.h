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
 * @details Called every second from HandleMainTimer.
 *          Checks current time against all enabled alarms.
 */
void CheckAlarmTriggers(HWND hwnd);

/**
 * @brief Handle single alarm trigger
 * @param hwnd Window handle
 * @param index Alarm index in g_AppConfig.alarm.alarms[]
 *
 * @details Shows notification, plays sound, disables one-time alarms.
 */
void HandleAlarmTrigger(HWND hwnd, int index);

/**
 * @brief Check if alarm should trigger today
 * @param alarm Pointer to AlarmEntry
 * @return TRUE if alarm should trigger, FALSE otherwise
 *
 * @details Checks recurring days against current day of week.
 */
BOOL ShouldTriggerToday(const AlarmEntry* alarm);

/**
 * @brief Check for missed alarms after system resume
 * @param hwnd Window handle
 *
 * @details Called from Timer_OnSystemResume.
 *          Shows notifications for alarms missed during sleep.
 */
void CheckMissedAlarms(HWND hwnd);

/**
 * @brief Add new alarm to configuration
 * @param hour Hour (0-23)
 * @param minute Minute (0-59)
 * @param message Alarm message (UTF-8)
 * @param recurring TRUE for repeat, FALSE for one-time
 * @param days Comma-separated days "0,1,2,3,4,5,6" or empty for daily
 * @return Index of new alarm, or -1 if full
 */
int AddAlarm(int hour, int minute, const char* message, BOOL recurring, const char* days);

/**
 * @brief Remove alarm from configuration
 * @param index Alarm index to remove
 * @return TRUE on success, FALSE on invalid index
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
 * @return TRUE on success, FALSE on invalid index
 */
BOOL UpdateAlarm(int index, int hour, int minute, const char* message, BOOL recurring, const char* days, BOOL enabled);

/**
 * @brief Toggle alarm enabled state
 * @param index Alarm index
 * @return New enabled state, or FALSE on invalid index
 */
BOOL ToggleAlarm(int index);

/**
 * @brief Clear all alarms
 */
void ClearAllAlarms(void);

/**
 * @brief Save alarm configuration to INI
 */
void SaveAlarmConfig(void);

/**
 * @brief Load alarm configuration from INI
 *
 * @details Called during ReadConfig initialization.
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