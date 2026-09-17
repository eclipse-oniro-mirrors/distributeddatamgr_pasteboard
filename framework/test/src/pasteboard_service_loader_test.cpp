/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <if_system_ability_manager.h>
#include <iservice_registry.h>

#include "message_parcel_warp.h"
#include "pasteboard_client_death_observer_stub.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_load_callback.h"
#include "pasteboard_samgr_listener.h"
#include "pasteboard_service_loader.h"
#include "system_ability_definition.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
namespace {
    const uint32_t DATAID_TEST = 1;
    const uint32_t RECORD_TEST = 1;
    const std::u16string DESCRIPTOR_TEST = u"test_descriptor";
}

class PasteboardServiceLoaderInterface {
public:
    PasteboardServiceLoaderInterface(){};
    virtual ~PasteboardServiceLoaderInterface(){};
    virtual bool Encode(std::vector<uint8_t> &buffer) const = 0;
};

class PasteboardServiceLoaderInterfaceMock : public PasteboardServiceLoaderInterface {
public:
    PasteboardServiceLoaderInterfaceMock();
    ~PasteboardServiceLoaderInterfaceMock() override;
    MOCK_CONST_METHOD1(Encode, bool(std::vector<uint8_t> &buffer));
};

static void *g_interface = nullptr;
static bool g_accountIds = false;

PasteboardServiceLoaderInterfaceMock::PasteboardServiceLoaderInterfaceMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardServiceLoaderInterfaceMock::~PasteboardServiceLoaderInterfaceMock()
{
    g_interface = nullptr;
}

static PasteboardServiceLoaderInterface *GetPasteboardServiceLoaderInterface()
{
    return reinterpret_cast<PasteboardServiceLoaderInterface*>(g_interface);
}

extern "C" {
bool TLVWriteable::Encode(std::vector<uint8_t> &buffer, bool isRemote) const
{
    (void)isRemote;
    PasteboardServiceLoaderInterface *interface = GetPasteboardServiceLoaderInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->Encode(buffer);
}
}
static bool g_addDeathRecipient = false;
class TestIRemoteObject : public IRemoteObject {
    TestIRemoteObject(): IRemoteObject(DESCRIPTOR_TEST) {}

    int32_t GetObjectRefCount()
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return 0;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return g_addDeathRecipient ;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }

    int Dump(int fd, const std::vector<std::u16string> &args)
    {
        return 0;
    }
};
class PasteboardServiceLoaderTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

void PasteboardServiceLoaderTest::SetUpTestCase(void){ }

void PasteboardServiceLoaderTest::TearDownTestCase(void){ }

void PasteboardServiceLoaderTest::SetUp(void) { }

void PasteboardServiceLoaderTest::TearDown(void) { }

