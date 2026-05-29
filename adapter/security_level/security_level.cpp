/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "dev_slinfo_mgr.h"
#include "device/dm_adapter.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
SecurityLevel::SecurityLevel() : securityLevel_(DATA_SEC_LEVEL0)
{
}

bool SecurityLevel::IsSupportedDistributed(bool needLog)
{
    uint32_t level = GetDeviceSecurityLevel();
    if (level >= DATA_SEC_LEVEL3) {
        return true;
    }
    if (needLog) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "device sec level is %{public}u less than 3.", level);
    }
    return false;
}

uint32_t SecurityLevel::GetDeviceSecurityLevel()
{
    uint32_t level = securityLevel_.load();
    if (level > DATA_SEC_LEVEL0) {
        return level;
    }
    return GetSensitiveLevel();
}

bool InitDEVSLQueryParams(DEVSLQueryParams *params, const std::string &udid)
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

uint32_t SecurityLevel::GetSensitiveLevel()
{
    std::lock_guard lock(mutex_);

    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    DEVSLQueryParams query;
    bool initRet = InitDEVSLQueryParams(&query, udid);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(initRet, DATA_SEC_LEVEL0, PASTEBOARD_MODULE_SERVICE,
        "init query params failed! udid:%{public}.6s", udid.c_str());

    auto status = DATASL_OnStart();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "datasl on start ret:%{public}d", status);

    uint32_t level = DATA_SEC_LEVEL0;
    int32_t result = DATASL_GetHighestSecLevel(&query, &level);
    if (result == DEVSL_SUCCESS) {
        securityLevel_.store(level);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "get highest level success(%{public}.6s)! level: %{public}u", udid.c_str(), level);
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "get highest level failed(%{public}.6s)! level:%{public}u, error:%{public}d", udid.c_str(), level, result);
    }

    DATASL_OnStop();
    return level;
}
} // namespace OHOS::MiscServices