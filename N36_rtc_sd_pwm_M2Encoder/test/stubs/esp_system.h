#pragma once
extern int fake_reset_reason;
inline int esp_reset_reason() { return fake_reset_reason; }
