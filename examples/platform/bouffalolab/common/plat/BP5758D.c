#include "BP5758D.h"
#include "bl702l_glb.h"
#include "bl702l_i2c.h"


#define I2C_SLAVE_ADDR_NORMAL      (0xB0 >> 1)
#define I2C_SLAVE_ADDR_SLEEP       (0x80 >> 1)

const uint16_t gamma_lut[] = {
     0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   2,   2,   2,   3,   3,   4,
     4,   5,   5,   6,   6,   7,   8,   8,   9,  10,  11,  12,  12,  13,  14,  15,
    16,  17,  18,  19,  21,  22,  23,  24,  25,  27,  28,  29,  31,  32,  34,  35,
    37,  38,  40,  41,  43,  45,  46,  48,  50,  52,  53,  55,  57,  59,  61,  63,
    65,  67,  69,  71,  73,  75,  78,  80,  82,  84,  87,  89,  92,  94,  96,  99,
   101, 104, 107, 109, 112, 115, 117, 120, 123, 126, 128, 131, 134, 137, 140, 143,
   146, 149, 152, 155, 159, 162, 165, 168, 172, 175, 178, 182, 185, 188, 192, 195,
   199, 202, 206, 210, 213, 217, 221, 225, 228, 232, 236, 240, 244, 248, 252, 256,
   260, 264, 268, 272, 276, 280, 285, 289, 293, 298, 302, 306, 311, 315, 320, 324,
   329, 333, 338, 343, 347, 352, 357, 362, 366, 371, 376, 381, 386, 391, 396, 401,
   406, 411, 416, 421, 426, 432, 437, 442, 448, 453, 458, 464, 469, 475, 480, 486,
   491, 497, 502, 508, 514, 519, 525, 531, 537, 543, 549, 554, 560, 566, 572, 578,
   585, 591, 597, 603, 609, 615, 622, 628, 634, 641, 647, 653, 660, 666, 673, 679,
   686, 693, 699, 706, 713, 719, 726, 733, 740, 747, 754, 760, 767, 774, 781, 789,
   796, 803, 810, 817, 824, 832, 839, 846, 853, 861, 868, 876, 883, 891, 898, 906,
   913, 921, 929, 936, 944, 952, 960, 967, 975, 983, 991, 999,1007,1015,1023,
};
static uint16_t current_r=0;
static uint16_t current_g=0;
static uint16_t current_b=0;
static BP5758D_Config_t led_cfg;

static int BP5758D_Init(int scl_pin, int sda_pin)
{
    GLB_GPIO_Type pinList[2];
    
    if(scl_pin < 0 || scl_pin > 31 || scl_pin % 2 != 0){
        return -1;
    }
    
    if(sda_pin < 0 || sda_pin > 31 || sda_pin % 2 != 1){
        return -2;
    }
    
    pinList[0] = scl_pin;
    pinList[1] = sda_pin;
    GLB_GPIO_Func_Init(GPIO_FUN_I2C, pinList, 2);
    
    return 0;
}

static int BP5758D_SendData(uint8_t addr, uint8_t *data, uint16_t len)
{
    I2C_Transfer_Cfg cfg = {
        .slaveAddr = addr,
        .slaveAddr10Bit = DISABLE,
        .stopEveryByte = DISABLE,
        .subAddrSize = 0,
        .subAddr = 0,
        .dataSize = len,
        .data = data,
        .clk = 100000,
    };
    
#if 0
    printf("Byte0: %02X\r\n", addr<<1);
    for(int i=0; i<len; i++){
        printf("Byte%d: %02X\r\n", i+1, data[i]);
    }
#endif
    
    if(I2C_MasterSendBlocking(I2C0_ID, &cfg) != SUCCESS){
        return -1;
    }
    
    return 0;
}
#if 0
static int BP5758D_Sleep(void)
{
    uint8_t data[1] = {0x00};
    
    if(BP5758D_SendData(I2C_SLAVE_ADDR_NORMAL, data, sizeof(data)) != 0){
        return -1;
    }
    
    if(BP5758D_SendData(I2C_SLAVE_ADDR_SLEEP, data, sizeof(data)) != 0){
        return -2;
    }
    
    return 0;
}
#endif
static int BP5758D_ApplyConfig(BP5758D_Config_t *cfg)
{
    uint8_t data[16];
    int i;
    
    if(cfg == NULL){
        return -1;
    }
    
    for(i=0; i<5; i++){
        if(cfg->out[i].curr_range < 0 || cfg->out[i].curr_range > 90){
            return -2;
        }
        
        if(cfg->out[i].gray_level < 0 || cfg->out[i].gray_level > 1023){
            return -3;
        }
    }
    
    data[0] = 0x00;
    
    for(i=0; i<5; i++){
        if(cfg->out[i].out_en){
            data[0] |= 1 << i;
        }
        
        if(cfg->out[i].curr_range < 64){
            data[i + 1] = cfg->out[i].curr_range;
        }else{
            data[i + 1] = 0x40 | (cfg->out[i].curr_range - 30);
        }
        
        data[i * 2 + 6] = (cfg->out[i].gray_level & 0x1F)|0xa0;
        data[i * 2 + 7] = (cfg->out[i].gray_level >> 5)|0xa0;
    }
    
    if(BP5758D_SendData(I2C_SLAVE_ADDR_NORMAL, data, sizeof(data)) != 0){
        return -4;
    }
    
    return 0;
}

