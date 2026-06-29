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

#ifndef PASTEBOARD_DISTRIBUTED_MANAGER_H
#define PASTEBOARD_DISTRIBUTED_MANAGER_H

#include "pasteboard_service.h"

namespace OHOS {
namespace MiscServices {

class PasteboardDistributedManager {
public:
    explicit PasteboardDistributedManager(PasteboardService& service);
    ~PasteboardDistributedManager();
    
    std::pair<std::shared_ptr<PasteData>, PasteDateResult> GetDistributedData(const Event& event, int32_t user);
    int32_t GetDistributedDelayData(const Event& evt, uint8_t version, std::vector<uint8_t>& rawData);
    int32_t GetDistributedDelayEntry(const Event& evt, uint32_t recordId, const std::string& utdId,
        std::vector<uint8_t>& rawData);
    
    bool SetDistributedData(int32_t user, PasteData& data);
    bool SetCurrentDistributedData(PasteData& data, Event event);
    bool IsDisallowDistributed();
    
    void CleanDistributedData(int32_t user);
    void CloseDistributedStore(int32_t user, bool isNeedClear);
    void ChangeStoreStatus(int32_t userId);
    bool IsValidCurrentEvent();
    bool IsConstraintEnabled(int32_t user);
    
    int32_t ProcessDistributedDelayUri(int32_t userId, PasteData& data, PasteDataEntry& entry,
        std::vector<uint8_t>& rawData);
    int32_t ProcessDistributedDelayHtml(PasteData& data, PasteDataEntry& entry, std::vector<uint8_t>& rawData);
    int32_t ProcessDistributedDelayEntry(PasteDataEntry& entry, std::vector<uint8_t>& rawData);
    
    std::shared_ptr<ClipPlugin> GetClipPlugin();
    
private:
    PasteboardService& service_;
    std::shared_ptr<ClipPlugin> clipPlugin_;
    DistributedModuleConfig moduleConfig_;
    
    bool SetCurrentData();
    void InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin);
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_DISTRIBUTED_MANAGER_H