#pragma once
extern unsigned fake_wdt_resets;
inline int esp_task_wdt_init(int, bool) { return 0; }
inline int esp_task_wdt_add(void*) { return 0; }
inline int esp_task_wdt_reset() { ++fake_wdt_resets; return 0; }
