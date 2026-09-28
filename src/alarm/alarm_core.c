/**
 * @file alarm_core.c
 * @brief Alarm core implementation
 */

#include "alarm/alarm.h"
#include "notification.h"
#include "audio_player.h"
#include "language.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strsafe.h>

#define ALARM_MISSED_WINDOW_MINUTES 5
#define FILETIME_TICKS_PER_MINUTE 600000000ULL
#define FILETIME_TICKS_PER_DAY 864000000000ULL
#define ALARM_NOTIFICATION_BUFFER 1024

typedef struct {
    ULONGLONG occurrenceTicks;
    BOOL valid;
} AlarmTriggerHistory;

static AlarmTriggerHistory s_triggerHistory[MAX_ALARMS];
static SYSTEMTIME s_lastCheckTime;
static BOOL s_schedulerInitialized = FALSE;

/* UTF-8 to wide char helper */
static BOOL Utf8ToWide(const char* utf8, wchar_t* wbuf, size_t wbufSize) {
    if (!wbuf || wbufSize == 0) return FALSE;
    wbuf[0] = L'\0';
    if (!utf8) return FALSE;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1,
                               wbuf, (int)wbufSize) > 0;
}

static BOOL SystemTimeToTicks(const SYSTEMTIME* time, ULONGLONG* ticks) {
    if (!time || !ticks) return FALSE;

    FILETIME fileTime;
    ULARGE_INTEGER value;
    if (!SystemTimeToFileTime(time, &fileTime)) return FALSE;
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    *ticks = value.QuadPart;
    return TRUE;
}

static BOOL GetCalendarDateOffset(const SYSTEMTIME* date, int days,
                                  SYSTEMTIME* result) {
    if (!date || !result) return FALSE;

    SYSTEMTIME midnight = *date;
    midnight.wHour = 0;
    midnight.wMinute = 0;
    midnight.wSecond = 0;
    midnight.wMilliseconds = 0;

    ULONGLONG ticks;
    if (!SystemTimeToTicks(&midnight, &ticks)) return FALSE;

    if (days < 0) {
        ULONGLONG delta = (ULONGLONG)(-days) * FILETIME_TICKS_PER_DAY;
        if (ticks < delta) return FALSE;
        ticks -= delta;
    } else {
        ticks += (ULONGLONG)days * FILETIME_TICKS_PER_DAY;
    }

    ULARGE_INTEGER value;
    FILETIME fileTime;
    value.QuadPart = ticks;
    fileTime.dwLowDateTime = value.LowPart;
    fileTime.dwHighDateTime = value.HighPart;
    return FileTimeToSystemTime(&fileTime, result);
}

BOOL ParseAlarmDays(const char* days, unsigned* dayMask) {
    if (!days || !dayMask) return FALSE;

    if (*days == '\0') {
        *dayMask = 0x7Fu;
        return TRUE;
    }

    unsigned mask = 0;
    const char* p = days;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p < '0' || *p > '9') return FALSE;

        unsigned day = 0;
        do {
            day = day * 10u + (unsigned)(*p - '0');
            if (day > 6u) return FALSE;
            p++;
        } while (*p >= '0' && *p <= '9');

        while (*p == ' ' || *p == '\t') p++;
        mask |= 1u << day;
        if (*p == '\0') break;
        if (*p != ',') return FALSE;
        p++;
        if (*p == '\0') return FALSE;
    }

    if (mask == 0) return FALSE;
    *dayMask = mask;
    return TRUE;
}

BOOL ShouldTriggerOnDate(const AlarmEntry* alarm, const SYSTEMTIME* date) {
    if (!alarm || !date) return FALSE;
    if (!alarm->recurring || alarm->days[0] == '\0') return TRUE;
    if (date->wDayOfWeek > 6) return FALSE;

    unsigned dayMask;
    if (!ParseAlarmDays(alarm->days, &dayMask)) return FALSE;
    return (dayMask & (1u << date->wDayOfWeek)) != 0;
}

BOOL ShouldTriggerToday(const AlarmEntry* alarm) {
    SYSTEMTIME now;
    GetLocalTime(&now);
    return ShouldTriggerOnDate(alarm, &now);
}

