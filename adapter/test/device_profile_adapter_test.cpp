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

#include "device_profile_adapter_test.h"

#include "device_profile_adapter.h"
#include "distributed_device_profile_errors.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
using namespace OHOS::MiscServices;
using namespace testing;
using namespace testing::ext;

namespace DistributedDeviceProfile {

DeviceProfileClient &DeviceProfileClient::GetInstance()
{
    static DeviceProfileClient instance;
    return instance;
}

int32_t DeviceProfileClient::PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile)
{
    if (DeviceProfileClientInterfaceMock::GetMock() != nullptr) {
        return DeviceProfileClientInterfaceMock::GetMock()->PutCharacteristicProfile(characteristicProfile);
    }
    return DP_SUCCESS;
}

int32_t DeviceProfileClient::GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
    const std::string &characteristicId, CharacteristicProfile &characteristicProfile)
{
    if (DeviceProfileClientInterfaceMock::GetMock() != nullptr) {
        return DeviceProfileClientInterfaceMock::GetMock()->GetCharacteristicProfile(deviceId, serviceName,
            characteristicId, characteristicProfile);
    }
    return DP_SUCCESS;
}

int32_t DeviceProfileClient::SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo)
{
    if (DeviceProfileClientInterfaceMock::GetMock() != nullptr) {
        return DeviceProfileClientInterfaceMock::GetMock()->SubscribeDeviceProfile(subscribeInfo);
    }
    return DP_SUCCESS;
}

int32_t DeviceProfileClient::UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo)
{
    if (DeviceProfileClientInterfaceMock::GetMock() != nullptr) {
        return DeviceProfileClientInterfaceMock::GetMock()->UnSubscribeDeviceProfile(subscribeInfo);
    }
    return DP_SUCCESS;
}

void DeviceProfileClient::SendSubscribeInfos()
{
    if (DeviceProfileClientInterfaceMock::GetMock() != nullptr) {
        DeviceProfileClientInterfaceMock::mockFlag_ = true;
        DeviceProfileClientInterfaceMock::GetMock()->SendSubscribeInfos();
    }
}

void DeviceProfileClient::ClearDeviceProfileService()
{
    if (DeviceProfileClientInterfaceMock::GetMock() != nullptr) {
        DeviceProfileClientInterfaceMock::mockFlag_ = true;
        DeviceProfileClientInterfaceMock::GetMock()->ClearDeviceProfileService();
    }
}
} // namespace DistributedDeviceProfile

bool DeviceProfileClientInterfaceMock::mockFlag_ = false;

class DeviceProfileAdapterTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeviceProfileAdapterTest SetUpTestCase");
    }

    static void TearDownTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeviceProfileAdapterTest TearDownTestCase");
    }

    void SetUp()
    {
        mock_ = new DeviceProfileClientInterfaceMock();
        DeviceProfileClientInterfaceMock::SetMock(mock_);
        adapter_ = GetDeviceProfileAdapter();
        DeviceProfileClientInterfaceMock::mockFlag_ = false;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeviceProfileAdapterTest SetUp");
    }

    void TearDown()
    {
        DeviceProfileClientInterfaceMock::SetMock(nullptr);
        delete mock_;
        mock_ = nullptr;
        adapter_ = nullptr;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeviceProfileAdapterTest TearDown");
    }

protected:
    DeviceProfileClientInterfaceMock *mock_ = nullptr;
    IDeviceProfileAdapter *adapter_ = nullptr;
};

