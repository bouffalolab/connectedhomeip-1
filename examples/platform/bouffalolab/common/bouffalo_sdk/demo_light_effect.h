#ifndef __DEMO_LIGHT_EFFECT__
#define __DEMO_LIGHT_EFFECT__
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define LAM_MAX_MIREDS_DEFAULT 370
#define LAM_MIN_MIREDS_DEFAULT 153
void demo_color_light_init(void);
void set_level(uint8_t currLevel);
void set_color_red(void);
void set_color_green(void);
void set_color_blue(void);
void set_warm_temperature(void);
void set_cold_temperature(void);
void hw_set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat);
void hw_set_temperature(uint8_t currLevel, uint16_t temperature);
void set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat);
void set_temperature(uint8_t currLevel, uint16_t temperature);
void bl61x_get_chip_temp(void);
#if 0
void bl61x_get_chip_temp_init(void);
#endif
#ifdef __cplusplus
}
#endif

#endif