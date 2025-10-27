/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "security_level.h"

#include "device/dm_adapter.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
uint32_t SecurityLevel::GetDeviceSecurityLevel()
{
#ifdef PB_DATACLASSIFICATION_ENABLE
    uint32_t level = securityLevel_.load();
    if (level > DATA_SEC_LEVEL0) {
        return level;
    }
#endif
    return GetSensitiveLevel();
}

#ifdef PB_DATACLASSIFICATION_ENABLE
bool SecurityLevel::InitDEVSLQueryParams(DEVSLQueryParams *params, const std::string &udid)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(params != nullptr && !udid.empty(), false, PASTEBOARD_MODULE_SERVICE,
        "params check failed, params is null %{public}d", params == nullptr);
    std::vector<uint8_t> vec(udid.begin(), udid.end());
    for (size_t i = 0; i < MAX_UDID_LENGTH && i < vec.size(); i++) {
        params->udid[i] = vec[i];
    }
    params->udidLen = static_cast<uint32_t>(MAX_UDID_LENGTH);
    return true;
}
#endif

uint32_t SecurityLevel::GetSensitiveLevel()
{
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
#ifdef PB_DATACLASSIFICATION_ENABLE
    DEVSLQueryParams query;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(InitDEVSLQueryParams(&query, udid), DATA_SEC_LEVEL0, PASTEBOARD_MODULE_SERVICE,
        "init query params failed! udid:%{public}.6s", udid.c_str());

    uint32_t level = DATA_SEC_LEVEL0;
    int32_t result = DATASL_GetHighestSecLevel(&query, &level);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result == DEVSL_SUCCESS, DATA_SEC_LEVEL0, PASTEBOARD_MODULE_SERVICE,
        "get highest level failed(%{public}.6s)! level:%{public}u, error:%{public}d", udid.c_str(), level, result);
    securityLevel_.store(level);
    PASTEBOARD_HILOGI(
        PASTEBOARD_MODULE_SERVICE, "get highest level success(%{public}.6s)! level: %{public}u", udid.c_str(), level);
    return level;
#else
    return 0;
#endif
}
} // namespace OHOS::MiscServices