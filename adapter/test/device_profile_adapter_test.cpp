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

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <mutex>

#include "device_profile_adapter.h"
#include "device_profile_client_mock.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace OHOS::DistributedDeviceProfile;
using namespace testing::ext;

static std::mutex g_callbackMutex;
static std::string g_callbackUdid;
static bool g_callbackStatus;

class AdapterDeviceProfileAdapterTest : public testing::Test {
public:
    void SetUp()
    {
        adapter = GetDeviceProfileAdapter();
        g_callbackUdid = "";
        g_callbackStatus = false;
        mockClient = new MockDeviceProfileClient();
    }

    void TearDown()
    {
        DeinitDeviceProfileAdapter();
        g_onProfileUpdateCallback = nullptr;
    }

    IDeviceProfileAdapter *adapter;
    MockDeviceProfileClient *mockClient;
};

static void TestProfileUpdateCallback(const std::string &testUdid, bool status)
{
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    g_callbackUdid = testUdid;
    g_callbackStatus = status;
}

/**
 * @tc.name: TestRegisterUpdateCallback001
 * @tc.desc: test RegisterUpdateCallback with valid callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback001 start");
    int32_t ret = adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
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
    int32_t ret = adapter->RegisterUpdateCallback(nullptr);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback002 end");
}

/**
 * @tc.name: TestRegisterUpdateCallback003
 * @tc.desc: test RegisterUpdateCallback multiple times
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback003 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    int32_t ret = adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback003 end");
}

/**
 * @tc.name: TestPutDeviceStatus001
 * @tc.desc: test PutDeviceStatus with status true
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus001 start");
    EXPECT_CALL(*mockClient, PutCharacteristicProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->PutDeviceStatus(udid, true);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus001 end");
}

/**
 * @tc.name: TestPutDeviceStatus002
 * @tc.desc: test PutDeviceStatus with status false
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 start");
    EXPECT_CALL(*mockClient, PutCharacteristicProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    
    std::string udid = "test_device_002";
    int32_t ret = adapter->PutDeviceStatus(udid, false);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 end");
}

/**
 * @tc.name: TestPutDeviceStatus003
 * @tc.desc: test PutDeviceStatus with DP_CACHE_EXIST
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus003 start");
    EXPECT_CALL(*mockClient, PutCharacteristicProfile(testing::_))
        .WillOnce(testing::Return(DP_CACHE_EXIST));
    
    std::string udid = "test_device_003";
    int32_t ret = adapter->PutDeviceStatus(udid, true);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus003 end");
}

/**
 * @tc.name: TestPutDeviceStatus004
 * @tc.desc: test PutDeviceStatus with error return
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus004 start");
    EXPECT_CALL(*mockClient, PutCharacteristicProfile(testing::_))
        .WillOnce(testing::Return(DP_GET_SERVICE_FAILED));
    
    std::string udid = "test_device_004";
    int32_t ret = adapter->PutDeviceStatus(udid, true);
    EXPECT_EQ(ret, DP_GET_SERVICE_FAILED);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus004 end");
}

/**
 * @tc.name: TestGetDeviceStatus001
 * @tc.desc: test GetDeviceStatus success
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 start");
    CharacteristicProfile mockProfile;
    mockProfile.SetCharacteristicValue("1");
    
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::DoAll([&mockProfile](const std::string &deviceId, 
            const std::string &serviceName, const std::string &characteristicId, 
            CharacteristicProfile &profile) {
            profile.SetDeviceId(deviceId);
            profile.SetServiceName(serviceName);
            profile.SetCharacteristicKey(characteristicId);
            profile.SetCharacteristicValue(mockProfile.GetCharacteristicValue());
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_001";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_TRUE(status);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 end");
}

/**
 * @tc.name: TestGetDeviceStatus002
 * @tc.desc: test GetDeviceStatus with error return
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus002 start");
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(DP_GET_SERVICE_FAILED));
    
    std::string udid = "test_device_002";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, DP_GET_SERVICE_FAILED);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus002 end");
}

/**
 * @tc.name: TestGetDeviceVersion001
 * @tc.desc: test GetDeviceVersion success
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 start");
    CharacteristicProfile mockProfile;
    mockProfile.SetCharacteristicValue("{\"PasteboardVersionId\":1}");
    
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_))
        .WillOnce(testing::DoAll([&mockProfile](const std::string &deviceId, 
            const std::string &serviceName, const std::string &characteristicId, 
            CharacteristicProfile &profile) {
            profile.SetDeviceId(deviceId);
            profile.SetServiceName(serviceName);
            profile.SetCharacteristicKey(characteristicId);
            profile.SetCharacteristicValue(mockProfile.GetCharacteristicValue());
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_001";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(versionId, 1u);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 end");
}

/**
 * @tc.name: TestGetDeviceVersion002
 * @tc.desc: test GetDeviceVersion with empty json
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion002 start");
    CharacteristicProfile mockProfile;
    mockProfile.SetCharacteristicValue("");
    
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_))
        .WillOnce(testing::DoAll([&mockProfile](const std::string &deviceId, 
            const std::string &serviceName, const std::string &characteristicId, 
            CharacteristicProfile &profile) {
            profile.SetDeviceId(deviceId);
            profile.SetServiceName(serviceName);
            profile.SetCharacteristicKey(characteristicId);
            profile.SetCharacteristicValue(mockProfile.GetCharacteristicValue());
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_002";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion002 end");
}

/**
 * @tc.name: TestGetDeviceVersion003
 * @tc.desc: test GetDeviceVersion with version not found
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion003 start");
    CharacteristicProfile mockProfile;
    mockProfile.SetCharacteristicValue("{}");
    
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_))
        .WillOnce(testing::DoAll([&mockProfile](const std::string &deviceId, 
            const std::string &serviceName, const std::string &characteristicId, 
            CharacteristicProfile &profile) {
            profile.SetDeviceId(deviceId);
            profile.SetServiceName(serviceName);
            profile.SetCharacteristicKey(characteristicId);
            profile.SetCharacteristicValue(mockProfile.GetCharacteristicValue());
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_003";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion003 end");
}

/**
 * @tc.name: TestGetDeviceVersion004
 * @tc.desc: test GetDeviceVersion with negative version
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion004 start");
    CharacteristicProfile mockProfile;
    mockProfile.SetCharacteristicValue("{\"PasteboardVersionId\":-1}");
    
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_))
        .WillOnce(testing::DoAll([&mockProfile](const std::string &deviceId, 
            const std::string &serviceName serviceName, const std::string &characteristicId, 
            CharacteristicProfile &profile) {
            profile.SetDeviceId(deviceId);
            profile.SetServiceName(serviceName);
            profile.SetCharacteristicKey(characteristicId);
            profile.SetCharacteristicValue(mockProfile.GetCharacteristicValue());
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_004";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion004 end");
}

/**
 * @tc.name: TestGetDeviceVersion005
 * @tc.desc: test GetDeviceVersion with error return
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion005 start");
    EXPECT_CALL(*mockClient, GetCharacteristicProfile(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(DP_GET_SERVICE_FAILED));
    
    std::string udid = "test_device_005";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion005 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent001
 * @tc.desc: test SubscribeProfileEvent with first subscription
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::DoAll([](const SubscribeInfo &subscribeInfo) {
            auto listener = subscribeInfo.GetListener();
            if (listener != nullptr) {
                CharacteristicProfile oldProfile;
                CharacteristicProfile newProfile;
                newProfile.SetDeviceId(subscribeInfo.GetSubscribeKey().substr(0, subscribeInfo.GetSubscribeKey().find('#')));
                newProfile.SetCharacteristicValue("1");
                listener->OnCharacteristicProfileUpdate(oldProfile, newProfile);
            }
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent002
 * @tc.desc: test SubscribeProfileEvent with duplicate subscription
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    int32_t ret2 = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_EQ(ret2, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent003
 * @tc.desc: test SubscribeProfileEvent with SubscribeDeviceProfile return error
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent003 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_GET_SERVICE_FAILED));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, DP_GET_SERVICE_FAILED);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent003 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent004
 * @tc.desc: test SubscribeProfileEvent with CHAR_PROFILE_ADD callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent004 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::DoAll([](const SubscribeInfo &subscribeInfo) {
            auto listener = subscribeInfo.GetListener();
            if (listener != nullptr) {
                CharacteristicProfile oldProfile;
                CharacteristicProfile newProfile;
                newProfile.SetDeviceId(subscribeInfo.GetSubscribeKey().substr(0, subscribeInfo.GetSubscribeKey().find('#')));
                newProfile.SetCharacteristicValue("1");
                listener->OnCharacteristicProfileAdd(oldProfile);
            }
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent004 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent005
 * @tc.desc: test SubscribeProfileEvent with CHAR_PROFILE_DELETE callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent005 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::DoAll([](const SubscribeInfo &subscribeInfo) {
            auto listener = subscribeInfo.GetListener();
            if (listener != nullptr) {
                CharacteristicProfile oldProfile;
                CharacteristicProfile newProfile;
                newProfile.SetDeviceId(subscribeInfo.GetSubscribeKey().substr(0, subscribeInfo.GetSubscribeKey().find('#')));
                newProfile.SetCharacteristicValue("1");
                listener->OnCharacteristicProfileDelete(oldProfile);
            }
            return DP_SUCCESS;
        }));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent005 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent001
 * @tc.desc: test UnSubscribeProfileEvent with valid udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent001, TestSize.Level1)
{
    P PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    EXPECT_CALL(*mockClient, UnSubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    int32_t ret2 = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_EQ(ret2, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent002
 * @tc.desc: test UnSubscribeProfileEvent with not exist udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 start");
    EXPECT_CALL(*mockClient, UnSubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    
    std::string udid = "test_device_002";
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent003
 * @tc.desc: test UnSubscribeProfileEvent with UnSubscribeDeviceProfile return error
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent003 start");
    EXPECT_CALL(*mockClient, SubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_SUCCESS));
    EXPECT_CALL(*mockClient, UnSubscribeDeviceProfile(testing::_))
        .WillOnce(testing::Return(DP_GET_SERVICE_FAILED));
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    int32_t ret2 = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_EQ(ret2, DP_GET_SERVICE_FAILED);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent003 end");
}

/**
 * @tc.name: TestSendSubscribeInfos001
 * @tc.desc: test SendSubscribeInfos with normal case
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSendSubscribeInfos001, Test::Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos001 start");
    EXPECT_CALL(*mockClient, SendSubscribeInfos());
    
    adapter->SendSubscribeInfos();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos001 end");
}

/**
 * @tc.name: TestSendSubscribeInfos002
 * @tc.desc: test SendSubscribeInfos with empty subscribe list
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSendSubscribeInfos002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSendSubscribeInfos002 start");
    EXPECT_CALL(*mockClient, SendSubscribeInfos());
    
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
    EXPECT_CALL(*mockClient, ClearDeviceProfileService());
    
    adapter->ClearDeviceProfileService();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestClearDeviceProfileService001 end");
}

/**
 * @tc.name: TestSubscribeDPChangeListener001
 * @tc.desc: test SubscribeDPChangeListener with callback registered
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeDPChangeListener001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeDPChangeListener001 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, udid);
    EXPECT_EQ(g_callbackStatus, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeDPChangeListener001 end");
}

/**
 * @tc.name: TestSubscribeDPChangeListener002
 * @tc.desc: test SubscribeDPChangeListener without callback registered
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeDPChangeListener002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeDPChangeListener002 start");
    
    std::string udid = "test_device_002";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeDPChangeListener002 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate001
 * @tc.desc: test OnCharacteristicProfileUpdate with callback and matching serviceId/characteristicId, status true
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate001 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_001";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, udid);
    EXPECT_EQ(g_callbackStatus, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate001 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate002
 * @tc.desc: test OnCharacteristicProfileUpdate with callback and matching serviceId/characteristicId, status false
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate002 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_002";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, udid);
    EXPECT_EQ(g_callbackStatus, false);
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
    std::string udid = "test_udid_003";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate003 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate004
 * @tc.desc: test OnCharacteristicProfileUpdate with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate004 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, udid);
    EXPECT_EQ(g_callbackStatus, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate004 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate005
 * @tc.desc: test OnCharacteristicProfileUpdate with status toggle
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdate005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate005 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);

    std::string udid = "test_udid_005";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    adapter->PutDeviceStatus(udid, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, udid);
    EXPECT_EQ(g_callbackStatus, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate005 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdateDirect001
 * @tc.desc: test SubscribeDPChangeListener::OnCharacteristicProfileUpdate with callback registered, status true
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdateDirect001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect001 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    g_onProfileUpdateCallback = TestProfileUpdateCallback;

    SubscribeDPChangeListener listener;
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_device_direct_001");
    newProfile.SetCharacteristicValue("1");

    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "test_device_direct_001");
    EXPECT_EQ(g_callbackStatus, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect001 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdateDirect002
 * @tc.desc: test SubscribeDPChangeListener::OnCharacteristicProfileUpdate with callback registered, status false
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdateDirect002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect002 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    g_onProfileUpdateCallback = TestProfileUpdateCallback;

    SubscribeDPChangeListener listener;
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_device_direct_002");
    newProfile.SetCharacteristicValue("0");

    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "test_device_direct_002");
    EXPECT_EQ(g_callbackStatus, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect002 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdateDirect003
 * @tc.desc: test SubscribeDPChangeListener::OnCharacteristicProfileUpdate without callback registered
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdateDirect003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect003 start");
    g_onProfileUpdateCallback = nullptr;

    SubscribeDPChangeListener listener;
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_device_direct_003");
    newProfile.SetCharacteristicValue("1");

    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect003 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdateDirect004
 * @tc.desc: test SubscribeDPChangeListener::OnCharacteristicProfileUpdate with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdateDirect004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect004 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    g_onProfileUpdateCallback = TestProfileUpdateCallback;

    SubscribeDPChangeListener listener;
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("");
    newProfile.SetCharacteristicValue("1");

    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "");
    EXPECT_EQ(g_callbackStatus, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect004 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdateDirect005
 * @tc.desc: test SubscribeDPChangeListener::OnCharacteristicProfileUpdate with invalid status value
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdateDirect005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect005 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    g_onProfileUpdateCallback = TestProfileUpdateCallback;

    SubscribeDPChangeListener listener;
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_device_direct_005");
    newProfile.SetCharacteristicValue("invalid");

    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "test_device_direct_005");
    EXPECT_EQ(g_callbackStatus, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect005 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdateDirect006
 * @tc.desc: test SubscribeDPChangeListener::OnCharacteristicProfileUpdate with empty status value
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristicProfileUpdateDirect006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect006 start");
    adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
    g_onProfileUpdateCallback = TestProfileUpdateCallback;

    SubscribeDPChangeListener listener;
    CharacteristicProfile oldProfile;
    CharacteristicProfile newProfile;
    newProfile.SetDeviceId("test_device_direct_006");
    newProfile.SetCharacteristicValue("");

    int32_t ret = listener.OnCharacteristicProfileUpdate(oldProfile, newProfile);
    EXPECT_EQ(ret, ERR_OK);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_callbackUdid, "test_device_direct_006");
    EXPECT_EQ(g_callbackStatus, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdateDirect006 end");
}

/**
 * @tc.name: TestRegisterUpdateCallback001
 * @tc.desc: test RegisterUpdateCallback with valid callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestRegisterUpdateCallback001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestRegisterUpdateCallback001 start");
    int32_t ret = adapter->RegisterUpdateCallback(TestProfileUpdateCallback);
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
    int32_t ret = adapter->RegisterUpdateCallback(nullptr);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
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
    std::string udid = "test_device_001";
    int32_t ret = adapter->PutDeviceStatus(udid, true);
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
    std::string udid = "test_device_002";
    int32_t ret = adapter->PutDeviceStatus(udid, false);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus002 end");
}

/**
 * @tc.name: TestPutDeviceStatus003
 * @tc.desc: test PutDeviceStatus with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestPutDeviceStatus003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus003 start");
    std::string udid = "";
    int32_t ret = adapter->PutDeviceStatus(udid, true);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestPutDeviceStatus003 end");
}

/**
 * @tc.name: TestGetDeviceStatus001
 * @tc.desc: test GetDeviceStatus with valid udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 start");
    std::string udid = "test_device_001";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus001 end");
}

/**
 * @tc.name: TestGetDeviceStatus002
 * @tc.desc: test GetDeviceStatus with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus002 start");
    std::string udid = "";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceStatus002 end");
}

/**
 * @tc.name: TestGetDeviceVersion001
 * @tc.desc: test GetDeviceVersion with valid udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 start");
    std::string udid = "test_device_001";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion001 end");
}

/**
 * @tc.name: TestGetDeviceVersion002
 * @tc.desc: test GetDeviceVersion with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestGetDeviceVersion002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion002 start");
    std::string udid = "";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestGetDeviceVersion002 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent001
 * @tc.desc: test SubscribeProfileEvent with valid udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 start");
    std::string udid = "test_device_001";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestSubscribeProfileEvent002
 * @tc.desc: test SubscribeProfileEvent with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 start");
    std::string udid = "";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestSubscribeProfileEvent002 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent001
 * @tc.desc: test UnSubscribeProfileEvent with valid udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 start");
    std::string udid = "test_device_001";
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent001 end");
}

/**
 * @tc.name: TestUnSubscribeProfileEvent002
 * @tc.desc: test UnSubscribeProfileEvent with empty udid
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestUnSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestUnSubscribeProfileEvent002 start");
    std::string udid = "";
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
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
    adapter->ClearDeviceProfileService();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestClearDeviceProfileService001 end");
}

} // namespace OHOS::MiscServices