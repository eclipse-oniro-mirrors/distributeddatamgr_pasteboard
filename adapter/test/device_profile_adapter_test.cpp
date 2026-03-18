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
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
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
    }

    void TearDown()
    {
        DeinitDeviceProfileAdapter();
        g_onProfileUpdateCallback = nullptr;
    }

    IDeviceProfileAdapter *adapter;
};

static void TestProfileUpdateCallback(const std::string &testUdid, bool status)
{
    std::lock_guard<std::mutex> lock(g_callbackMutex);
    g_callbackUdid = testUdid;
    g_callbackStatus = status;
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

} // namespace OHOS::MiscServices