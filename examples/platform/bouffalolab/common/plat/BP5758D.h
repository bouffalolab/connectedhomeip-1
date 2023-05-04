#ifndef __BP5758D_H__
#define __BP5758D_H__


#include <stdint.h>


#define BP5758D_CURR_RANGE_MIN     0   // mA
#define BP5758D_CURR_RANGE_MAX     90  // mA

#define BP5758D_GRAY_LEVEL_MIN     0
#define BP5758D_GRAY_LEVEL_MAX     1023


typedef struct {
    struct {
        int out_en;
        int curr_range;
        int gray_level;
    }out[5];
}BP5758D_Config_t;

#ifdef __cplusplus
extern "C" {
#endif

void BP5758D_Config_Init(void);
void BP5758D_Set_Color(uint8_t currLevel, uint8_t currHue, uint8_t currSat);
void BP5758D_Set_level(uint8_t currLevel);

#ifdef __cplusplus
}
#endif

#endif
