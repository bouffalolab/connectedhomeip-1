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
#include <stdio.h>
#ifdef BL616_COLOR_LIGHT
#include "demo_light_effect.h"
#else 
#include <demo_pwm.h>
#endif
#include <mboard.h>
#include "LEDWidget.h"

bool LEDWidget::GetOnoff(void)
{
    return mOnoff ? true : false;
}

void DimmableLEDWidget::Init()
{
    mOnoff = light_v = 0;
#ifdef BL616_COLOR_LIGHT
    demo_color_light_init();
#else
    demo_pwm_init();
    demo_pwm_start();
#endif
}

void DimmableLEDWidget::Toggle(void)
{
    SetOnoff(1 - mOnoff);
}

void DimmableLEDWidget::SetOnoff(bool state)
{
    mOnoff = state;
#ifdef MAX_PWM_CHANNEL
    if (mOnoff)
    {
        if (light_v)
        {
            #ifdef BL616_COLOR_LIGHT

            #else
                set_level(light_v);
            #endif
        }
        else
        {
            #ifdef BL616_COLOR_LIGHT

            #else
                set_level(254);
            #endif
        }
    }
    else
    {
            #ifdef BL616_COLOR_LIGHT

            #else
                set_level(0);
            #endif
    }
#endif
}

void DimmableLEDWidget::SetLevel(uint8_t level)
{
    #ifdef BL616_COLOR_LIGHT

    #else
        set_level(level);
    #endif
    light_v = level;
    mOnoff  = light_v > 0;
}

void ColorLEDWidget::Init()
{
    mOnoff = light_v = light_s = light_h = light_t = 0;
#ifdef BL616_COLOR_LIGHT
    demo_color_light_init();
#else 
    demo_pwm_init();
    demo_pwm_start();
#endif
}

void ColorLEDWidget::Toggle(void)
{
    SetOnoff(1 - mOnoff);
}

void ColorLEDWidget::SetOnoff(bool state)
{
#ifdef MAX_PWM_CHANNEL
    mOnoff = state;
    if (mOnoff)
    {
        if (0 == light_v)
        {
            #ifdef BL616_COLOR_LIGHT
                if (mColor_Mode == 2)
                {
                    set_temperature(254, light_t);
                }
                else
                {
                    set_color(254, light_h, light_s);
                }
            #else
                set_color(254, light_h, light_s);
            #endif
        }
        else
        {
            #ifdef BL616_COLOR_LIGHT
                if (mColor_Mode == 2)
                {
                    set_temperature(light_v, light_t);
                }
                else
                {
                    set_color(light_v, light_h, light_s);
                }
            #else
                set_color(light_v, light_h, light_s);
            #endif
        }
    }
    else
    {
        #ifdef BL616_COLOR_LIGHT
            if (mColor_Mode == 2)
            {
                set_temperature(0, light_t);
            }
            else
            {
                set_color(0, light_h, light_s);
            }
        #else
            set_color(0, light_h, light_s);
        #endif
    }
#endif
}

void ColorLEDWidget::SetLevel(uint8_t level, uint8_t color_mode)
{
    if (color_mode == 2)
    {
        SetTemperature(level, light_t);
    }
    else
    {
        SetColor(level, light_h, light_s);
    }
    mColor_Mode = color_mode;
}

void ColorLEDWidget::SetColor(uint8_t level, uint8_t hue, uint8_t sat)
{
    #ifdef BL616_COLOR_LIGHT
        set_color(level, hue, sat);
    #else
        set_color(level, hue, sat);
    #endif
    light_v = level;
    light_h = hue;
    light_s = sat;
    mOnoff  = light_v > 0;
    mColor_Mode = 0;
}

void ColorLEDWidget::SetTemperature(uint8_t level, uint16_t temperature)
{
    #ifdef BL616_COLOR_LIGHT
        set_temperature(level,temperature);
    #else

    #endif
    light_v     = level;
    light_t     = temperature;
    mOnoff      = light_v > 0;
    mColor_Mode = 2;
}