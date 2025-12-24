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

#include <fuzzer/FuzzedDataProvider.h>

#include "message_parcel_warp.h"
#include "mock_native_token.h"
#include "pasteboard_service.h"

namespace {
using namespace OHOS::MiscServices;

constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
std::mutex g_fdpMutex;
PasteboardService *g_pasteboardService = new PasteboardService();

void FuzzSetPasteData(FuzzedDataProvider &fdp)
{
    std::vector<uint8_t> pasteDataTlv;
    {
        std::lock_guard lock(g_fdpMutex);
        pasteDataTlv = fdp.ConsumeRemainingBytes<uint8_t>();
    }

    MessageParcelWarp mpw;
    OHOS::MessageParcel mp;
    int fd = -1;
    if (pasteDataTlv.size() < MIN_ASHMEM_DATA_SIZE) {
        fd = mpw.CreateTmpFd();
    } else {
        if (!mpw.WriteRawData(mp, pasteDataTlv.data(), pasteDataTlv.size())) {
            return;
        }
        fd = mpw.GetWriteDataFd();
    }

    g_pasteboardService->SetPasteDataOnly(fd, pasteDataTlv.size(), pasteDataTlv);
}

void FuzzGetPasteData(FuzzedDataProvider &fdp)
{
    int64_t size = 0;
    int32_t syncTime = 0;
    int32_t realErrCode = 0;
    std::string pasteId;
    std::vector<uint8_t> rawData;
    {
        std::lock_guard lock(g_fdpMutex);
        size = fdp.ConsumeIntegral<int64_t>();
        syncTime = fdp.ConsumeIntegral<int32_t>();
        realErrCode = fdp.ConsumeIntegral<int32_t>();
        pasteId = fdp.ConsumeRandomLengthString();
        rawData = fdp.ConsumeRemainingBytes<uint8_t>();
    }

    MessageParcelWarp mpw;
    int fd = mpw.CreateTmpFd();
    g_pasteboardService->GetPasteData(fd, size, rawData, pasteId, syncTime, realErrCode);
}

void FuzzClear(FuzzedDataProvider &fdp)
{
    (void)fdp;
    g_pasteboardService->Clear();
}

const std::function<void(FuzzedDataProvider&)> FUNC_LIST[] = {
    FuzzSetPasteData,
    FuzzGetPasteData,
    FuzzClear,
};
} // anonymous namespace

extern "C" int LLVMFuzzerInitialize(int *argc, char **argv)
{
    (void)argc;
    (void)argv;
    static OHOS::Security::AccessToken::MockNativeToken token("pasteboard_service");
    g_pasteboardService->OnStart();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    auto func = fdp.PickValueInArray(FUNC_LIST);
    func(fdp);
    return 0;
}
