/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include <thread>
#include <chrono>
#include <mutex>

#include "device_profile_adapter.h"
#include "device_profile_client.h"
#include "distributed_device_profile_errors.h"
#include "distributed_device_profile_proxy.h"
#include "iservice_registry.h"
#include "common/pasteboard_common_utils.h"
#include "system_ability_definition.h"
#include "system_ability_manager_mock.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace OHOS::DistributedDeviceProfile;
using namespace testing::ext;
using testing::NiceMock;

constexpr const char *SWITCH_ID = "SwitchSwitchStatus_Key_Distributed_Pasteboard";
constexpr const char *CHARACTER_ID = "SwitchStatus";
constexpr const char *SERVICE_ID = "pasteboardService";
constexpr const char *STATIC_CHARACTER_ID = "static_capability";
constexpr const char *VERSION_ID = "PasteboardVersionId";

static std::mutex g_callbackMutex;
static std::string g_callbackUdid;
static bool g_callbackStatus;

class MockDistributedDeviceProfileStub : public IRemoteStub<IDistributedDeviceProfile> {
public:
    int32_t PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile) override
    {
        (void)characteristicProfile;
        return putCharacteristicProfileRet_;
    }

    int32_t GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile) override
    {
        (void)deviceId;
        (void)serviceName;
        (void)characteristicId;
        if (getCharacteristicProfileRet_ != DP_SUCCESS) {
            return getCharacteristicProfileRet_;
        }
        characteristicProfile = mockProfile_;
        return DP_SUCCESS;
    }

    int32_t SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) override
    {
        (void)subscribeInfo;
        return subscribeDeviceProfileRet_;
    }

    int32_t UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) override
    {
        (void)subscribeInfo;
        return unSubscribeDeviceProfileRet_;
    }

    int32_t SendSubscribeInfos(std::map<std::string, SubscribeInfo> listenerMap) override
    {
        (void)listenerMap;
        return ERR_OK;
    }

    int32_t putCharacteristicProfileRet_ = DP_SUCCESS;
    int32_t getCharacteristicProfileRet_ = DP_SUCCESS;
    int32_t subscribeDeviceProfileRet_ = DP_SUCCESS;
    int32_t unSubscribeDeviceProfileRet_ = DP_SUCCESS;
    CharacteristicProfile mockProfile_;
};

static sptr<MockDistributedDeviceProfileStub> g_mockStub;

class AdapterDeviceProfileAdapterTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        sptr<ISystemAbilityManager> samgr = sptr<SystemAbilityManager>::MakeSptr();
        SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(samgr);
        g_mockStub = new MockDistributedDeviceProfileStub();
        sptr<IRemoteObject> remoteObject = g_mockStub;
        DeviceProfileClient::GetInstance().dpProxy_ = iface_cast<IDistributedDeviceProfile>(remoteObject);
    }

    static void TearDownTestCase()
    {
        DeviceProfileClient::GetInstance().ClearDeviceProfileService();
    }

    void SetUp()
    {
        adapter = GetDeviceProfileAdapter();
        g_mockStub->putCharacteristicProfileRet_ = DP_SUCCESS;
        g_mockStub->getCharacteristicProfileRet_ = DP_SUCCESS;
        g_mockStub->subscribeDeviceProfileRet_ = DP_SUCCESS;
        g_mockStub->unSubscribeDeviceProfileRet_ = DP_SUCCESS;
    }

    void TearDown()
    {
        DeinitDeviceProfileAdapter();
    }

    IDeviceProfileAdapter *adapter;
};

static void TestProfileUpdateCallback(const std::string &udid, bool status)
{
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    g_callbackUdid = udid;
    g_callbackStatus = status;
}

