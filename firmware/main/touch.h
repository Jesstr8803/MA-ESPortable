// touch.h — FT3168 capacitive touch (portrait-mapped).
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void touch_init(void);                       // call after I2C is up (display_init)
bool touch_read(uint16_t *x, uint16_t *y);   // portrait coords; true if pressed
#ifdef __cplusplus
}
#endif
