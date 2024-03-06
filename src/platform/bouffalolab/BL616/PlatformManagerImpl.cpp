/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    Copyright (c) 2019 Nest Labs, Inc.
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

#include <stdlib.h>
#include <crypto/CHIPCryptoPAL.h>
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/PlatformManager.h>
#include <platform/FreeRTOS/SystemTimeSupport.h>
#include <platform/bouffalolab/BL616/NetworkCommissioningDriver.h>
#include <platform/bouffalolab/common/DiagnosticDataProviderImpl.h>
#include <platform/internal/GenericPlatformManagerImpl_FreeRTOS.ipp>

#include <lwip/tcpip.h>

#include <bflb_sec_trng.h>
extern "C" {
#include <rfparam_adapter.h>
}
#include "wifi_mgmr_portable.h"


namespace chip {
namespace DeviceLayer {

static int app_entropy_source(void * data, unsigned char * output, size_t len, size_t * olen)
{
    bflb_trng_readlen(reinterpret_cast<uint8_t *>(output), static_cast<int>(len));
    *olen = len;
    return 0;
}

CHIP_ERROR PlatformManagerImpl::_InitChipStack(void)
{
    CHIP_ERROR err                 = CHIP_NO_ERROR;
    TaskHandle_t backup_eventLoopTask;
    int iret_rfInit = -1;

    VerifyOrDieWithMsg(0 == (iret_rfInit = rfparam_init(0, NULL, 0)), DeviceLayer, "rfparam_init failed with %d", iret_rfInit);

    // Initialize LwIP.
    tcpip_init(NULL, NULL);

    wifi_start_firmware_task();
    err = chip::Crypto::add_entropy_source(app_entropy_source, NULL, 16);
    SuccessOrExit(err);

    // Call _InitChipStack() on the generic implementation base class
    // to finish the initialization process.
    /** weiyin, backup mEventLoopTask which is reset in _InitChipStack */
    backup_eventLoopTask = Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::mEventLoopTask;
    err                  = Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_InitChipStack();
    SuccessOrExit(err);
    Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::mEventLoopTask = backup_eventLoopTask;

exit:
    return err;
}

} // namespace DeviceLayer
} // namespace chip
