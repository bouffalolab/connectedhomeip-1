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
#include <BindingHandler.h>
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

#include "mboard.h"
#include "AppTask.h"

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

#if CONFIG_ENABLE_CHIP_SHELL
using namespace chip::Shell;
#endif

namespace {

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


    ButtonInit();

    /** Without RESET PIN defined, factory reset will be executed if power cycle count(resetCnt) >= APP_REBOOT_RESET_COUNT */
    uint32_t resetCnt      = 0;
    size_t saved_value_len = 0;
    ef_get_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt), &saved_value_len);

    if (resetCnt > APP_REBOOT_RESET_COUNT)
    {
        resetCnt = 0;
        /** To share with RESET PIN logic, mButtonPressedTime is used to recorded resetCnt increased.
         * +1 makes sure mButtonPressedTime is not zero;
         * a power cycle during factory reset confirm time APP_BUTTON_PRESS_LONG will cancel factoryreset */
        GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count() + 1;
    }
    else
    {
        resetCnt++;
        GetAppTask().mButtonPressedTime = 0;
        GetAppTask().mRestcutTime = System::SystemClock().GetMonotonicMilliseconds64().count() + 1;
    }
    ef_set_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt));

    GetAppTask().sTimer = xTimerCreate("lightTmr", pdMS_TO_TICKS(1000), false, NULL, AppTask::TimerCallback);
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

    vTaskSuspend(NULL);

    // ChipLogProgress(NotSpecified, "App Task started, with SRAM heap %d left\r\n", xPortGetFreeHeapSize());

    while (true)
    {
        appEvent                 = APP_EVENT_NONE;
        BaseType_t eventReceived = xTaskNotifyWait(0, APP_EVENT_ALL_MASK, (uint32_t *) &appEvent, portMAX_DELAY);

        if (eventReceived)
        {
            PlatformMgr().LockChipStack();

            if (APP_EVENT_BTN_SHORT & appEvent)
            {
                if (Server::GetInstance().GetFabricTable().FabricCount())
                {
                    BindingCommandData * data = Platform::New<BindingCommandData>();
                    data->commandId           = chip::app::Clusters::OnOff::Commands::Toggle::Id;
                    data->clusterId           = chip::app::Clusters::OnOff::Id;

                    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
                }
                else
                {

                }
            }

#ifdef BOOT_PIN_RESET
            if (APP_EVENT_BTN_LONG & appEvent) 
            {

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

            TimerEventHandler(appEvent);

            PlatformMgr().UnlockChipStack();
        }
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
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime > APP_BUTTON_PRESS_LONG)
        {
            /** factory reset confirm timeout */
            GetAppTask().mButtonPressedTime = 0;
            GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
        }
        else
        {

        }
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
static void app_task_gpio_isr(int irq, void *arg) 
{
    bool intstatus = bflb_gpio_get_intstatus(app_task_gpio_var, BOOT_PIN_RESET);
    if (intstatus) {
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
    if (ButtonPressed())
    {       
        GetAppTask().mTimerIntvl = 1000;
        GetAppTask().PostEvent(APP_EVENT_BTN_SHORT);
    }
    else
    {
        GetAppTask().mTimerIntvl = 1000;
        GetAppTask().PostEvent(APP_EVENT_BTN_SHORT);
    }
}
#endif
