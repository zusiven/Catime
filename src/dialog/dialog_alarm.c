/**
 * @file dialog_alarm.c
 * @brief Alarm configuration dialog implementation
 */

#include "dialog/dialog_alarm.h"
#include "dialog/dialog_common.h"
#include "alarm/alarm.h"
#include "language.h"
#include "config.h"
#include "../resource/resource.h"
#include <stdio.h>
#include <string.h>
#include <strsafe.h>

static HWND g_hwndAlarmParent = NULL;
static int g_alarmEditIndex = -1;

static const int ALARM_DAY_CONTROL_IDS[7] = {
    IDC_ALARM_DAY_SUNDAY,
    IDC_ALARM_DAY_MONDAY,
    IDC_ALARM_DAY_TUESDAY,
    IDC_ALARM_DAY_WEDNESDAY,
    IDC_ALARM_DAY_THURSDAY,
    IDC_ALARM_DAY_FRIDAY,
    IDC_ALARM_DAY_SATURDAY
};

static void UpdateRepeatControls(HWND hDlg) {
    BOOL oneTime = IsDlgButtonChecked(hDlg, IDC_ALARM_RECURRING_CHECK) == BST_CHECKED;
    BOOL daily = IsDlgButtonChecked(hDlg, IDC_ALARM_DAILY_CHECK) == BST_CHECKED;
    for (size_t i = 0; i < ARRAYSIZE(ALARM_DAY_CONTROL_IDS); i++) {
        EnableWindow(GetDlgItem(hDlg, ALARM_DAY_CONTROL_IDS[i]), !oneTime && !daily);
    }
}

static void SetAlarmMessageText(HWND hDlg, const char* utf8) {
    wchar_t message[128] = {0};
    if (!utf8 || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1,
                                     message, ARRAYSIZE(message)) == 0) {
        StringCchCopyW(message, ARRAYSIZE(message), L"Alarm!");
    }
    SetDlgItemTextW(hDlg, IDC_ALARM_MESSAGE_EDIT, message);
}

static void ShowAlarmSaveError(HWND hDlg) {
    MessageBoxW(hDlg,
                GetLocalizedString(L"保存闹钟失败，请检查配置文件权限或磁盘空间。", L"Could not save alarm settings. Check file permissions and disk space."),
                GetLocalizedString(L"错误", L"Error"),
                MB_OK | MB_ICONERROR);
}

