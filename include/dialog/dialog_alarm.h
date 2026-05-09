/**
 * @file dialog_alarm.h
 * @brief Alarm configuration dialog
 */

#ifndef DIALOG_ALARM_H
#define DIALOG_ALARM_H

#include <windows.h>

/**
 * @brief Show alarm configuration dialog
 * @param hwndParent Parent window handle
 * @param editIndex Index of alarm to edit (-1 for new alarm)
 *
 * @details Modeless dialog for setting alarm time, message, and repeat options.
 */
void ShowAlarmDialog(HWND hwndParent, int editIndex);

/**
 * @brief Dialog result notification message
 */
#define WM_DIALOG_ALARM (WM_USER + 19)

#endif /* DIALOG_ALARM_H */