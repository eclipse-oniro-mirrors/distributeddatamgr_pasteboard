/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_APP_INFO_HELPER_H
#define PASTEBOARD_APP_INFO_HELPER_H

#include "pasteboard_service.h"
#include "bundle_mgr_proxy.h"

namespace OHOS {
namespace MiscServices {

class PasteboardAppInfoHelper {
public:
    explicit PasteboardAppInfoHelper(PasteboardService& service);
    ~PasteboardAppInfoHelper();
    
    AppInfo GetAppInfo(uint32_t tokenId) const;
    void FillHapAppInfo(uint32_t tokenId, AppInfo& info) const;
    void FillNativeAppInfo(uint32_t tokenId, AppInfo& info) const;
    static std::string GetAppBundleName(const AppInfo& appInfo);
    std::string GetAppLabel(uint32_t tokenId);
    
    sptr<OHOS::AppExecFwk::IBundleMgr> GetAppBundleManager();
    
    bool IsFocusedApp(uint32_t tokenId);
    FocusedAppInfo GetFocusedAppInfo() const;
    
    static void SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData& pasteData);
    
private:
    PasteboardService& service_;
    std::mutex bundleMutex_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_APP_INFO_HELPER_H