static INT_PTR CALLBACK AlarmDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            Dialog_RegisterInstance(DIALOG_INSTANCE_ALARM, hDlg);

            /* Center dialog on screen (not parent window) */
            RECT rcDlg;
            GetWindowRect(hDlg, &rcDlg);

            int dlgWidth = rcDlg.right - rcDlg.left;
            int dlgHeight = rcDlg.bottom - rcDlg.top;

            /* Get screen work area (excluding taskbar) */
            RECT rcWork;
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);

            int x = rcWork.left + (rcWork.right - rcWork.left - dlgWidth) / 2;
            int y = rcWork.top + (rcWork.bottom - rcWork.top - dlgHeight) / 2;

            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

            /* Set default values */
            if (g_alarmEditIndex >= 0 && g_alarmEditIndex < g_AppConfig.alarm.count) {
                AlarmEntry* alarm = &g_AppConfig.alarm.alarms[g_alarmEditIndex];

                /* Set time */
                char timeStr[16];
                FormatAlarmTime(alarm->hour, alarm->minute, timeStr, sizeof(timeStr));
                SetDlgItemTextA(hDlg, IDC_ALARM_TIME_EDIT, timeStr);

                /* Set message */
                SetAlarmMessageText(hDlg, alarm->message);

                /* Set repeat options */
                CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, !alarm->recurring ? BST_CHECKED : BST_UNCHECKED);
                BOOL isDaily = alarm->recurring && alarm->days[0] == '\0';
                CheckDlgButton(hDlg, IDC_ALARM_DAILY_CHECK, isDaily ? BST_CHECKED : BST_UNCHECKED);
                if (alarm->recurring && !isDaily) {
                    unsigned dayMask = 0;
                    if (ParseAlarmDays(alarm->days, &dayMask)) {
                        isDaily = dayMask == 0x7Fu;
                        if (isDaily) {
                            CheckDlgButton(hDlg, IDC_ALARM_DAILY_CHECK, BST_CHECKED);
                        } else {
                            for (size_t day = 0; day < ARRAYSIZE(ALARM_DAY_CONTROL_IDS); day++) {
                                CheckDlgButton(hDlg, ALARM_DAY_CONTROL_IDS[day],
                                               (dayMask & (1u << day)) ? BST_CHECKED : BST_UNCHECKED);
                            }
                        }
                    }
                }
            } else {
                /* Default: current time + 1 minute */
                SYSTEMTIME st;
                GetLocalTime(&st);
                st.wMinute++;
                if (st.wMinute >= 60) {
                    st.wMinute = 0;
                    st.wHour++;
                    if (st.wHour >= 24) st.wHour = 0;
                }

                char timeStr[16];
                snprintf(timeStr, sizeof(timeStr), "%02d:%02d", (int)st.wHour, (int)st.wMinute);
                SetDlgItemTextA(hDlg, IDC_ALARM_TIME_EDIT, timeStr);

                SetAlarmMessageText(hDlg, "Alarm!");

                CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, BST_CHECKED);
            }
            SendDlgItemMessageW(hDlg, IDC_ALARM_MESSAGE_EDIT, EM_SETLIMITTEXT, 99, 0);
            UpdateRepeatControls(hDlg);

            return TRUE;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == IDOK && wmEvent == BN_CLICKED) {
                /* Get time input */
                char timeStr[32];
                GetDlgItemTextA(hDlg, IDC_ALARM_TIME_EDIT, timeStr, sizeof(timeStr));

                int hour, minute;
                if (!ParseAlarmTime(timeStr, &hour, &minute)) {
                    MessageBoxW(hDlg,
                                GetLocalizedString(L"请输入有效的时间格式（如 14:30）", L"Please enter valid time (e.g., 14:30)"),
                                GetLocalizedString(L"错误", L"Error"),
                                MB_OK | MB_ICONERROR);
                    return TRUE;
                }

                /* Get UTF-16 input and encode it using the alarm API's UTF-8 contract. */
                wchar_t messageW[100] = {0};
                GetDlgItemTextW(hDlg, IDC_ALARM_MESSAGE_EDIT, messageW, ARRAYSIZE(messageW));
                if (messageW[0] == L'\0') {
                    StringCchCopyW(messageW, ARRAYSIZE(messageW), L"Alarm!");
                }

                char message[100];
                if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, messageW, -1,
                                        message, sizeof(message), NULL, NULL) == 0) {
                    MessageBoxW(hDlg,
                                GetLocalizedString(L"消息过长，最多可输入 99 个 UTF-8 字节。", L"Message is too long. The limit is 99 UTF-8 bytes."),
                                GetLocalizedString(L"错误", L"Error"),
                                MB_OK | MB_ICONERROR);
                    return TRUE;
                }

                BOOL oneTime = IsDlgButtonChecked(hDlg, IDC_ALARM_RECURRING_CHECK) == BST_CHECKED;
                BOOL daily = IsDlgButtonChecked(hDlg, IDC_ALARM_DAILY_CHECK) == BST_CHECKED;
                BOOL recurring = !oneTime;
                char days[16] = {0};

                if (recurring && !daily) {
                    unsigned dayMask = 0;
                    for (size_t day = 0; day < ARRAYSIZE(ALARM_DAY_CONTROL_IDS); day++) {
                        if (IsDlgButtonChecked(hDlg, ALARM_DAY_CONTROL_IDS[day]) == BST_CHECKED) {
                            dayMask |= 1u << day;
                        }
                    }
                    if (dayMask == 0) {
                        MessageBoxW(hDlg,
                                    GetLocalizedString(L"请选择至少一个重复日期。", L"Select at least one repeat day."),
                                    GetLocalizedString(L"错误", L"Error"),
                                    MB_OK | MB_ICONERROR);
                        return TRUE;
                    }
                    if (dayMask != 0x7Fu) {
                        size_t used = 0;
                        for (unsigned day = 0; day <= 6; day++) {
                            if ((dayMask & (1u << day)) == 0) continue;
                            int written = _snprintf_s(days + used, sizeof(days) - used,
                                                      _TRUNCATE, "%s%u",
                                                      used ? "," : "", day);
                            if (written < 0) break;
                            used += (size_t)written;
                        }
                    }
                }

                /* Add or update alarm */
                if (g_alarmEditIndex >= 0 && g_alarmEditIndex < g_AppConfig.alarm.count) {
                    if (!UpdateAlarm(g_alarmEditIndex, hour, minute, message, recurring, days, TRUE)) {
                        ShowAlarmSaveError(hDlg);
                        return TRUE;
                    }
                } else {
                    int result = AddAlarm(hour, minute, message, recurring, days);
                    if (result < 0) {
                        if (result == ALARM_ADD_SAVE_FAILED) {
                            ShowAlarmSaveError(hDlg);
                        } else {
                            MessageBoxW(hDlg,
                                        GetLocalizedString(L"闹钟数量已达上限（10个）", L"Maximum alarms reached (10)"),
                                        GetLocalizedString(L"错误", L"Error"),
                                        MB_OK | MB_ICONERROR);
                        }
                        return TRUE;
                    }
                }

                /* Post notification to parent */
                if (g_hwndAlarmParent) {
                    PostMessage(g_hwndAlarmParent, WM_DIALOG_ALARM, 0, 0);
                }

                EndDialog(hDlg, IDOK);
                Dialog_UnregisterInstance(DIALOG_INSTANCE_ALARM);
                return TRUE;
            }

            if (wmId == IDCANCEL && wmEvent == BN_CLICKED) {
                EndDialog(hDlg, IDCANCEL);
                Dialog_UnregisterInstance(DIALOG_INSTANCE_ALARM);
                return TRUE;
            }

            /* Keep one-time, daily, and selected weekdays mutually exclusive. */
            if (wmId == IDC_ALARM_RECURRING_CHECK) {
                if (IsDlgButtonChecked(hDlg, IDC_ALARM_RECURRING_CHECK) == BST_CHECKED) {
                    CheckDlgButton(hDlg, IDC_ALARM_DAILY_CHECK, BST_UNCHECKED);
                    for (size_t day = 0; day < ARRAYSIZE(ALARM_DAY_CONTROL_IDS); day++) {
                        CheckDlgButton(hDlg, ALARM_DAY_CONTROL_IDS[day], BST_UNCHECKED);
                    }
                }
                UpdateRepeatControls(hDlg);
            }

            if (wmId == IDC_ALARM_DAILY_CHECK) {
                if (IsDlgButtonChecked(hDlg, IDC_ALARM_DAILY_CHECK) == BST_CHECKED) {
                    CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, BST_UNCHECKED);
                    for (size_t day = 0; day < ARRAYSIZE(ALARM_DAY_CONTROL_IDS); day++) {
                        CheckDlgButton(hDlg, ALARM_DAY_CONTROL_IDS[day], BST_UNCHECKED);
                    }
                }
                UpdateRepeatControls(hDlg);
            }

            for (size_t day = 0; day < ARRAYSIZE(ALARM_DAY_CONTROL_IDS); day++) {
                if (wmId == ALARM_DAY_CONTROL_IDS[day]) {
                    if (IsDlgButtonChecked(hDlg, wmId) == BST_CHECKED) {
                        CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, BST_UNCHECKED);
                        CheckDlgButton(hDlg, IDC_ALARM_DAILY_CHECK, BST_UNCHECKED);
                    }
                    UpdateRepeatControls(hDlg);
                    break;
                }
            }

            break;
        }

        case WM_CLOSE: {
            EndDialog(hDlg, IDCANCEL);
            Dialog_UnregisterInstance(DIALOG_INSTANCE_ALARM);
            return TRUE;
        }
    }

    return FALSE;
}

void ShowAlarmDialog(HWND hwndParent, int editIndex) {
    if (Dialog_IsOpen(DIALOG_INSTANCE_ALARM)) {
        HWND existing = Dialog_GetInstance(DIALOG_INSTANCE_ALARM);
        SetForegroundWindow(existing);
        return;
    }

    g_hwndAlarmParent = hwndParent;
    g_alarmEditIndex = editIndex;

    DialogBoxParamW(
        GetModuleHandle(NULL),
        MAKEINTRESOURCEW(CLOCK_IDD_ALARM_DIALOG),
        hwndParent,
        AlarmDlgProc,
        0
    );
}
