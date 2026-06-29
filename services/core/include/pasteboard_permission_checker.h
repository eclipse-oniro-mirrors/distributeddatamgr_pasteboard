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

#ifndef PASTEBOARD_PERMISSION_CHECKER_H
#define PASTEBOARD_PERMISSION_CHECKER_H

#include "pasteboard_service.h"

namespace OHOS {
namespace MiscServices {

class PasteboardPermissionChecker {
public:
    explicit PasteboardPermissionChecker(PasteboardService& service);
    ~PasteboardPermissionChecker();
    
    bool VerifyPermission(uint32_t tokenId);
    int32_t IsDataValid(PasteData& pasteData, uint32_t tokenId, int32_t userId);
    bool IsPermissionGranted(const std::string& perm, uint32_t tokenId);
    bool IsSystemAppByFullTokenID(uint64_t tokenId);
    bool IsCopyable(uint32_t tokenId) const;
    
    int32_t GetSdkVersion(uint32_t tokenId);
    bool IsCallerUidValid();
    
    bool CheckMdmShareOption(PasteData& pasteData);
    void UpdateShareOption(PasteData& pasteData);
    
    int32_t SetGlobalShareOption(const std::unordered_map<uint32_t, int32_t>& globalShareOptions);
    int32_t RemoveGlobalShareOption(const std::vector<uint32_t>& tokenIds);
    int32_t GetGlobalShareOption(const std::vector<uint32_t>& tokenIds,
        std::unordered_map<uint32_t, int32_t>& funcResult);
    
    int32_t SetAppShareOptions(int32_t shareOptions);
    int32_t RemoveAppShareOptions();
    
private:
    PasteboardService& service_;
    
    enum GlobalShareOptionSource {
        MDM = 0,
        APP = 1,
    };
    
    struct GlobalShareOption {
        GlobalShareOptionSource source;
        ShareOption shareOption;
    };
    
    ConcurrentMap<uint32_t, GlobalShareOption> globalShareOptions_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_PERMISSION_CHECKER_H