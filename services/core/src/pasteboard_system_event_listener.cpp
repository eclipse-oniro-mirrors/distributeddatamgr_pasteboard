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

#include "pasteboard_system_event_listener.h"
#include "pasteboard_hilog.h"
#include "pasteboard_common_utils.h"
#include "device/distributed_module_config.h"
#include "dev_profile.h"
#include "dm_adapter.h"
#include "memory/MemMgrClient.h"
#include "system_ability_definition.h"
#include "common_event_support.h"
#include "os_account_manager.h"

namespace OHOS {
namespace MiscServices {

namespace {
constexpr int32_t WIFI_DISABLED = 0;
constexpr int32_t LISTENING_SERVICE[] = {
    DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID,
    MEMORY_MANAGER_SA_ID,
    DISTRIBUTED_DEVICE_PROFILE_SA_ID,
};
}

PasteboardSystemEventListener::PasteboardSystemEventListener(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardSystemEventListener constructed.");
}

PasteboardSystemEventListener::~PasteboardSystemEventListener()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardSystemEventListener destructed.");
}

void PasteboardSystemEventListener::AddSysAbilityListener()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "begin.");
    for (uint32_t i = 0; i < sizeof(LISTENING_SERVICE) / sizeof(LISTENING_SERVICE[0]); i++) {
        auto ret = service_.AddSystemAbilityListener(LISTENING_SERVICE[i]);
        if (ret) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Add listener success, serviceId = %{public}d.",
                LISTENING_SERVICE[i]);
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Add listener failed, serviceId = %{public}d.",
                LISTENING_SERVICE[i]);
        }
    }
}

void PasteboardSystemEventListener::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    (void)deviceId;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "systemAbilityId=%{public}d", systemAbilityId);

    switch (systemAbilityId) {
        case DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID:
            OnAddDeviceManager();
            break;
        case MEMORY_MANAGER_SA_ID:
            OnAddMemoryManager();
            break;
        case DISTRIBUTED_DEVICE_PROFILE_SA_ID:
            OnAddDeviceProfile();
            break;
        default:
            break;
    }
}

void PasteboardSystemEventListener::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    (void)deviceId;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "systemAbilityId=%{public}d", systemAbilityId);

    switch (systemAbilityId) {
        case DISTRIBUTED_DEVICE_PROFILE_SA_ID:
            OnRemoveDeviceProfile();
            break;
        default:
            break;
    }
}

void PasteboardSystemEventListener::OnAddDeviceManager()
{
    DMAdapter::GetInstance().Initialize();
}

void PasteboardSystemEventListener::OnAddMemoryManager()
{
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 1, PASTEBOARD_SERVICE_ID);
    service_.SetCriticalTimer();
}

void PasteboardSystemEventListener::OnAddDeviceProfile()
{
    DevProfile::GetInstance().SendSubscribeInfos();
}

void PasteboardSystemEventListener::OnRemoveDeviceProfile()
{
    DevProfile::GetInstance().ClearDeviceProfileService();
}

void PasteboardSystemEventListener::HandleWifiOffAndClearDistributedEvent(int32_t userId)
{
    bool isdeviceCollabSwitch = service_.switch_.GetDeviceCollabSwitch(userId);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(!isdeviceCollabSwitch, PASTEBOARD_MODULE_SERVICE,
        "wifi off but DeviceCollabSwitch is on");
    PASTEBOARD_CHECK_AND_RETURN_LOGD(service_.distributedManager_->IsValidCurrentEvent(), PASTEBOARD_MODULE_SERVICE, 
        "wifi off but no valid event");
    service_.distributedManager_->CleanDistributedData(userId);
}

void PasteboardSystemEventListener::CommonEventSubscriber()
{
    if (commonEventSubscriber_ != nullptr) {
        return;
    }
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_POWER_STATE);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    commonEventSubscriber_ = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, &service_);
    EventFwk::CommonEventManager::SubscribeCommonEvent(commonEventSubscriber_);
}

void PasteboardSystemEventListener::AccountStateSubscriber()
{
    if (accountStateSubscriber_ != nullptr) {
        return;
    }
    std::set<AccountSA::OsAccountState> states = { AccountSA::OsAccountState::STOPPING,
        AccountSA::OsAccountState::CREATED, AccountSA::OsAccountState::SWITCHING,
        AccountSA::OsAccountState::SWITCHED, AccountSA::OsAccountState::UNLOCKED,
        AccountSA::OsAccountState::STOPPED, AccountSA::OsAccountState::REMOVED };
    AccountSA::OsAccountSubscribeInfo subscribeInfo(states, true);
    accountStateSubscriber_ = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, &service_);
    AccountSA::OsAccountManager::SubscribeOsAccount(accountStateSubscriber_);
}

