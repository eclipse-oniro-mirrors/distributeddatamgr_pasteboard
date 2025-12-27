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
#include "pasteboard_error.h"
#include "pasteboard_service.h"
#include "string_ex.h"

namespace {
using namespace OHOS;
using namespace OHOS::MiscServices;

constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
constexpr uint32_t MAX_ENUM_VALUE = 5;
std::mutex g_fdpMutex;
PasteboardService *g_pasteboardService = new PasteboardService();

class ChangedObserverImpl : public IPasteboardChangedObserver {
public:
    void OnPasteboardChanged()
    {
    }

    void OnPasteboardEvent(const PasteboardChangedEvent &event)
    {
        (void)event;
    }

    sptr<IRemoteObject> AsObject()
    {
        return nullptr;
    }
};

class EntityRecognitionObserverImpl : public IEntityRecognitionObserver {
public:
    void OnRecognitionEvent(EntityType type, std::string &entity) override
    {
        (void)type;
        (void)entity;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

class DisposableObserverImpl : public IPasteboardDisposableObserver {
public:
    void OnTextReceived(const std::string &text, int32_t errCode) override
    {
        (void)text;
        (void)errCode;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

class DelayGetterImpl : public IPasteboardDelayGetter {
public:
    void GetPasteData(const std::string &type, PasteData &data) override
    {
        (void)type;
        (void)data;
    }

    void GetUnifiedData(const std::string &type, UDMF::UnifiedData &data) override
    {
        (void)type;
        (void)data;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

class EntryGetterImpl : public IPasteboardEntryGetter {
public:
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &value) override
    {
        (void)recordId;
        (void)value;
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

class ClientDeathObserver : public IRemoteObject {
public:
    explicit ClientDeathObserver(const std::u16string descriptor) : IRemoteObject(descriptor)
    {
    }

    virtual ~ClientDeathObserver()
    {
    }

    int32_t GetObjectRefCount()
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        (void)code;
        (void)data;
        (void)reply;
        (void)option;
        return 0;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        (void)recipient;
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        (void)recipient;
        return true;
    }

    int Dump(int fd, const std::vector<std::u16string> &args)
    {
        (void)fd;
        (void)args;
        return 0;
    }
};

void FuzzSetPasteDataOnly(FuzzedDataProvider &fdp)
{
    std::vector<uint8_t> pasteDataTlv;
    {
        std::lock_guard lock(g_fdpMutex);
        pasteDataTlv = fdp.ConsumeRemainingBytes<uint8_t>();
    }

    MessageParcelWarp mpw;
    OHOS::MessageParcel mp;
    int fd = -1;
    if (pasteDataTlv.size() <= MIN_ASHMEM_DATA_SIZE) {
        fd = mpw.CreateTmpFd();
    } else {
        if (!mpw.WriteRawData(mp, pasteDataTlv.data(), pasteDataTlv.size())) {
            return;
        }
        fd = mpw.GetWriteDataFd();
    }

    g_pasteboardService->SetPasteDataOnly(fd, pasteDataTlv.size(), pasteDataTlv);
}

void FuzzSetPasteDataDelayData(FuzzedDataProvider &fdp)
{
    std::vector<uint8_t> pasteDataTlv;
    {
        std::lock_guard lock(g_fdpMutex);
        pasteDataTlv = fdp.ConsumeRemainingBytes<uint8_t>();
    }

    MessageParcelWarp mpw;
    OHOS::MessageParcel mp;
    int fd = -1;
    if (pasteDataTlv.size() <= MIN_ASHMEM_DATA_SIZE) {
        fd = mpw.CreateTmpFd();
    } else {
        if (!mpw.WriteRawData(mp, pasteDataTlv.data(), pasteDataTlv.size())) {
            return;
        }
        fd = mpw.GetWriteDataFd();
    }

    auto delayGetter = sptr<DelayGetterImpl>::MakeSptr();
    g_pasteboardService->SetPasteDataDelayData(fd, pasteDataTlv.size(), pasteDataTlv, delayGetter);
}

void FuzzSetPasteDataEntryData(FuzzedDataProvider &fdp)
{
    std::vector<uint8_t> pasteDataTlv;
    {
        std::lock_guard lock(g_fdpMutex);
        pasteDataTlv = fdp.ConsumeRemainingBytes<uint8_t>();
    }

    MessageParcelWarp mpw;
    OHOS::MessageParcel mp;
    int fd = -1;
    if (pasteDataTlv.size() <= MIN_ASHMEM_DATA_SIZE) {
        fd = mpw.CreateTmpFd();
    } else {
        if (!mpw.WriteRawData(mp, pasteDataTlv.data(), pasteDataTlv.size())) {
            return;
        }
        fd = mpw.GetWriteDataFd();
    }

    auto entryGetter = sptr<EntryGetterImpl>::MakeSptr();
    g_pasteboardService->SetPasteDataEntryData(fd, pasteDataTlv.size(), pasteDataTlv, entryGetter);
}

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
    if (pasteDataTlv.size() <= MIN_ASHMEM_DATA_SIZE) {
        fd = mpw.CreateTmpFd();
    } else {
        if (!mpw.WriteRawData(mp, pasteDataTlv.data(), pasteDataTlv.size())) {
            return;
        }
        fd = mpw.GetWriteDataFd();
    }

    auto delayGetter = sptr<DelayGetterImpl>::MakeSptr();
    auto entryGetter = sptr<EntryGetterImpl>::MakeSptr();
    g_pasteboardService->SetPasteData(fd, pasteDataTlv.size(), pasteDataTlv, delayGetter, entryGetter);
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

void FuzzClearByUser(FuzzedDataProvider &fdp)
{
    int32_t userId = -1;
    {
        std::lock_guard lock(g_fdpMutex);
        userId = fdp.ConsumeIntegral<int32_t>();
    }

    g_pasteboardService->ClearByUser(userId);
}

void FuzzGetRecordValueByType(FuzzedDataProvider &fdp)
{
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::vector<uint8_t> buffer;
    {
        std::lock_guard lock(g_fdpMutex);
        dataId = fdp.ConsumeIntegral<uint32_t>();
        recordId = fdp.ConsumeIntegral<uint32_t>();
        buffer = fdp.ConsumeRemainingBytes<uint8_t>();
    }

    MessageParcelWarp mpw;
    OHOS::MessageParcel mp;
    int fd = -1;
    int64_t rawDataSize = buffer.size();
    if (buffer.size() <= MIN_ASHMEM_DATA_SIZE) {
        fd = mpw.CreateTmpFd();
    } else {
        if (!mpw.WriteRawData(mp, buffer.data(), buffer.size())) {
            return;
        }
        fd = mpw.GetWriteDataFd();
    }

    g_pasteboardService->GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
}

void FuzzHasPasteData(FuzzedDataProvider &fdp)
{
    bool result = false;
    {
        std::lock_guard lock(g_fdpMutex);
        result = fdp.ConsumeBool();
    }

    g_pasteboardService->HasPasteData(result);
}

void FuzzIsRemoteData(FuzzedDataProvider &fdp)
{
    bool result = false;
    {
        std::lock_guard lock(g_fdpMutex);
        result = fdp.ConsumeBool();
    }

    g_pasteboardService->IsRemoteData(result);
}

void FuzzSyncDelayedData(FuzzedDataProvider &fdp)
{
    (void)fdp;
    g_pasteboardService->SyncDelayedData();
}

void FuzzGetMimeTypes(FuzzedDataProvider &fdp)
{
    std::vector<std::string> mimeTypes;
    {
        std::lock_guard lock(g_fdpMutex);
        std::string item = fdp.ConsumeRandomLengthString();
        mimeTypes.push_back(item);
    }

    g_pasteboardService->GetMimeTypes(mimeTypes);
}

void FuzzHasDataType(FuzzedDataProvider &fdp)
{
    std::string mimeType;
    bool result = false;
    {
        std::lock_guard lock(g_fdpMutex);
        result = fdp.ConsumeBool();
        mimeType = fdp.ConsumeRandomLengthString();
    }

    g_pasteboardService->HasDataType(mimeType, result);
}

void FuzzHasUtdType(FuzzedDataProvider &fdp)
{
    std::string mimeType;
    bool result = false;
    {
        std::lock_guard lock(g_fdpMutex);
        result = fdp.ConsumeBool();
        mimeType = fdp.ConsumeRandomLengthString();
    }

    g_pasteboardService->HasUtdType(mimeType, result);
}

void FuzzGetDataSource(FuzzedDataProvider &fdp)
{
    std::string bundleName;
    {
        std::lock_guard lock(g_fdpMutex);
        bundleName = fdp.ConsumeRandomLengthString();
    }

    g_pasteboardService->GetDataSource(bundleName);
}

void FuzzGetChangeCount(FuzzedDataProvider &fdp)
{
    uint32_t changeCount = 0;
    {
        std::lock_guard lock(g_fdpMutex);
        changeCount = fdp.ConsumeIntegral<uint32_t>();
    }

    g_pasteboardService->GetChangeCount(changeCount);
}

void FuzzDetectPatterns(FuzzedDataProvider &fdp)
{
    std::vector<Pattern> patterns;
    std::vector<Pattern> results;
    {
        std::lock_guard lock(g_fdpMutex);
        Pattern pattern = static_cast<Pattern>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
        patterns.push_back(pattern);
        pattern = static_cast<Pattern>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
        results.push_back(pattern);
    }

    g_pasteboardService->DetectPatterns(patterns, results);
}

void FuzzSetGlobalShareOption(FuzzedDataProvider &fdp)
{
    std::unordered_map<uint32_t, int32_t> globalShareOptions;
    {
        std::lock_guard lock(g_fdpMutex);
        uint32_t tokenId = fdp.ConsumeIntegral<uint32_t>();
        int32_t shareOption = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_ENUM_VALUE);
        globalShareOptions[tokenId] = shareOption;
    }

    g_pasteboardService->SetGlobalShareOption(globalShareOptions);
}

void FuzzRemoveGlobalShareOption(FuzzedDataProvider &fdp)
{
    std::vector<uint32_t> tokenIds;
    {
        std::lock_guard lock(g_fdpMutex);
        uint32_t tokenId = fdp.ConsumeIntegral<uint32_t>();
        tokenIds.push_back(tokenId);
    }

    g_pasteboardService->RemoveGlobalShareOption(tokenIds);
}

void FuzzGetGlobalShareOption(FuzzedDataProvider &fdp)
{
    std::vector<uint32_t> tokenIds;
    std::unordered_map<uint32_t, int32_t> globalShareOptions;
    {
        std::lock_guard lock(g_fdpMutex);
        uint32_t tokenId = fdp.ConsumeIntegral<uint32_t>();
        tokenIds.push_back(tokenId);
        tokenId = fdp.ConsumeIntegral<uint32_t>();
        int32_t shareOption = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_ENUM_VALUE);
        globalShareOptions[tokenId] = shareOption;
    }

    g_pasteboardService->GetGlobalShareOption(tokenIds, globalShareOptions);
}

void FuzzSetAppShareOptions(FuzzedDataProvider &fdp)
{
    int32_t shareOption = 0;
    {
        std::lock_guard lock(g_fdpMutex);
        shareOption = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_ENUM_VALUE);
    }

    g_pasteboardService->SetAppShareOptions(shareOption);
}

void FuzzRemoveAppShareOptions(FuzzedDataProvider &fdp)
{
    (void)fdp;
    g_pasteboardService->RemoveAppShareOptions();
}

void FuzzPasteStart(FuzzedDataProvider &fdp)
{
    std::string pasteId;
    {
        std::lock_guard lock(g_fdpMutex);
        pasteId = fdp.ConsumeRandomLengthString();
    }

    g_pasteboardService->PasteStart(pasteId);
}

void FuzzPasteComplete(FuzzedDataProvider &fdp)
{
    std::string pasteId;
    std::string deviceId;
    {
        std::lock_guard lock(g_fdpMutex);
        pasteId = fdp.ConsumeRandomLengthString();
        deviceId = fdp.ConsumeRandomLengthString();
    }

    g_pasteboardService->PasteComplete(deviceId, pasteId);
}

void FuzzDetachPasteboard(FuzzedDataProvider &fdp)
{
    (void)fdp;
    g_pasteboardService->DetachPasteboard();
}

void FuzzDump(FuzzedDataProvider &fdp)
{
    std::vector<std::u16string> args;
    {
        std::lock_guard lock(g_fdpMutex);
        std::string str = fdp.ConsumeRandomLengthString();
        args.push_back(Str8ToStr16(str));
    }

    MessageParcelWarp mpw;
    int fd = mpw.CreateTmpFd();

    g_pasteboardService->Dump(fd, args);
}

void FuzzSubscribeDisposableObserver(FuzzedDataProvider &fdp)
{
    DisposableType type = DisposableType::MAX;
    int32_t windowId = -1;
    uint32_t maxLen = 0;
    {
        std::lock_guard lock(g_fdpMutex);
        windowId = fdp.ConsumeIntegral<int32_t>();
        type = static_cast<DisposableType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
        maxLen = fdp.ConsumeIntegral<uint32_t>();
    }

    auto observer = sptr<DisposableObserverImpl>::MakeSptr();
    g_pasteboardService->SubscribeDisposableObserver(observer, windowId, type, maxLen);
}

void FuzzRegisterClientDeathObserver(FuzzedDataProvider &fdp)
{
    std::u16string descriptor;
    {
        std::lock_guard lock(g_fdpMutex);
        std::string str = fdp.ConsumeRandomLengthString();
        descriptor = Str8ToStr16(str);
    }

    sptr<IRemoteObject> observer = sptr<ClientDeathObserver>::MakeSptr(descriptor);
    g_pasteboardService->RegisterClientDeathObserver(observer);
}

void FuzzShowProgress(FuzzedDataProvider &fdp)
{
    std::u16string descriptor;
    std::string progressKey;
    {
        std::lock_guard lock(g_fdpMutex);
        progressKey = fdp.ConsumeRandomLengthString();
        std::string str = fdp.ConsumeRandomLengthString();
        descriptor = Str8ToStr16(str);
    }

    sptr<IRemoteObject> observer = sptr<ClientDeathObserver>::MakeSptr(descriptor);
    g_pasteboardService->ShowProgress(progressKey, observer);
}

void FuzzSubscribeEntityObserver(FuzzedDataProvider &fdp)
{
    EntityType type = EntityType::MAX;
    uint32_t expectedDataLength = 0;
    {
        std::lock_guard lock(g_fdpMutex);
        type = static_cast<EntityType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
        expectedDataLength = fdp.ConsumeIntegral<uint32_t>();
    }

    auto observer = sptr<EntityRecognitionObserverImpl>::MakeSptr();
    g_pasteboardService->SubscribeEntityObserver(type, expectedDataLength, observer);
}

void FuzzUnsubscribeEntityObserver(FuzzedDataProvider &fdp)
{
    EntityType type = EntityType::MAX;
    uint32_t expectedDataLength = 0;
    {
        std::lock_guard lock(g_fdpMutex);
        type = static_cast<EntityType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
        expectedDataLength = fdp.ConsumeIntegral<uint32_t>();
    }

    auto observer = sptr<EntityRecognitionObserverImpl>::MakeSptr();
    g_pasteboardService->UnsubscribeEntityObserver(type, expectedDataLength, observer);
}

void FuzzSubscribeObserver(FuzzedDataProvider &fdp)
{
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    {
        std::lock_guard lock(g_fdpMutex);
        type = static_cast<PasteboardObserverType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    }

    auto observer = sptr<ChangedObserverImpl>::MakeSptr();
    g_pasteboardService->SubscribeObserver(type, observer);
}

void FuzzResubscribeObserver(FuzzedDataProvider &fdp)
{
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    {
        std::lock_guard lock(g_fdpMutex);
        type = static_cast<PasteboardObserverType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    }

    auto observer = sptr<ChangedObserverImpl>::MakeSptr();
    g_pasteboardService->ResubscribeObserver(type, observer);
}

void FuzzUnsubscribeObserver(FuzzedDataProvider &fdp)
{
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    {
        std::lock_guard lock(g_fdpMutex);
        type = static_cast<PasteboardObserverType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    }

    auto observer = sptr<ChangedObserverImpl>::MakeSptr();
    g_pasteboardService->UnsubscribeObserver(type, observer);
}

void FuzzUnsubscribeAllObserver(FuzzedDataProvider &fdp)
{
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    {
        std::lock_guard lock(g_fdpMutex);
        type = static_cast<PasteboardObserverType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    }

    g_pasteboardService->UnsubscribeAllObserver(type);
}

const std::function<void(FuzzedDataProvider&)> FUNC_LIST[] = {
    FuzzSetPasteData,
    FuzzGetPasteData,
    FuzzClear,
    FuzzClearByUser,
    FuzzGetRecordValueByType,
    FuzzHasPasteData,
    FuzzIsRemoteData,
    FuzzSyncDelayedData,
    FuzzGetMimeTypes,
    FuzzGetChangeCount,
    FuzzHasDataType,
    FuzzHasUtdType,
    FuzzGetDataSource,
    FuzzDetectPatterns,
    FuzzSetGlobalShareOption,
    FuzzRemoveGlobalShareOption,
    FuzzGetGlobalShareOption,
    FuzzSetAppShareOptions,
    FuzzRemoveAppShareOptions,
    FuzzPasteStart,
    FuzzPasteComplete,
    FuzzDetachPasteboard,
    FuzzSubscribeDisposableObserver,
    FuzzSetPasteDataDelayData,
    FuzzSetPasteDataEntryData,
    FuzzSetPasteDataOnly,
    FuzzRegisterClientDeathObserver,
    FuzzSubscribeEntityObserver,
    FuzzUnsubscribeEntityObserver,
    FuzzSubscribeObserver,
    FuzzResubscribeObserver,
    FuzzUnsubscribeObserver,
    FuzzUnsubscribeAllObserver,
    FuzzDump,
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
