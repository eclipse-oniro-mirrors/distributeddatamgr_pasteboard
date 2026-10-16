/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_UTILS_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_UTILS_H

#include <broadcast_manager.h>
#include <endian.h>
#include "clip/clip_plugin.h"
#include "clip_para.h"
#include "types.h"

namespace OHOS::MiscServices {
class Utils {
public:
    using Entry = DistributedKv::Entry;
    using Event = ClipPlugin::GlobalEvent;
    using SendInfoBrief = ClipPara::SendInfoBrief;

    static constexpr size_t MAX_VALUE_SIZE = 4 * 1024 * 1024;
    static constexpr size_t MAX_DISTRIBUTED_DATA_SIZE = 128 * 1024 * 1024;
    static constexpr int32_t ERROR_USERID = -1;
    static constexpr int64_t MILLI_TO_SEC = 1000;
    static constexpr int64_t NANO_TO_SEC = 1000000000;
    static constexpr int64_t SIZE_K = 1024;
    static constexpr int64_t MIN_LOCAL_CAPACITY = 1; // 1M
    static constexpr int64_t DEFAULT_LOCAL_CAPACITY = 128; // 128M
    static constexpr int64_t MAX_LOCAL_CAPACITY = 2048; // 2G
    static constexpr int64_t NANO_TO_MILLI = NANO_TO_SEC / MILLI_TO_SEC;
    static constexpr const char *DATA_PREFIX = "DATA";
    static constexpr const char *ENTRY_PREFIX = "ENTRY";
    static constexpr const char *BROADCAST_PREFIX = "BROADCAST";
    static constexpr const char *REQUEST_PREFIX = "REQUEST";
    static constexpr const char *RESPONSE_PREFIX = "RESPONSE";
    static std::string GetDataKey(const Event &event, uint32_t frame);
    static std::string GetDataPrefix(const Event &event);
    static std::string GetEntryKey(const Event &event, uint32_t recordId, const std::string &utdId, uint32_t frame);
    static std::string GetEntryPrefix(const Event &event, uint32_t recordId, const std::string &utdId);
    static std::string GetStoreEventKey(const std::string &deviceId, bool isRequest);
    static int GetFrameNumFromDataKey(const std::string& key);
    static std::string GetAllDataPrefix(const Event &event);
    static std::vector<Entry> Marshal(const Event &event, const std::vector<uint8_t> &data);
    static std::vector<Entry> Marshal(const Event &event, uint32_t recordId, const std::string &utdId,
        const std::vector<uint8_t> &data);
    static std::vector<uint8_t> Unmarshal(std::vector<Entry> &entries);
    static std::string AnonymousDeviceId(std::string deviceId);
    static int32_t GetCurrentAccountId();
    static uint16_t GetAccountHashByUserId(int32_t userId);
    static void FillUidHash(uint8_t* uidHash, uint16_t accountHash);
    static uint16_t ExtractAccountHash(const uint8_t* uidHash);

    static bool Encode(const SendInfoBrief& sendInfoBrief, uint8_t *buf);
    static bool Decode(SendInfoBrief& sendInfoBrief, const uint8_t *buf, uint32_t dataLen);
    static int64_t GetBootTimeMs(void);
    static int64_t GetTimeMsByClockId(clockid_t clockId);
    static int64_t GetBootTimeSeconds(void);

private:
    static constexpr const char *SPLIT = "_";
    static constexpr uint32_t MAX_RECORD = 1;
    static constexpr uint32_t FRAME_NUMBER_INDEX = 3;

    static void SortEntries(std::vector<Entry> &entries);
    static std::vector<std::string> Split(const std::string &source, const std::string &pattern);
    static bool IsDigit(const std::string &s);

    static bool Write(uint8_t *buf, size_t size, size_t &offset, uint8_t value);
    static bool Write(uint8_t *buf, size_t size, size_t &offset, uint16_t value);
    static bool Read(const uint8_t *buf, size_t size, size_t &offset, uint8_t &value);
    static bool Read(const uint8_t *buf, size_t size, size_t &offset, uint16_t &value);

    static std::atomic<uint32_t> index_;
};
}

#endif // DISTRIBUTED_PASTEBOARD_FRAMEWORK_UTILS_H