#ifdef PB_COCKPIT_PLATFORM_ENABLE
void PasteboardSystemEventListener::DistributedAccountSubscriber()
{
    if (distributedAccountSubscriber_ != nullptr) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "distributedAccountSubscriber_ already exists");
        return;
    }
    
    std::set<AccountSA::DistributedAccountSpaceEventType> types = {
        AccountSA::DistributedAccountSpaceEventType::SWITCHING
    };
    
    distributedAccountSubscriber_ = std::make_shared<PasteboardDistributedAccountSubscriber>(&service_);
    
    ErrCode err = AccountSA::OsAccountManager::SubscribeDistributedAccountSpaceEvents(
        types, distributedAccountSubscriber_);
    
    if (err != AccountSA::ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, 
            "SubscribeDistributedAccountSpaceEvents failed: %{public}d", err);
        distributedAccountSubscriber_ = nullptr;
        return;
    }
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, 
        "DistributedAccountSubscriber initialized, subscribed SWITCHING event");
}
#endif

void PasteboardSystemEventListener::PasteboardEventSubscriber()
{
    EventCenter::GetInstance().Subscribe(PasteboardEvent::DISCONNECT, [this](const OHOS::MiscServices::Event& event) {
        auto& evt = static_cast<const PasteboardEvent&>(event);
        auto networkId = evt.GetNetworkId();
        if (networkId.empty()) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
            return;
        }
        service_.p2pManager_->RemoveP2PLinkByNetworkId(networkId);
    });
}

void PasteboardSystemEventListener::InitScreenStatus()
{
#ifdef PB_SCREENLOCK_MGR_ENABLE
    auto screenLockManager = OHOS::ScreenLock::ScreenLockManager::GetInstance();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(screenLockManager != nullptr, PASTEBOARD_MODULE_SERVICE,
        "ScreenLockManager instance is null.");
    auto foregroundUsers = service_.ResolveForegroundUsers();
    for (const auto& ctx : foregroundUsers) {
        if (!ctx.isValid) {
            continue;
        }
        bool isLocked = false;
        auto ret = screenLockManager->IsLockedWithUserId(ctx.userId, isLocked);
        if (ret == ERR_OK) {
            service_.screenStatusMap_.InsertOrAssign(ctx.userId,
                isLocked ? ScreenEvent::ScreenLocked : ScreenEvent::ScreenUnlocked);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen status userId=%{public}d is %{public}d",
                ctx.userId, isLocked ? ScreenEvent::ScreenLocked : ScreenEvent::ScreenUnlocked);
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "IsLockedWithUserId failed, userId=%{public}d, ret=%{public}d", ctx.userId, ret);
        }
    }
#else
    PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "PB_SCREENLOCK_MGR_ENABLE not defined");
    return;
#endif
}

ScreenEvent PasteboardSystemEventListener::GetScreenStatus(int32_t userId)
{
    auto [found, status] = service_.screenStatusMap_.Find(userId);
    if (found) {
        return status;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "screen status not found for userId=%{public}d", userId);
    return ScreenEvent::Default;
}

void PasteboardSystemEventListener::OnConfigChange(bool isOn)
{
    std::thread thread([=]() {
        OnConfigChangeInner(isOn);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "OnConfigChange");
    thread.detach();
}

void PasteboardSystemEventListener::OnConfigChangeInner(bool isOn)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConfigChange isOn: %{public}d.", isOn);
    if (!isOn) {
        service_.p2pManager_->ClearAllP2PLinks();
    }
    std::lock_guard<decltype(service_.mutex)> lockGuard(service_.mutex);
    if (!isOn) {
        PASTEBOARD_CHECK_AND_RETURN_LOGE(service_.clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        int32_t userId = service_.ResolveMainDisplayUserId();
        PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE,
            "main display user invalid");
        service_.clipPlugin_->Close(userId);
        service_.clipPlugin_ = nullptr;
        return;
    }
    service_.SetCriticalTimer();
    auto isSupported = service_.securityLevel_.IsSupportedDistributed(true);
    if (!isSupported) {
        return;
    }
    if (service_.clipPlugin_ != nullptr) {
        return;
    }
    service_.SubscribeKeyboardEvent();
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin* plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    service_.clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    service_.InitPlugin(service_.clipPlugin_);
}

} // namespace MiscServices
} // namespace OHOS