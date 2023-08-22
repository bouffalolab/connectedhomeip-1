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

#include <bl616_glb.h>

#include <FreeRTOS.h>
#include <task.h>

#include <bflb_mtd.h>
#include <bl616dk/board.h>
#include <easyflash.h>
#include <plat.h>
void * __dso_handle;
extern void __libc_init_array(void);
extern void shell_init_with_task(struct bflb_device_s * shell);

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
static int btblecontroller_em_config(void)
{
    extern uint8_t __LD_CONFIG_EM_SEL;
    volatile uint32_t em_size;

    em_size = (uint32_t) &__LD_CONFIG_EM_SEL;

    if (em_size == 0)
    {
        GLB_Set_EM_Sel(GLB_WRAM160KB_EM0KB);
    }
    else if (em_size == 32 * 1024)
    {
        GLB_Set_EM_Sel(GLB_WRAM128KB_EM32KB);
    }
    else if (em_size == 64 * 1024)
    {
        GLB_Set_EM_Sel(GLB_WRAM96KB_EM64KB);
    }
    else
    {
        GLB_Set_EM_Sel(GLB_WRAM96KB_EM64KB);
    }

    return 0;
}
#endif

void platform_port_init(void)
{
    board_init();

#if CONFIG_ENABLE_CHIP_SHELL
    struct bflb_device_s * uart0 = bflb_device_get_by_name("uart0");
    shell_init_with_task(uart0);
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
    btblecontroller_em_config();
#endif

    __libc_init_array();

    bflb_mtd_init();
}

void vAssertCalled(void)
{
    void * ra = (void *) __builtin_return_address(0);

    taskDISABLE_INTERRUPTS();
    if (xPortIsInsideInterrupt())
    {
        printf("vAssertCalled, ra = %p in ISR\r\n", (void *) ra);
    }
    else
    {
        printf("vAssertCalled, ra = %p in task %s\r\n", (void *) ra, pcTaskGetName(NULL));
    }

    while (true)
        ;
}