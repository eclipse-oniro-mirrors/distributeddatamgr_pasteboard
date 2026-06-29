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

#include "pasteboard_observer_manager.h"
#include "pasteboard_hilog.h"
#include "permission_utils.h"

namespace OHOS {
namespace MiscServices {

PasteboardObserverManager::PasteboardObserverManager(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardObserverManager constructed.");
}

PasteboardObserverManager::~PasteboardObserverManager()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardObserverManager destructed.");
}

int32_t PasteboardObserverManager::SubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer)
{
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = service_.GetAppInfo(IPCSkeleton::GetCallingTokenID());
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : appInfo.userId;
    service_.SetInputMethodPid(userId, callPid);
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    bool addSucc = false;
    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_LOCAL)) {
        addSucc = AddObserver(userId, observer, observerLocalChangedMap_) || addSucc;
    }

    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_REMOTE)) {
        addSucc = AddObserver(userId, observer, observerRemoteChangedMap_) || addSucc;
    }

    if (isEventType && service_.IsCallerUidValid()) {
        addSucc = AddObserver(userId, observer, observerEventMap_) || addSucc;
    }
    return addSucc ? ERR_OK : static_cast<int32_t>(PasteboardError::ADD_OBSERVER_FAILED);
}

int32_t PasteboardObserverManager::ResubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer)
{
    auto appInfo = service_.GetAppInfo(IPCSkeleton::GetCallingTokenID());
    if (appInfo.tokenType == ATokenTypeEnum::TOKEN_HAP) {
        return SubscribeObserver(type, observer);
    }
    return ERR_OK;
}

int32_t PasteboardObserverManager::UnsubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver>& observer)
{
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = service_.GetAppInfo(IPCSkeleton::GetCallingTokenID());
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : appInfo.userId;
    service_.ClearInputMethodPidByPid(userId, callPid);
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_LOCAL)) {
        RemoveSingleObserver(userId, observer, observerLocalChangedMap_);
    }

    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_REMOTE)) {
        RemoveSingleObserver(userId, observer, observerRemoteChangedMap_);
    }

    if (isEventType && service_.IsCallerUidValid()) {
        RemoveSingleObserver(userId, observer, observerEventMap_);
    }
    return ERR_OK;
}

int32_t PasteboardObserverManager::UnsubscribeAllObserver(PasteboardObserverType type)
{
    service_.ClearInputMethodPid();
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : service_.GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_LOCAL)) {
        RemoveAllObserver(userId, observerLocalChangedMap_);
    }

    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_REMOTE)) {
        RemoveAllObserver(userId, observerRemoteChangedMap_);
    }

    if (isEventType && service_.IsCallerUidValid()) {
        RemoveAllObserver(userId, observerEventMap_);
    }
    return ERR_OK;
}

int32_t PasteboardObserverManager::SubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver>& observer)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(static_cast<uint32_t>(entityType) < static_cast<uint32_t>(EntityType::MAX),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read entityType data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        observer != nullptr, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read observer data");
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
        "start, type=%{public}u, len=%{public}u", static_cast<uint32_t>(entityType), expectedDataLength);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(expectedDataLength <= MAX_RECOGNITION_LENGTH,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "expected data length exceeds limitation");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(service_.VerifyPermission(tokenId),
        static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR), PASTEBOARD_MODULE_SERVICE,
        "check permission failed");
    auto callingPid = IPCSkeleton::GetCallingPid();
    bool overLimit = false;
    entityObserverMap_.Compute(
        callingPid, [entityType, expectedDataLength, tokenId, &observer, &overLimit](const auto&, auto& observerList) {
            auto it = std::find_if(observerList.begin(), observerList.end(),
                [entityType, expectedDataLength](const EntityObserverInfo& observer) {
                    return observer.entityType == entityType && observer.expectedDataLength == expectedDataLength;
                });
            if (it != observerList.end()) {
                it->tokenId = tokenId;
                it->observer = observer;
                return true;
            }
            if (observerList.size() >= MAX_ENTITY_OBSERVER_COUNT) {
                overLimit = true;
                return true;
            }
            observerList.emplace_back(entityType, expectedDataLength, tokenId, observer);
            return true;
        });
    if (overLimit) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "entity observer count over limit");
        return static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION);
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "subscribe entityObserver finished");
    return ERR_OK;
}