static BOOL GetMostRecentAlarmOccurrence(const AlarmEntry* alarm,
                                         const SYSTEMTIME* now,
                                         SYSTEMTIME* occurrence,
                                         ULONGLONG* occurrenceTicks) {
    if (!alarm || !now || !occurrence || !occurrenceTicks) return FALSE;

    SYSTEMTIME date = *now;
    if (now->wHour * 60 + now->wMinute < alarm->hour * 60 + alarm->minute) {
        if (!GetCalendarDateOffset(now, -1, &date)) return FALSE;
    }

    date.wHour = (WORD)alarm->hour;
    date.wMinute = (WORD)alarm->minute;
    date.wSecond = 0;
    date.wMilliseconds = 0;
    if (!SystemTimeToTicks(&date, occurrenceTicks)) return FALSE;
    *occurrence = date;
    return TRUE;
}

BOOL GetNextAlarmOccurrence(const AlarmEntry* alarm, const SYSTEMTIME* now,
                            SYSTEMTIME* occurrence, int* minutesUntil) {
    if (!alarm || !now || !occurrence || !minutesUntil) return FALSE;

    ULONGLONG nowTicks;
    if (!SystemTimeToTicks(now, &nowTicks)) return FALSE;

    for (int dayOffset = 0; dayOffset <= 7; dayOffset++) {
        SYSTEMTIME candidate;
        if (!GetCalendarDateOffset(now, dayOffset, &candidate)) return FALSE;
        candidate.wHour = (WORD)alarm->hour;
        candidate.wMinute = (WORD)alarm->minute;
        candidate.wSecond = 0;
        candidate.wMilliseconds = 0;

        if (!ShouldTriggerOnDate(alarm, &candidate)) continue;

        ULONGLONG candidateTicks;
        if (!SystemTimeToTicks(&candidate, &candidateTicks) || candidateTicks <= nowTicks) continue;

        ULONGLONG delta = candidateTicks - nowTicks;
        *minutesUntil = (int)((delta + FILETIME_TICKS_PER_MINUTE - 1) /
                              FILETIME_TICKS_PER_MINUTE);
        *occurrence = candidate;
        return TRUE;
    }

    return FALSE;
}

void AlarmScheduler_Initialize(void) {
    memset(s_triggerHistory, 0, sizeof(s_triggerHistory));
    GetLocalTime(&s_lastCheckTime);
    s_schedulerInitialized = TRUE;
}

void CheckAlarmTriggers(HWND hwnd) {
    SYSTEMTIME now;
    GetLocalTime(&now);

    if (!s_schedulerInitialized) {
        s_lastCheckTime = now;
        s_schedulerInitialized = TRUE;
        return;
    }

    ULONGLONG nowTicks;
    ULONGLONG lastCheckTicks;
    if (!SystemTimeToTicks(&now, &nowTicks) ||
        !SystemTimeToTicks(&s_lastCheckTime, &lastCheckTicks)) {
        s_lastCheckTime = now;
        return;
    }

    /* A backward clock correction establishes a new baseline without replaying alarms. */
    if (nowTicks < lastCheckTicks) {
        s_lastCheckTime = now;
        return;
    }

    wchar_t combinedMessage[ALARM_NOTIFICATION_BUFFER] = {0};
    BOOL changed = FALSE;

    for (int i = 0; i < g_AppConfig.alarm.count; i++) {
        AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];
        if (!alarm->enabled) continue;

        SYSTEMTIME occurrence;
        ULONGLONG occurrenceTicks;
        if (!GetMostRecentAlarmOccurrence(alarm, &now, &occurrence,
                                          &occurrenceTicks)) continue;
        if (!ShouldTriggerOnDate(alarm, &occurrence)) continue;
        if (occurrenceTicks <= lastCheckTicks || occurrenceTicks > nowTicks) continue;
        if (nowTicks - occurrenceTicks >
            (ULONGLONG)ALARM_MISSED_WINDOW_MINUTES * FILETIME_TICKS_PER_MINUTE) continue;

        if (s_triggerHistory[i].valid &&
            s_triggerHistory[i].occurrenceTicks == occurrenceTicks) continue;
        s_triggerHistory[i].valid = TRUE;
        s_triggerHistory[i].occurrenceTicks = occurrenceTicks;

        wchar_t messageW[128];
        if (!Utf8ToWide(alarm->message, messageW, ARRAYSIZE(messageW))) {
            StringCchCopyW(messageW, ARRAYSIZE(messageW), L"Alarm");
        }

        if (combinedMessage[0] != L'\0') {
            StringCchCatW(combinedMessage, ARRAYSIZE(combinedMessage), L"\n");
        }
        StringCchCatW(combinedMessage, ARRAYSIZE(combinedMessage), messageW);

        if (!alarm->recurring) {
            alarm->enabled = FALSE;
            changed = TRUE;
        }
    }

    if (combinedMessage[0] != L'\0') {
        ShowNotification(hwnd, combinedMessage);
        PlayNotificationSound(hwnd);
        if (changed && !SaveAlarmConfig()) {
            LOG_ERROR("Failed to persist one-time alarms after triggering");
            MessageBoxW(hwnd,
                        GetLocalizedString(L"闹钟已在本次运行中关闭，但未能写入配置文件；重启后可能再次触发。", L"The one-time alarm was disabled for this session, but could not be saved. It may trigger again after restart."),
                        GetLocalizedString(L"闹钟保存失败", L"Alarm save failed"),
                        MB_OK | MB_ICONERROR);
        }
    }

    s_lastCheckTime = now;
}

