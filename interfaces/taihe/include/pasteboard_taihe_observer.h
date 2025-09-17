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

#ifndef PASTEBOARD_TAIHE_OBSERVER_H
#define PASTEBOARD_TAIHE_OBSERVER_H

#include <memory>

#include "ohos.pasteboard.pasteboard.proj.hpp"
#include "pasteboard_observer.h"

namespace OHOS {
namespace MiscServices {

class PasteboardTaiheObserver : public PasteboardObserver {
public:
    explicit PasteboardTaiheObserver(std::shared_ptr<::taihe::callback<void()>> cb) : cb_(cb) {};
    void OnPasteboardChanged() override;

private:
    std::shared_ptr<::taihe::callback<void()>> cb_ = nullptr;
};

} // namespace MiscServices
} // namespace OHOS
#endif