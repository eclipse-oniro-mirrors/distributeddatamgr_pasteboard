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

#include "device_profile_client.h"
#include "distributed_device_profile_errors.h"
#include "distributed_device_profile_proxy.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "system_ability_manager_mock.h"

namespace OHOS::MiscServices {
using namespace OHOS::DistributedDeviceProfile;
using namespace testing::ext;
using testing::NiceMock;

class AdapterDeviceProfileClientTest : public testing::Test {
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

int32_t LoadSystemAbilityFailImpl(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback)
{
    std::thread thread([=] {
        if (callback == nullptr) {
            return;
        }
        callback->OnLoadSystemAbilityFail(systemAbilityId);
    });
    pthread_setname_np(thread.native_handle(), "LoadSaFail");
    thread.detach();
    return ERR_OK;
}

int32_t LoadSystemAbilitySuccImpl(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback)
{
    std::thread thread([=] {
        if (callback == nullptr) {
            return;
        }
        sptr<IRemoteObject> remoteObject = new DistributedDeviceProfileStub();
        callback->OnLoadSystemAbilitySuccess(systemAbilityId, remoteObject);
    });
    pthread_setname_np(thread.native_handle(), "LoadSaSucc");
    thread.detach();
    return ERR_OK;
}

/**
 * @tc.name: TestClearDeviceProfileService001
 * @tc.desc: test ClearDeviceProfileService
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileClientTest, TestClearDeviceProfileService001, TestSize.Level1)
{
    DeviceProfileClient::GetInstance().ClearDeviceProfileService();
    EXPECT_EQ(DeviceProfileClient::GetInstance().dpProxy_, nullptr);
}

/**
 * @tc.name: TestGetDeviceProfileService001
 * @tc.desc: test GetDeviceProfileService OnLoadSystemAbilityFail
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileClientTest, TestGetDeviceProfileService001, TestSize.Level1)
{
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilityFailImpl);

    auto proxy = DeviceProfileClient::GetInstance().GetDeviceProfileService();
    EXPECT_EQ(proxy, nullptr);
}

/**
 * @tc.name: TestGetDeviceProfileService002
 * @tc.desc: test GetDeviceProfileService OnLoadSystemAbilitySuccess
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileClientTest, TestGetDeviceProfileService002, TestSize.Level1)
{
    NiceMock<SystemAbilityManagerMock> mock;
    EXPECT_CALL(mock, CheckSystemAbility).WillRepeatedly(testing::Return(nullptr));
    EXPECT_CALL(mock, LoadSystemAbility).WillRepeatedly(LoadSystemAbilitySuccImpl);

    auto proxy = DeviceProfileClient::GetInstance().GetDeviceProfileService();
    EXPECT_NE(proxy, nullptr);
}

/**
 * @tc.name: TestPutCharacteristicProfile001
 * @tc.desc: test PutCharacteristicProfile
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileClientTest, TestPutCharacteristicProfile001, TestSize.Level1)
{
    std::string deviceId = "udid1";
    std::string serviceName = "SwitchStatus_Key_Distributed_Pasteboard";
    std::string characteristicKey = "SwitchStatus";
    std::string characteristicValue = "1";
    CharacteristicProfile profile(deviceId, serviceName, characteristicKey, characteristicValue);

    int32_t ret = DeviceProfileClient::GetInstance().PutCharacteristicProfile(profile);
    EXPECT_EQ(ret, DP_SUCCESS);
}

/**
 * @tc.name: TestGetCharacteristicProfile001
 * @tc.desc: test GetCharacteristicProfile
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileClientTest, TestGetCharacteristicProfile001, TestSize.Level1)
{
    std::string deviceId = "udid1";
    std::string serviceName = "SwitchStatus_Key_Distributed_Pasteboard";
    std::string characteristicKey = "SwitchStatus";
    CharacteristicProfile profile;

    int32_t ret = DeviceProfileClient::GetInstance().GetCharacteristicProfile(deviceId, serviceName, characteristicKey,
        profile);
    EXPECT_EQ(ret, DP_SUCCESS);
}

/**
 * @tc.name: TestSubscribeDeviceProfile001
 * @tc.desc: test SubscribeDeviceProfile, SendSubscribeInfos, UnSubscribeDeviceProfile
 * @tc.type: FUNC
 */
HWTEST_F(AdapterDeviceProfileClientTest, TestSubscribeDeviceProfile001, TestSize.Level1)
{
    DeviceProfileClient::GetInstance().SendSubscribeInfos();
    EXPECT_EQ(DeviceProfileClient::GetInstance().subscribeInfos_.size(), 0);

    int32_t saId = PASTEBOARD_SERVICE_ID;
    std::string subscribeKey = "UDID#SWITCH_ID#CHARACTER_ID#CHARACTERISTIC_VALUE";
    SubscribeInfo subscribeInfo(saId, subscribeKey);

    int32_t ret = DeviceProfileClient::GetInstance().SubscribeDeviceProfile(subscribeInfo);
    EXPECT_EQ(ret, DP_SUCCESS);

    DeviceProfileClient::GetInstance().SendSubscribeInfos();
    EXPECT_EQ(DeviceProfileClient::GetInstance().subscribeInfos_.size(), 1);

    ret = DeviceProfileClient::GetInstance().UnSubscribeDeviceProfile(subscribeInfo);
    EXPECT_EQ(ret, DP_SUCCESS);
    EXPECT_EQ(DeviceProfileClient::GetInstance().subscribeInfos_.size(), 0);
}
} // namespace OHOS::MiscServices
