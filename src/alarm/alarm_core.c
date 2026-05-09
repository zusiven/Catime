/**
 * @file alarm_core.c
 * @brief Alarm core implementation
 */

#include "alarm/alarm.h"
#include "notification.h"
#include "audio_player.h"
#include "language.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Track last triggered minute to prevent duplicate triggers */
static int s_lastTriggeredMinute = -1;
static int s_lastTriggeredHour = -1;

/* UTF-8 to wide char helper */
static void Utf8ToWide(const char* utf8, wchar_t* wbuf, size_t wbufSize) {
    if (!utf8 || !wbuf) return;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, (int)wbufSize);
}

/* Check if alarm should trigger today */
BOOL ShouldTriggerToday(const AlarmEntry* alarm) {
    if (!alarm) return FALSE;

    /* One-time alarms always trigger */
    if (!alarm->recurring) return TRUE;

    /* Empty days string means daily */
    if (strlen(alarm->days) == 0) return TRUE;

    /* Check current day of week against days string */
    SYSTEMTIME st;
    GetLocalTime(&st);
    int dayOfWeek = (int)st.wDayOfWeek;  /* 0=Sunday */

    /* Parse days string "0,1,2,3,4,5,6" */
    const char* p = alarm->days;
    while (*p) {
        int d = atoi(p);
        if (d == dayOfWeek) return TRUE;
        p = strchr(p, ',');
        if (p) p++;
        else break;
    }

    return FALSE;
}

/* Check all alarms for triggers */
void CheckAlarmTriggers(HWND hwnd) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    /* Only check once per minute (at second 0) */
    if (st.wSecond != 0) return;

    /* Prevent duplicate triggers in same minute */
    if (st.wHour == s_lastTriggeredHour && st.wMinute == s_lastTriggeredMinute) return;
    s_lastTriggeredHour = (int)st.wHour;
    s_lastTriggeredMinute = (int)st.wMinute;

    /* Check each alarm */
    for (int i = 0; i < g_AppConfig.alarm.count; i++) {
        AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];

        /* Skip disabled alarms */
        if (!alarm->enabled) continue;

        /* Check if should trigger today */
        if (!ShouldTriggerToday(alarm)) continue;

        /* Check time match */
        if (alarm->hour == (int)st.wHour && alarm->minute == (int)st.wMinute) {
            HandleAlarmTrigger(hwnd, i);
        }
    }
}

/* Handle alarm trigger */
void HandleAlarmTrigger(HWND hwnd, int index) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return;

    AlarmEntry* alarm = &g_AppConfig.alarm.alarms[index];

    /* Show notification */
    wchar_t messageW[256];
    Utf8ToWide(alarm->message, messageW, 256);
    ShowNotification(hwnd, messageW);

    /* Play sound */
    PlayNotificationSound(hwnd);

    /* Disable one-time alarms after trigger */
    if (!alarm->recurring) {
        alarm->enabled = FALSE;
        SaveAlarmConfig();
    }
}

/* Check missed alarms after system resume */
void CheckMissedAlarms(HWND hwnd) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    /* Check alarms that might have been missed (within last 5 minutes) */
    int currentMinuteTotal = (int)st.wHour * 60 + (int)st.wMinute;

    for (int i = 0; i < g_AppConfig.alarm.count; i++) {
        AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];
        if (!alarm->enabled) continue;
        if (!ShouldTriggerToday(alarm)) continue;

        int alarmMinuteTotal = alarm->hour * 60 + alarm->minute;

        /* Check if alarm time was in the past 5 minutes */
        int diff = currentMinuteTotal - alarmMinuteTotal;
        if (diff > 0 && diff <= 5) {
            /* Show missed alarm notification */
            wchar_t messageW[256];
            Utf8ToWide(alarm->message, messageW, 256);
            ShowNotification(hwnd, messageW);
            PlayNotificationSound(hwnd);

            /* Disable one-time alarms */
            if (!alarm->recurring) {
                alarm->enabled = FALSE;
                SaveAlarmConfig();
            }
        }
    }
}

