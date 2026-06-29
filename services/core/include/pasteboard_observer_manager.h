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

#ifndef PASTEBOARD_OBSERVER_MANAGER_H
#define PASTEBOARD_OBSERVER_MANAGER_H

#include "pasteboard_service.h"
#include "ientity_recognition_observer.h"

namespace OHOS {
namespace MiscServices {

class PasteboardObserverManager {
public:
    explicit PasteboardObserverManager(PasteboardService& service);
    ~PasteboardObserverManager();
    
    int32_t SubscribeObserver(PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer);
    int32_t ResubscribeObserver(PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer);
    int32_t UnsubscribeObserver(PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer);
    int32_t UnsubscribeAllObserver(PasteboardObserverType type);
    
    int32_t SubscribeEntityObserver(EntityType entityType, uint32_t expectedDataLength,
        const sptr<IEntityRecognitionObserver>& observer);
    int32_t UnsubscribeEntityObserver(EntityType entityType, uint32_t expectedDataLength,
        const sptr<IEntityRecognitionObserver>& observer);
    void UnsubscribeAllEntityObserver();
    void NotifyEntityObservers(std::string& entity, EntityType entityType, uint32_t dataLength);
    
    int32_t SubscribeDisposableObserver(const sptr<IPasteboardDisposableObserver>& observer,
        int32_t targetWindowId, DisposableType type, uint32_t maxLength);
    
    void NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status);
    void NotifyRemoteObservers();
    void RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap& observerMap);
    
    void RemoveAllObserversByPid(int32_t userId, pid_t pid);
    void RemoveEntityObserverByPid(pid_t pid);
    
    bool HasEntityObservers();
    bool HasEventObservers();
    
    uint32_t GetObserversSize(int32_t userId, pid_t pid, ObserverMap& observerMap);
    uint32_t GetAllObserversSize(int32_t userId, pid_t pid);
    
private:
    PasteboardService& service_;
    std::mutex observerMutex_;
    ObserverMap observerLocalChangedMap_;
    ObserverMap observerRemoteChangedMap_;
    ObserverMap observerEventMap_;
    ConcurrentMap<pid_t, std::vector<EntityObserverInfo>> entityObserverMap_;
    
    bool AddObserver(int32_t userId, const sptr<IPasteboardChangedObserver>& observer, ObserverMap& observerMap);
    void RemoveSingleObserver(int32_t userId, const sptr<IPasteboardChangedObserver>& observer,
        ObserverMap& observerMap);
    void RemoveAllObserver(int32_t userId, ObserverMap& observerMap);
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_OBSERVER_MANAGER_H