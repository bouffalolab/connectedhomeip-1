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

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>

#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <system/SystemClock.h>

#if HEAP_MONITORING
#include "MemMonitoring.h"
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#include <platform/bouffalolab/common/ThreadStackManagerImpl.h>
#include <utils_list.h>
#endif

#if CONFIG_ENABLE_CHIP_SHELL
#include <ChipShellCollection.h>
#include <lib/shell/Engine.h>
#endif

#include <LEDWidget.h>
#include <plat.h>

#include <easyflash.h>

#if CHIP_DEVICE_LAYER_TARGET_BL616
#ifdef BOOT_PIN_RESET
#include <bflb_gpio.h>
#endif
#else
extern "C" {
#include <bl_gpio.h>
#include <hal_gpio.h>
#include <hosal_gpio.h>
}
#endif

#ifdef BL616_COLOR_LIGHT
#include "demo_light_effect.h"
#endif

#include "AppTask.h"
#include "mboard.h"

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

#if CONFIG_ENABLE_CHIP_SHELL
using namespace chip::Shell;
#endif

namespace {

#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT) || defined(BL616_COLOR_LIGHT)
ColorLEDWidget sLightLED;
#else
DimmableLEDWidget sLightLED;
#endif

Identify sIdentify = {
    APP_LIGHT_ENDPOINT_ID,
    AppTask::IdentifyStartHandler,
    AppTask::IdentifyStopHandler,
    Clusters::Identify::IdentifyTypeEnum::kLightOutput,
};

} // namespace

AppTask AppTask::sAppTask;
StackType_t AppTask::appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t AppTask::appTaskStruct;

void StartAppTask(void)
{
    GetAppTask().sAppTaskHandle = xTaskCreateStatic(GetAppTask().AppTaskMain, APP_TASK_NAME, ArraySize(GetAppTask().appStack), NULL,
                                                    APP_TASK_PRIORITY, GetAppTask().appStack, &GetAppTask().appTaskStruct);
    if (GetAppTask().sAppTaskHandle == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create app task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }
}

#if CONFIG_ENABLE_CHIP_SHELL

#if CHIP_DEVICE_LAYER_TARGET_BL616

CHIP_ERROR AppTask::StartAppShellTask()
{
    Engine::Root().Init();

    cmd_misc_init();

    Engine::Root().RunMainLoop();

    return CHIP_NO_ERROR;
}
#else
void AppTask::AppShellTask(void * args)
{
    Engine::Root().RunMainLoop();
}

CHIP_ERROR AppTask::StartAppShellTask()
{
    static TaskHandle_t shellTask;

    Engine::Root().Init();

    cmd_misc_init();

    xTaskCreate(AppTask::AppShellTask, "chip_shell", 1024 / sizeof(configSTACK_DEPTH_TYPE), NULL, APP_TASK_PRIORITY, &shellTask);

    return CHIP_NO_ERROR;
}
#endif
#endif

void AppTask::PostEvent(app_event_t event)
{
    if (xPortIsInsideInterrupt())
    {
        BaseType_t higherPrioTaskWoken = pdFALSE;
        xTaskNotifyFromISR(sAppTaskHandle, event, eSetBits, &higherPrioTaskWoken);
    }
    else
    {
        xTaskNotify(sAppTaskHandle, event, eSetBits);
    }
}

void AppTask::AppTaskMain(void * pvParameter)
{
    app_event_t appEvent;
    bool onoff = false;
    uint16_t temperature;
    uint8_t colormode;
    uint8_t level;
#ifndef BL706_ETHERNET
    sLightLED.Init();
#endif

#ifdef BOOT_PIN_RESET
    ButtonInit();
#else
    /** Without RESET PIN defined, factory reset will be executed if power cycle count(resetCnt) >= APP_REBOOT_RESET_COUNT */
    uint32_t resetCnt      = 1;
    size_t saved_value_len = 0;
    ef_get_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt), &saved_value_len);
    printf("reset cnt %ld\r\n ", resetCnt);
    if (resetCnt > APP_REBOOT_RESET_COUNT)
    {
        /** To share with RESET PIN logic, mButtonPressedTime is used to recorded resetCnt increased.
         * +1 makes sure mButtonPressedTime is not zero;
         * a power cycle during factory reset confirm time APP_BUTTON_PRESS_LONG will cancel factoryreset */
        GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count() + 1;
        ChipLogProgress(NotSpecified, "factory reset confirm timeout.");
    }
    else
    {
        resetCnt++;
        GetAppTask().mRestcutTime = System::SystemClock().GetMonotonicMilliseconds64().count() + 1;
    }
    ef_set_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt));
