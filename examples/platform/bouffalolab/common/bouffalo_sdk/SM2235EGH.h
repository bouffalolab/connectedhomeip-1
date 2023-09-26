#ifndef __SM2235EGH_H__
#define __SM2235EGH_H__

#include <stdint.h>

typedef struct
{
    uint8_t max_curr_rgb;  // Red/Green/Blue: range from 0 to 15, 0->4mA, 1->8mA ... 15->64mA
    uint8_t max_curr_wy;   // White/Yellow: range from 0 to 15, 0->5mA, 1->10mA ... 15->80mA
    uint16_t gray_level_r; // Red: range from 0 to 1023
    uint16_t gray_level_g; // Green: range from 0 to 1023
    uint16_t gray_level_b; // Blue: range from 0 to 1023
    uint16_t gray_level_w; // White: range from 0 to 1023
    uint16_t gray_level_y; // Yellow: range from 0 to 1023
} SM2235EGH_Config_t;

void SM2235EGH_Config_Init(void);
void SM2235EGH_Set_Color(uint32_t p_Rduty, uint32_t p_Gduty, uint32_t p_Bduty, uint32_t p_Cduty, uint32_t p_Wduty);
#endif
