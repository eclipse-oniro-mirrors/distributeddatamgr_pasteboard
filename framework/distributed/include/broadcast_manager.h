/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_BROADCAST_MANAGER_H
#define OHOS_BROADCAST_MANAGER_H

#include <mutex>
#include <string>
#include "broadcast_manager_listener.h"

namespace OHOS {
namespace MiscServices {
class BroadcastManager {
public:
    static constexpr int32_t BROADCAST_OK = 0;
    static constexpr int32_t BROADCAST_INVALID_PARAM = -1;

    static BroadcastManager &GetInstance();
    int32_t SendPasteboardEvent(uint8_t* sendData, uint32_t dataLen, uint8_t* exData, uint32_t exDataLen,
        uint16_t accountHash = 0);
    int32_t StopPasteboardEvent();
    int32_t RegisterPasteboardEventListener(const std::shared_ptr<BroadcastManagerListener>& listener,
        uint16_t accountHash = 0);
    int32_t UnregisterPasteboardEventListener();
    void OnBroadcastReceive(std::string& networkId, uint8_t* data, uint32_t dataLen,
        uint8_t* exData, uint32_t exDataLen, uint16_t accountHash);

private:
    BroadcastManager();
    std::mutex listenerMutex_;
    std::shared_ptr<BroadcastManagerListener> broadcastManagerListener_;
    uint16_t registeredAccountHash_ = 0;
    std::string pkgName_ = "ohos.distributeddatamgr.pasteboard";
    virtual ~BroadcastManager() = default;
};
} // namespace MiscServices
} // namespace OHOS
#endif // OHOS_BROADCAST_MANAGER_H
