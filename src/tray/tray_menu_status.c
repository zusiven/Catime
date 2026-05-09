/**
 * @file tray_menu_status.c
 * @brief Status display menu implementation
 */

#include "tray/tray_menu_status.h"
#include "timer/timer.h"
#include "alarm/alarm.h"
#include "config.h"
#include "language.h"
#include "../resource/resource.h"
#include <stdio.h>
#include <time.h>

/* External timer state variables */
extern BOOL CLOCK_SHOW_CURRENT_TIME;
extern BOOL CLOCK_COUNT_UP;
extern BOOL CLOCK_IS_PAUSED;
extern int CLOCK_TOTAL_TIME;
extern int countdown_elapsed_time;
extern int countup_elapsed_time;
extern int64_t g_target_end_time;
extern int64_t g_start_time;

/* External function for absolute time */
extern int64_t GetAbsoluteTimeMs(void);

/* Format time helper */
static void FormatRemainingTime(int seconds, wchar_t* buffer, size_t size) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0) {
        _snwprintf_s(buffer, size, _TRUNCATE, L"%dh %dm %ds", hours, minutes, secs);
    } else if (minutes > 0) {
        _snwprintf_s(buffer, size, _TRUNCATE, L"%dm %ds", minutes, secs);
    } else {
        _snwprintf_s(buffer, size, _TRUNCATE, L"%ds", secs);
    }
}

/* Get next alarm info */
static void GetNextAlarmInfo(wchar_t* buffer, size_t size) {
    if (g_AppConfig.alarm.count == 0) {
        _snwprintf_s(buffer, size, _TRUNCATE,
                    GetLocalizedString(L"无闹钟", L"No alarms"));
        return;
    }

    /* Find next enabled alarm */
    SYSTEMTIME st;
    GetLocalTime(&st);
    int currentMinuteTotal = (int)st.wHour * 60 + (int)st.wMinute;

    int nextAlarmIdx = -1;
    int minDiff = 24 * 60;  /* Max 24 hours */

    for (int i = 0; i < g_AppConfig.alarm.count; i++) {
        AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];
        if (!alarm->enabled) continue;

        int alarmMinuteTotal = alarm->hour * 60 + alarm->minute;

        /* Calculate difference */
        int diff;
        if (alarmMinuteTotal > currentMinuteTotal) {
            diff = alarmMinuteTotal - currentMinuteTotal;
        } else {
            /* Tomorrow */
            diff = (24 * 60 - currentMinuteTotal) + alarmMinuteTotal;
        }

        if (diff < minDiff) {
            minDiff = diff;
            nextAlarmIdx = i;
        }
    }

    if (nextAlarmIdx < 0) {
        _snwprintf_s(buffer, size, _TRUNCATE,
                    GetLocalizedString(L"无启用闹钟", L"No enabled alarms"));
        return;
    }

    AlarmEntry* nextAlarm = &g_AppConfig.alarm.alarms[nextAlarmIdx];

    /* Format next alarm time */
    wchar_t statusText[128];
    _snwprintf_s(statusText, 128, _TRUNCATE, L"%02d:%02d",
                nextAlarm->hour, nextAlarm->minute);

    /* Format remaining time */
    wchar_t remainingText[32];
    if (minDiff < 60) {
        _snwprintf_s(remainingText, 32, _TRUNCATE,
                    GetLocalizedString(L"%d分钟后", L"In %d min"), minDiff);
    } else {
        int h = minDiff / 60;
        int m = minDiff % 60;
        if (m == 0) {
            _snwprintf_s(remainingText, 32, _TRUNCATE,
                        GetLocalizedString(L"%d小时后", L"In %d h"), h);
        } else {
            _snwprintf_s(remainingText, 32, _TRUNCATE,
                        GetLocalizedString(L"%d小时%d分钟后", L"In %dh %dm"), h, m);
        }
    }

    _snwprintf_s(buffer, size, _TRUNCATE, L"%s %s", statusText, remainingText);
}

void BuildStatusMenu(HMENU hMenu) {
    /* Build status string */
    wchar_t statusText[256];
    wchar_t timerStatus[128] = {0};
    wchar_t alarmStatus[128] = {0};

    /* Timer status */
    if (CLOCK_SHOW_CURRENT_TIME) {
        _snwprintf_s(timerStatus, 128, _TRUNCATE,
                    GetLocalizedString(L"时钟模式", L"Clock mode"));
    } else if (CLOCK_COUNT_UP) {
        if (CLOCK_IS_PAUSED) {
            wchar_t timeStr[32];
            FormatRemainingTime(countup_elapsed_time, timeStr, 32);
            _snwprintf_s(timerStatus, 128, _TRUNCATE,
                        GetLocalizedString(L"正计时 %s (暂停)", L"Count-up %s (paused)"), timeStr);
        } else {
            wchar_t timeStr[32];
            FormatRemainingTime(countup_elapsed_time, timeStr, 32);
            _snwprintf_s(timerStatus, 128, _TRUNCATE,
                        GetLocalizedString(L"正计时 %s", L"Count-up %s"), timeStr);
        }
    } else if (CLOCK_TOTAL_TIME > 0) {
        /* Calculate remaining time */
        int64_t current_time_ms = GetAbsoluteTimeMs();
        int64_t remaining_ms = g_target_end_time - current_time_ms;
        if (remaining_ms < 0) remaining_ms = 0;
        int remaining_sec = (int)((remaining_ms + 999) / 1000);

        if (remaining_sec <= 0) {
            _snwprintf_s(timerStatus, 128, _TRUNCATE,
                        GetLocalizedString(L"倒计时已结束", L"Countdown finished"));
        } else {
            wchar_t timeStr[32];
            FormatRemainingTime(remaining_sec, timeStr, 32);

            if (CLOCK_IS_PAUSED) {
                _snwprintf_s(timerStatus, 128, _TRUNCATE,
                            GetLocalizedString(L"倒计时 %s (暂停)", L"Countdown %s (paused)"), timeStr);
            } else {
                _snwprintf_s(timerStatus, 128, _TRUNCATE,
                            GetLocalizedString(L"倒计时 %s", L"Countdown %s"), timeStr);
            }
        }
    } else {
        _snwprintf_s(timerStatus, 128, _TRUNCATE,
                    GetLocalizedString(L"无运行定时器", L"No timer running"));
    }

    /* Alarm status */
    GetNextAlarmInfo(alarmStatus, 128);

    /* Combine status */
    _snwprintf_s(statusText, 256, _TRUNCATE, L"⏱ %s | 🔔 %s", timerStatus, alarmStatus);

    /* Add status menu item (grayed, info only) */
    AppendMenuW(hMenu, MF_STRING | MF_GRAYED, CLOCK_IDM_STATUS_DISPLAY, statusText);
}