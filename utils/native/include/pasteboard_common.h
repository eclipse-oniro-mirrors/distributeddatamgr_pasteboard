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

#ifndef PASTEBOARD_COMMON_H
#define PASTEBOARD_COMMON_H

#include "bundle_mgr_proxy.h"
#include "singleton.h"

namespace OHOS {
namespace MiscServices {
class PasteBoardCommon : public Singleton<PasteBoardCommon> {
public:
    PasteBoardCommon() = default;
    ~PasteBoardCommon() = default;

    static inline bool IsPasteboardService()
    {
        constexpr uid_t PASTEBOARD_SERVICE_UID = 3816;
        return getuid() == PASTEBOARD_SERVICE_UID;
    }
    static bool IsValidMimeType(const std::string &mimeType);
    static std::string GetAnonymousString(const std::string &str);
    static sptr<AppExecFwk::IBundleMgr> GetAppBundleManager(void);
    int32_t GetApiTargetVersionForSelf(void);
    static int32_t GetDirByBundleNameAndAppIndex(const std::string &bundleName, int32_t appIndex,
        std::string &dataDir);
    static std::string GetDirByAuthority(const std::pair<std::string, int32_t> &authority);

private:
    int32_t apiTargetVersion_ = 0;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_COMMON_H