#endif
    GetAppTask().mcommissionTime = 0;
    GetAppTask().sTimer          = xTimerCreate("lightTmr", pdMS_TO_TICKS(1000), false, NULL, AppTask::TimerCallback);
    if (GetAppTask().sTimer == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create timer task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }

    ChipLogProgress(NotSpecified, "Starting Platform Manager Event Loop");
    CHIP_ERROR ret = PlatformMgr().StartEventLoopTask();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "PlatformMgr().StartEventLoopTask() failed");
        appError(ret);
    }

    GetAppTask().PostEvent(APP_EVENT_TIMER);
    GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);

    vTaskSuspend(NULL);

    while (true)
    {
        appEvent                 = APP_EVENT_NONE;
        BaseType_t eventReceived = xTaskNotifyWait(0, APP_EVENT_ALL_MASK, (uint32_t *) &appEvent, portMAX_DELAY);
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
        if (ConnectivityMgr().IsWiFiStationConnected() == true)
#else
        if (ConnectivityMgr().IsThreadAttached() == true)
#endif
        {
            if (Server::GetInstance().GetFabricTable().FabricCount() == 0)
            {
                DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
            }
            if ((Server::GetInstance().GetFabricTable().FabricCount() >= 1) &&
                (chip::Server::GetInstance().GetFailSafeContext().IsFailSafeArmed() == false))
            {
                if (GetAppTask().mcommissionTime != 0)
                {
                    appEvent      = APP_EVENT_COMMISON_COMPLETE;
                    eventReceived = true;
                }
            }
        }
        if (eventReceived)
        {
            PlatformMgr().LockChipStack();

            if (APP_EVENT_LIGHTING_MASK & appEvent)
            {
                LightingUpdate(appEvent);
            }

            if (APP_EVENT_BTN_SHORT & appEvent)
            {
                if (Server::GetInstance().GetFabricTable().FabricCount())
                {
                    Clusters::OnOff::Attributes::OnOff::Get(GetAppTask().GetEndpointId(), &onoff);
                    onoff = !onoff;
                    Clusters::OnOff::Attributes::OnOff::Set(GetAppTask().GetEndpointId(), onoff);
                }
                else
                {
                    sLightLED.Toggle();
                }
            }

#ifdef BOOT_PIN_RESET
            if (APP_EVENT_BTN_LONG & appEvent)
            {
                /** Turn off light to indicate button long press for factory reset is confirmed */
                sLightLED.SetOnoff(false);
            }
#endif
            if (APP_EVENT_IDENTIFY_MASK & appEvent)
            {
                IdentifyHandleOp(appEvent);
            }

            if (APP_EVENT_FACTORY_RESET & appEvent)
            {
                DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
            }
            if (APP_EVENT_COMMISON_TOOGLE == appEvent)
            {
                GetAppTask().mTimerIntvl       = 1000;
                GetAppTask().mcommissionToggle = true;
            }
            if (APP_EVENT_COMMISON_START == appEvent)
            {
                GetAppTask().mcommissionToggle  = false;
                GetAppTask().mcommission        = 0;
                GetAppTask().mTimerIntvl        = 3000;
                GetAppTask().mcommission_target = 0xfe;
                hw_set_temperature(0, 154);
                GetAppTask().mcommissionTime = System::SystemClock().GetMonotonicMilliseconds64().count();
            }
            if (APP_EVENT_COMMISON_COMPLETE == appEvent)
            {
                GetAppTask().mcommissionTime = 0;
                GetAppTask().mTimerIntvl     = 1000;
                set_cold_temperature();
                onoff       = 1;
                colormode   = 2;
                temperature = 154;
                level       = 254;
                Clusters::OnOff::Attributes::OnOff::Set(GetAppTask().GetEndpointId(), onoff);
                Clusters::LevelControl::Attributes::CurrentLevel::Set(GetAppTask().GetEndpointId(), level);
                Clusters::ColorControl::Attributes::ColorMode::Set(GetAppTask().GetEndpointId(), colormode);
                Clusters::ColorControl::Attributes::ColorTemperatureMireds::Set(GetAppTask().GetEndpointId(), temperature);
            }
            TimerEventHandler(appEvent);

            PlatformMgr().UnlockChipStack();
        }
    }
}

