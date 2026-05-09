/**
 * @file tray_menu_alarm.c
 * @brief Alarm menu implementation
 */

#include "tray/tray_menu_alarm.h"
#include "alarm/alarm.h"
#include "config.h"
#include "language.h"
#include "../resource/resource.h"
#include <stdio.h>

void BuildAlarmMenu(HMENU hMenu) {
    HMENU hAlarmMenu = CreatePopupMenu();

    /* Add Alarm item */
    AppendMenuW(hAlarmMenu, MF_STRING, CLOCK_IDM_ALARM_ADD,
                GetLocalizedString(L"添加闹钟", L"Add Alarm"));

    /* List existing alarms */
    if (g_AppConfig.alarm.count > 0) {
        AppendMenuW(hAlarmMenu, MF_SEPARATOR, 0, NULL);

        for (int i = 0; i < g_AppConfig.alarm.count; i++) {
            AlarmEntry* alarm = &g_AppConfig.alarm.alarms[i];

            /* Format: "HH:MM ✓ Message" or "HH:MM ○ Message" */
            wchar_t itemText[128];
            wchar_t statusMark[4] = {0};

            if (alarm->enabled) {
                statusMark[0] = L'✓';
            } else {
                statusMark[0] = L'○';
            }

            /* Convert message to wide char */
            wchar_t messageW[64];
            MultiByteToWideChar(CP_UTF8, 0, alarm->message, -1, messageW, 64);

            /* Format display text */
            _snwprintf_s(itemText, 128, _TRUNCATE, L"%02d:%02d %s %s",
                        alarm->hour, alarm->minute,
                        statusMark,
                        messageW[0] ? messageW : GetLocalizedString(L"闹钟", L"Alarm"));

            UINT flags = MF_STRING;
            if (alarm->enabled) flags |= MF_CHECKED;

            AppendMenuW(hAlarmMenu, flags, CLOCK_IDM_ALARM_BASE + i, itemText);
        }

        AppendMenuW(hAlarmMenu, MF_SEPARATOR, 0, NULL);

        /* Clear All item */
        AppendMenuW(hAlarmMenu, MF_STRING, CLOCK_IDM_ALARM_CLEAR_ALL,
                    GetLocalizedString(L"清除全部", L"Clear All"));
    }

    /* Append submenu to parent */
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hAlarmMenu,
                GetLocalizedString(L"闹钟", L"Alarms"));
}