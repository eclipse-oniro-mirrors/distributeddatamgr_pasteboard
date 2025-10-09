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
#ifndef CLIPBOARD_COUNTER_H
#define CLIPBOARD_COUNTER_H

#include "pasteboard_client.h" // 鸿蒙剪贴板客户端头文件
#include "pasteboard_common.h" // 鸿蒙剪贴板公共头文件
#include <string>

namespace OHOS {
namespace MiscServices {
class ClipboardCounter {
public:
    // 获取单例实例
    static ClipboardCounter &GetInstance();

    // 封装的 SetData 方法，替代官方的 PasteboardClient::GetInstance()->SetData(...)
    int32_t SetData(PasteData &data);

    // 获取 SetData 的调用次数
    uint64_t GetSetDataCount() const;

    // 重置计数器（可选）
    void ResetCount();

private:
    ClipboardCounter() = default; // 私有化构造函数以实现单例
    ~ClipboardCounter() = default;

    // 删除拷贝构造函数和赋值操作符
    ClipboardCounter(const ClipboardCounter &) = delete;
    ClipboardCounter &operator=(const ClipboardCounter &) = delete;

    std::atomic<uint64_t> setDataCount_ { 0 }; // 使用原子操作保证线程安全
};

} // namespace MiscServices
} // namespace OHOS

#endif