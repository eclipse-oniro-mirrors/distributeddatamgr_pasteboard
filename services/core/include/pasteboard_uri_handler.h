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

#ifndef PASTEBOARD_URI_HANDLER_H
#define PASTEBOARD_URI_HANDLER_H

#include "pasteboard_service.h"

namespace OHOS {
namespace MiscServices {

class PasteboardUriHandler {
public:
    explicit PasteboardUriHandler(PasteboardService& service);
    ~PasteboardUriHandler();
    
    int32_t CheckAndGrantRemoteUri(PasteData& data, const AppInfo& appInfo, const std::string& pasteId,
        std::shared_ptr<BlockObject<int32_t>> pasteBlock);
    int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>>& grantUris,
        uint32_t targetTokenId, bool isRemoteData);
    int32_t GrantPermission(const std::vector<Uri>& grantUris, uint32_t permFlag,
        bool isRemoteData, uint32_t targetTokenId);
    
    std::map<uint32_t, std::vector<Uri>> CheckUriPermission(
        PasteData& data, const std::pair<std::string, int32_t>& targetBundleAppIndex);
    void RemoveInvalidRemoteUri(std::vector<Uri>& grantUris);
    void GenerateDistributedUri(PasteData& data);
    bool IsBundleOwnUriPermission(const std::string& bundleName, Uri& uri);
    
    void ClearUriOnUninstall(int32_t userId, int32_t tokenId);
    void ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData);
    bool HasRemoteUri(std::shared_ptr<PasteData> pasteData);
    
private:
    PasteboardService& service_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_URI_HANDLER_H