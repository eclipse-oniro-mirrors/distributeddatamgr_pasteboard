/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_ENTITY_RECOGNITION_MANAGER_H
#define PASTEBOARD_ENTITY_RECOGNITION_MANAGER_H

#include <mutex>
#include <string>
#include <vector>

#include "common/concurrent_map.h"
#include "ientity_recognition_observer.h"
#include "pasteboard_service.h"

namespace OHOS {
namespace MiscServices {

struct EntityObserverInfo {
    EntityType entityType;
    uint32_t expectedDataLength;
    uint32_t tokenId;
    sptr<IEntityRecognitionObserver> observer;
    EntityObserverInfo(EntityType entityType_, uint32_t expectedDataLength_, uint32_t tokenId_,
        sptr<IEntityRecognitionObserver> observer_) : entityType(entityType_),
        expectedDataLength(expectedDataLength_), tokenId(tokenId_), observer(observer_) { }
};

class PasteboardEntityRecognitionManager {
public:
    explicit PasteboardEntityRecognitionManager(PasteboardService &service);
    ~PasteboardEntityRecognitionManager();

    int32_t ExtractEntity(const std::string &entity, std::string &location);
    int32_t GetAllEntryPlainText(uint32_t dataId, uint32_t recordId,
        std::vector<std::shared_ptr<PasteDataEntry>> &entries, std::string &primaryText);
    std::string GetAllPrimaryText(const PasteData &pasteData);
    void RecognizePasteData(PasteData &pasteData);
    void OnRecognizePasteData(const std::string &primaryText);
    void OnRecognizePasteDataInner(const std::string &primaryText, void *nulHandle);
    void NotifyEntityObservers(std::string &entity, EntityType entityType, uint32_t dataLength);
    int32_t SubscribeEntityObserver(EntityType entityType, uint32_t expectedDataLength,
        const sptr<IEntityRecognitionObserver> &observer);
    int32_t UnsubscribeEntityObserver(EntityType entityType, uint32_t expectedDataLength,
        const sptr<IEntityRecognitionObserver> &observer);
    void UnsubscribeAllEntityObserver();

private:
    PasteboardService &service_;
    ConcurrentMap<pid_t, std::vector<EntityObserverInfo>> entityObserverMap_;
    std::mutex entityRecognizeMutex_;
};

} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_ENTITY_RECOGNITION_MANAGER_H