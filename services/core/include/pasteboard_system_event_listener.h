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

#ifndef PASTEBOARD_SYSTEM_EVENT_LISTENER_H
#define PASTEBOARD_SYSTEM_EVENT_LISTENER_H

#include "pasteboard_service.h"
#include "pasteboard_common_event_subscriber.h"
#include "pasteboard_account_state_subscriber.h"
#ifdef PB_COCKPIT_PLATFORM_ENABLE
#include "pasteboard_distributed_account_subscriber.h"
#endif

namespace OHOS {
namespace MiscServices {

class PasteboardSystemEventListener {
public:
    explicit PasteboardSystemEventListener(PasteboardService& service);
    ~PasteboardSystemEventListener();
    
    void AddSysAbilityListener();
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId);
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId);
    
    void OnAddDeviceManager();
    void OnAddMemoryManager();
    void OnAddDeviceProfile();
    void OnRemoveDeviceProfile();
    
    void CommonEventSubscriber();
    void AccountStateSubscriber();
    void PasteboardEventSubscriber();
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    void DistributedAccountSubscriber();
#endif
    
    void InitScreenStatus();
    ScreenEvent GetScreenStatus(int32_t userId);
    
    void OnConfigChange(bool isOn);
    void OnConfigChangeInner(bool isOn);
    
    void HandleWifiOffAndClearDistributedEvent(int32_t userId);
    
private:
    PasteboardService& service_;
    std::shared_ptr<PasteBoardCommonEventSubscriber> commonEventSubscriber_;
    std::shared_ptr<PasteBoardAccountStateSubscriber> accountStateSubscriber_;
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    std::shared_ptr<PasteboardDistributedAccountSubscriber> distributedAccountSubscriber_;
#endif
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_SYSTEM_EVENT_LISTENER_H