void CheckMissedAlarms(HWND hwnd) {
    CheckAlarmTriggers(hwnd);
}

/* Add new alarm */
int AddAlarm(int hour, int minute, const char* message, BOOL recurring, const char* days) {
    if (g_AppConfig.alarm.count >= MAX_ALARMS) return -1;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
    if (recurring && days) {
        unsigned dayMask;
        if (strlen(days) >= sizeof(g_AppConfig.alarm.alarms[0].days) ||
            !ParseAlarmDays(days, &dayMask)) return -1;
    }
    if (message && strlen(message) >= sizeof(g_AppConfig.alarm.alarms[0].message)) return -1;

    wchar_t messageCheck[128];
    if (message && !Utf8ToWide(message, messageCheck, ARRAYSIZE(messageCheck))) return -1;

    int index = g_AppConfig.alarm.count;
    AlarmEntry* alarm = &g_AppConfig.alarm.alarms[index];
    AlarmEntry previous = *alarm;

    alarm->hour = hour;
    alarm->minute = minute;
    alarm->enabled = TRUE;
    alarm->recurring = recurring;

    if (message) {
        strncpy(alarm->message, message, sizeof(alarm->message) - 1);
        alarm->message[sizeof(alarm->message) - 1] = '\0';
    } else {
        strncpy(alarm->message, "Alarm!", sizeof(alarm->message) - 1);
        alarm->message[sizeof(alarm->message) - 1] = '\0';
    }

    if (recurring && days) {
        strncpy(alarm->days, days, sizeof(alarm->days) - 1);
        alarm->days[sizeof(alarm->days) - 1] = '\0';
    } else {
        alarm->days[0] = '\0';
    }

    g_AppConfig.alarm.count++;
    if (!SaveAlarmConfig()) {
        *alarm = previous;
        g_AppConfig.alarm.count--;
        return ALARM_ADD_SAVE_FAILED;
    }
    memset(&s_triggerHistory[index], 0, sizeof(s_triggerHistory[index]));

    return index;
}

/* Remove alarm */
BOOL RemoveAlarm(int index) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return FALSE;

    AlarmEntry previous[MAX_ALARMS];
    memcpy(previous, g_AppConfig.alarm.alarms, sizeof(previous));
    int previousCount = g_AppConfig.alarm.count;

    /* Shift remaining alarms */
    for (int i = index; i < g_AppConfig.alarm.count - 1; i++) {
        g_AppConfig.alarm.alarms[i] = g_AppConfig.alarm.alarms[i + 1];
    }

    g_AppConfig.alarm.count--;
    memset(&g_AppConfig.alarm.alarms[g_AppConfig.alarm.count], 0,
           sizeof(g_AppConfig.alarm.alarms[0]));
    if (!SaveAlarmConfig()) {
        memcpy(g_AppConfig.alarm.alarms, previous, sizeof(previous));
        g_AppConfig.alarm.count = previousCount;
        return FALSE;
    }
    if (index < g_AppConfig.alarm.count) {
        memmove(&s_triggerHistory[index], &s_triggerHistory[index + 1],
                (size_t)(g_AppConfig.alarm.count - index) * sizeof(s_triggerHistory[0]));
    }
    memset(&s_triggerHistory[g_AppConfig.alarm.count], 0,
           sizeof(s_triggerHistory[0]));

    return TRUE;
}