void AppTask::LightingUpdate(app_event_t status)
{
    uint8_t hue, sat;
    bool onoff;
    DataModel::Nullable<uint8_t> v(0);
    EndpointId endpoint = GetAppTask().GetEndpointId();
    uint16_t temperature;
    uint8_t colormode;
    if (APP_EVENT_LIGHTING_MASK & status)
    {

        if (Server::GetInstance().GetFabricTable().FabricCount())
        {
            do
            {
                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::OnOff::Attributes::OnOff::Get(endpoint, &onoff))
                {
                    break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::LevelControl::Attributes::CurrentLevel::Get(endpoint, v))
                {
                    break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::ColorControl::Attributes::CurrentHue::Get(endpoint, &hue))
                {
                    break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::ColorControl::Attributes::CurrentSaturation::Get(endpoint, &sat))
                {
                    break;
                }
                if (EMBER_ZCL_STATUS_SUCCESS !=
                    Clusters::ColorControl::Attributes::ColorTemperatureMireds::Get(endpoint, &temperature))
                {
                    break;
                }
                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::ColorControl::Attributes::ColorMode::Get(endpoint, &colormode))
                {
                    break;
                }
                printf("%s onoff =%d  level= %d,hue =%d  sat %d colormode %d temperature %d\r\n", __func__, onoff, v.Value(), hue,
                       sat, colormode, temperature);
                if (!onoff)
                {
                    sLightLED.SetLevel(0, colormode);
                }
                else
                {
                    if (v.IsNull())
                    {
                        v.SetNonNull(254);
                    }
#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT) || defined(BL616_COLOR_LIGHT)

                    if (colormode != 2)
                    {
                        if (ConnectivityMgr().IsWiFiStationConnected() == false)
                        {
                            hw_set_color(v.Value(), hue, sat);
                        }
                        else
                        {
                            sLightLED.SetColor(v.Value(), hue, sat);
                        }
                    }
                    else
                    {
                        if (ConnectivityMgr().IsWiFiStationConnected() == false)
                        {
                            hw_set_temperature(v.Value(), temperature);
                        }
                        else
                        {
                            sLightLED.SetTemperature(v.Value(), temperature);
                        }
                        printf("%s level= %d,temperature =%d \r\n", __func__, v.Value(), temperature);
                    }
#else
                    sLightLED.SetLevel(v.Value());
#endif
                }
            } while (0);
        }
        else
        {}
    }
}

bool AppTask::StartTimer(void)
{
    if (xTimerIsTimerActive(GetAppTask().sTimer))
    {
        CancelTimer();
    }

    if (GetAppTask().mTimerIntvl == 0)
    {
        GetAppTask().mTimerIntvl = 1000;
    }

    if (xTimerChangePeriod(GetAppTask().sTimer, pdMS_TO_TICKS(GetAppTask().mTimerIntvl), pdMS_TO_TICKS(100)) != pdPASS)
    {
        ChipLogProgress(NotSpecified, "Failed to access timer with 100 ms delay.");
    }

    return true;
}

void AppTask::CancelTimer(void)
{
    xTimerStop(GetAppTask().sTimer, 0);
}

void AppTask::TimerCallback(TimerHandle_t xTimer)
{
    GetAppTask().PostEvent(APP_EVENT_TIMER);
}

