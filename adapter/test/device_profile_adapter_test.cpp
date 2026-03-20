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
#include <unistd.h>

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

constexpr const char *STATUS_ENABLE = "1";
constexpr const char *STATUS_DISABLE = "0";

static IDeviceProfileAdapter::OnProfileUpdateCallback g_testCallback = nullptr;

class TestSubscribeDPChangeListener : public ProfileChangeListenerStub {
public:
    TestSubscribeDPChangeListener() = default;
    ~TestSubscribeDPChangeListener() = default;

    int32_t OnCharacteristicProfileUpdate(const CharacteristicProfile &oldProfile,
        const CharacteristicProfile &newProfile) override
    {
        (void)oldProfile;
        std::string udid = newProfile.GetDeviceId();
        std::string status = newProfile.GetCharacteristicValue();
        if (g_testCallback != nullptr) {
            std::thread thread(g_testCallback, udid, status == STATUS_ENABLE);
            PasteBoardCommonUtils::SetThreadTaskName(thread, "OnProfileUpdate");
            thread.detach();
        }
        return ERR_OK;
    }
};

class AdapterDeviceProfileAdapterTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        sptr<ISystemAbilityManager> samgr = sptr<SystemAbilityManager>::MakeSptr();
        SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(samgr);
    }

    static void TearDownTestCase()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

class DistributedDeviceProfileStub : public IRemoteStub<IDistributedDeviceProfile> {
public:
    int32_t PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile) override
    {
        (void)characteristicProfile;
        return ERR_OK;
    }

    int32_t GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile) override
    {
        (void)deviceId;
        (void)serviceName;
        (void)characteristicId;
        (void)characteristicProfile;
        return ERR_OK;
    }

    int32_t SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) override
    {
        (void)subscribeInfo;
        return ERR_OK;
    }

    int32_t UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) override
    {
        (void)subscribeInfo;
        return ERR_OK;
    }

    int32_t SendSubscribeInfos(std::map<std::string, SubscribeInfo> listenerMap) override
    {
        (void)listenerMap;
        return ERR_OK;
    }
};

int32_t LoadSystemAbilitySuccImpl(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback)
{
    std::thread thread([=] {
        if (callback == nullptr) {
            return;
        }
        sptr<IRemoteObject> remoteObject = new DistributedDeviceProfileStub();
        callback->OnLoadSystemAbilitySuccess(systemAbilityId, remoteObject);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "LoadSaSucc");
    thread.detach();
    return ERR_OK;
}

/**
 * @tc.name: TestRegisterUpdateCallback001
 * @tc.desc: test RegisterUpdateCallback with valid callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback001 start");
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    auto callback = [](const std::string &udid, bool status) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "callback called: udid=%{public}s, status=%{public}d",
            udid.c_str(), status);
    };
    
    int32_t ret = adapter->RegisterUpdateCallback(callback);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback001 end");
}

/**
 * @tc.name: TestRegisterUpdateCallback002
 * @tc.desc: test RegisterUpdateCallback with null callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback002 start");
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    int32_t ret = adapter->RegisterUpdateCallback(nullptr);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback002 end");
}

/**
 * @tc.name: TestPutDeviceStatus001
 * @tc.desc: test PutDeviceStatus with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus001 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_001";
    bool status = true;
    
    int32_t ret = adapter->PutDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus001 end");
}

/**
 * @tc.name: TestPutDeviceStatus002
 * @tc.desc: test PutDeviceStatus with false status
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_002";
    bool status = false;
    
    int32_t ret = adapter->PutDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 end");
}

/**
 * @tc.name: TestGetDeviceStatus001
 * @tc.desc: test GetDeviceStatus with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_003";
    bool status = false;
    
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 end");
}

/**
 * @tc.name: TestGetDeviceVersion001
 * @tc.desc: test GetDeviceVersion with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_004";
    uint32_t versionId = 0;
    
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_TRUE(ret);
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent001
 * @tc.desc: test SubscribeProfileEvent with valid parameters
 * @tc.type: {FUNC}
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_005";
    
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent002
 * @tc.desc: test SubscribeProfileEvent with duplicate udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_006";
    
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent001
 * @tc.desc: test UnSubscribeProfileEvent with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_007";
    
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent002
 * @tc.desc: test UnSubscribeProfileEvent with non-existent udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 start");
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_008";
    
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestSendSubscribeInfos001
 * @tc.desc: test SendSubscribeInfos
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSendSubscribeInfos001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos001 start");
    
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    std::string udid = "test_udid_009";
    
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    adapter->SendSubscribeInfos();
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos001 end");
}

/**
 * @tc.name: TestClearDeviceProfileService001
 * @tc.desc: test ClearDeviceProfileService
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestClearDeviceProfileService001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestClearDeviceProfileService001 start");
    
    IDeviceProfileAdapter *adapter = GetDeviceProfileAdapter();
    ASSERT_NE(adapter, nullptr);
    
    adapter->ClearDeviceProfileService();
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestClearDeviceProfileService001 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate001
 * @tc.desc: test OnCharacteristicProfileUpdate with enabled status
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate001 start");
    
    std::string testUdid = "";
    bool testStatus = false;
    bool callbackCalled = false;
    
    auto callback = [&](const std::string &udid, bool status) {
        callbackCalled = true;
        testUdid = udid;
        testStatus = status;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "callback called: udid=%{public}s, status=%{public}d",
            udid.c_str(), status);
    };
    
    g_testCallback = callback;
    
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_udid_010");
    newProfile.SetCharacteristicValue("1");
    
    TestSubscribeDPChangeListener listener;
    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    sleep(1);
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(testUdid, "test_udid_010");
    EXPECT_TRUE(testStatus);
    
    g_testCallback = nullptr;
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate001 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate002
 * @tc.desc: test OnCharacteristicProfileUpdate with disabled status
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate002 start");
    
    std::string testUdid = "";
    bool testStatus = true;
    bool callbackCalled = false;
    
    auto callback = [&](const std::string &udid, bool status) {
        callbackCalled = true;
        testUdid = udid;
        testStatus = status;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "callback called: udid=%{public}s, status=%{public}d",
            udid.c_str(), status);
    };
    
    g_testCallback = callback;
    
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_udid_011");
    newProfile.SetCharacteristicValue("0");
    
    TestSubscribeDPChangeListener listener;
    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    sleep(1);
    
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(testUdid, "test_udid_011");
    EXPECT_FALSE(testStatus);
    
    g_testCallback = nullptr;
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate002 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate003
 * @tc.desc: test OnCharacteristicProfileUpdate without callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate003 start");
    
    g_testCallback = nullptr;
    
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_udid_012");
    newProfile.SetCharacteristicValue("1");
    
    TestSubscribeDPChangeListener listener;
    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate003 end");
}

} // namespace OHOS::MiscServices