/* Update alarm */
BOOL UpdateAlarm(int index, int hour, int minute, const char* message, BOOL recurring, const char* days, BOOL enabled) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return FALSE;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return FALSE;
    if (recurring && days) {
        unsigned dayMask;
        if (strlen(days) >= sizeof(g_AppConfig.alarm.alarms[0].days) ||
            !ParseAlarmDays(days, &dayMask)) return FALSE;
    }
    if (message && strlen(message) >= sizeof(g_AppConfig.alarm.alarms[0].message)) return FALSE;

    wchar_t messageCheck[128];
    if (message && !Utf8ToWide(message, messageCheck, ARRAYSIZE(messageCheck))) return FALSE;

    AlarmEntry* alarm = &g_AppConfig.alarm.alarms[index];
    AlarmEntry previous = *alarm;

    alarm->hour = hour;
    alarm->minute = minute;
    alarm->enabled = enabled;
    alarm->recurring = recurring;

    if (message) {
        strncpy(alarm->message, message, sizeof(alarm->message) - 1);
        alarm->message[sizeof(alarm->message) - 1] = '\0';
    }

    if (recurring && days) {
        strncpy(alarm->days, days, sizeof(alarm->days) - 1);
        alarm->days[sizeof(alarm->days) - 1] = '\0';
    } else {
        alarm->days[0] = '\0';
    }

    if (!SaveAlarmConfig()) {
        *alarm = previous;
        return FALSE;
    }
    memset(&s_triggerHistory[index], 0, sizeof(s_triggerHistory[index]));

    return TRUE;
}

/* Toggle alarm enabled state */
BOOL ToggleAlarm(int index, BOOL* enabledAfterToggle) {
    if (index < 0 || index >= g_AppConfig.alarm.count) return FALSE;

    g_AppConfig.alarm.alarms[index].enabled ^= TRUE;
    if (!SaveAlarmConfig()) {
        g_AppConfig.alarm.alarms[index].enabled ^= TRUE;
        return FALSE;
    }
    if (enabledAfterToggle) {
        *enabledAfterToggle = g_AppConfig.alarm.alarms[index].enabled;
    }
    return TRUE;
}

/* Clear all alarms */
BOOL ClearAllAlarms(void) {
    AlarmEntry previous[MAX_ALARMS];
    memcpy(previous, g_AppConfig.alarm.alarms, sizeof(previous));
    int previousCount = g_AppConfig.alarm.count;

    g_AppConfig.alarm.count = 0;
    memset(g_AppConfig.alarm.alarms, 0, sizeof(g_AppConfig.alarm.alarms));
    if (!SaveAlarmConfig()) {
        memcpy(g_AppConfig.alarm.alarms, previous, sizeof(previous));
        g_AppConfig.alarm.count = previousCount;
        return FALSE;
    }
    memset(s_triggerHistory, 0, sizeof(s_triggerHistory));
    return TRUE;
}

/* Parse alarm time string */
BOOL ParseAlarmTime(const char* timeStr, int* hour, int* minute) {
    if (!timeStr || !hour || !minute) return FALSE;

    size_t length = strlen(timeStr);
    if (length != 5 && length != 8) return FALSE;
    if (timeStr[2] != ':' || (length == 8 && timeStr[5] != ':')) return FALSE;

    const int digitPositions[] = {0, 1, 3, 4, 6, 7};
    size_t digitCount = length == 8 ? ARRAYSIZE(digitPositions) : 4;
    for (size_t i = 0; i < digitCount; i++) {
        char c = timeStr[digitPositions[i]];
        if (c < '0' || c > '9') return FALSE;
    }

    int h = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    int m = (timeStr[3] - '0') * 10 + (timeStr[4] - '0');
    if (h > 23 || m > 59) return FALSE;

    if (length == 8) {
        int seconds = (timeStr[6] - '0') * 10 + (timeStr[7] - '0');
        if (seconds > 59) return FALSE;
    }

    *hour = h;
    *minute = m;
    return TRUE;
}

/* Format alarm time to string */
void FormatAlarmTime(int hour, int minute, char* buffer, size_t bufferSize) {
    if (!buffer) return;
    snprintf(buffer, bufferSize, "%02d:%02d", hour, minute);
}

