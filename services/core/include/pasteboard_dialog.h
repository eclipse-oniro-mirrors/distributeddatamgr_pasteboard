/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_DIALOG_H
#define PASTEBOARD_DIALOG_H

#include "iremote_object.h"

namespace OHOS::MiscServices {
class PasteboardDialog {
public:
    struct ProgressMessageInfo {
        std::string promptText{ DEFAULT_LABEL };
        std::string remoteDeviceName{ DEFAULT_LABEL };
        std::string progressKey{ DEFAULT_LABEL };
        bool isRemote { false };
        int32_t windowId { 0 };
        sptr<IRemoteObject> callerToken { nullptr };
        sptr<IRemoteObject> clientCallback { nullptr };
    };

    static constexpr const char *DEFAULT_LABEL = "unknown";

    static int32_t ShowProgress(const ProgressMessageInfo &message);
};
} // namespace OHOS::MiscServices
#endif // PASTEBOARD_DIALOG_H