/**
 * @tc.name: RegisterUpdateCallback001
 * @tc.desc: test RegisterUpdateCallback with nullptr callback
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, RegisterUpdateCallback001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "RegisterUpdateCallback001 start");
    int32_t ret = adapter_->RegisterUpdateCallback(nullptr);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "RegisterUpdateCallback001 end");
}

/**
 * @tc.name: RegisterUpdateCallback002
 * @tc.desc: test RegisterUpdateCallback with valid callback
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, RegisterUpdateCallback002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "RegisterUpdateCallback002 start");
    IDeviceProfileAdapter::OnProfileUpdateCallback callback = [](const std::string &udid, bool status) {
        (void)udid;
        (void)status;
    };
    int32_t ret = adapter_->RegisterUpdateCallback(callback);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "RegisterUpdateCallback002 end");
}

/**
 * @tc.name: PutDeviceStatus001
 * @tc.desc: test PutDeviceStatus with DP_SUCCESS
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, PutDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "PutDeviceStatus001 start");
    std::string udid = "test_udid_001";
    bool status = true;

    EXPECT_CALL(*mock_, PutCharacteristicProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));

    int32_t ret = adapter_->PutDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "PutDeviceStatus001 end");
}

/**
 * @tc.name: PutDeviceStatus002
 * @tc.desc: test PutDeviceStatus with DP_CACHE_EXIST
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, PutDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "PutDeviceStatus002 start");
    std::string udid = "test_udid_002";
    bool status = false;

    EXPECT_CALL(*mock_, PutCharacteristicProfile(testing::_))
        .WillOnce(Return(DP_CACHE_EXIST));

    int32_t ret = adapter_->PutDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "PutDeviceStatus002 end");
}

/**
 * @tc.name: PutDeviceStatus003
 * @tc.desc: test PutDeviceStatus with error
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, PutDeviceStatus003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "PutDeviceStatus003 start");
    std::string udid = "test_udid_003";
    bool status = true;

    EXPECT_CALL(*mock_, PutCharacteristicProfile(testing::_))
        .WillOnce(Return(DP_INVALID_PARAMS));

    int32_t ret = adapter_->PutDeviceStatus(udid, status);
    EXPECT_EQ(ret, DP_INVALID_PARAMS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "PutDeviceStatus003 end");
}

/**
 * @tc.name: GetDeviceStatus001
 * @tc.desc: test GetDeviceStatus with DP_SUCCESS and enabled status
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceStatus001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceStatus001 start");
    std::string udid = "test_udid_004";
    bool status = false;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, CharacteristicProfile &characteristicProfile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            characteristicProfile.SetCharacteristicValue("1");
            return DP_SUCCESS;
        });

    int32_t ret = adapter_->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_TRUE(status);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceStatus001 end");
}

/**
 * @tc.name: GetDeviceStatus002
 * @tc.desc: test GetDeviceStatus with DP_SUCCESS and disabled status
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceStatus002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceStatus002 start");
    std::string udid = "test_udid_005";
    bool status = false;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, CharacteristicProfile &characteristicProfile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            characteristicProfile.SetCharacteristicValue("0");
            return DP_SUCCESS;
        });

    int32_t ret = adapter_->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_FALSE(status);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceStatus002 end");
}

/**
 * @tc.name: GetDeviceStatus003
 * @tc.desc: test GetDeviceStatus with error
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceStatus003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceStatus003 start");
    std::string udid = "test_udid_006";
    bool status = false;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(Return(DP_INVALID_PARAMS));

    int32_t ret = adapter_->GetDeviceStatus(udid, status);
    EXPECT_EQ(ret, DP_INVALID_PARAMS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceStatus003 end");
}

/**
 * @tc.name: GetDeviceVersion001
 * @tc.desc: test GetDeviceVersion with valid version
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion001 start");
    std::string udid = "test_udid_007";
    uint32_t versionId = 0;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, CharacteristicProfile &characteristicProfile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            characteristicProfile.SetCharacteristicValue(R"({"PasteboardVersionId": 10})");
            return DP_SUCCESS;
        });

    bool ret = adapter_->GetDeviceVersion(udid, versionId);
    EXPECT_TRUE(ret);
    EXPECT_EQ(versionId, 10u);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion001 end");
}

/**
 * @tc.name: GetDeviceVersion002
 * @tc.desc: test GetDeviceVersion with error
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion002 start");
    std::string udid = "test_udid_008";
    uint32_t versionId = 0;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(Return(DP_INVALID_PARAMS));

    bool ret = adapter_->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion002 end");
}

/**
 * @tc.name: GetDeviceVersion003
 * @tc.desc: test GetDeviceVersion with invalid JSON
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion003 start");
    std::string udid = "test_udid_009";
    uint32_t versionId = 0;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, CharacteristicProfile &characteristicProfile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            characteristicProfile.SetCharacteristicValue("invalid_json");
            return DP_SUCCESS;
        });

    bool ret = adapter_->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion003 end");
}

/**
 * @tc.name: GetDeviceVersion004
 * @tc.desc: test GetDeviceVersion with version not found
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion004 start");
    std::string udid = "test_udid_010";
    uint32_t versionId = 0;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, CharacteristicProfile &characteristicProfile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            characteristicProfile.SetCharacteristicValue(R"({"otherKey": 10})");
            return DP_SUCCESS;
        });

    bool ret = adapter_->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion004 end");
}

/**
 * @tc.name: GetDeviceVersion005
 * @tc.desc: test GetDeviceVersion with negative version
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion005 start");
    std::string udid = "test_udid_011";
    uint32_t versionId = 0;

    EXPECT_CALL(*mock_, GetCharacteristicProfile(testing::_, testing::_, testing::_, testing::_))
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, CharacteristicProfile &characteristicProfile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            characteristicProfile.SetCharacteristicValue(R"({"PasteboardVersionId": -1})");
            return DP_SUCCESS;
        });

    bool ret = adapter_->GetDeviceVersion(udid, versionId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "GetDeviceVersion005 end");
}

/**
 * @tc.name: SubscribeProfileEvent001
 * @tc.desc: test SubscribeProfileEvent with success
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent001 start");
    std::string udid = "test_udid_012";

    EXPECT_CALL(*mock_, SubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent001 end");
}

/**
 * @tc.name: SubscribeProfileEvent002
 * @tc.desc: test SubscribeProfileEvent with duplicate udid
 * @tc.type: FUNC
    */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent002 start");
    std::string udid = "test_udid_013";

    EXPECT_CALL(*mock_, SubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent002 end");
}

