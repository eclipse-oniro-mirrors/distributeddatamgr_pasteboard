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

#include "pasteboarddata_fuzzer.h"

#include <fuzzer/FuzzedDataProvider.h>

#include "paste_data.h"

namespace {
using namespace OHOS::MiscServices;

void TestPasteData(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    PasteData pasteData;
    bool isRemote = fdp.ConsumeBool();
    std::vector<uint8_t> buffer = fdp.ConsumeRemainingBytes<uint8_t>();
    pasteData.Decode(buffer);
    pasteData.Encode(buffer, isRemote);

    OHOS::Parcel parcel;
    parcel.WriteUInt8Vector(buffer);
    auto ptr = PasteData::Unmarshalling(parcel);
    if (ptr != nullptr) {
        delete ptr;
    }
    PasteData pasteData2;
    pasteData2.Marshalling(parcel);
}

void TestPasteDataRecord(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    PasteDataRecord record;
    bool isRemote = fdp.ConsumeBool();
    std::vector<uint8_t> buffer = fdp.ConsumeRemainingBytes<uint8_t>();
    record.Decode(buffer);
    record.Encode(buffer, isRemote);
}

void TestPasteDataEntry(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    PasteDataEntry entry;
    bool isRemote = fdp.ConsumeBool();
    std::vector<uint8_t> buffer = fdp.ConsumeRemainingBytes<uint8_t>();
    entry.Decode(buffer);
    entry.Encode(buffer, isRemote);
}

void TestPasteDataId(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    std::string pasteId = fdp.ConsumeRandomLengthString();
    PasteData::IsValidPasteId(pasteId);
}
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    TestPasteData(data, size);
    TestPasteDataRecord(data, size);
    TestPasteDataEntry(data, size);
    TestPasteDataId(data, size);
    return 0;
}