int32_t PasteboardObserverManager::UnsubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver>& observer)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(static_cast<uint32_t>(entityType) < static_cast<uint32_t>(EntityType::MAX),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read entityType data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        observer != nullptr, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read observer data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(expectedDataLength <= MAX_RECOGNITION_LENGTH,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "expected data length exceeds limitation");
    auto callingPid = IPCSkeleton::GetCallingPid();
    auto result =
        entityObserverMap_.ComputeIfPresent(callingPid, [entityType, expectedDataLength](auto, auto& observerList) {
            auto it = std::find_if(observerList.begin(), observerList.end(),
                [entityType, expectedDataLength](const EntityObserverInfo& observer) {
                    return observer.entityType == entityType && observer.expectedDataLength == expectedDataLength;
                });
            if (it == observerList.end()) {
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
                    "Failed to unsubscribe, observer not found, type is %{public}u, length is %{public}u.",
                    static_cast<uint32_t>(entityType), expectedDataLength);
                return true;
            }
            observerList.erase(it);
            if (observerList.empty()) {
                return false;
            }
            return true;
        });
    return ERR_OK;
}

void PasteboardObserverManager::UnsubscribeAllEntityObserver()
{
    entityObserverMap_.Clear();
}

void PasteboardObserverManager::NotifyEntityObservers(std::string& entity, EntityType entityType, uint32_t dataLength)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "entityType=%{public}u, dataLength=%{public}u",
        static_cast<uint32_t>(entityType), dataLength);
    entityObserverMap_.ForEach([this, &entity, entityType, dataLength](const auto& key, auto& value) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pid=%{public}u, listSize=%{public}zu", key, value.size());
        for (auto entityObserver : value) {
            if (entityType == entityObserver.entityType && dataLength <= entityObserver.expectedDataLength &&
                service_.VerifyPermission(entityObserver.tokenId)) {
                entityObserver.observer->OnRecognitionEvent(entityType, entity);
            }
        }
        return false;
    });
}

int32_t PasteboardObserverManager::SubscribeDisposableObserver(
    const sptr<IPasteboardDisposableObserver>& observer, int32_t targetWindowId, DisposableType type, uint32_t maxLength)
{
    constexpr pid_t SELECTION_SERVICE_UID = 1080;
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uid == SELECTION_SERVICE_UID,
        static_cast<int32_t>(PasteboardError::NOT_SUPPORT), PASTEBOARD_MODULE_SERVICE, "not support");

    pid_t pid = IPCSkeleton::GetCallingPid();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    DisposableInfo info(pid, tokenId, targetWindowId, type, maxLength, observer);
    int32_t ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "add observer info failed, ret=%{public}d", ret);
    return ERR_OK;
}

void PasteboardObserverManager::NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    auto [hasPid, pid] = service_.imeMap_.Find(userId);
    if (hasPid && service_.IsNeedThaw(status)) {
        service_.ThawInputMethod(pid);
    }
    std::thread thread([this, bundleName, userId, status]() {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto& observers : observerLocalChangedMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerLocalChangedMap_.second is nullptr");
                continue;
            }
            for (const auto& observer : *(observers.second)) {
                if (status != PasteboardEventStatus::PASTEBOARD_READ && userId == observers.first.first) {
                    observer->OnPasteboardChanged();
                }
            }
        }
        IPasteboardChangedObserver::PasteboardChangedEvent event;
        event.status = static_cast<int32_t>(status);
        event.userId = userId;
        event.bundleName = bundleName;
        for (auto& observers : observerEventMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerEventMap_.second is nullptr");
                continue;
            }
            for (const auto& observer : *(observers.second)) {
                observer->OnPasteboardEvent(event);
            }
        }
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "NotifyObservers");
    thread.detach();
}

void PasteboardObserverManager::RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap& observerMap)
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callObserverKey = std::make_pair(userId, pid);
    auto it = observerMap.find(callObserverKey);
    if (it == observerMap.end()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
            "RemoveObserverByPid: no observer found for userId=%{public}d, pid=%{public}d", userId, pid);
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "RemoveObserverByPid: removing observer for userId=%{public}d, pid=%{public}d", userId, pid);
    observerMap.erase(callObserverKey);
}
{
    auto localObserverSize = GetObserversSize(userId, pid, observerLocalChangedMap_);
    auto remoteObserverSize = GetObserversSize(userId, pid, observerRemoteChangedMap_);
    auto eventObserverSize = GetObserversSize(COMMON_USERID, pid, observerEventMap_);
    return localObserverSize + remoteObserverSize + eventObserverSize;
}

