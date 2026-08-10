/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_ADAPTER_DATA_SHARE_DELEGATE_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_ADAPTER_DATA_SHARE_DELEGATE_H

#include <mutex>

#include "datashare_helper.h"

namespace OHOS::MiscServices {
using ChangeInfo = DataShare::DataShareObserver::ChangeInfo;
const constexpr char *DEVICE_COLLAB_SWITCH = "settings.collaboration.multi_device_collaboration_service_switch";
class DataShareDelegate {
public:
    static DataShareDelegate &GetInstance();
    int32_t RegisterObserver(const std::string &key, sptr<AAFwk::IDataAbilityObserver> observer);
    int32_t UnregisterObserver(const std::string &key, sptr<AAFwk::IDataAbilityObserver> observer);
    int32_t GetValue(const std::string &key, std::string &value);
    void SetUserId(int32_t userId);

private:
    DataShareDelegate() = default;
    ~DataShareDelegate() = default;
    DISALLOW_COPY_AND_MOVE(DataShareDelegate);

    static void Initialize();
    bool ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> helper);
    std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper(const std::string &key);
    Uri MakeUri(const std::string &key);

    static DataShareDelegate *instance_;
    std::mutex userIdMutex_;
    std::string userId_ = "100";
};
} // namespace OHOS::MiscServices

#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_ADAPTER_DATA_SHARE_DELEGATE_H