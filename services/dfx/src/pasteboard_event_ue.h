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

#ifndef DISTRIBUTEDDATAMGR_PASTEBOARD_EVENT_UE_H
#define DISTRIBUTEDDATAMGR_PASTEBOARD_EVENT_UE_H

#include "hisysevent.h"
#include "pasteboard_event_common.h"

namespace OHOS {
namespace MiscServices {
namespace UeReporter {
using namespace OHOS::HiviewDFX;

constexpr HiviewDFX::HiSysEvent::EventType UE_OPERATION_TYPE = HiviewDFX::HiSysEvent::EventType::BEHAVIOR;
constexpr HiviewDFX::HiSysEvent::EventType UE_STATUS_TYPE = HiviewDFX::HiSysEvent::EventType::STATISTIC;

constexpr const char *VERSION = "1.0";
constexpr char UE_DOMAIN[] = "PASTEBOARD_UE";

enum SwitchStatus : std::int32_t {
    SWITCH_CLOSE = 0,
    SWITCH_OPEN = 1,
};

#define UE_SWITCH(eventName, eventType, switchStatus, ...)                                                          \
    ({                                                                                                              \
        HiSysEventWrite(UeReporter::UE_DOMAIN, eventName, eventType, "PNAMEID", "pasteboard_service", "PVERSIONID", \
            UeReporter::VERSION, "SWITCH_STATUS", switchStatus, ##__VA_ARGS__);                                     \
    })

#define UE_REPORT(eventName, reportInfo, ...)                                                                       \
    ({                                                                                                              \
        HiSysEventWrite(UeReporter::UE_DOMAIN, eventName, UeReporter::UE_OPERATION_TYPE, "PNAMEID",                 \
            "pasteboard_service", "PVERSIONID", UeReporter::VERSION,                                                \
            "PASTEDATA_TYPE", reportInfo.dataType, "BUNDLE_NAME", reportInfo.bundleName,                            \
            "PASTE_RESULT", reportInfo.ret, "DEVICE_TYPE", reportInfo.commonInfo.deviceType,                        \
            "DATA_TIMESTAMP", reportInfo.timestamp, ##__VA_ARGS__);                                                 \
    })
} // namespace UeReporter
} // namespace MiscServices
} // namespace OHOS
#endif //DISTRIBUTEDDATAMGR_PASTEBOARD_EVENT_UE_H