uint32_t PasteboardObserverManager::GetObserversSize(int32_t userId, pid_t pid, ObserverMap& observerMap)
{
    auto countKey = std::make_pair(userId, pid);
    auto it = observerMap.find(countKey);
    if (it != observerMap.end()) {
        return it->second->size();
    }
    return 0;
}

bool PasteboardObserverManager::AddObserver(
    int32_t userId, const sptr<IPasteboardChangedObserver>& observer, ObserverMap& observerMap)
{
    if (observer == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer null.");
        return false;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callPid = IPCSkeleton::GetCallingPid();
    auto callObserverKey = std::make_pair(userId, callPid);
    auto it = observerMap.find(callObserverKey);
    std::shared_ptr<std::set<sptr<IPasteboardChangedObserver>, classcomp>> observers;
    if (it != observerMap.end()) {
        observers = it->second;
    } else {
        observers = std::make_shared<std::set<sptr<IPasteboardChangedObserver>, classcomp>>();
        observerMap.insert(std::make_pair(callObserverKey, observers));
    }
    auto allObserverCount = GetAllObserversSize(userId, callPid);
    if (allObserverCount >= MAX_OBSERVER_COUNT) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer count over limit. callPid:%{public}d", callPid);
        return false;
    }
    observers->insert(observer);
    RADAR_REPORT(DFX_OBSERVER, DFX_ADD_OBSERVER, DFX_SUCCESS);
    PASTEBOARD_HILOGI(
        PASTEBOARD_MODULE_SERVICE, "observers->size = %{public}u.", static_cast<unsigned int>(observers->size()));
    return true;
}

void PasteboardObserverManager::RemoveSingleObserver(
    int32_t userId, const sptr<IPasteboardChangedObserver>& observer, ObserverMap& observerMap)
{
    if (observer == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer null.");
        return;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callPid = IPCSkeleton::GetCallingPid();
    auto callObserverKey = std::make_pair(userId, callPid);
    auto it = observerMap.find(callObserverKey);
    if (it == observerMap.end()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id not found userId is %{public}d", userId);
        return;
    }
    auto observers = it->second;
    PASTEBOARD_HILOGD(
        PASTEBOARD_MODULE_SERVICE, "observers size: %{public}u.", static_cast<unsigned int>(observers->size()));
    auto eraseNum = observers->erase(observer);
    RADAR_REPORT(DFX_OBSERVER, DFX_REMOVE_SINGLE_OBSERVER, DFX_SUCCESS);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "observers size = %{public}u, eraseNum = %{public}zu",
        static_cast<unsigned int>(observers->size()), eraseNum);
}

void PasteboardObserverManager::RemoveAllObserver(int32_t userId, ObserverMap& observerMap)
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto it = observerMap.begin(); it != observerMap.end();) {
        if (it->first.first == userId) {
            it = observerMap.erase(it);
        } else {
            ++it;
        }
    }
    RADAR_REPORT(DFX_OBSERVER, DFX_REMOVE_ALL_OBSERVER, DFX_SUCCESS);
}

void PasteboardObserverManager::RemoveAllObserversByPid(int32_t userId, pid_t pid)
{
    RemoveObserverByPid(userId, pid, observerLocalChangedMap_);
    RemoveObserverByPid(userId, pid, observerRemoteChangedMap_);
    RemoveObserverByPid(COMMON_USERID, pid, observerEventMap_);
}

void PasteboardObserverManager::RemoveEntityObserverByPid(pid_t pid)
{
    entityObserverMap_.Erase(pid);
}

bool PasteboardObserverManager::HasEntityObservers()
{
    return entityObserverMap_.Size() != 0;
}

bool PasteboardObserverManager::HasEventObservers()
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    return observerEventMap_.size() != 0;
}

void PasteboardObserverManager::NotifyRemoteObservers()
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto& observers : observerRemoteChangedMap_) {
        if (observers.second == nullptr) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerRemoteChangedMap_.second is nullptr");
            continue;
        }
        for (const auto& observer : *(observers.second)) {
            observer->OnPasteboardChanged();
        }
    }
}

} // namespace MiscServices
} // namespace OHOS