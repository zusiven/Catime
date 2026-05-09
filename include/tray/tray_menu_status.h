/**
 * @file tray_menu_status.h
 * @brief Status display menu showing current timer and alarm states
 */

#ifndef TRAY_MENU_STATUS_H
#define TRAY_MENU_STATUS_H

#include <windows.h>

/**
 * @brief Build status display menu item and append to parent menu
 * @param hMenu Parent menu handle
 *
 * @details Shows current state:
 *          - Countdown: running/paused, remaining time
 *          - Alarm: next alarm time
 *          Grayed if no active timer/alarm.
 */
void BuildStatusMenu(HMENU hMenu);

#endif /* TRAY_MENU_STATUS_H */