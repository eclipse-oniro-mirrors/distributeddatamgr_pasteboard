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

constexpr const char *SWITCH_ID = "SwitchStatus_Key_Distributed_Pasteboard";
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
);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate002 end");
}

/**
 * @tc.name: TestOnCharacteristicProfileUpdate003
 * @tc.desc: test OnCharacteristicProfileUpdate without callback
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileAdapterTest, TestOnCharacteristic characteristicUpdate003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate003 start");
    std::string udid = "test_udid_003";
    adapter->SubscribeProfileEvent(udid);
    adapter->PutDeviceStatus(udid, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "TestOnCharacteristicProfileUpdate005 end");
}

} // namespace OHOS::MiscServices
