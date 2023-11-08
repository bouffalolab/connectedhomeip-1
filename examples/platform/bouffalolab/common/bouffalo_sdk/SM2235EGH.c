#include "SM2235EGH.h"
#include "FreeRTOS.h"
#include "bl616_glb_gpio.h"
#include "mboard.h"
#include "task.h"

SM2235EGH_Config_t led_cfg;
#define I2C_SLAVE_ADDR_NORMAL (0xD8 >> 1)
#define I2C_SLAVE_ADDR_SLEEP (0xC0 >> 1)

#define SM2135_DELAY 16

// Software I2C
typedef struct softI2C_s
{
    short pin_clk;
    short pin_data;
    // I really have to place it here for a GN6932 driver, which is an SPI version of TM1637
    // short pin_stb;
    // I must somehow be able to tell which proto we have?
    // short protocolType;
} softI2C_t;

static softI2C_t g_softI2C;

void usleep(int r) // delay function do 10*r nops, because rtos_delay_milliseconds is too much
{
    for (volatile int i = 0; i < r; i++)
        __asm__("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
}

void Soft_I2C_SetLow(uint8_t pin)
{
    // HAL_PIN_Setup_Output(pin);
    // HAL_PIN_SetOutputValue(pin, 0);
    GLB_GPIO_Write((GLB_GPIO_Type) pin, 0);
}

void Soft_I2C_SetHigh(uint8_t pin)
{
    // HAL_PIN_Setup_Input_Pullup(pin);
    GLB_GPIO_Write((GLB_GPIO_Type) pin, 1);
}

bool Soft_I2C_PreInit(softI2C_t * i2c)
{
    // HAL_PIN_SetOutputValue(i2c->pin_data, 0);
    // HAL_PIN_SetOutputValue(i2c->pin_clk, 0);
    // Soft_I2C_SetHigh(i2c->pin_data);
    // Soft_I2C_SetHigh(i2c->pin_clk);
    // return (!((HAL_PIN_ReadDigitalInput(i2c->pin_data) == 0 || HAL_PIN_ReadDigitalInput(i2c->pin_clk) == 0)));
    GLB_GPIO_Write((GLB_GPIO_Type) i2c->pin_data, 1);
    GLB_GPIO_Write((GLB_GPIO_Type) i2c->pin_clk, 1);
    GLB_GPIO_Cfg_Type cfg;
    cfg.drive      = 0;
    cfg.smtCtrl    = 1;
    cfg.outputMode = 0;
    cfg.gpioFun    = GPIO_FUN_GPIO;
    cfg.gpioMode   = GPIO_MODE_OUTPUT;
    cfg.pullType   = GPIO_PULL_NONE;
    cfg.gpioPin    = i2c->pin_data;
    GLB_GPIO_Init(&cfg);
    cfg.gpioPin = i2c->pin_clk;
    GLB_GPIO_Init(&cfg);
    return 0;
}

bool Soft_I2C_WriteByte(softI2C_t * i2c, uint8_t value)
{
    uint8_t curr;
    uint8_t ack;

    for (curr = 0X80; curr != 0; curr >>= 1)
    {
        if (curr & value)
        {
            Soft_I2C_SetHigh(i2c->pin_data);
        }
        else
        {
            Soft_I2C_SetLow(i2c->pin_data);
        }
        Soft_I2C_SetHigh(i2c->pin_clk);
        usleep(SM2135_DELAY);
        Soft_I2C_SetLow(i2c->pin_clk);
    }
    // get Ack or Nak
    Soft_I2C_SetHigh(i2c->pin_data);
    Soft_I2C_SetHigh(i2c->pin_clk);
    usleep(SM2135_DELAY / 2);
    ack = 0; // HAL_PIN_ReadDigitalInput(i2c->pin_data);
    Soft_I2C_SetLow(i2c->pin_clk);
    usleep(SM2135_DELAY / 2);
    Soft_I2C_SetLow(i2c->pin_data);
    return (0 == ack);
}

bool Soft_I2C_Start(softI2C_t * i2c, uint8_t addr)
{
    Soft_I2C_SetLow(i2c->pin_data);
    usleep(SM2135_DELAY);
    Soft_I2C_SetLow(i2c->pin_clk);
    return Soft_I2C_WriteByte(i2c, addr);
}

void Soft_I2C_Stop(softI2C_t * i2c)
{
    Soft_I2C_SetLow(i2c->pin_data);
    usleep(SM2135_DELAY);
    Soft_I2C_SetHigh(i2c->pin_clk);
    usleep(SM2135_DELAY);
    Soft_I2C_SetHigh(i2c->pin_data);
    usleep(SM2135_DELAY);
}

int SM2235EGH_Init(int scl_pin, int sda_pin)
{
    if (scl_pin < 0 || scl_pin > 34)
    {
        return -1;
    }

    if (sda_pin < 0 || sda_pin > 34)
    {
        return -2;
    }

    if (scl_pin == sda_pin)
    {
        return -3;
    }

    g_softI2C.pin_data = sda_pin;
    g_softI2C.pin_clk  = scl_pin;
    Soft_I2C_PreInit(&g_softI2C);

    return 0;
}

int SM2235EGH_SendData(uint8_t * data, uint16_t len)
{
#if 0
    for(int i=0; i<len; i++){
        printf("Byte%d: %02X\r\n", i, data[i]);
    }
#endif
    // vTaskEnterCritical();
    Soft_I2C_Start(&g_softI2C, data[0]);
    for (int i = 1; i < len; i++)
    {
        Soft_I2C_WriteByte(&g_softI2C, data[i]);
    }
    Soft_I2C_Stop(&g_softI2C);
    // vTaskEnterCritical();
    return 0;
}

int SM2235EGH_Standby(void)
{
    uint8_t data[2];

    data[0] = I2C_SLAVE_ADDR_SLEEP << 1;
    data[1] = 0x24;

    if (SM2235EGH_SendData(data, sizeof(data)) != 0)
    {
        return -1;
    }

    return 0;
}

int SM2235EGH_ApplyConfig(SM2235EGH_Config_t * cfg)
{
    uint8_t data[12];

    if (cfg->max_curr_rgb > 15)
    {
        cfg->max_curr_rgb = 15;
    }

    if (cfg->max_curr_wy > 15)
    {
        cfg->max_curr_wy = 15;
    }

    if (cfg->gray_level_r > 1023)
    {
        cfg->gray_level_r = 1023;
    }

    if (cfg->gray_level_g > 1023)
    {
        cfg->gray_level_g = 1023;
    }

    if (cfg->gray_level_b > 1023)
    {
        cfg->gray_level_b = 1023;
    }

    if (cfg->gray_level_w > 1023)
    {
        cfg->gray_level_w = 1023;
    }

    if (cfg->gray_level_y > 1023)
    {
        cfg->gray_level_y = 1023;
    }

    data[0]  = I2C_SLAVE_ADDR_NORMAL << 1;
    data[1]  = (cfg->max_curr_rgb << 4) | cfg->max_curr_wy;
    data[2]  = cfg->gray_level_b >> 8;
    data[3]  = cfg->gray_level_b & 0xFF;
    data[4]  = cfg->gray_level_g >> 8;
    data[5]  = cfg->gray_level_g & 0xFF;
    data[6]  = cfg->gray_level_r >> 8;
    data[7]  = cfg->gray_level_r & 0xFF;
    data[8]  = cfg->gray_level_w >> 8;
    data[9]  = cfg->gray_level_w & 0xFF;
    data[10] = cfg->gray_level_y >> 8;
    data[11] = cfg->gray_level_y & 0xFF;

    if (SM2235EGH_SendData(data, sizeof(data)) != 0)
    {
        return -1;
    }
    return 0;
}
void SM2235EGH_Config_Init(void)
{
    int status;
    status = SM2235EGH_Init(SM2235_SCL, SM2235_SDA);
    printf("SM2235EGH Init: %d\r\n", status);
}

void SM2235EGH_Set_Color(uint32_t p_Rduty, uint32_t p_Gduty, uint32_t p_Bduty, uint32_t p_Cduty, uint32_t p_Wduty)
{
    int status;
    led_cfg.max_curr_rgb = 3;
    led_cfg.max_curr_wy  = 10;
    led_cfg.gray_level_r = p_Rduty;
    led_cfg.gray_level_b = p_Bduty;
    led_cfg.gray_level_g = p_Gduty;
    led_cfg.gray_level_w = p_Cduty;
    led_cfg.gray_level_y = p_Wduty;
    status               = SM2235EGH_ApplyConfig(&led_cfg);
}