/**
 * @tc.name: GetPasteboardServiceProxyTest001
 * @tc.desc: GetPasteboardServiceProxy is normal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, GetPasteboardServiceProxyTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceProxyTest001 start");
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardServiceProxy();
    EXPECT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceProxyTest001 end");
}

/**
 * @tc.name: GetPasteboardServiceTest001
 * @tc.desc: GetPasteboardService is normal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, GetPasteboardServiceTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceTest001 start");
    sptr<ISystemAbilityManager> saMgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(saMgrProxy, nullptr);
    sptr<IRemoteObject> remoteObject = saMgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    EXPECT_NE(remoteObject, nullptr);
    PasteboardServiceLoader::GetInstance().SetPasteboardServiceProxy(remoteObject);
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardService();
    EXPECT_NE(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceTest001 end");
}

/**
 * @tc.name: GetPasteboardServiceTest002
 * @tc.desc: constructing_ is true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, GetPasteboardServiceTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceTest002 start");
    PasteboardServiceLoader::GetInstance().constructing_ = true;
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardService();
    EXPECT_NE(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceTest002 end");
}

/**
 * @tc.name: GetPasteboardServiceTest003
 * @tc.desc: GetPasteboardService is normal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, GetPasteboardServiceTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceTest003 start");
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardService();
    EXPECT_NE(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardServiceTest003 end");
}

/**
 * @tc.name: SetPasteboardServiceProxyTest001
 * @tc.desc: SetPasteboardServiceProxy is normal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, SetPasteboardServiceProxyTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteboardServiceProxyTest001 start");
    sptr<ISystemAbilityManager> saMgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(saMgrProxy, nullptr);
    sptr<IRemoteObject> remoteObject = saMgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    EXPECT_NE(remoteObject, nullptr);
    PasteboardServiceLoader::GetInstance().SetPasteboardServiceProxy(remoteObject);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteboardServiceProxyTest001 end");
}

/**
 * @tc.name: SetPasteboardServiceProxyTest003
 * @tc.desc: AddDeathRecipient is fail
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, SetPasteboardServiceProxyTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteboardServiceProxyTest003 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    EXPECT_NE(rObject, nullptr);
    g_addDeathRecipient = false;
    PasteboardServiceLoader::GetInstance().SetPasteboardServiceProxy(rObject);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteboardServiceProxyTest003 end");
}

/**
 * @tc.name: ReleaseDeathRecipientTest001
 * @tc.desc: Release DeathRecipient
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ReleaseDeathRecipientTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReleaseDeathRecipientTest001 start");
    sptr<ISystemAbilityManager> saMgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(saMgrProxy, nullptr);
    sptr<IRemoteObject> remoteObject = saMgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    EXPECT_NE(remoteObject, nullptr);
    PasteboardServiceLoader::GetInstance().ReleaseDeathRecipient();
    EXPECT_TRUE(PasteboardServiceLoader::GetInstance().deathRecipient_ == nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReleaseDeathRecipientTest001 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest001
 * @tc.desc: GetRecordValueByType is return SERIALIZATION_ERROR
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, GetRecordValueByTypeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRecordValueByTypeTest001 start");
    auto value = std::make_shared<PasteDataEntry>();
    NiceMock<PasteboardServiceLoaderInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(false));
    int32_t result = PasteboardServiceLoader::GetInstance().GetRecordValueByType(DATAID_TEST, RECORD_TEST, *value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRecordValueByTypeTest001 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest002
 * @tc.desc: GetRecordValueByType is return INVALID_PARAM_ERROR
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, GetRecordValueByTypeTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRecordValueByTypeTest002 start");
    auto value = std::make_shared<PasteDataEntry>();
    NiceMock<PasteboardServiceLoaderInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    int32_t result = PasteboardServiceLoader::GetInstance().GetRecordValueByType(DATAID_TEST, RECORD_TEST, *value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRecordValueByTypeTest002 end");
}

/**
 * @tc.name: ProcessPasteDataTest001
 * @tc.desc: ProcessPasteData is ok
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ProcessPasteDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest001 start");
    std::vector<uint8_t> sendTLV(0);
    int64_t tlvSize = 1;
    auto mpw = std::make_shared<MessageParcelWarp>();
    auto value = std::make_shared<PasteDataEntry>();
    int fd = mpw->CreateTmpFd();
    int32_t result = PasteboardServiceLoader::GetInstance().ProcessPasteData(*value, tlvSize, fd, sendTLV);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    mpw->writeRawDataFd_ = -1;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest001 end");
}

/**
 * @tc.name: ProcessPasteDataTest002
 * @tc.desc: fd is -1
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ProcessPasteDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest002 start");
    std::vector<uint8_t> sendTLV(0);
    auto value = std::make_shared<PasteDataEntry>();
    int64_t tlvSize = 1;
    int fd = -1;
    int32_t result = PasteboardServiceLoader::GetInstance().ProcessPasteData(*value, tlvSize, fd, sendTLV);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest002 end");
}

/**
 * @tc.name: ProcessPasteDataTest003
 * @tc.desc: rawDataSize = 0
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ProcessPasteDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest003 start");
    auto value = std::make_shared<PasteDataEntry>();
    auto mpw = std::make_shared<MessageParcelWarp>();
    int64_t rawDataSize = 0;
    int fd = mpw->CreateTmpFd();
    std::vector<uint8_t> sendTLV(0);
    int32_t result = PasteboardServiceLoader::GetInstance().ProcessPasteData(*value, rawDataSize, fd, sendTLV);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    mpw->writeRawDataFd_ = -1;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest003 end");
}

/**
 * @tc.name: ProcessPasteDataTest004
 * @tc.desc: rawDataSize > DEFAULT_MAX_RAW_DATA_SIZE
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ProcessPasteDataTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest004 start");
    auto mpw = std::make_shared<MessageParcelWarp>();
    auto value = std::make_shared<PasteDataEntry>();
    int64_t rawDataSize = DEFAULT_MAX_RAW_DATA_SIZE + 1;
    int fd = mpw->CreateTmpFd();
    std::vector<uint8_t> sendTLV(0);
    int32_t result = PasteboardServiceLoader::GetInstance().ProcessPasteData(*value, rawDataSize, fd, sendTLV);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    mpw->writeRawDataFd_ = -1;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest004 end");
}

/**
 * @tc.name: ProcessPasteDataTest005
 * @tc.desc: rawDataSize < DEFAULT_MAX_RAW_DATA_SIZE
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ProcessPasteDataTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest005 start");
    auto mpw = std::make_shared<MessageParcelWarp>();
    auto value = std::make_shared<PasteDataEntry>();
    int64_t rawDataSize = DEFAULT_MAX_RAW_DATA_SIZE - 1;
    int fd = mpw->CreateTmpFd();
    std::vector<uint8_t> sendTLV(0);
    int32_t result = PasteboardServiceLoader::GetInstance().ProcessPasteData(*value, rawDataSize, fd, sendTLV);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    mpw->writeRawDataFd_ = -1;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ProcessPasteDataTest005 end");
}

/**
 * @tc.name: IsStaticDestroyedTest001
 * @tc.desc: IsStaticDestroyed returns false under normal conditions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, IsStaticDestroyedTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsStaticDestroyedTest001 start");
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsStaticDestroyedTest001 end");
}

/**
 * @tc.name: ClearPasteboardServiceProxyTest001
 * @tc.desc: ClearPasteboardServiceProxy clears proxy under normal conditions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ClearPasteboardServiceProxyTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ClearPasteboardServiceProxyTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().ClearPasteboardServiceProxy();
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ClearPasteboardServiceProxyTest001 end");
}

/**
 * @tc.name: ClearPasteboardServiceProxyGuardTest001
 * @tc.desc: ClearPasteboardServiceProxy early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, ClearPasteboardServiceProxyGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ClearPasteboardServiceProxyGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardServiceLoader::GetInstance().ClearPasteboardServiceProxy();
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ClearPasteboardServiceProxyGuardTest001 end");
}

/**
 * @tc.name: OnRemoteSaDiedGuardTest001
 * @tc.desc: OnRemoteSaDied early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnRemoteSaDiedGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteSaDiedGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    wptr<IRemoteObject> remote = nullptr;
    PasteboardServiceLoader::GetInstance().OnRemoteSaDied(remote);
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteSaDiedGuardTest001 end");
}

/**
 * @tc.name: LoadSystemAbilityFailGuardTest001
 * @tc.desc: LoadSystemAbilityFail early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, LoadSystemAbilityFailGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilityFailGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardServiceLoader::GetInstance().LoadSystemAbilityFail();
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilityFailGuardTest001 end");
}

/**
 * @tc.name: LoadSystemAbilitySuccessGuardTest001
 * @tc.desc: LoadSystemAbilitySuccess early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, LoadSystemAbilitySuccessGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilitySuccessGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    sptr<IRemoteObject> remoteObject = nullptr;
    PasteboardServiceLoader::GetInstance().LoadSystemAbilitySuccess(remoteObject);
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilitySuccessGuardTest001 end");
}

/**
 * @tc.name: OnRemoteDiedGuardTest001
 * @tc.desc: PasteboardSaDeathRecipient::OnRemoteDied early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnRemoteDiedGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteDiedGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardSaDeathRecipient recipient;
    wptr<IRemoteObject> remote = nullptr;
    recipient.OnRemoteDied(remote);
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteDiedGuardTest001 end");
}

/**
 * @tc.name: OnLoadSystemAbilitySuccessGuardTest001
 * @tc.desc: PasteboardLoadCallback::OnLoadSystemAbilitySuccess early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnLoadSystemAbilitySuccessGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilitySuccessGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardLoadCallback callback;
    sptr<IRemoteObject> remoteObject = nullptr;
    callback.OnLoadSystemAbilitySuccess(PASTEBOARD_SERVICE_ID, remoteObject);
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilitySuccessGuardTest001 end");
}

/**
 * @tc.name: OnLoadSystemAbilityFailGuardTest001
 * @tc.desc: PasteboardLoadCallback::OnLoadSystemAbilityFail early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnLoadSystemAbilityFailGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilityFailGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardLoadCallback callback;
    callback.OnLoadSystemAbilityFail(PASTEBOARD_SERVICE_ID);
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilityFailGuardTest001 end");
}

/**
 * @tc.name: OnRemoteSaDiedTest001
 * @tc.desc: OnRemoteSaDied normal path (IsDestroyed=false) clears proxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnRemoteSaDiedTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteSaDiedTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    wptr<IRemoteObject> remote = nullptr;
    PasteboardServiceLoader::GetInstance().OnRemoteSaDied(remote);
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteSaDiedTest001 end");
}

/**
 * @tc.name: LoadSystemAbilityFailTest001
 * @tc.desc: LoadSystemAbilityFail normal path (IsDestroyed=false) clears proxy and notifies
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, LoadSystemAbilityFailTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilityFailTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardServiceLoader::GetInstance().LoadSystemAbilityFail();
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilityFailTest001 end");
}

/**
 * @tc.name: LoadSystemAbilitySuccessTest001
 * @tc.desc: LoadSystemAbilitySuccess normal path (IsDestroyed=false) proceeds past guard
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, LoadSystemAbilitySuccessTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilitySuccessTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().deathRecipient_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    sptr<IRemoteObject> remoteObject = nullptr;
    PasteboardServiceLoader::GetInstance().LoadSystemAbilitySuccess(remoteObject);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "LoadSystemAbilitySuccessTest001 end");
}

/**
 * @tc.name: OnRemoteDiedTest001
 * @tc.desc: OnRemoteDied normal path (IsStaticDestroyed=false) delegates to OnRemoteSaDied
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnRemoteDiedTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteDiedTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardSaDeathRecipient recipient;
    wptr<IRemoteObject> remote = nullptr;
    recipient.OnRemoteDied(remote);
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteDiedTest001 end");
}

/**
 * @tc.name: OnLoadSystemAbilitySuccessTest001
 * @tc.desc: OnLoadSystemAbilitySuccess normal path (IsStaticDestroyed=false) delegates to LoadSystemAbilitySuccess
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnLoadSystemAbilitySuccessTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilitySuccessTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().deathRecipient_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardLoadCallback callback;
    sptr<IRemoteObject> remoteObject = nullptr;
    callback.OnLoadSystemAbilitySuccess(PASTEBOARD_SERVICE_ID, remoteObject);
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilitySuccessTest001 end");
}

/**
 * @tc.name: OnLoadSystemAbilityFailTest001
 * @tc.desc: OnLoadSystemAbilityFail normal path (IsStaticDestroyed=false) delegates to LoadSystemAbilityFail
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnLoadSystemAbilityFailTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilityFailTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardLoadCallback callback;
    callback.OnLoadSystemAbilityFail(PASTEBOARD_SERVICE_ID);
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnLoadSystemAbilityFailTest001 end");
}

/**
 * @tc.name: OnAddSystemAbilityGuardTest001
 * @tc.desc: PasteboardSaMgrListener::OnAddSystemAbility early-returns when static is destroyed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnAddSystemAbilityGuardTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnAddSystemAbilityGuardTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = true;
    EXPECT_TRUE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardSaMgrListener listener;
    listener.hasDied_ = false;
    listener.OnAddSystemAbility(PASTEBOARD_SERVICE_ID, "");
    PasteboardServiceLoader::staticDestroyMonitor_.destroyed_ = false;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnAddSystemAbilityGuardTest001 end");
}

/**
 * @tc.name: OnAddSystemAbilityTest001
 * @tc.desc: OnAddSystemAbility normal path (IsStaticDestroyed=false) clears proxy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceLoaderTest, OnAddSystemAbilityTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnAddSystemAbilityTest001 start");
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    EXPECT_FALSE(PasteboardServiceLoader::IsStaticDestroyed());
    PasteboardSaMgrListener listener;
    listener.hasDied_ = false;
    listener.OnAddSystemAbility(PASTEBOARD_SERVICE_ID, "");
    EXPECT_EQ(PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnAddSystemAbilityTest001 end");
}
}
