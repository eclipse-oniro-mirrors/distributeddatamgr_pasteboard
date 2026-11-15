/*
* Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_CLIP_PARA_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_CLIP_PARA_H

#include <atomic>
#include <mutex>
#include <string>

namespace OHOS {
namespace MiscServices {
class ClipPara {
public:
    struct SendInformation {
        uint64_t timeStamp = 0;
        uint8_t numType = 0;
        uint8_t version = 0;
        uint16_t userId = 0;
        uint16_t seqId = 0;
        uint32_t currentIndex = 0;
    };
    struct SendInfoBrief {
        uint16_t seqId = 0;
        uint8_t dataDescription = 0;
        uint8_t dataSize = 0;
        uint8_t version = 0;
        uint16_t accountHash = 0;
    };
    enum MessageLen {
        LENGTH_THREE = 3,
        LENGTH_FOUR = 4,
        LENGTH_FIVE = 5,
        LENGTH_DEFAULT = 7
    };
    
    constexpr static uint8_t TIMESTAMP_PARA_TYPE = 0x00;
    constexpr static uint8_t DATA_PARA_TYPE = 0x01;

    static ClipPara &GetInstance();
    void InitMemberVariable();
    uint64_t GetLocalExpiration();
    uint64_t GetRemoteExpiration();
    bool GetPullEvent();

    void SetLastLocalSyncKey(const std::string &lastSyncKey);
    void SetLastRemoteSyncKey(const std::string lastRemoteSyncKey);
    void SetLastSyncNetworkId(const std::string lastSyncNetworkId);
    void SetLocalExpiration(uint64_t expiration);
    void SetRemoteExpiration(uint64_t remoteExpiration);
    void SetSendInformation(const ClipPara::SendInformation &senderInformation);
    void SetPullEvent(const bool isPullEvent);
    void SetPasted(const bool isPasted);
    void SetFrameNum(uint8_t frameNum);

private:
    ClipPara();
    ~ClipPara() = default;
    std::mutex mutex_;
    std::string lastLocalSyncKey_;
    std::string lastRemoteSyncKey_;
    std::string lastSyncNetworkId_;
    std::atomic<bool> isPullEvent_ = false;
    std::atomic<bool> isPullEventResult_ = false;
    std::atomic<bool> isPasted_ = false;
    std::atomic<bool> activeSync_ = false;
    std::atomic<uint8_t> frameNum_ = 0;
    std::atomic<uint64_t> localExpiration_ = 0;
    std::atomic<uint64_t> remoteExpiration_ = 0;
    std::shared_ptr<ClipPara::SendInformation> sendInformation_{ nullptr };
};
} // namespace MiscServices
} // namespace OHOS
#endif // DISTRIBUTED_PASTEBOARD_FRAMEWORK_PARA_H