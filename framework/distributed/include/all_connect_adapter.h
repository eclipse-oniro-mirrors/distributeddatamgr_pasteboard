/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_ALL_CONNECT_ADAPTER_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_ALL_CONNECT_ADAPTER_H

#include <mutex>

#include "allconnect_manager.h"
#include "clip/clip_plugin.h"
#include "common/block_object.h"
#include "common/concurrent_map.h"

namespace OHOS {
namespace MiscServices {
class AllConnectAdapter {
public:
    enum ErrorCode {
        SUCCESS = 0,
        APPLY_RESOURCE_PASS = 1004720001,
        APPLY_RESOURCE_REJECT = 1004720002,
    }; // defined by allconnect

    AllConnectAdapter();
    virtual ~AllConnectAdapter();

    int32_t RegisterLifecycleCallback();
    int32_t UnRegisterLifeCycleCallback();
    sptr<CollaborationFwk::ResourceRequest> BuildResourceRequest();
    int32_t ApplyAdvancedResource(const std::string &deviceId);
    int32_t Stop(std::string peerNetworkId, std::string eventName, std::string serviceName, int pid);
    int32_t ApplyResult(std::string deviceId, std::string serviceName, std::string reason,
        int32_t errorCode, int32_t result);
    int32_t PublishServiceState(const std::string& networkId, ClipPlugin::ServiceStatus status);

private:
    class ApplyCallback;
    int32_t systemAbilityObserverId_ = 0;
    sptr<CollaborationFwk::CallbackSkeleton> applyCallback_ = nullptr;
    std::mutex applyCallbackMutex_;
    ConcurrentMap<std::string, bool> applying_;
    ConcurrentMap<std::string, std::shared_ptr<BlockObject<int32_t>>> applyResultBlock_;
    
    class ApplyResultTaskManager {
    public:
        using TasksMap = ConcurrentMap<uint32_t, std::shared_ptr<BlockObject<int32_t>>>;
        void Notify(const std::string &deviceId, int32_t ret);
        void ClearApplyResultTask(const std::string &deviceId);
        void AddApplyResultTask(const std::string &deviceId);
        int32_t WaitApplyResult(const std::string &deviceId);

    private:
        std::atomic<uint32_t> mapKey_ = 0;
        std::mutex mutex_;
        std::map<std::string, std::shared_ptr<TasksMap>> dataTasks_;
    };
    ApplyResultTaskManager taskMgr_;
};
} // namespace MiscServices
} // namespace OHOS
#endif  // DISTRIBUTED_PASTEBOARD_FRAMEWORK_ALL_CONNECT_ADAPTER_H

