/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_PASTEBOARD_EVENT_COMMON_H
#define DISTRIBUTEDDATAMGR_PASTEBOARD_EVENT_COMMON_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
struct DataDescription {
    uint32_t recordNum = 0;
    std::vector<int32_t> entryNum;
    std::vector<std::string> mimeTypes;
};

struct CommonInfo {
    int32_t deviceType = 0;
    int32_t currentAccountId = -1;
    int64_t dataSize = 0;
};

struct RadarPasteInfo {
    bool isPeerOnline;
    uint32_t onlineDevNum;
    int32_t networkType;
    std::string peerNetId;
    std::string peerUdid;
    std::string pasteId;
    std::string peerBundleName;
};

struct RadarReportInfo {
    int32_t stageRes;
    std::string bundleName;
    DataDescription description;
    CommonInfo commonInfo;
    RadarPasteInfo pasteInfo;
};

struct UePasteInfo {
    bool isPeerOnline;
    bool isDistributed;
    uint32_t onlineDevNum;
    int32_t networkType;
    std::string peerBundleName = "";
};

struct UeReportInfo {
    int32_t ret = -1;
    uint8_t dataType = 0;
    std::string bundleName = "";
    DataDescription description;
    CommonInfo commonInfo;
    UePasteInfo pasteInfo;
};
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_PASTEBOARD_EVENT_COMMON_H
