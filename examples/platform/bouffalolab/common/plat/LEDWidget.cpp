/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <bl_gpio.h>
#include <bl_sys.h>
#include <board.h>
#if defined(CFG_USE_BP5758D)
#include "BP5758D.h"
#else

#include <demo_pwm.h>
#include <hosal_gpio.h>

#endif
#include "LEDWidget.h"

void LEDWidget::Init()
{
#ifdef LED1_PIN
    mPin = LED1_PIN;

    hosal_gpio_dev_t gpio_led = { .config = OUTPUT_OPEN_DRAIN_NO_PULL, .priv = NULL };
    gpio_led.port             = mPin;

    hosal_gpio_init(&gpio_led);
#endif
    SetOnoff(false);
}

void LEDWidget::Toggle(void)
{
    SetOnoff(1 - mOnoff);
}

void LEDWidget::SetOnoff(bool state)
{
#ifdef LED1_PIN
    hosal_gpio_dev_t gpio_led = { .port = mPin, .config = OUTPUT_OPEN_DRAIN_NO_PULL, .priv = NULL };

    mOnoff = state;

    if (state)
    {
        hosal_gpio_output_set(&gpio_led, 1);
    }
    else
    {
        hosal_gpio_output_set(&gpio_led, 0);
    }
#endif
}

bool LEDWidget::GetOnoff(void)
{
    return mOnoff ? true : false;
}

void DimmableLEDWidget::Init()
{
    mOnoff = light_v = 0;
#if defined(CFG_USE_BP5758D)
    BP5758D_Config_Init();
#else

#ifdef MAX_PWM_CHANNEL
    demo_hosal_pwm_init();
    demo_hosal_pwm_start();
#endif

#endif
}

void DimmableLEDWidget::Toggle(void)
{
    SetOnoff(1 - mOnoff);
}

void DimmableLEDWidget::SetOnoff(bool state)
{
    mOnoff = state;
#if defined(MAX_PWM_CHANNEL) || defined(CFG_USE_BP5758D)
    if (mOnoff)
    {
        if (light_v)
        {
#if defined(CFG_USE_BP5758D)
            BP5758D_Set_level(light_v);
#else
            set_level(light_v);
#endif
        }
        else
        {
#if defined(CFG_USE_BP5758D)
            BP5758D_Set_level(254);
#else
            set_level(254);
#endif
        }
    }
    else
    {
#if defined(CFG_USE_BP5758D)
        BP5758D_Set_level(0);
#else
        set_level(0);
#endif
    }
#endif
}

void DimmableLEDWidget::SetLevel(uint8_t level)
{

#if defined(CFG_USE_BP5758D)
    BP5758D_Set_level(level);
#else
#if defined(MAX_PWM_CHANNEL)
    set_level(level);
#endif
#endif
    light_v = level;
    mOnoff  = light_v > 0;
}

void ColorLEDWidget::Init()
{
    mOnoff = light_v = light_s = light_h = 0;
#if defined(CFG_USE_BP5758D)
    BP5758D_Config_Init();
#else

#ifdef MAX_PWM_CHANNEL
    demo_hosal_pwm_init();
    demo_hosal_pwm_start();
#endif

#endif
}

void ColorLEDWidget::Toggle(void)
{
    SetOnoff(1 - mOnoff);
}

void ColorLEDWidget::SetOnoff(bool state)
{
#if defined(MAX_PWM_CHANNEL) || defined(CFG_USE_BP5758D)
    mOnoff = state;
    if (mOnoff)
    {
        if (0 == light_v)
        {
#if defined(CFG_USE_BP5758D)
            BP5758D_Set_Color(254, light_h, light_s);
#else
            set_color(254, light_h, light_s);
#endif
        }
        else
        {
#if defined(CFG_USE_BP5758D)
            BP5758D_Set_Color(light_v, light_h, light_s);
#else
            set_color(light_v, light_h, light_s);
#endif
        }
    }
    else
    {
#if defined(CFG_USE_BP5758D)
        BP5758D_Set_Color(0, light_h, light_s);
#else
        set_color(0, light_h, light_s);
#endif
    }
#endif
}

void ColorLEDWidget::SetLevel(uint8_t level)
{
    SetColor(level, light_h, light_s);
}

void ColorLEDWidget::SetColor(uint8_t level, uint8_t hue, uint8_t sat)
{
#if defined(CFG_USE_BP5758D)
    BP5758D_Set_Color(level, hue, sat);
#else

#ifdef MAX_PWM_CHANNEL
    set_color(level, hue, sat);
#endif

#endif
    light_v = level;
    light_h = hue;
    light_s = sat;
    mOnoff  = light_v > 0;
}
