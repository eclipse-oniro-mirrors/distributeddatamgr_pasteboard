/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_SYNC_CALLBACK_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_SYNC_CALLBACK_H

#include "common/block_object.h"
#include "kvstore_sync_callback.h"
#include "distributed_clip.h"

namespace OHOS::MiscServices {
class SyncCallback : public DistributedKv::KvStoreSyncCallback {
public:
    using Status = DistributedKv::Status;
    SyncCallback(uint32_t seqId, uint32_t interval, bool isPreSync = false, int32_t userId = 0,
        std::string deviceId = "", DistributedClip *distributedClip = nullptr)
        : seqId_(seqId), result_(interval), isPreSync_(isPreSync), userId_(userId), deviceId_(deviceId),
          distributedClip_(distributedClip)
    {}

    virtual ~SyncCallback() = default;
    void SyncCompleted(const std::map<std::string, Status> &results) override;
    Status GetSyncStatus();

private:
    uint32_t seqId_ = 0;
    BlockObject<std::map<std::string, Status>> result_;
    bool isPreSync_ = false;
    int32_t userId_ = 0;
    std::string deviceId_;
    DistributedClip* distributedClip_ = nullptr;
};
} // namespace OHOS::MiscServices
#endif // DISTRIBUTED_PASTEBOARD_FRAMEWORK_SYNC_CALLBACK_H
