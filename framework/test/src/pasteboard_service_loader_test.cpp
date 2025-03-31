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
bool TLVWriteable::Encode(std::vector<uint8_t> &buffer) const
{
    PasteboardServiceLoaderInterface *interface = GetPasteboardServiceLoaderInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->Encode(buffer);
}
}
static bool g_adddeathrecipient = false;
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
        return g_adddeathrecipient ;
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
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardServiceProxy();
    EXPECT_EQ(ret, nullptr);
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
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(samgrProxy, nullptr);
    sptr<IRemoteObject> remoteObject = samgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    EXPECT_NE(remoteObject, nullptr);
    PasteboardServiceLoader::GetInstance().SetPasteboardServiceProxy(remoteObject);
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardService();
    EXPECT_NE(ret, nullptr);
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
    PasteboardServiceLoader::GetInstance().constructing_ = true;
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardService();
    EXPECT_NE(ret, nullptr);
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
    auto ret = PasteboardServiceLoader::GetInstance().GetPasteboardService();
    EXPECT_NE(ret, nullptr);
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
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(samgrProxy, nullptr);
    sptr<IRemoteObject> remoteObject = samgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    EXPECT_NE(remoteObject, nullptr);
    PasteboardServiceLoader::GetInstance().SetPasteboardServiceProxy(remoteObject);
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
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    EXPECT_NE(rObject, nullptr);
    g_adddeathrecipient = false;
    PasteboardServiceLoader::GetInstance().SetPasteboardServiceProxy(rObject);
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
    auto value = std::make_shared<PasteDataEntry>();
    NiceMock<PasteboardServiceLoaderInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(false));
    int32_t result = PasteboardServiceLoader::GetInstance().GetRecordValueByType(DATAID_TEST, RECORD_TEST, *value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
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
    auto value = std::make_shared<PasteDataEntry>();
    NiceMock<PasteboardServiceLoaderInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    int32_t result = PasteboardServiceLoader::GetInstance().GetRecordValueByType(DATAID_TEST, RECORD_TEST, *value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}
}