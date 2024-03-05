/*
 *    Copyright (c) 2022 Project CHIP Authors
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

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/bouffalolab/common/BLConfig.h>

namespace chip {
namespace DeviceLayer {
namespace Internal {

void BLConfig::Init(void)
{
}

CHIP_ERROR BLConfig::ReadConfigValue(const char * key, uint8_t * val, size_t size, size_t & readsize)
{
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

CHIP_ERROR BLConfig::ReadConfigValue(const char * key, bool & val)
{
    size_t readlen = 0;
    return ReadConfigValue(key, (uint8_t *) &val, 1, readlen);
}

CHIP_ERROR BLConfig::ReadConfigValue(const char * key, uint32_t & val)
{
    size_t readlen = 0;
    return ReadConfigValue(key, (uint8_t *) &val, sizeof(val), readlen);
}

CHIP_ERROR BLConfig::ReadConfigValue(const char * key, uint64_t & val)
{
    size_t readlen = 0;
    return ReadConfigValue(key, (uint8_t *) &val, sizeof(val), readlen);
}

CHIP_ERROR BLConfig::ReadConfigValueStr(const char * key, char * buf, size_t bufSize, size_t & outLen)
{
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

CHIP_ERROR BLConfig::ReadConfigValueBin(const char * key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

CHIP_ERROR BLConfig::WriteConfigValue(const char * key, uint8_t * val, size_t size)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLConfig::WriteConfigValue(const char * key, bool val)
{
    return WriteConfigValue(key, (uint8_t *) &val, sizeof(val));
}

CHIP_ERROR BLConfig::WriteConfigValue(const char * key, uint32_t val)
{
    return WriteConfigValue(key, (uint8_t *) &val, sizeof(val));
}

CHIP_ERROR BLConfig::WriteConfigValue(const char * key, uint64_t val)
{
    return WriteConfigValue(key, (uint8_t *) &val, sizeof(val));
}

CHIP_ERROR BLConfig::WriteConfigValueStr(const char * key, const char * str)
{
    return WriteConfigValue(key, (uint8_t *) str, strlen(str) + 1);
}

CHIP_ERROR BLConfig::WriteConfigValueStr(const char * key, const char * str, size_t strLen)
{
    return WriteConfigValue(key, (uint8_t *) str, strLen);
}

CHIP_ERROR BLConfig::WriteConfigValueBin(const char * key, const uint8_t * data, size_t dataLen)
{
    return WriteConfigValue(key, (uint8_t *) data, dataLen);
}

CHIP_ERROR BLConfig::ClearConfigValue(const char * key)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLConfig::FactoryResetConfig(void)
{
    return CHIP_NO_ERROR;
}

void BLConfig::RunConfigUnitTest() {}

bool BLConfig::ConfigValueExists(const char * key)
{
    return true;
}

CHIP_ERROR BLConfig::ReadKVS(const char * key, void * value, size_t value_size, size_t * read_bytes_size, size_t offset_bytes)
{
    return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

CHIP_ERROR BLConfig::WriteKVS(const char * key, const void * value, size_t value_size)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLConfig::ClearKVS(const char * key)
{
    return CHIP_NO_ERROR;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
