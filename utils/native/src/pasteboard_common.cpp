/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "pasteboard_common.h"

#include <if_system_ability_manager.h>
#include <iservice_registry.h>
#include <system_ability_definition.h>
#include <unistd.h>

#include "pasteboard_hilog.h"
#include "pasteboard_error.h"

namespace OHOS {
namespace MiscServices {
constexpr int32_t MIMETYPE_MAX_SIZE = 1024;
sptr<AppExecFwk::IBundleMgr> PasteBoardCommon::GetAppBundleManager(void)
{
    auto sysAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        sysAbilityMgr != nullptr, nullptr, PASTEBOARD_MODULE_SERVICE, "Failed to get system ability manager.");
    auto remoteObject = sysAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        remoteObject != nullptr, nullptr, PASTEBOARD_MODULE_SERVICE, "Failed to get bundle mgr service.");
    return iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
}

int32_t PasteBoardCommon::GetApiTargetVersionForSelf(void)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(
        apiTargetVersion_ <= 0, apiTargetVersion_, PASTEBOARD_MODULE_SERVICE, "apiTargetVersion valid.");
    static constexpr int32_t API_VERSION_MOD = 1000;
    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = GetAppBundleManager();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(
        bundleMgrProxy != nullptr, 0, PASTEBOARD_MODULE_SERVICE, "Failed to get bundle manager proxy.");
    AppExecFwk::BundleInfo bundleInfo;
    auto flags = AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_APPLICATION;
    auto ret = bundleMgrProxy->GetBundleInfoForSelf(static_cast<int32_t>(flags), bundleInfo);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(
        ret == ERR_OK, 0, PASTEBOARD_MODULE_SERVICE, "GetBundleInfoForSelf: bundleName get failed %{public}d.", ret);
    int32_t targetApiVersion = bundleInfo.applicationInfo.apiTargetVersion % API_VERSION_MOD;
    apiTargetVersion_ = targetApiVersion;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "Got target API version %{public}d.", targetApiVersion);
    return targetApiVersion;
}

std::string PasteBoardCommon::GetAnonymousString(const std::string &str)
{
    const int32_t ANONYMOUS_LEN_LIMIT = 8;
    const int32_t TWO_TIMES = 2;
    if (str.length() <= ANONYMOUS_LEN_LIMIT) {
        return str;
    }
    int32_t printLen = ANONYMOUS_LEN_LIMIT / TWO_TIMES;
    return str.substr(0, printLen) + "**" + str.substr(str.length() - printLen);
}

int32_t PasteBoardCommon::GetDirByBundleNameAndAppIndex(const std::string &bundleName, int32_t appIndex,
    std::string &dataDir)
{
    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = PasteBoardCommon::GetAppBundleManager();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(bundleMgrProxy != nullptr,
        static_cast<int32_t>(PasteboardError::GET_BUNDLE_MGR_FAILED), PASTEBOARD_MODULE_SERVICE,
        "Get bunlde manager failed, bundleName:%{public}s, appIndex:%{public}d",
        bundleName.c_str(), appIndex);
    auto ret = bundleMgrProxy->GetDirByBundleNameAndAppIndex(bundleName, appIndex, dataDir);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == ERR_OK,
        static_cast<int32_t>(ret), PASTEBOARD_MODULE_SERVICE,
        "Get dir failed, bundleName:%{public}s, appIndex:%{public}d, ret:%{public}d",
        bundleName.c_str(), appIndex, ret);
    return ERR_OK;
}

std::string PasteBoardCommon::GetDirByAuthority(const std::pair<std::string, int32_t> &authority)
{
    std::string bundleIndex;
    int32_t ret = GetDirByBundleNameAndAppIndex(authority.first, authority.second, bundleIndex);
    return ret == ERR_OK ? bundleIndex : authority.first;
}

bool PasteBoardCommon::IsValidMimeType(const std::string &mimeType)
{
    const bool isNonEmpty = !mimeType.empty();
    const bool withinSizeLimit = mimeType.size() <= MIMETYPE_MAX_SIZE;
    return isNonEmpty && withinSizeLimit;
}

int32_t PasteBoardCommon::Stat(const std::string &path, struct stat *buf)
{
    return stat(path.c_str(), buf);
}
} // namespace MiscServices
} // namespace OHOS
