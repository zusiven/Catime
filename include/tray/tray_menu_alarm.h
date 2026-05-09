/**
 * @file tray_menu_alarm.h
 * @brief Alarm menu construction for tray context menu
 */

#ifndef TRAY_MENU_ALARM_H
#define TRAY_MENU_ALARM_H

#include <windows.h>

/**
 * @brief Build alarm submenu and append to parent menu
 * @param hMenu Parent menu handle
 *
 * @details Creates submenu with:
 *          - "Add Alarm" item
 *          - List of existing alarms (toggle on click)
 *          - "Clear All" item (if alarms exist)
 */
void BuildAlarmMenu(HMENU hMenu);

#endif /* TRAY_MENU_ALARM_H */