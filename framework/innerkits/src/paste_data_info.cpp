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
#include "paste_data_info.h"

#include "int_wrapper.h"
#include "ipc_skeleton.h"
#include "long_wrapper.h"
#include "pasteboard_common.h"
#include "pasteboard_hilog.h"
#include "message_parcel.h"

namespace OHOS {
namespace MiscServices {

PasteDataInfo::PasteDataInfo()
    : rawDataSize(0), textDataSize(0), htmlDataSize(0), isDelayedData(false), isDelayedRecord(false)
{
}

PasteDataInfo::~PasteDataInfo()
{
}

PasteDataInfo::PasteDataInfo(const PasteDataInfo &dataInfo)
    : mimeTypes(dataInfo.mimeTypes), rawDataSize(dataInfo.rawDataSize),
      textDataSize(dataInfo.textDataSize), htmlDataSize(dataInfo.htmlDataSize),
      isDelayedData(dataInfo.isDelayedData), isDelayedRecord(dataInfo.isDelayedRecord)
{
}

PasteDataInfo &PasteDataInfo::operator=(const PasteDataInfo &dataInfo)
{
    if (this == &dataInfo) {
        return *this;
    }
    mimeTypes = dataInfo.mimeTypes;
    rawDataSize = dataInfo.rawDataSize;
    textDataSize = dataInfo.textDataSize;
    htmlDataSize = dataInfo.htmlDataSize;
    isDelayedData = dataInfo.isDelayedData;
    isDelayedRecord = dataInfo.isDelayedRecord;
    return *this;
}

bool PasteDataInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(rawDataSize)) {
        return false;
    }
    if (!parcel.WriteInt32(textDataSize)) {
        return false;
    }
    if (!parcel.WriteInt32(htmlDataSize)) {
        return false;
    }
    if (!parcel.WriteBool(isDelayedData)) {
        return false;
    }
    if (!parcel.WriteBool(isDelayedRecord)) {
        return false;
    }
    uint32_t mimeTypesSize = mimeTypes.size();
    if (!parcel.WriteUint32(mimeTypesSize)) {
        return false;
    }
    for (const auto &mimeType : mimeTypes) {
        if (!parcel.WriteString(mimeType)) {
            return false;
        }
    }
    return true;
}

PasteDataInfo *PasteDataInfo::Unmarshalling(Parcel &parcel)
{
    auto *info = new (std::nothrow) PasteDataInfo();
    if (info == nullptr) {
        return nullptr;
    }
    if (!parcel.ReadInt32(info->rawDataSize)) {
        delete info;
        return nullptr;
    }
    if (!parcel.ReadInt32(info->textDataSize)) {
        delete info;
        return nullptr;
    }
    if (!parcel.ReadInt32(info->htmlDataSize)) {
        delete info;
        return nullptr;
    }
    if (!parcel.ReadBool(info->isDelayedData)) {
        delete info;
        return nullptr;
    }
    if (!parcel.ReadBool(info->isDelayedRecord)) {
        delete info;
        return nullptr;
    }
    uint32_t mimeTypesSize = 0;
    if (!parcel.ReadUint32(mimeTypesSize)) {
        delete info;
        return nullptr;
    }
    for (uint32_t i = 0; i < mimeTypesSize; ++i) {
        std::string mimeType;
        if (!parcel.ReadString(mimeType)) {
            delete info;
            return nullptr;
        }
        info->mimeTypes.push_back(mimeType);
    }
    return info;
}

}
}