#ifndef __DEMO_LIGHT_EFFECT__
#define __DEMO_LIGHT_EFFECT__

#ifdef __cplusplus
extern "C" {
#endif
#define LAM_MAX_MIREDS_DEFAULT 454
#define LAM_MIN_MIREDS_DEFAULT 154
void demo_color_light_init(void);
void set_level(uint8_t currLevel);
void set_color_red(uint8_t currLevel);
void set_color_green(uint8_t currLevel);
void set_color_yellow(uint8_t currLevel);
void set_color(uint8_t currLevel, uint8_t currHue, uint8_t currSat);
void set_temperature(uint8_t currLevel, uint16_t temperature);
void set_warm_temperature(void);
void set_cold_temperature(void);
#ifdef __cplusplus
}
#endif

#endif