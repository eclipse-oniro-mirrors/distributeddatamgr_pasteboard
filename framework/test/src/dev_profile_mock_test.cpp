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

#include "dev_profile_mock_test.h"

#include "device/dev_profile.h"
#include "device/dm_adapter.h"
#include "distributed_device_profile_errors.h"
#include "pasteboard_error.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MiscServices {

void DevProfileMockTest::SetUpTestCase(void)
{
    DistributedHardware::PasteDeviceManager::pasteDeviceManager = deviceManagerMock_;
}

void DevProfileMockTest::TearDownTestCase(void)
{
    DevProfile::GetInstance().ClearDeviceProfileService();
    DistributedHardware::PasteDeviceManager::pasteDeviceManager = nullptr;
    deviceManagerMock_ = nullptr;
}

void DevProfileMockTest::SetUp(void) { }

void DevProfileMockTest::TearDown(void)
{
    testing::Mock::VerifyAndClear(deviceManagerMock_.get());
}

/**
 * @tc.name: GetDeviceStatusTest001
 * @tc.desc: GetDeviceStatus should return E_OK when query valid networkId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceStatusTest001, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly(testing::Return(static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR)));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    bool enabledStatus = false;
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    int32_t ret = DevProfile::GetInstance().GetDeviceStatus(networkId, enabledStatus);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR), ret);
}

/**
 * @tc.name: GetDeviceStatusTest002
 * @tc.desc: GetDeviceStatus should return E_OK when query valid networkId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceStatusTest002, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly(testing::Return(DistributedDeviceProfile::DP_SUCCESS));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    bool enabledStatus = false;
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    int32_t ret = DevProfile::GetInstance().GetDeviceStatus(networkId, enabledStatus);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
    ret = DevProfile::GetInstance().GetDeviceStatus(networkId, enabledStatus);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
}

/**
 * @tc.name: PutDeviceStatus001
 * @tc.desc: PutDeviceStatus
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, PutDeviceStatus001, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, PutCharacteristicProfile)
        .WillRepeatedly(testing::Return(DistributedDeviceProfile::DP_SUCCESS));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    bool enabledStatus = true;
    DevProfile::GetInstance().PutDeviceStatus(enabledStatus);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: PutDeviceStatus002
 * @tc.desc: PutDeviceStatus
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, PutDeviceStatus002, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, PutCharacteristicProfile)
        .WillRepeatedly(testing::Return(DistributedDeviceProfile::DP_CACHE_EXIST));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    bool enabledStatus = true;
    DevProfile::GetInstance().PutDeviceStatus(enabledStatus);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: PutDeviceStatus003
 * @tc.desc: PutDeviceStatus
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, PutDeviceStatus003, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, PutCharacteristicProfile)
        .WillRepeatedly(testing::Return(DistributedDeviceProfile::DP_INVALID_PARAMS));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    bool enabledStatus = true;
    DevProfile::GetInstance().PutDeviceStatus(enabledStatus);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: GetDeviceVersionTest001
 * @tc.desc: GetDeviceVersionTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceVersionTest001, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly(testing::Return(DistributedDeviceProfile::DP_SUCCESS));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().proxy_ = nullptr;
    DevProfile::GetInstance().subscribeUdidList_.clear();
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: GetDeviceVersionTest002
 * @tc.desc: GetDeviceVersionTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceVersionTest002, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly(testing::Return(DistributedDeviceProfile::DP_INVALID_PARAMS));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: GetDeviceVersionTest003
 * @tc.desc: GetDeviceVersionTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceVersionTest003, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly([](auto, auto, auto, DistributedDeviceProfile::CharacteristicProfile &characteristicProfile) {
            characteristicProfile.characteristicValue_ = "{}";
            return DistributedDeviceProfile::DP_SUCCESS;
        });
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: GetDeviceVersionTest004
 * @tc.desc: GetDeviceVersionTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceVersionTest004, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly([](auto, auto, auto, DistributedDeviceProfile::CharacteristicProfile &characteristicProfile) {
            characteristicProfile.characteristicValue_ = R"({"PasteboardVersionId": "invalid"})";
            return DistributedDeviceProfile::DP_SUCCESS;
        });
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: GetDeviceVersionTest005
 * @tc.desc: GetDeviceVersionTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceVersionTest005, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly([](auto, auto, auto, DistributedDeviceProfile::CharacteristicProfile &characteristicProfile) {
            characteristicProfile.characteristicValue_ = R"({"PasteboardVersionId": 10})";
            return DistributedDeviceProfile::DP_SUCCESS;
        });
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: GetDeviceVersionTest006
 * @tc.desc: GetDeviceVersionTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, GetDeviceVersionTest006, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(dpMock, GetCharacteristicProfile)
        .WillRepeatedly([](auto, auto, auto, DistributedDeviceProfile::CharacteristicProfile &characteristicProfile) {
            characteristicProfile.characteristicValue_ = R"({"PasteboardVersionId": -1})";
            return DistributedDeviceProfile::DP_SUCCESS;
        });
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: SubscribeProfileEvent001
 * @tc.desc: Sub scribe Profile Event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileMockTest, SubscribeProfileEventTest001, TestSize.Level0)
{
    NiceMock<DistributedDeviceProfile::DeviceProfileClientMock> dpMock;
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "SubscribeProfileEventTest001";
            return 0;
        });
    DevProfile::GetInstance().proxy_ = nullptr;
    DevProfile::GetInstance().subscribeUdidList_.clear();
    std::string bundleName = "com.pro.proEvent";
    DevProfile::GetInstance().SendSubscribeInfos();
    DevProfile::GetInstance().SubscribeProfileEvent(bundleName);
    DevProfile::GetInstance().SendSubscribeInfos();
    DevProfile::GetInstance().UnSubscribeProfileEvent(bundleName);
    EXPECT_TRUE(true);
}
} // namespace MiscServices
} // namespace OHOS