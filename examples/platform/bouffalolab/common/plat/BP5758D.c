#include "BP5758D.h"
#include "bl702l_glb.h"
#include "bl702l_i2c.h"


#define I2C_SLAVE_ADDR_NORMAL      (0xB0 >> 1)
#define I2C_SLAVE_ADDR_SLEEP       (0x80 >> 1)
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
        
        data[i * 2 + 6] = cfg->out[i].gray_level & 0x1F;
        data[i * 2 + 7] = cfg->out[i].gray_level >> 5;
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
    
    status = BP5758D_Init(12, 11);
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
    // red
    led_cfg.out[0].out_en = 1;
    led_cfg.out[0].curr_range = 60;
    led_cfg.out[0].gray_level = red * 1023 / 254;

    // blue
    led_cfg.out[1].out_en = 1;
    led_cfg.out[1].curr_range = 60;
    led_cfg.out[1].gray_level = blue * 1023 / 254;

    // green
    led_cfg.out[2].out_en = 1;
    led_cfg.out[2].curr_range = 60;
    led_cfg.out[2].gray_level = green * 1023 / 254;

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
    led_cfg.out[0].gray_level = led_cfg.out[0].gray_level * currLevel / 254;

    // blue
    led_cfg.out[1].out_en = 1;
    led_cfg.out[1].curr_range = 60;
    led_cfg.out[1].gray_level = led_cfg.out[1].gray_level * currLevel / 254;

    // green
    led_cfg.out[2].out_en = 1;
    led_cfg.out[2].curr_range = 60;
    led_cfg.out[2].gray_level = led_cfg.out[2].gray_level * currLevel / 254;

    status = BP5758D_ApplyConfig(&led_cfg);
    printf("BP5758D_Set_level: %d\r\n", status);
}