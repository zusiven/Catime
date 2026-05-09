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
                SetDlgItemTextA(hDlg, IDC_ALARM_MESSAGE_EDIT, alarm->message);

                /* Set repeat options */
                CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, !alarm->recurring ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hDlg, IDC_ALARM_DAILY_CHECK, alarm->recurring && strlen(alarm->days) == 0 ? BST_CHECKED : BST_UNCHECKED);
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

                SetDlgItemTextA(hDlg, IDC_ALARM_MESSAGE_EDIT, "Alarm!");

                CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, BST_CHECKED);
            }

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

                /* Get message input */
                char message[100];
                GetDlgItemTextA(hDlg, IDC_ALARM_MESSAGE_EDIT, message, sizeof(message));
                if (strlen(message) == 0) {
                    strncpy(message, "Alarm!", sizeof(message) - 1);
                }

                /* Get repeat options */
                BOOL isOneTime = IsDlgButtonChecked(hDlg, IDC_ALARM_RECURRING_CHECK) == BST_CHECKED;
                BOOL isDaily = IsDlgButtonChecked(hDlg, IDC_ALARM_DAILY_CHECK) == BST_CHECKED;

                BOOL recurring = !isOneTime;
                const char* days = isDaily ? "" : "0,1,2,3,4,5,6";  /* Daily or all days */

                /* Add or update alarm */
                if (g_alarmEditIndex >= 0 && g_alarmEditIndex < g_AppConfig.alarm.count) {
                    UpdateAlarm(g_alarmEditIndex, hour, minute, message, recurring, days, TRUE);
                } else {
                    int result = AddAlarm(hour, minute, message, recurring, days);
                    if (result < 0) {
                        MessageBoxW(hDlg,
                                    GetLocalizedString(L"闹钟数量已达上限（10个）", L"Maximum alarms reached (10)"),
                                    GetLocalizedString(L"错误", L"Error"),
                                    MB_OK | MB_ICONERROR);
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

            /* Handle checkbox interactions - make them mutually exclusive */
            if (wmId == IDC_ALARM_RECURRING_CHECK) {
                if (IsDlgButtonChecked(hDlg, IDC_ALARM_RECURRING_CHECK) == BST_CHECKED) {
                    CheckDlgButton(hDlg, IDC_ALARM_DAILY_CHECK, BST_UNCHECKED);
                }
            }

            if (wmId == IDC_ALARM_DAILY_CHECK) {
                if (IsDlgButtonChecked(hDlg, IDC_ALARM_DAILY_CHECK) == BST_CHECKED) {
                    CheckDlgButton(hDlg, IDC_ALARM_RECURRING_CHECK, BST_UNCHECKED);
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