void BP5758D_Config_Init(void)
{
    int status;
    
    for(int i=0; i<5; i++){
        led_cfg.out[i].out_en = 0;
        led_cfg.out[i].curr_range = 0;
        led_cfg.out[i].gray_level = 0;
    }
    
    status = BP5758D_Init(22, 29);
    printf("BP5758D_Init: %d\r\n", status);
    status = BP5758D_ApplyConfig(&led_cfg);
    printf("BP5758D_ApplyConfig: %d\r\n", status);
}

void BP5758D_Set_Color(uint8_t currLevel, uint8_t currHue, uint8_t currSat)
{
    int status;

    uint16_t hue = (uint16_t) currHue * 360 / 254;
    uint8_t sat  = (uint16_t) currSat * 100 / 254;

    if (currLevel <= 5 && currLevel >= 1)
    {
        currLevel = 5; // avoid demo off
    }

    if (sat > 100)
    {
        sat = 100;
    }

    uint16_t i       = hue / 60;
    uint16_t rgb_max = currLevel;
    uint16_t rgb_min = rgb_max * (100 - sat) / 100;
    uint16_t diff    = hue % 60;
    uint16_t rgb_adj = (rgb_max - rgb_min) * diff / 60;
    uint32_t red, green, blue;

    switch (i)
    {
    case 0:
        red   = rgb_max;
        green = rgb_min + rgb_adj;
        blue  = rgb_min;
        break;
    case 1:
        red   = rgb_max - rgb_adj;
        green = rgb_max;
        blue  = rgb_min;
        break;
    case 2:
        red   = rgb_min;
        green = rgb_max;
        blue  = rgb_min + rgb_adj;
        break;
    case 3:
        red   = rgb_min;
        green = rgb_max - rgb_adj;
        blue  = rgb_max;
        break;
    case 4:
        red   = rgb_min + rgb_adj;
        green = rgb_min;
        blue  = rgb_max;
        break;
    default:
        red   = rgb_max;
        green = rgb_min;
        blue  = rgb_max - rgb_adj;
        break;
    }
    printf("BP5758D_Set_Color: R %ld, G %ld, B %ld\r\n", red,green,blue);
    current_r=red;
    current_g=green;
    current_b=blue;
    // red
    led_cfg.out[0].out_en = 1;
    led_cfg.out[0].curr_range = 60;
    led_cfg.out[0].gray_level = gamma_lut[red];

    // blue
    led_cfg.out[1].out_en = 1;
    led_cfg.out[1].curr_range = 60;
    led_cfg.out[1].gray_level = gamma_lut[blue];

    // green
    led_cfg.out[2].out_en = 1;
    led_cfg.out[2].curr_range = 60;
    led_cfg.out[2].gray_level = gamma_lut[green];

    led_cfg.out[3].out_en = 0;
    led_cfg.out[3].curr_range = 0;
    led_cfg.out[3].gray_level = 0;

    led_cfg.out[4].out_en = 0;
    led_cfg.out[4].curr_range = 0;
    led_cfg.out[4].gray_level = 0;

    status = BP5758D_ApplyConfig(&led_cfg);
    printf("BP5758D_Set_Color: %d\r\n", status);
   
}


void BP5758D_Set_level(uint8_t currLevel)
{
    int status;

    if (currLevel <= 5 && currLevel >= 1)
    {
        currLevel = 5; // avoid demo off
    }

    // red
    led_cfg.out[0].out_en = 1;
    led_cfg.out[0].curr_range = 60;
    led_cfg.out[0].gray_level = gamma_lut[(current_r * currLevel / 254)];

    // blue
    led_cfg.out[1].out_en = 1;
    led_cfg.out[1].curr_range = 60;
    led_cfg.out[1].gray_level = gamma_lut[(current_b * currLevel / 254)];

    // green
    led_cfg.out[2].out_en = 1;
    led_cfg.out[2].curr_range = 60;
    led_cfg.out[2].gray_level =  gamma_lut[(current_g * currLevel / 254)];

    status = BP5758D_ApplyConfig(&led_cfg);
    printf("BP5758D_Set_level: %d\r\n", status);
}
void BP5758D_Set_Temperature(uint8_t currLevel,uint16_t temperature)
{
    int status;
    uint32_t hw_temp_delta=LAM_MAX_MIREDS_DEFAULT-LAM_MIN_MIREDS_DEFAULT;
    uint32_t soft_temp_delta;

    if(temperature>LAM_MAX_MIREDS_DEFAULT)
    {
        temperature=LAM_MAX_MIREDS_DEFAULT;
    }
    else if(temperature<LAM_MIN_MIREDS_DEFAULT)
    {
        temperature=LAM_MIN_MIREDS_DEFAULT;
    }
    
    soft_temp_delta=temperature-LAM_MIN_MIREDS_DEFAULT;
    soft_temp_delta*=100;

    uint32_t warm = (254*(soft_temp_delta/hw_temp_delta))/100;
    uint32_t clod  = 254-warm;

    for(int i=0; i<3; i++)
    {
        led_cfg.out[i].out_en = 0;
        led_cfg.out[i].curr_range = 0;
        led_cfg.out[i].gray_level = 0;
    }
    led_cfg.out[3].out_en = 1;
    led_cfg.out[3].curr_range = 60;
    led_cfg.out[3].gray_level = gamma_lut[warm*currLevel/254];
    
    led_cfg.out[4].out_en = 1;
    led_cfg.out[4].curr_range = 60;
    led_cfg.out[4].gray_level = gamma_lut[clod*currLevel/254];
    
    status = BP5758D_ApplyConfig(&led_cfg);
    printf("BP5758D_Set_Temperature: %d\r\n", status);
}
