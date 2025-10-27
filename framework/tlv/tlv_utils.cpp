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

#include "tlv_utils.h"

#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
RawMem TLVUtils::Parcelable2Raw(const Parcelable *value)
{
    RawMem rawMem{};
    if (value == nullptr) {
        return rawMem;
    }

    rawMem.parcel = std::make_shared<Parcel>(nullptr);
    bool ret = value->Marshalling(*rawMem.parcel);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, rawMem, PASTEBOARD_MODULE_COMMON, "Marshalling failed");

    rawMem.buffer = rawMem.parcel->GetData();
    rawMem.bufferLen = rawMem.parcel->GetDataSize();
    return rawMem;
}

bool TLVUtils::Raw2Parcel(const RawMem &rawMem, Parcel &parcel)
{
    if (rawMem.buffer == 0 || rawMem.bufferLen == 0) {
        return false;
    }
    auto *temp = malloc(rawMem.bufferLen); // free by Parcel!
    if (temp == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "malloc failed, size=%{public}zu", rawMem.bufferLen);
        return false;
    }
    auto err = memcpy_s(temp, rawMem.bufferLen, reinterpret_cast<const void *>(rawMem.buffer), rawMem.bufferLen);
    if (err != EOK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "memcpy RawMem failed, size=%{public}zu", rawMem.bufferLen);
        free(temp);
        return false;
    }
    bool ret = parcel.ParseFrom(reinterpret_cast<uintptr_t>(temp), rawMem.bufferLen);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "parse from RawMem failed, size=%{public}zu", rawMem.bufferLen);
        free(temp);
        return false;
    }
    return true;
}

std::shared_ptr<Media::PixelMap> TLVUtils::Vector2PixelMap(std::vector<std::uint8_t> &value)
{
    return value.empty() ? nullptr : std::shared_ptr<Media::PixelMap>(Media::PixelMap::DecodeTlv(value));
}

std::vector<std::uint8_t> TLVUtils::PixelMap2Vector(std::shared_ptr<Media::PixelMap> pixelMap)
{
    if (pixelMap == nullptr) {
        return {};
    }

    std::vector<std::uint8_t> value;
    bool ret = pixelMap->EncodeTlv(value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, {}, PASTEBOARD_MODULE_COMMON, "EncodeTlv failed");

    return value;
}
} // namespace OHOS::MiscServices
