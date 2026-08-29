/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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
#include "pasteboard_service.h"

#include "accesstoken_kit.h"
#include "common/pasteboard_common_utils.h"
#include "errors.h"
#include "ipasteboard_changed_observer.h"
#include "ipasteboard_disposable_observer.h"
#include "ipc_skeleton.h"
#include "pasteboard_disposable_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "reporter.h"

namespace OHOS {
namespace MiscServices {
namespace {
constexpr int32_t COMMON_USERID = 0;
} // namespace

using namespace Security::AccessToken;

int32_t PasteboardService::SubscribeDisposableObserver(const sptr<IPasteboardDisposableObserver> &observer,
    int32_t targetWindowId, DisposableType type, uint32_t maxLength)
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

int32_t PasteboardService::SubscribeObserver(PasteboardObserverType type,
    const sptr<IPasteboardChangedObserver> &observer)
{
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : appInfo.userId;
    SetInputMethodPid(userId, callPid);
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

    if (isEventType && IsCallerUidValid()) {
        addSucc = AddObserver(userId, observer, observerEventMap_) || addSucc;
    }
    return addSucc ? ERR_OK : static_cast<int32_t>(PasteboardError::ADD_OBSERVER_FAILED);
}

int32_t PasteboardService::ResubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer)
{
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    if (appInfo.tokenType == ATokenTypeEnum::TOKEN_HAP) {
        return SubscribeObserver(type, observer);
    }
    return ERR_OK;
}

int32_t PasteboardService::UnsubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer)
{
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : appInfo.userId;
    ClearInputMethodPidByPid(userId, callPid);
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

    if (isEventType && IsCallerUidValid()) {
        RemoveSingleObserver(userId, observer, observerEventMap_);
    }
    return ERR_OK;
}

int32_t PasteboardService::UnsubscribeAllObserver(PasteboardObserverType type)
{
    ClearInputMethodPid();
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
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

    if (isEventType && IsCallerUidValid()) {
        RemoveAllObserver(userId, observerEventMap_);
    }
    return ERR_OK;
}

uint32_t PasteboardService::GetAllObserversSize(int32_t userId, pid_t pid)
{
    auto localObserverSize = GetObserversSize(userId, pid, observerLocalChangedMap_);
    auto remoteObserverSize = GetObserversSize(userId, pid, observerRemoteChangedMap_);
    auto eventObserverSize = GetObserversSize(COMMON_USERID, pid, observerEventMap_);
    return localObserverSize + remoteObserverSize + eventObserverSize;
}

uint32_t PasteboardService::GetObserversSize(int32_t userId, pid_t pid, ObserverMap &observerMap)
{
    auto countKey = std::make_pair(userId, pid);
    auto it = observerMap.find(countKey);
    if (it != observerMap.end()) {
        return it->second->size();
    }
    return 0;
}

bool PasteboardService::AddObserver(
    int32_t userId, const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap)
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

void PasteboardService::RemoveSingleObserver(
    int32_t userId, const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap)
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

void PasteboardService::RemoveAllObserver(int32_t userId, ObserverMap &observerMap)
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

void PasteboardService::NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    auto [hasPid, pid] = imeMap_.Find(userId);
    if (hasPid && IsNeedThaw(status)) {
        ThawInputMethod(pid);
    }
    std::thread thread([this, bundleName, userId, status]() {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto &observers : observerLocalChangedMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerLocalChangedMap_.second is nullptr");
                continue;
            }
            for (const auto &observer : *(observers.second)) {
                if (status != PasteboardEventStatus::PASTEBOARD_READ && userId == observers.first.first) {
                    observer->OnPasteboardChanged();
                }
            }
        }
        IPasteboardChangedObserver::PasteboardChangedEvent event;
        event.status = static_cast<int32_t>(status);
        event.userId = userId;
        event.bundleName = bundleName;
        for (auto &observers : observerEventMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerEventMap_.second is nullptr");
                continue;
            }
            for (const auto &observer : *(observers.second)) {
                observer->OnPasteboardEvent(event);
            }
        }
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "NotifyObservers");
    thread.detach();
}

void PasteboardService::RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap &observerMap)
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

int32_t PasteboardService::AppExit(pid_t pid, int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid %{public}d exit, userId %{public}d.", pid, userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE, "invalid userId");
    RemoveObserverByPid(userId, pid, observerLocalChangedMap_);
    RemoveObserverByPid(userId, pid, observerRemoteChangedMap_);
    RemoveObserverByPid(COMMON_USERID, pid, observerEventMap_);
    entityObserverMap_.Erase(pid);
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);
    ClearInputMethodPidByPid(userId, pid);
    std::vector<std::string> networkIds;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.EraseIf([pid, &networkIds, this](auto &networkId, auto &pidMap) {
            pidMap.EraseIf([pid, this, networkId](const auto &key, const auto &value) {
                if (value.callPid == pid) {
                    PasteStart(networkId);
                    return true;
                }
                return false;
            });
            if (pidMap.Empty()) {
                networkIds.emplace_back(networkId);
                return true;
            }
            return false;
        });
    }
    for (const auto &id : networkIds) {
        CloseP2PLink(id);
    }
    bool isExist = clients_.ComputeIfPresent(pid, [pid](auto, auto &value) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "find client death recipient succeed, pid=%{public}d", pid);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(value.first != nullptr && value.second != nullptr, false,
            PASTEBOARD_MODULE_SERVICE, "client death recipient is null");
        value.first->RemoveDeathRecipient(value.second);
        return false;
    });
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(isExist, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "find client death recipient failed, pid=%{public}d", pid);
    return ERR_OK;
}

void PasteboardService::PasteboardDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    (void)remote;
    service_.AppExit(pid_, userId_);
}

int32_t PasteboardService::RegisterClientDeathObserver(const sptr<IRemoteObject> &observer)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    int32_t userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE, "invalid userId");
    sptr<PasteboardDeathRecipient> deathRecipient = sptr<PasteboardDeathRecipient>::MakeSptr(*this, pid, userId);
    observer->AddDeathRecipient(deathRecipient);
    clients_.InsertOrAssign(pid, std::make_pair(observer, deathRecipient));
    return ERR_OK;
}

int32_t PasteboardService::DetachPasteboard()
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    return AppExit(pid, GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId);
}
} // namespace MiscServices
} // namespace OHOS
