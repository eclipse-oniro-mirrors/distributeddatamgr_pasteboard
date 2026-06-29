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

#include "pasteboard_delay_data_handler.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

PasteboardDelayDataHandler::PasteboardDelayDataHandler(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDelayDataHandler constructed.");
}

PasteboardDelayDataHandler::~PasteboardDelayDataHandler()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDelayDataHandler destructed.");
}

// TODO: Implement methods in Phase 6

} // namespace MiscServices
} // namespace OHOS