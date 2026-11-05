/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICE_PASTEBOARD_SWITCH_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICE_PASTEBOARD_SWITCH_H

#include "data_ability_observer_stub.h"

namespace OHOS::MiscServices {
class PastedSwitchObserver : public AAFwk::DataAbilityObserverStub {
public:
    using ObserverCallback = std::function<void()>;
    explicit PastedSwitchObserver(ObserverCallback func) : func_(func) {}
    ~PastedSwitchObserver() {}

    void OnChange() override;

private:
    ObserverCallback func_;
};

class PastedSwitch {
public:
    PastedSwitch();
    void Init(int32_t userId);
    void DeInit();
    void SetSwitch(int32_t userId);
    bool GetDeviceCollabSwitch(int32_t userId);
    bool GetAccessToastSwitch(int32_t userId);
    int32_t userId_;
private:
    void ReportUeSwitchEvent();
    sptr<PastedSwitchObserver> switchObserver_;
};
} // namespace OHOS::MiscServices

#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICE_PASTEBOARD_SWITCH_H