/**
 * @tc.name: TestRegisterUpdateCallback001
 * @tc.desc: test RegisterUpdateCallback with null callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback001 start");
    int32_t ret = adapter->RegisterUpdateCallback(nullptr);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback001 end");
}

/**
 * @tc.name: TestRegisterUpdateCallback002
 * @tc.desc: test RegisterUpdateCallback with valid callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback002 start");
    int32_t ret = adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback002 end");
}

/**
 * @tc.name: TestPutDeviceStatus001
 * @tc.desc: test PutDeviceStatus with success
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus001 start");
    std::string udid = "test_udid_001";
    int32_t ret = adapter->PutDeviceStatus(udid, true);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus001 end");
}

/**
 * @tc.name: TestPutDeviceStatus002
 * @tc.desc: test PutDeviceStatus with DP_CACHE_EXIST
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 start");
    std::string udid = "test_udid_002";
    g_mockStub->putCharacteristicProfileRet_ = DP_CACHE_EXIST;
    int32_t ret = adapter->PutDeviceStatus(udid, false);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 end");
}

/**
 * @tc.name: TestPutDeviceStatus003
 * @tc.desc: test PutDeviceStatus with DP_INVALID_PARAMS
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus003 start");
    std::string udid = "test_udid_003";
    g_mockStub->putCharacteristicProfileRet_ = DP_INVALID_PARAMS;
    int32_t ret = adapter->PutDeviceStatus(udid, true);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus003 end");
}

/**
 * @tc.name: TestPutDeviceStatus004
 * @tc.desc: test PutDeviceStatus with DP_DB_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus004 start");
    std::string udid = "test_udid_004";
    g_mockStub->putCharacteristicProfileRet_ = DP_DB_ERROR;
    int32_t ret = adapter->PutDeviceStatus(udid, false);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus004 end");
}

/**
 * @tc.name: TestGetDeviceStatus001
 * @tc.desc: test GetDeviceStatus with status true
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_005");
    g_mockStub->mockProfile_.SetServiceName(SWITCH_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("1");

    std::string udid = "test_udid_005";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_TRUE(status);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 end");
}

/**
 * @tc.name: TestGetDeviceStatus002
 * @tc.desc: test GetDeviceStatus with status false
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus002 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_006");
    g_mockStub->mockProfile_.SetServiceName(SWITCH_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("0");

    std::string udid = "test_udid_006";
    bool status = true;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_FALSE(status);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus002 end");
}

/**
 * @tc.name: TestGetDeviceStatus003
 * @tc.desc: test GetDeviceStatus with invalid value
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus003 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_007");
    g_mockStub->mockProfile_.SetServiceName(SWITCH_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("invalid_value");

    std::string udid = "test_udid_007";
    bool status = true;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_FALSE(status);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus003 end");
}

/**
 * @tc.name: TestGetDeviceStatus004
 * @tc.desc: test GetDeviceStatus with DP_INVALID_PARAMS
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus004 start");
    g_mockStub->getCharacteristicProfileRet_ = DP_INVALID_PARAMS;

    std::string udid = "test_udid_008";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus004 end");
}

/**
 * @tc.name: TestGetDeviceStatus005
 * @tc.desc: test GetDeviceStatus with DP_DB_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus005 start");
    g_mockStub->getCharacteristicProfileRet_ = DP_DB_ERROR;

    std::string udid = "test_udid_009";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus005 end");
}

/**
 * @tc.name: TestGetDeviceVersion001
 * @tc.desc: test GetDeviceVersion with valid version
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_010");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"PasteboardVersionId\": 123}");

    std::string udid = "test_udid_010";
    uint32_t versionId = 0;
    bool (ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(versionId, 123);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 end");
}

/**
 * @tc.name: TestGetDeviceVersion002
 * @tc.desc: test GetDeviceVersion with version 0
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion002 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_011");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"PasteboardVersionId\": 0}");

    std::string udid = "test_udid_011";
    uint32_t versionId = 999;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(versionId, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion002 end");
}

/**
 * @tc.name: TestGetDeviceVersion003
 * @tc.desc: test GetDeviceVersion with max uint32 value
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion003 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_012");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"PasteboardVersionId\": 4294967295}");

    std::string udid = "test_udid_012";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(versionId, 4294967295);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion003 end");
}

/**
 * @tc.name: TestGetDeviceVersion004
 * @tc.desc: test GetDeviceVersion with DP_INVALID_PARAMS
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion004 start");
    g_mockStub->getCharacteristicProfileRet_ = DP_INVALID_PARAMS;

    std::string udid = "test_udid_013";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion004 end");
}

/**
 * @tc.name: TestGetDeviceVersion005
 * @tc.desc: test GetDeviceVersion with invalid JSON
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion005 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_014");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("invalid json string");

    std::string udid = "test_udid_014";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion005 end");
}

/**
 * @tc.name: TestGetDeviceVersion006
 * @tc.desc: test GetDeviceVersion with empty JSON object
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion006 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_015");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{}");

    std::string udid = "test_udid_015";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion006 end");
}

/**
 * @tc.name: TestGetDeviceVersion007
 * @tc.desc: test GetDeviceVersion with non-number version
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion007 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_016");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"PasteboardVersionId\": \"not_a_number\"}");

    std::string uudid = "test_udid_016";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion007 end");
}

/**
 * @tc.name: TestGetDeviceVersion008
 * @tc.desc: test GetDeviceVersion with negative version -1
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion008 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_017");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"PasteboardVersionId\": -1}");

    std::string uudid = "test_udid_017";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion008 end");
}

/**
 * @tc.name: TestGetDeviceVersion009
 * @tc.desc: test GetDeviceVersion with negative version -100
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion009 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_018");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"PasteboardVersionId\": -100}");

    std::string udid = "test_udid_018";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion009 end");
}

/**
 * @tc.name: TestGetDeviceVersion010
 * @tc.desc: test GetDeviceVersion with missing version field
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion010 start");
    g_mockStub->mockProfile_.SetDeviceId("test_udid_019");
    g_mockStub->mockProfile_.SetServiceName(SERVICE_ID);
    g_mockStub->mockProfile_.SetCharacteristicKey(STATIC_CHARACTER_ID);
    g_mockStub->mockProfile_.SetCharacteristicValue("{\"OtherKey\": 123}");

    std::string udid = "test_udid_019";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion010 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent001
 * @tc.desc: test SubscribeProfileEvent with success
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 start");
    std::string udid = "test_udid_020";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent002
 * @tc.desc: test SubscribeProfileEvent with already subscribed
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 start");
    std::string udid = "test_udid_021";
    adapter->SubscribeProfileEvent(udid);
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent003
 * @tc.desc: test SubscribeProfileEvent with DP_INVALID_PARAMS
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent003 start");
    g_mockStub->subscribeDeviceProfileRet_ = DP_INVALID_PARAMS;

    std::string udid = "test_udid_022";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent003 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent004
 * @tc.desc: test SubscribeProfileEvent with DP_DB_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent004 start");
    g_mockStub->subscribeDeviceProfileRet_ = DP_DB_ERROR;

    std::string udid = "test_udid_023";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent004 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent001
 * @tc.desc: test UnSubscribeProfileEvent with success
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 start");
    std::string udid = "test_udid_024";
    adapter->SubscribeProfileEvent(udid);
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent002
 * @tc.desc: test UnSubscribeProfileEvent with not subscribed
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 start");
    std::string udid = "test_udid_025";
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent003
 * @tc.desc: test UnSubscribeProfileEvent with DP_INVALID_PARAMS
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent003 start");
    std::string udid = "test_udid_026";
    adapter->SubscribeProfileEvent(udid);
    g_mockStub->unSubscribeDeviceProfileRet_ = DP_INVALID_PARAMS;

    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent003 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent004
 * @tc.desc: test UnSubscribeProfileEvent with DP_DB_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent004 start");
    std::string udid = "test_udid_027";
    adapter->SubscribeProfileEvent(udid);
    g_mockStub->unSubscribeDeviceProfileRet_ = DP_DB_ERROR;

    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent004 end");
}

/**
 * @tc.name: TestSendSubscribeInfos001
 * @tc.desc: test SendSubscribeInfos with empty cache
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSendSubscribeInfos001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos001 start");
    adapter->SendSubscribeInfos();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos001 end");
}

/**
 * @tc.name: TestSendSubscribeInfos002
 * @tc.desc: test SendSubscribeInfos with non-empty cache
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSendSubscribeInfos002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos002 start");
    std::string udid = "test_udid_028";
    adapter->SubscribeProfileEvent(udid);
    adapter->SendSubscribeInfos();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos002 end");
}

/**
 * @tc.name: TestClearDeviceProfileService001
 * @tc.desc: test ClearDeviceProfileService
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestClearDeviceProfileService001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestClearDeviceProfileService001 start");
    adapter->ClearDeviceProfileService();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestClearDeviceProfileService001 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback001
 * @tc.desc: test OnProfileUpdateCallback with callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback001 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_029";
    adapter->SubscribeProfileEvent(udid);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback001 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback002
 * @tc.desc: test OnProfileUpdateCallback without callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback002 start");
    std::string udid = "test_udid_030";
    adapter->SubscribeProfileEvent(udid);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback002 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback003
 * @tc.desc: test OnProfileUpdateCallback with double register
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback003 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_031";
    adapter->SubscribeProfileEvent(udid);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback003 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback004
 * @tc.desc: test OnProfileUpdateCallback with multiple udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback004 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid1 = "test_udid_032";
    std::string udid2 = "test_udid_033";
    adapter->SubscribeProfileEvent(udid1);
    adapter->SubscribeProfileEvent(udid2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback004 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback005
 * @tc.desc: test OnProfileUpdateCallback with unsubscribe
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback005 start");
    std::string udid = "test_udid_034";
    adapter->SubscribeProfileEvent(udid);
    adapter->UnSubscribeProfileEvent(udid);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback005 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback006
 * @tc.desc: test OnProfileUpdateCallback with status true
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback006 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_035";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback006 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback007
 * @tc.desc: test OnProfileUpdateCallback with status false
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback007 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_036";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback007 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback008
 * @tc.desc: test OnProfileUpdateCallback with status toggle
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback008 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_037";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    adapter->PutDeviceStatus(udid, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback008 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback009
 * @tc.desc: test OnProfileUpdateCallback without callback and update
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback009 start");
    std::string udid = "test_udid_038";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback009 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback010
 * @tc.desc: test OnProfileUpdateCallback with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback010 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback010 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback011
 * @tc.desc: test OnProfileUpdateCallback with unsubscribe and update
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback011, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback011 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_039";
    adapter->SubscribeProfileEvent(udid);
    adapter->UnSubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback011 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback012
 * @tc.desc: test OnProfileUpdateCallback with SendSubscribeInfos before update
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback012, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback012 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_040";
    adapter->SubscribeProfileEvent(udid);
    adapter->SendSubscribeInfos();
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback012 end");
}

/**
 * @tc.name: TestOnProfileUpdateCallback013
 * @tc.desc: test OnProfileUpdateCallback with SendSubscribeInfos after update
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnProfileUpdateCallback013, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback013 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_041";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    adapter->SendSubscribeInfos();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnProfileUpdateCallback013 end");
}

} // namespace OHOS::MiscServices
