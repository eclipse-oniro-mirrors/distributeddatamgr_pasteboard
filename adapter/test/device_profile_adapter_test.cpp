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

} // namespace OHOS::MiscServices