/* Add new alarm */
int AddAlarm(int hour, int minute, const char* message, BOOL recurring, const char* days) {
    if (g_AppConfig.alarm.count >= MAX_ALARMS) return -1;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;

    int index = g_AppConfig.alarm.count;
    AlarmEntry* alarm = &g_AppConfig.alarm.alarms[index];

    alarm->hour = hour;
    alarm->minute = minute;
    alarm->enabled = TRUE;
    alarm->recurring = recurring;

    if (message) {
        strncpy(alarm->message, message, sizeof(alarm->message) - 1);
        alarm->message[sizeof(alarm->message) - 1] = '\0';
    } else {
        strncpy(alarm->message, "Alarm!", sizeof(alarm->message) - 1);
    }

    if (days) {
        strncpy(alarm->days, days, sizeof(alarm->days) - 1);
        alarm->days[sizeof(alarm->days) - 1] = '\0';
    } else {
        alarm->days[0] = '\0';
    }

    g_AppConfig.alarm.count++;
    SaveAlarmConfig();

    return index;
}

/* Remove alarm */
BOOL RemoveAlarm(int index) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return FALSE;

    /* Shift remaining alarms */
    for (int i = index; i < g_AppConfig.alarm.count - 1; i++) {
        g_AppConfig.alarm.alarms[i] = g_AppConfig.alarm.alarms[i + 1];
    }

    g_AppConfig.alarm.count--;
    SaveAlarmConfig();

    return TRUE;
}

/* Update alarm */
BOOL UpdateAlarm(int index, int hour, int minute, const char* message, BOOL recurring, const char* days, BOOL enabled) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return FALSE;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return FALSE;

    AlarmEntry* alarm = &g_AppConfig.alarm.alarms[index];

    alarm->hour = hour;
    alarm->minute = minute;
    alarm->enabled = enabled;
    alarm->recurring = recurring;

    if (message) {
        strncpy(alarm->message, message, sizeof(alarm->message) - 1);
        alarm->message[sizeof(alarm->message) - 1] = '\0';
    }

    if (days) {
        strncpy(alarm->days, days, sizeof(alarm->days) - 1);
        alarm->days[sizeof(alarm->days) - 1] = '\0';
    }

    SaveAlarmConfig();

    return TRUE;
}

/* Toggle alarm enabled state */
BOOL ToggleAlarm(int index) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return FALSE;

    g_AppConfig.alarm.alarms[index].enabled ^= TRUE;
    SaveAlarmConfig();

    return g_AppConfig.alarm.alarms[index].enabled;
}

/* Clear all alarms */
void ClearAllAlarms(void) {
    g_AppConfig.alarm.count = 0;
    SaveAlarmConfig();
}

/* Parse alarm time string */
BOOL ParseAlarmTime(const char* timeStr, int* hour, int* minute) {
    if (!timeStr || !hour || !minute) return FALSE;

    /* Format: "HH:MM" or "HH:MM:SS" */
    int h, m, s;
    int parsed = sscanf(timeStr, "%d:%d:%d", &h, &m, &s);

    if (parsed >= 2) {
        if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
            *hour = h;
            *minute = m;
            return TRUE;
        }
    }

    return FALSE;
}

/* Format alarm time to string */
void FormatAlarmTime(int hour, int minute, char* buffer, size_t bufferSize) {
    if (!buffer) return;
    snprintf(buffer, bufferSize, "%02d:%02d", hour, minute);
}