void AppTask::TimerEventHandler(app_event_t event)
{
    if (GetAppTask().mButtonPressedTime)
    {
#ifdef BOOT_PIN_RESET
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime > APP_BUTTON_PRESS_LONG)
        {
            GetAppTask().PostEvent(APP_EVENT_BTN_LONG);
        }
        else if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime >=
                 APP_BUTTON_PRESS_SHORT)
        {
#if defined(BL602_NIGHT_LIGHT) || defined(BL706_NIGHT_LIGHT) || defined(BL616_COLOR_LIGHT)
            /** change color to indicate to wait factory reset confirm */
            sLightLED.SetColor(254, 0, 210);
#else
            /** toggle led to indicate to wait factory reset confirm */
            sLightLED.Toggle();
#endif
        }
#else
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime > APP_BUTTON_PRESS_LONG)
        {
            ChipLogProgress(NotSpecified, "factory reset confirm timeout.");
            GetAppTask().mButtonPressedTime = 0;
            GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
        }
        else
        {
#if defined(BL602_NIGHT_LIGHT) || defined(BL706_NIGHT_LIGHT) || defined(BL616_COLOR_LIGHT)
            /** change color to indicate to wait factory reset confirm */
            // sLightLED.SetColor(254, 0, 210);
#else
            /** toggle led to indicate to wait factory reset confirm */
            // sLightLED.Toggle();
#endif
        }
#endif
    }
    if (GetAppTask().mcommissionTime)
    {
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mcommissionTime > APP_EVENT_COMMISON_TIME)
        {
            GetAppTask().mTimerIntvl     = 1000;
            GetAppTask().mcommissionTime = 0;
            hw_set_temperature(0xfe, 154);
        }
        else
        {
            set_temperature(GetAppTask().mcommission_target, 154);
            if (GetAppTask().mcommission_target == 0xf)
            {
                GetAppTask().mcommission_target = 0xfe;
            }
            else if (GetAppTask().mcommission_target == 0xfe)
            {
                GetAppTask().mcommission_target = 0xf;
            }
        }
    }
    if (GetAppTask().mcommissionToggle == true)
    {

        if (GetAppTask().mcommission == 0)
        {
            set_warm_temperature();
        }
        else if (GetAppTask().mcommission == 1)
        {
            set_cold_temperature();
        }
        else if (GetAppTask().mcommission == 2)
        {
            set_color_red();
        }
        else if (GetAppTask().mcommission == 3)
        {
            set_color_green();
        }
        else if (GetAppTask().mcommission == 4)
        {
            set_color_blue();
        }
        else if (GetAppTask().mcommission == 5)
        {
            GetAppTask().mcommissionToggle = false;
            GetAppTask().PostEvent(AppTask::APP_EVENT_COMMISON_START);
        }
        GetAppTask().mcommission++;
    }
    if (GetAppTask().mRestcutTime)
    {
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mRestcutTime > APP_BUTTON_PRESS_LONG)
        {
            GetAppTask().mRestcutTime = 0;
            uint32_t resetCnt         = 1;
            ef_set_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt));
        }
    }
    // bl61x_get_chip_temp();
    StartTimer();
}

void AppTask::IdentifyStartHandler(Identify *)
{
    GetAppTask().PostEvent(APP_EVENT_IDENTIFY_START);
}

void AppTask::IdentifyStopHandler(Identify *)
{
    GetAppTask().PostEvent(APP_EVENT_IDENTIFY_STOP);
}

void AppTask::IdentifyHandleOp(app_event_t event)
{
    static uint32_t identifyState = 0;

    if (APP_EVENT_IDENTIFY_START & event)
    {
        identifyState = 1;
        ChipLogProgress(NotSpecified, "identify start");
    }

    if ((APP_EVENT_IDENTIFY_IDENTIFY & event) && identifyState)
    {
        sLightLED.Toggle();
        ChipLogProgress(NotSpecified, "identify");
    }

    if (APP_EVENT_IDENTIFY_STOP & event)
    {
        identifyState = 0;
        GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);
        ChipLogProgress(NotSpecified, "identify stop");
    }
}

