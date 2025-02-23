/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_ADAPTER_PASTEBOARD_PROGRESS_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_ADAPTER_PASTEBOARD_PROGRESS_H

#include "iremote_object.h"

namespace OHOS::MiscServices {
class PasteBoardProgress {
public:
    static PasteBoardProgress &GetInstance();
    int32_t InsertValue(std::string &key, std::string &value);
    int32_t UpdateValue(std::string &key, std::string value);

private:
    PasteBoardProgress() = default;
    ~PasteBoardProgress() = default;
    DISALLOW_COPY_AND_MOVE(PasteBoardProgress);

    static void Initialize();

    static std::mutex mutex_;
    static PasteBoardProgress *instance_;
    static sptr<IRemoteObject> remoteObj_;
};
} // namespace OHOS::MiscServices

#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_ADAPTER_PASTEBOARD_PROGRESS_H