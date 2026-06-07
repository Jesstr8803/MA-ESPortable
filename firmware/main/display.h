// display.h — SH8601 AMOLED + LVGL bring-up.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Init QSPI panel + LVGL + the LVGL task, then build the UI (ui_init()).
void display_init(void);

// LVGL is not thread-safe: take/release this lock around any LVGL calls made
// from outside the LVGL task.
bool display_lock(int timeout_ms);   // -1 = wait forever
void display_unlock(void);

#ifdef __cplusplus
}
#endif