void AppTask::ButtonEventHandler(uint8_t btnIdx, uint8_t btnAction)
{
    GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
}

#ifdef BOOT_PIN_RESET
#if CHIP_DEVICE_LAYER_TARGET_BL616
static struct bflb_device_s * app_task_gpio_var = NULL;
static void app_task_gpio_isr(int irq, void * arg)
{
    bool intstatus = bflb_gpio_get_intstatus(app_task_gpio_var, BOOT_PIN_RESET);
    if (intstatus)
    {
        bflb_gpio_int_clear(app_task_gpio_var, BOOT_PIN_RESET);
    }

    GetAppTask().ButtonEventHandler(arg);
}
#else
static hosal_gpio_dev_t gpio_key = { .port = BOOT_PIN_RESET, .config = INPUT_HIGH_IMPEDANCE, .priv = NULL };
#endif

void AppTask::ButtonInit(void)
{
    GetAppTask().mButtonPressedTime = 0;

#if CHIP_DEVICE_LAYER_TARGET_BL616
    app_task_gpio_var = bflb_device_get_by_name("gpio");

    bflb_gpio_int_init(app_task_gpio_var, BOOT_PIN_RESET, GPIO_INT_TRIG_MODE_SYNC_FALLING_RISING_EDGE);
    bflb_gpio_int_mask(app_task_gpio_var, BOOT_PIN_RESET, false);

    bflb_irq_attach(app_task_gpio_var->irq_num, app_task_gpio_isr, app_task_gpio_var);
    bflb_irq_enable(app_task_gpio_var->irq_num);
#else
    hosal_gpio_init(&gpio_key);
    hosal_gpio_irq_set(&gpio_key, HOSAL_IRQ_TRIG_POS_PULSE, GetAppTask().ButtonEventHandler, NULL);
#endif
}

bool AppTask::ButtonPressed(void)
{
#if CHIP_DEVICE_LAYER_TARGET_BL616
    return bflb_gpio_read(app_task_gpio_var, BOOT_PIN_RESET);
#else
    uint8_t val = 1;

    hosal_gpio_input_get(&gpio_key, &val);

    return val == 1;
#endif
}

void AppTask::ButtonEventHandler(void * arg)
{
    uint32_t presstime;

    if (ButtonPressed())
    {
#if CHIP_DEVICE_LAYER_TARGET_BL702L
        bl_set_gpio_intmod(gpio_key.port, HOSAL_IRQ_TRIG_NEG_LEVEL);
#elif !CHIP_DEVICE_LAYER_TARGET_BL616
        bl_set_gpio_intmod(gpio_key.port, 1, HOSAL_IRQ_TRIG_NEG_LEVEL);
#endif
        GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count();
        GetAppTask().mTimerIntvl        = APP_BUTTON_PRESS_JITTER;
        GetAppTask().PostEvent(APP_EVENT_TIMER);
    }
    else
    {
#if CHIP_DEVICE_LAYER_TARGET_BL702L
        bl_set_gpio_intmod(gpio_key.port, HOSAL_IRQ_TRIG_POS_PULSE);
#elif !CHIP_DEVICE_LAYER_TARGET_BL616
        bl_set_gpio_intmod(gpio_key.port, 1, HOSAL_IRQ_TRIG_POS_PULSE);
#endif

        if (GetAppTask().mButtonPressedTime)
        {
            presstime = System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime;
            if (presstime >= APP_BUTTON_PRESS_LONG)
            {
                GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
            }
            else if (presstime <= APP_BUTTON_PRESS_SHORT && presstime >= APP_BUTTON_PRESS_JITTER)
            {
                GetAppTask().mTimerIntvl = 1000;
                GetAppTask().PostEvent(APP_EVENT_BTN_SHORT);
            }
            else
            {
                GetAppTask().mTimerIntvl = 1000;
                GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);
            }
        }

        GetAppTask().mButtonPressedTime = 0;
    }
}
#endif
