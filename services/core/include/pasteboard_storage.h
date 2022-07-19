/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_STORAGE_H
#define PASTE_BOARD_STORAGE_H

#include <map>

#include "i_pasteboard_storage.h"
#include "paste_data.h"

namespace OHOS {
namespace MiscServices {
class PasteboardStorage : public IPasteboardStorage {
public:
    static std::shared_ptr<PasteboardStorage> Create(const std::string &file);
    void SaveData(std::map<int32_t, std::shared_ptr<PasteData>> data) override;
    std::map<int32_t, std::shared_ptr<PasteData>> LoadData() override;
    ~PasteboardStorage() override;
private:
    explicit PasteboardStorage(std::string file);

    std::string file;
};
} // MiscServices
} // OHOS
#endif // PASTE_BOARD_STORAGE_H