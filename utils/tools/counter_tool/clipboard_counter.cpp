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
#include "clipboard_counter.h"
#include <hilog/log.h>

#undef LOG_TAG
#define LOG_TAG "ClipboardCounter"

namespace OHOS {
namespace MiscServices {

ClipboardCounter &ClipboardCounter::GetInstance()
{
    static ClipboardCounter instance;
    return instance;
}

int32_t ClipboardCounter::SetData(PasteData &data)
{
    // 1. 执行实际的 SetData 操作
    auto pasteboardClient = PasteboardClient::GetInstance();
    if (pasteboardClient == nullptr) {
        HILOGE("Failed to get PasteboardClient instance.");
        return -1; // 返回错误码
    }

    int32_t ret = pasteboardClient->SetData(data);
    if (ret == 0) {
        // 2. 如果操作成功，则增加计数
        setDataCount_++;
        HILOGI("SetData called successfully. Current count: %{public}llu",
            static_cast<unsigned long long>(GetSetDataCount()));

        // 3. (可选) 将计数持久化到文件或数据库，防止应用重启后丢失
        SaveCountToFile();
    } else {
        HILOGE("PasteboardClient->SetData failed, ret = %{public}d", ret);
    }

    return ret;
}

uint64_t ClipboardCounter::GetSetDataCount() const
{
    return setDataCount_.load();
}

void ClipboardCounter::ResetCount()
{
    setDataCount_ = 0;
    HILOGI("Clipboard SetData count has been reset.");
}

} // namespace MiscServices
} // namespace OHOS