/* Save the complete alarm list in one atomic INI update. */
BOOL SaveAlarmConfig(void) {
    char configPath[MAX_PATH];
    GetConfigPath(configPath, sizeof(configPath));
    if (g_AppConfig.alarm.count < 0 || g_AppConfig.alarm.count > MAX_ALARMS) {
        return FALSE;
    }

    enum { UPDATE_COUNT = 1 + MAX_ALARMS * 5 };
    IniKeyValue updates[UPDATE_COUNT];
    char keys[UPDATE_COUNT][32];
    char values[UPDATE_COUNT][sizeof(((AlarmEntry*)0)->message)];
    size_t updateIndex = 0;

    snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "ALARM_COUNT");
    snprintf(values[updateIndex], sizeof(values[updateIndex]), "%d",
             g_AppConfig.alarm.count);
    updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                         keys[updateIndex], values[updateIndex]};
    updateIndex++;

    for (int i = 0; i < MAX_ALARMS; i++) {
        char keyTime[32], keyEnabled[32], keyRecurring[32], keyDays[32], keyMessage[32];
        snprintf(keyTime, sizeof(keyTime), "ALARM_%d_TIME", i + 1);
        snprintf(keyEnabled, sizeof(keyEnabled), "ALARM_%d_ENABLED", i + 1);
        snprintf(keyRecurring, sizeof(keyRecurring), "ALARM_%d_RECURRING", i + 1);
        snprintf(keyDays, sizeof(keyDays), "ALARM_%d_DAYS", i + 1);
        snprintf(keyMessage, sizeof(keyMessage), "ALARM_%d_MESSAGE", i + 1);

        if (i < g_AppConfig.alarm.count) {
            AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];

            FormatAlarmTime(alarm->hour, alarm->minute, values[updateIndex],
                            sizeof(values[updateIndex]));
            snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "%s", keyTime);
            updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                                 keys[updateIndex], values[updateIndex]};
            updateIndex++;

            snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "%s", keyEnabled);
            snprintf(values[updateIndex], sizeof(values[updateIndex]), "%s",
                     alarm->enabled ? "TRUE" : "FALSE");
            updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                                 keys[updateIndex], values[updateIndex]};
            updateIndex++;

            snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "%s", keyRecurring);
            snprintf(values[updateIndex], sizeof(values[updateIndex]), "%s",
                     alarm->recurring ? "TRUE" : "FALSE");
            updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                                 keys[updateIndex], values[updateIndex]};
            updateIndex++;

            snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "%s", keyDays);
            snprintf(values[updateIndex], sizeof(values[updateIndex]), "%s", alarm->days);
            updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                                 keys[updateIndex], values[updateIndex]};
            updateIndex++;

            snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "%s", keyMessage);
            snprintf(values[updateIndex], sizeof(values[updateIndex]), "%s", alarm->message);
            updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                                 keys[updateIndex], values[updateIndex]};
            updateIndex++;
        } else {
            const char* emptyValues[] = {"", "FALSE", "FALSE", "", ""};
            const char* slotKeys[] = {keyTime, keyEnabled, keyRecurring, keyDays, keyMessage};
            for (size_t field = 0; field < ARRAYSIZE(slotKeys); field++) {
                snprintf(keys[updateIndex], sizeof(keys[updateIndex]), "%s", slotKeys[field]);
                snprintf(values[updateIndex], sizeof(values[updateIndex]), "%s", emptyValues[field]);
                updates[updateIndex] = (IniKeyValue){INI_SECTION_ALARM,
                                                     keys[updateIndex], values[updateIndex]};
                updateIndex++;
            }
        }
    }

    return WriteIniMultipleAtomic(configPath, updates, updateIndex);
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

        /* Read and validate time before allowing this entry to trigger. */
        char timeStr[32];
        ReadIniString(INI_SECTION_ALARM, keyTime, "", timeStr, sizeof(timeStr), configPath);
        BOOL validTime = ParseAlarmTime(timeStr, &alarm->hour, &alarm->minute);
        alarm->enabled = ReadIniBool(INI_SECTION_ALARM, keyEnabled, FALSE, configPath);
        alarm->recurring = ReadIniBool(INI_SECTION_ALARM, keyRecurring, FALSE, configPath);

        ReadIniString(INI_SECTION_ALARM, keyDays, "", alarm->days, sizeof(alarm->days), configPath);
        ReadIniString(INI_SECTION_ALARM, keyMessage, "Alarm!", alarm->message, sizeof(alarm->message), configPath);

        unsigned dayMask;
        if (!validTime || (alarm->recurring &&
                           !ParseAlarmDays(alarm->days, &dayMask))) {
            alarm->hour = 0;
            alarm->minute = 0;
            alarm->enabled = FALSE;
            LOG_WARNING("Disabled invalid alarm configuration at index %d", i);
        }

        wchar_t messageCheck[128];
        if (!Utf8ToWide(alarm->message, messageCheck, ARRAYSIZE(messageCheck))) {
            StringCchCopyA(alarm->message, sizeof(alarm->message), "Alarm!");
            LOG_WARNING("Replaced invalid UTF-8 alarm message at index %d", i);
        }
    }

    memset(s_triggerHistory, 0, sizeof(s_triggerHistory));
}