/* Save alarm configuration to INI */
void SaveAlarmConfig(void) {
    char configPath[MAX_PATH];
    GetConfigPath(configPath, sizeof(configPath));

    /* Write alarm count */
    UpdateConfigIntAtomic(INI_SECTION_ALARM, "ALARM_COUNT", g_AppConfig.alarm.count);

    /* Write each alarm (up to MAX_ALARMS) */
    for (int i = 0; i < MAX_ALARMS; i++) {
        char keyTime[32], keyEnabled[32], keyRecurring[32], keyDays[32], keyMessage[32];
        snprintf(keyTime, sizeof(keyTime), "ALARM_%d_TIME", i + 1);
        snprintf(keyEnabled, sizeof(keyEnabled), "ALARM_%d_ENABLED", i + 1);
        snprintf(keyRecurring, sizeof(keyRecurring), "ALARM_%d_RECURRING", i + 1);
        snprintf(keyDays, sizeof(keyDays), "ALARM_%d_DAYS", i + 1);
        snprintf(keyMessage, sizeof(keyMessage), "ALARM_%d_MESSAGE", i + 1);

        if (i < g_AppConfig.alarm.count) {
            AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];

            /* Write time as "HH:MM" */
            char timeStr[16];
            FormatAlarmTime(alarm->hour, alarm->minute, timeStr, sizeof(timeStr));
            UpdateConfigKeyValueAtomic(INI_SECTION_ALARM, keyTime, timeStr);

            UpdateConfigBoolAtomic(INI_SECTION_ALARM, keyEnabled, alarm->enabled);
            UpdateConfigBoolAtomic(INI_SECTION_ALARM, keyRecurring, alarm->recurring);
            UpdateConfigKeyValueAtomic(INI_SECTION_ALARM, keyDays, alarm->days);
            UpdateConfigKeyValueAtomic(INI_SECTION_ALARM, keyMessage, alarm->message);
        } else {
            /* Clear unused slots */
            UpdateConfigKeyValueAtomic(INI_SECTION_ALARM, keyTime, "");
            UpdateConfigBoolAtomic(INI_SECTION_ALARM, keyEnabled, FALSE);
            UpdateConfigBoolAtomic(INI_SECTION_ALARM, keyRecurring, FALSE);
            UpdateConfigKeyValueAtomic(INI_SECTION_ALARM, keyDays, "");
            UpdateConfigKeyValueAtomic(INI_SECTION_ALARM, keyMessage, "");
        }
    }
}

/* Load alarm configuration from INI */
void LoadAlarmConfig(void) {
    char configPath[MAX_PATH];
    GetConfigPath(configPath, sizeof(configPath));

    /* Read alarm count */
    int count = ReadIniInt(INI_SECTION_ALARM, "ALARM_COUNT", 0, configPath);
    if (count < 0) count = 0;
    if (count > MAX_ALARMS) count = MAX_ALARMS;

    g_AppConfig.alarm.count = count;

    /* Read each alarm */
    for (int i = 0; i < count; i++) {
        AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];

        char keyTime[32], keyEnabled[32], keyRecurring[32], keyDays[32], keyMessage[32];
        snprintf(keyTime, sizeof(keyTime), "ALARM_%d_TIME", i + 1);
        snprintf(keyEnabled, sizeof(keyEnabled), "ALARM_%d_ENABLED", i + 1);
        snprintf(keyRecurring, sizeof(keyRecurring), "ALARM_%d_RECURRING", i + 1);
        snprintf(keyDays, sizeof(keyDays), "ALARM_%d_DAYS", i + 1);
        snprintf(keyMessage, sizeof(keyMessage), "ALARM_%d_MESSAGE", i + 1);

        /* Read time and parse */
        char timeStr[32];
        ReadIniString(INI_SECTION_ALARM, keyTime, "", timeStr, sizeof(timeStr), configPath);
        ParseAlarmTime(timeStr, &alarm->hour, &alarm->minute);

        alarm->enabled = ReadIniBool(INI_SECTION_ALARM, keyEnabled, FALSE, configPath);
        alarm->recurring = ReadIniBool(INI_SECTION_ALARM, keyRecurring, FALSE, configPath);

        ReadIniString(INI_SECTION_ALARM, keyDays, "", alarm->days, sizeof(alarm->days), configPath);
        ReadIniString(INI_SECTION_ALARM, keyMessage, "Alarm!", alarm->message, sizeof(alarm->message), configPath);
    }
}