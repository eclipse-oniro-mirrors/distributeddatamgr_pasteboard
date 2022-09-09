/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_INTERFACES_KITS_NAPI_SRC_PASTE_BOARD_DAILOG_H
#define PASTEBOARD_INTERFACES_KITS_NAPI_SRC_PASTE_BOARD_DAILOG_H
#include "common/block_object.h"
namespace OHOS::MiscServicesNapi {
class PasteBoardDialog {
public:
    static constexpr uint32_t POPUP_INTERVAL = 1;  // seconds
    static constexpr uint32_t MAX_LIFE_TIME = 300; // seconds
    static PasteBoardDialog &GetInstance();
    int32_t ShowDialog(std::shared_ptr<BlockObject<uint32_t>> block, const std::string &message = "unknown");
    int32_t CancelDialog(int32_t id);

private:
    struct Rect {
        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 1920;
        int32_t height = 1080;
    };
    Rect GetDisplayRect();
};
}
#endif // PASTEBOARD_INTERFACES_KITS_NAPI_SRC_PASTE_BOARD_DAILOG_H