/**
 * @tc.name: SubscribeProfileEvent003
 * @tc.desc: test SubscribeProfileEvent with error
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent003 start");
    std::string udid = "test_udid_014";

    EXPECT_CALL(*mock_, SubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_INVALID_PARAMS));

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, DP_INVALID_PARAMS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent003 end");
}

/**
 * @tc.name: SubscribeProfileEvent004
 * @tc.desc: test SubscribeProfileEvent callback
 * @tc.type: FUNC
    */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent004 start");
    std::string udid = "SubscribeProfileEvent004";

    sptr<IProfileChangeListener> listenerProxy = nullptr;

    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce([&](const SubscribeInfo &subscribeInfo) {
            listenerProxy = iface_cast<IProfileChangeListener>(subscribeInfo.GetListener());
            return DP_SUCCESS;
        });

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_NE(listenerProxy, nullptr);

    CharacteristicProfile oldDeviceProfile;
    CharacteristicProfile newDeviceProfile;
    listenerProxy->OnCharacteristicProfileUpdate(oldDeviceProfile, newDeviceProfile);

    ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SubscribeProfileEvent004 end");
}

/**
 * @tc.name: UnSubscribeProfileEvent001
 * @tc.desc: test UnSubscribeProfileEvent with success
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, UnSubscribeProfileEvent001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "UnSubscribeProfileEvent001 start");
    std::string udid = "test_udid_015";

    EXPECT_CALL(*mock_, SubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));
    EXPECT_CALL(*mock_, UnSubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    ret = adapter_->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "UnSubscribeProfileEvent001 end");
}

/**
 * @tc.name: UnSubscribeProfileEvent002
 * @tc.desc: test UnSubscribeProfileEvent with not found udid
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, UnSubscribeProfileEvent002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "UnSubscribeProfileEvent002 start");
    std::string udid = "test_udid_016";

    int32_t ret = adapter_->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "UnSubscribeProfileEvent002 end");
}

/**
 * @tc.name: UnSubscribeProfileEvent003
 * @tc.desc: test UnSubscribeProfileEvent with error
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, UnSubscribeProfileEvent003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "UnSubscribeProfileEvent003 start");
    std::string udid = "test_udid_017";

    EXPECT_CALL(*mock_, SubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));
    EXPECT_CALL(*mock_, UnSubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_INVALID_PARAMS));

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    ret = adapter_->UnSubscribeProfileEvent(udid);
    EXPECT_EQ(ret, DP_INVALID_PARAMS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "UnSubscribeProfileEvent003 end");
}

/**
 * @tc.name: SendSubscribeInfos001
 * @tc.desc: test SendSubscribeInfos with empty cache
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, SendSubscribeInfos001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SendSubscribeInfos001 start");
    EXPECT_CALL(*mock_, SendSubscribeInfos())
        .Times(1);
    adapter_->SendSubscribeInfos();
    EXPECT_TRUE(DeviceProfileClientInterfaceMock::mockFlag_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SendSubscribeInfos001 end");
}

/**
 * @tc.name: SendSubscribeInfos002
 * @tc.desc: test SendSubscribeInfos with subscribed devices
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, SendSubscribeInfos002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SendSubscribeInfos002 start");
    std::string udid = "test_udid_018";

    EXPECT_CALL(*mock_, SubscribeDeviceProfile(testing::_))
        .WillOnce(Return(DP_SUCCESS));
    EXPECT_CALL(*mock_, SendSubscribeInfos())
        .Times(1);

    int32_t ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    adapter_->SendSubscribeInfos();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "SendSubscribeInfos002 end");
}

/**
 * @tc.name: ClearDeviceProfileService001
 * @tc.desc: test ClearDeviceProfileService
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, ClearDeviceProfileService001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "ClearDeviceProfileService001 start");
    EXPECT_CALL(*mock_, ClearDeviceProfileService())
        .Times(1);

    adapter_->ClearDeviceProfileService();
    EXPECT_TRUE(DeviceProfileClientInterfaceMock::mockFlag_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "ClearDeviceProfileService001 end");
}

/**
 * @tc.name: DeinitDeviceProfileAdapter001
 * @tc.desc: test DeinitDeviceProfileAdapter
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, DeinitDeviceProfileAdapter001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeinitDeviceProfileAdapter001 start");
    EXPECT_CALL(*mock_, ClearDeviceProfileService())
        .Times(1);

    DeinitDeviceProfileAdapter();
    EXPECT_TRUE(DeviceProfileClientInterfaceMock::mockFlag_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeinitDeviceProfileAdapter001 end");
}

/**
 * @tc.name: DeinitDeviceProfileAdapter002
 * @tc.desc: test DeinitDeviceProfileAdapter g_onProfileUpdateCallback
 * @tc.type: FUNC
 */
HWTEST_F(DeviceProfileAdapterTest, DeinitDeviceProfileAdapter002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeinitDeviceProfileAdapter002 start");

    sptr<IProfileChangeListener> listenerProxy = nullptr;
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce([&](const SubscribeInfo &subscribeInfo) {
            listenerProxy = iface_cast<IProfileChangeListener>(subscribeInfo.GetListener());
            return DP_SUCCESS;
        });

    std::string udid = "DeinitDeviceProfileAdapter002";
    int32_t ret = adapter_->SubscribeProfileEvent(udid);

    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_NE(listenerProxy, nullptr);

    
    EXPECT_CALL(*mock_, ClearDeviceProfileService())
        .Times(1);
    DeinitDeviceProfileAdapter();

    EXPECT_TRUE(DeviceProfileClientInterfaceMock::mockFlag_);

    CharacteristicProfile oldDeviceProfile;
    CharacteristicProfile newDeviceProfile;
    listenerProxy->OnCharacteristicProfileUpdate(oldDeviceProfile, newDeviceProfile);

    ret = adapter_->SubscribeProfileEvent(udid);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "DeinitDeviceProfileAdapter002 end");
}
} // namespace OHOS::MiscServices
