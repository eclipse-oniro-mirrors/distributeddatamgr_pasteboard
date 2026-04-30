/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "dm_adapter_mock_test.h"

#include "device/dm_adapter.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MiscServices {

void DMAdapterMockTest::SetUpTestCase(void)
{
    DistributedHardware::PasteDeviceManager::pasteDeviceManager = deviceManagerMock_;
}

void DMAdapterMockTest::TearDownTestCase(void)
{
    DistributedHardware::PasteDeviceManager::pasteDeviceManager = nullptr;
    deviceManagerMock_ = nullptr;
}

void DMAdapterMockTest::SetUp(void) { }

void DMAdapterMockTest::TearDown(void)
{
    testing::Mock::VerifyAndClear(deviceManagerMock_.get());
}

/**
 * @tc.name: OnDeviceChanged001
 * @tc.desc: OnDeviceChanged.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, OnDeviceChanged001, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceChanged001 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, IsSameAccount(testing::_)).Times(1).WillRepeatedly(testing::Return(true));
    DmDeviceInfo info;
    std::string networkId = "testNetworkId";
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    auto stateObserver = std::make_shared<DmStateObserver>(
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        });
    stateObserver->OnDeviceChanged(info);
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceChanged001 end");
}

/**
 * @tc.name: OnDeviceChanged002
 * @tc.desc: OnDeviceChanged.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, OnDeviceChanged002, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceChanged002 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, IsSameAccount(testing::_)).Times(1).WillRepeatedly(testing::Return(false));
    DmDeviceInfo info;
    std::string networkId = "testNetworkId";
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    auto stateObserver = std::make_shared<DmStateObserver>(
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        });
    stateObserver->OnDeviceChanged(info);
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceChanged002 end");
}

/**
 * @tc.name: GetLocalDeviceUdid001
 * @tc.desc: GetLocalDeviceUdid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetLocalDeviceUdid001, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid001 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetLocalDeviceInfo(testing::_, testing::_))
        .Times(1)
        .WillRepeatedly(testing::Return(1));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_)).Times(0);
    DMAdapter::GetInstance().localDeviceUdid_ = "";
    std::string udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
    DMAdapter::GetInstance().localDeviceUdid_ = "";
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid001 end");
}

/**
 * @tc.name: GetLocalDeviceUdid002
 * @tc.desc: GetLocalDeviceUdid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetLocalDeviceUdid002, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid002 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetLocalDeviceInfo(testing::_, testing::_))
        .Times(1)
        .WillRepeatedly(testing::Return(0));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "";
            return 0;
        });
    DMAdapter::GetInstance().localDeviceUdid_ = "";
    std::string udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
    DMAdapter::GetInstance().localDeviceUdid_ = "";
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid002 end");
}

/**
 * @tc.name: GetLocalDeviceUdid003
 * @tc.desc: GetLocalDeviceUdid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetLocalDeviceUdid003, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid003 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetLocalDeviceInfo(testing::_, testing::_))
        .Times(1)
        .WillRepeatedly(testing::Return(0));
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 0;
        });
    DMAdapter::GetInstance().localDeviceUdid_ = "";
    std::string udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_FALSE(udid.empty());
    DMAdapter::GetInstance().localDeviceUdid_ = "";
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid003 end");
}

/**
 * @tc.name: GetUdidByNetworkId001
 * @tc.desc: GetUdidByNetworkId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetUdidByNetworkId001, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId001 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "testUdid";
            return 1;
        });
    std::string networkId = "testNetworkId";
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    ASSERT_TRUE(udid.empty());
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId001 end");
}

/**
 * @tc.name: GetUdidByNetworkId002
 * @tc.desc: GetUdidByNetworkId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetUdidByNetworkId002, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId002 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "";
            return 1;
        });
    std::string networkId = "testNetworkId";
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    ASSERT_TRUE(udid.empty());
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId002 end");
}

/**
 * @tc.name: GetUdidByNetworkId003
 * @tc.desc: GetUdidByNetworkId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetUdidByNetworkId003, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId003 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetUdidByNetworkId(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::string &udid) {
            udid = "";
            return 0;
        });
    std::string networkId = "testNetworkId";
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    ASSERT_TRUE(udid.empty());
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId003 end");
}

/**
 * @tc.name: GetLocalDeviceType001
 * @tc.desc: GetLocalDeviceType.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, GetLocalDeviceType001, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceType001 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetLocalDeviceType(testing::_, testing::_))
        .Times(1)
        .WillRepeatedly(testing::Return(1));
    int32_t deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    ASSERT_EQ(DmDeviceType::DEVICE_TYPE_UNKNOWN, deviceType);
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceType001 end");
}

/**
 * @tc.name: SetDevices001
 * @tc.desc: SetDevices.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, SetDevices001, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetDevices001 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetTrustedDeviceList(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::vector<DmDeviceInfo> &deviceList) {
            DmDeviceInfo info;
            std::string networkId = "testNetworkId";
            std::copy(networkId.begin(), networkId.end(), info.networkId);
            deviceList.emplace_back(info);
            return 1;
        });
    EXPECT_CALL(*deviceManagerMock_, IsSameAccount(testing::_)).Times(0);
    DMAdapter::GetInstance().SetDevices();
    ASSERT_TRUE(true);
    DMAdapter::GetInstance().devices_.clear();
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetDevices001 end");
}

/**
 * @tc.name: SetDevices002
 * @tc.desc: SetDevices.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, SetDevices002, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetDevices002 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetTrustedDeviceList(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::vector<DmDeviceInfo> &deviceList) {
            DmDeviceInfo info;
            std::string networkId = "testNetworkId";
            std::copy(networkId.begin(), networkId.end(), info.networkId);
            deviceList.emplace_back(info);
            return 0;
        });
    EXPECT_CALL(*deviceManagerMock_, IsSameAccount(testing::_)).Times(1).WillRepeatedly(testing::Return(false));
    DMAdapter::GetInstance().SetDevices();
    ASSERT_TRUE(true);
    DMAdapter::GetInstance().devices_.clear();
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetDevices002 end");
}

/**
 * @tc.name: SetDevices003
 * @tc.desc: SetDevices.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterMockTest, SetDevices003, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetDevices003 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    EXPECT_CALL(*deviceManagerMock_, GetTrustedDeviceList(testing::_, testing::_, testing::_))
        .Times(1)
        .WillRepeatedly([](auto, auto, std::vector<DmDeviceInfo> &deviceList) {
            DmDeviceInfo info;
            std::string networkId = "testNetworkId";
            std::copy(networkId.begin(), networkId.end(), info.networkId);
            deviceList.emplace_back(info);
            return 0;
        });
    EXPECT_CALL(*deviceManagerMock_, IsSameAccount(testing::_)).Times(1).WillRepeatedly(testing::Return(true));
    DMAdapter::GetInstance().SetDevices();
    ASSERT_TRUE(true);
    DMAdapter::GetInstance().devices_.clear();
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetDevices003 end");
}

} // namespace MiscServices
} // namespace OHOS
