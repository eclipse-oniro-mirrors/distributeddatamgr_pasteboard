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
#include <chrono>

#include "device_profile_adapter.h"
#include "device_profile_client_mock.h"
#include "distributed_device_profile_errors.h"
#include "pasteboard_error.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MiscServices {

class DeviceProfileAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        mock_ = new DistributedDeviceProfile::DeviceProfileClientMock();
    }

    static void TearDownTestCase(void)
    {
        if (mock_ != nullptr) {
            delete mock_;
            mock_ = nullptr;
        }
    }

    void SetUp(void) {}
    void TearDown(void) {}

    static inline DistributedDeviceProfile::DeviceProfileClientMock *mock_ = nullptr;
};

/**
 * @tc.name: RegisterUpdateCallback001
 * @tc.desc: RegisterUpdateCallback with null callback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, RegisterUpdateCallback001, TestSize.Level0)
{
    auto adapter = GetDeviceProfileAdapter();
    int32_t ret = adapter->RegisterUpdateCallback(nullptr);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
}

/**
 * @tc.name: RegisterUpdateCallback002
 * @tc.desc: RegisterUpdateCallback with valid callback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, RegisterUpdateCallback002, TestSize.Level0)
{
    auto adapter = GetDeviceProfileAdapter();
    IDeviceProfileAdapter::OnProfileUpdateCallback callback = [](const std::string &udid, bool status) {
        (void)udid;
        (void)status;
    };
    int32_t ret = adapter->RegisterUpdateCallback(callback);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
}

/**
 * @tc.name: PutDeviceStatus001
 * @tc.desc: PutDeviceStatus with success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, PutDeviceStatus001, TestSize.Level0)
{
    EXPECT_CALL(*mock_, PutCharacteristicProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    bool status = true;
    int32_t ret = adapter->PutDeviceStatus(udid, status);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
}

/**
 * @tc.name: PutDeviceStatus002
 * @tc.desc: PutDeviceStatus with cache exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, PutDeviceStatus002, TestSize.Level0)
{
    EXPECT_CALL(*mock_, PutCharacteristicProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_CACHE_EXIST));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    bool status = false;
    int32_t ret = adapter->PutDeviceStatus(udid, status);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
}

/**
 * @tc.name: PutDeviceStatus003
 * @tc.desc: PutDeviceStatus with error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, PutDeviceStatus003, TestSize.Level0)
{
    EXPECT_CALL(*mock_, PutCharacteristicProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_INVALID_PARAMS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    bool status = true;
    int32_t ret = adapter->PutDeviceStatus(udid, status);
    ASSERT_EQ(DistributedDeviceProfile::DP_INVALID_PARAMS, ret);
}

/**
 * @tc.name: GetDeviceStatus001
 * @tc.desc: GetDeviceStatus with success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceStatus001, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, DistributedDeviceProfile::CharacteristicProfile &profile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            profile.SetCharacteristicValue("1");
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
    ASSERT_TRUE(status);
}

/**
 * @tc.name: GetDeviceStatus002
 * @tc.desc: GetDeviceStatus with false status
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceStatus002, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, DistributedDeviceProfile::CharacteristicProfile &profile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            profile.SetCharacteristicValue("0");
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    bool status = true;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
    ASSERT_FALSE(status);
}

/**
 * @tc.name: GetDeviceStatus003
 * @tc.desc: GetDeviceStatus with error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceStatus003, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_GET_SERVICE_FAILED));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    bool status = false;
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    ASSERT_EQ(DistributedDeviceProfile::DP_GET_SERVICE_FAILED, ret);
}

/**
 * @tc.name: GetDeviceVersion001
 * @tc.desc: GetDeviceVersion with success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion001, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, DistributedDeviceProfile::CharacteristicProfile &profile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            profile.SetCharacteristicValue(R"({"PasteboardVersionId": 10})");
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    ASSERT_TRUE(ret);
    ASSERT_EQ(10u, versionId);
}

/**
 * @tc.name: GetDeviceVersion002
 * @tc.desc: GetDeviceVersion with error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion002, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_GET_SERVICE_FAILED));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: GetDeviceVersion003
 * @tc.desc: GetDeviceVersion with invalid json
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion003, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, DistributedDeviceProfile::CharacteristicProfile &profile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            profile.SetCharacteristicValue("invalid json");
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: GetDeviceVersion004
 * @tc.desc: GetDeviceVersion with version not found
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion004, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, DistributedDeviceProfile::CharacteristicProfile &profile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            profile.SetCharacteristicValue(R"({"otherKey": 10})");
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: GetDeviceVersion005
 * @tc.desc: GetDeviceVersion with negative version
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, GetDeviceVersion005, TestSize.Level0)
{
    EXPECT_CALL(*mock_, GetCharacteristicProfile)
        .WillOnce([](const std::string &deviceId, const std::string &serviceName,
            const std::string &characteristicId, DistributedDeviceProfile::CharacteristicProfile &profile) {
            (void)deviceId;
            (void)serviceName;
            (void)characteristicId;
            profile.SetCharacteristicValue(R"({"PasteboardVersionId": -1})");
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    uint32_t versionId = 0;
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: SubscribeProfileEvent001
 * @tc.desc: SubscribeProfileEvent with success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent001, TestSize.Level0)
{
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
}

/**
 * @tc.name: SubscribeProfileEvent002
 * @tc.desc: SubscribeProfileEvent with duplicate udid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent002, TestSize.Level0)
{
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);

    ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
}

/**
 * @tc.name: SubscribeProfileEvent003
 * @tc.desc: SubscribeProfileEvent with error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEvent003, TestSize.Level0)
{
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_INVALID_PARAMS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(DistributedDeviceProfile::DP_INVALID_PARAMS, ret);
}

/**
 * @tc.name: UnSubscribeProfileEvent001
 * @tc.desc: UnSubscribeProfileEvent with success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, UnSubscribeProfileEvent001, TestSize.Level0)
{
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));
    EXPECT_CALL(*mock_, UnSubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);

    ret = adapter->UnSubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
}

/**
 * @tc.name: UnSubscribeProfileEvent002
 * @tc.desc: UnSubscribeProfileEvent with not found
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, UnSubscribeProfileEvent002, TestSize.Level0)
{
    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
}

/**
 * @tc.name: UnSubscribeProfileEvent003
 * @tc.desc: UnSubscribeProfileEvent with error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, UnSubscribeProfileEvent003, TestSize.Level0)
{
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));
    EXPECT_CALL(*mock_, UnSubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_INVALID_PARAMS));

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);

    ret = adapter->UnSubscribeProfileEvent(udid);
    ASSERT_EQ(DistributedDeviceProfile::DP_INVALID_PARAMS, ret);
}

/**
 * @tc.name: SendSubscribeInfos001
 * @tc.desc: SendSubscribeInfos with no subscribe info
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, SendSubscribeInfos001, TestSize.Level0)
{
    auto adapter = GetDeviceProfileAdapter();
    adapter->SendSubscribeInfos();
    SUCCEED();
}

/**
 * @tc.name: SendSubscribeInfos002
 * @tc.desc: SendSubscribeInfos with subscribe info
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, SendSubscribeInfos002, TestSize.Level0)
{
    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce(Return(DistributedDeviceProfile::DP_SUCCESS));
    EXPECT_CALL(*mock_, SendSubscribeInfos)
        .Times(1);

    auto adapter = GetDeviceProfileAdapter();
    std::string udid = "testUdid";
    int32_t ret = adapter->SubscribeProfileEvent(udid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);

    adapter->SendSubscribeInfos();
}
/**
 * @tc.name: ClearDeviceProfileService001
 * @tc.desc: ClearDeviceProfileService
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, ClearDeviceProfileService001, TestSize.Level0)
{
    EXPECT_CALL(*mock_, ClearDeviceProfileService)
        .Times(1);

    auto adapter = GetDeviceProfileAdapter();
    adapter->ClearDeviceProfileService();
    SUCCEED();
}

/**
 * @tc.name: SubscribeProfileEventWithCallback001
 * @tc.desc: SubscribeProfileEvent with callback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DeviceProfileAdapterTest, SubscribeProfileEventWithCallback001, TestSize.Level0)
{
    bool callbackCalled = false;
    std::string callbackUdid;
    bool callbackStatus = false;

    IDeviceProfileAdapter::OnProfileUpdateCallback callback = [&](const std::string &udid, bool status) {
        callbackCalled = true;
        callbackUdid = udid;
        callbackStatus = status;
    };

    auto adapter = GetDeviceProfileAdapter();
    int32_t ret = adapter->RegisterUpdateCallback(callback);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);

    std::string testUdid = "testUdid";
    bool testStatus = true;

    EXPECT_CALL(*mock_, SubscribeDeviceProfile)
        .WillOnce([&](const DistributedDeviceProfile::SubscribeInfo &subscribeInfo) {
            (void)subscribeInfo;
            callback(testUdid, testStatus);
            return DistributedDeviceProfile::DP_SUCCESS;
        });

    ret = adapter->SubscribeProfileEvent(testUdid);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(callbackCalled);
    ASSERT_EQ(testUdid, callbackUdid);
    ASSERT_EQ(testStatus, callbackStatus);
}

} // namespace MiscServices
} // namespace OHOS
