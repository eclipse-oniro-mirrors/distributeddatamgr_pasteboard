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

void TestPasteData(FuzzedDataProvider &fdp)
{
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

void TestPasteDataRecord(FuzzedDataProvider &fdp)
{
    PasteDataRecord record;
    bool isRemote = fdp.ConsumeBool();
    std::vector<uint8_t> buffer = fdp.ConsumeRemainingBytes<uint8_t>();
    record.Decode(buffer);
    record.Encode(buffer, isRemote);
}

void TestPasteDataEntry(FuzzedDataProvider &fdp)
{
    PasteDataEntry entry;
    bool isRemote = fdp.ConsumeBool();
    std::vector<uint8_t> buffer = fdp.ConsumeRemainingBytes<uint8_t>();
    entry.Decode(buffer);
    entry.Encode(buffer, isRemote);
}

void TestCreatePasteIdWithPid(FuzzedDataProvider &fdp)
{
    std::string name = fdp.ConsumeRandomLengthString();
    uint32_t sequence = fdp.ConsumeIntegral<uint32_t>();
    pid_t pid = fdp.ConsumeIntegral<pid_t>();
    PasteData::CreatePasteId(name, sequence, pid);
}

using DoFunc = void (*)(FuzzedDataProvider &);
constexpr DoFunc FUNC_LIST[] = {
    TestPasteData,
    TestPasteDataRecord,
    TestPasteDataEntry,
    TestCreatePasteIdWithPid,
};
constexpr uint8_t FUNC_COUNT = sizeof(FUNC_LIST) / sizeof(FUNC_LIST[0]);
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    uint8_t index = fdp.ConsumeIntegral<uint8_t>() % FUNC_COUNT;
    FUNC_LIST[index](fdp);
    return 0;
}
