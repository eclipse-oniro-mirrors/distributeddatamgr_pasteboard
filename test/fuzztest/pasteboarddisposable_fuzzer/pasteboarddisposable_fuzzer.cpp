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

#include "pasteboarddisposable_fuzzer.h"

#include <fuzzer/FuzzedDataProvider.h>

#include "pasteboard_disposable_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_window_manager.h"
#include "ffrt/ffrt_utils.h"

namespace {
using namespace OHOS::MiscServices;

constexpr uint32_t MAX_ENUM_VALUE = 5;

class DisposableObserverImpl : public IPasteboardDisposableObserver {
public:
    void OnTextReceived(const std::string &text, int32_t errCode) override
    {
        (void)text;
        (void)errCode;
    }

    OHOS::sptr<OHOS::IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

class DelayGetterImpl : public IPasteboardDelayGetter {
public:
    void GetPasteData(const std::string &type, PasteData &data) override
    {
        (void)type;
        data.AddTextRecord(text_);
    }

    void GetUnifiedData(const std::string &type, OHOS::UDMF::UnifiedData &data) override
    {
        (void)type;
        (void)data;
    }

    OHOS::sptr<OHOS::IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    std::string text_;
};

class EntryGetterImpl : public IPasteboardEntryGetter {
public:
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &value) override
    {
        (void)recordId;
        value.SetValue(text_);
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    OHOS::sptr<OHOS::IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    std::string text_;
};

class TestEnv {
public:
    TestEnv()
    {
        SetUpTestCase();
    }

    ~TestEnv()
    {
        TearDownTestCase();
    }

    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
        FFRTPool::Clear();
        std::lock_guard lock(DisposableManager::GetInstance().disposableInfoMutex_);
        DisposableManager::GetInstance().disposableInfoList_.clear();
    }
};

void TestAddDisposableInfo(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    pid_t pid = fdp.ConsumeIntegral<pid_t>();
    uint32_t tokenId = fdp.ConsumeIntegral<uint32_t>();
    int32_t windowId = fdp.ConsumeIntegral<int32_t>();
    DisposableType type = static_cast<DisposableType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    uint32_t maxLen = fdp.ConsumeIntegral<uint32_t>();
    auto observer = fdp.ConsumeBool() ? nullptr : OHOS::sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info(pid, tokenId, windowId, type, maxLen, observer);
    DisposableManager::GetInstance().AddDisposableInfo(info);
}

void TestTryProcessDisposableData(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    pid_t pid = fdp.ConsumeIntegral<pid_t>();
    uint32_t tokenId = fdp.ConsumeIntegral<uint32_t>();
    int32_t windowId = fdp.ConsumeIntegral<int32_t>();
    DisposableType type = static_cast<DisposableType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    uint32_t maxLen = fdp.ConsumeIntegral<uint32_t>();
    auto observer = fdp.ConsumeBool() ? nullptr : OHOS::sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info(pid, tokenId, windowId, type, maxLen, observer);

    pid_t pid2 = fdp.ConsumeIntegral<pid_t>();
    uint32_t tokenId2 = fdp.ConsumeIntegral<uint32_t>();
    int32_t windowId2 = fdp.ConsumeIntegral<int32_t>();
    DisposableType type2 = static_cast<DisposableType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    uint32_t maxLen2 = fdp.ConsumeIntegral<uint32_t>();
    auto observer2 = fdp.ConsumeBool() ? nullptr : OHOS::sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info2(pid2, tokenId2, windowId2, type2, maxLen2, observer2);

    std::string text = fdp.ConsumeRandomLengthString();
    PasteData pasteData;
    pasteData.AddTextRecord(text);
    auto delayGetter = OHOS::sptr<DelayGetterImpl>::MakeSptr();
    delayGetter->text_ = text;
    auto entryGetter = OHOS::sptr<EntryGetterImpl>::MakeSptr();
    entryGetter->text_ = text;
    WindowManager::SetFocusWindowId(windowId);

    {
        std::lock_guard lock(DisposableManager::GetInstance().disposableInfoMutex_);
        DisposableManager::GetInstance().disposableInfoList_ = {info, info2};
    }
    DisposableManager::GetInstance().TryProcessDisposableData(pasteData, delayGetter, entryGetter);
}

void TestRemoveDisposableInfo(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    pid_t pid = fdp.ConsumeIntegral<pid_t>();
    uint32_t tokenId = fdp.ConsumeIntegral<uint32_t>();
    int32_t windowId = fdp.ConsumeIntegral<int32_t>();
    DisposableType type = static_cast<DisposableType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    uint32_t maxLen = fdp.ConsumeIntegral<uint32_t>();
    auto observer = fdp.ConsumeBool() ? nullptr : OHOS::sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info(pid, tokenId, windowId, type, maxLen, observer);

    pid_t pid2 = fdp.ConsumeIntegral<pid_t>();
    uint32_t tokenId2 = fdp.ConsumeIntegral<uint32_t>();
    int32_t windowId2 = fdp.ConsumeIntegral<int32_t>();
    DisposableType type2 = static_cast<DisposableType>(fdp.ConsumeIntegralInRange<uint32_t>(0, MAX_ENUM_VALUE));
    uint32_t maxLen2 = fdp.ConsumeIntegral<uint32_t>();
    auto observer2 = fdp.ConsumeBool() ? nullptr : OHOS::sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info2(pid2, tokenId2, windowId2, type2, maxLen2, observer2);

    {
        std::lock_guard lock(DisposableManager::GetInstance().disposableInfoMutex_);
        DisposableManager::GetInstance().disposableInfoList_ = {info, info2};
    }
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, fdp.ConsumeBool());
}
} // anonymous namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static TestEnv env;
    env.SetUp();

    TestAddDisposableInfo(data, size);
    TestTryProcessDisposableData(data, size);
    TestRemoveDisposableInfo(data, size);

    env.TearDown();
    return 0;
}