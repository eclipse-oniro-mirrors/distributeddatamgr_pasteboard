/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef PASTE_BOARD_DATA_INFO_H
#define PASTE_BOARD_DATA_INFO_H

#include <vector>
#include <string>

#include "api/visibility.h"
#include "message_parcel.h"

namespace OHOS {
namespace MiscServices {
class API_EXPORT PasteDataInfo : public Parcelable {
public:
    PasteDataInfo();
    ~PasteDataInfo();
    PasteDataInfo(const PasteDataInfo & dataInfo);
    PasteDataInfo &operator=(const PasteDataInfo &dataInfo);

    bool Marshalling(Parcel &parcel) const override;
    static PasteDataInfo *Unmarshalling(Parcel &parcel);

    std::vector<std::string> mimeTypes;
    int32_t rawDataSize;
    int32_t textDataSize;
    int32_t htmlDataSize;
    bool isDelayedData;
    bool isDelayedRecord;
};
}
}
#endif // PASTE_BOARD_DATA_INFO_H