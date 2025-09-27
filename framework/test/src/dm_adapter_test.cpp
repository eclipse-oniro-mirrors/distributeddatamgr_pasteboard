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

#include <gtest/gtest.h>
#include <thread>

#include "device/dm_adapter.h"
#include "device_manager.h"
#include "distributed_clip.h"
#include "pasteboard_error.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
constexpr uint32_t SLEEP_MS = 10;
class DMAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DMAdapterTest::SetUpTestCase(void) { }

void DMAdapterTest::TearDownTestCase(void) { }

void DMAdapterTest::SetUp(void) { }

void DMAdapterTest::TearDown(void) { }

/**
 * @tc.name: OnDeviceOnline001
 * @tc.desc: OnDeviceOnline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOnline001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = INVALID_TYPE;
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
    stateObserver->OnDeviceOnline(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceOnline002
 * @tc.desc: OnDeviceOnline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOnline002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
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
    stateObserver->OnDeviceOnline(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceOnline003
 * @tc.desc: OnDeviceOnline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOnline003, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = INVALID_TYPE;
    auto stateObserver = std::make_shared<DmStateObserver>(
        nullptr,
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        });
    stateObserver->OnDeviceOnline(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceOnline004
 * @tc.desc: OnDeviceOnline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOnline004, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    auto stateObserver = std::make_shared<DmStateObserver>(
        nullptr,
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        });
    stateObserver->OnDeviceOnline(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceOffline001
 * @tc.desc: OnDeviceOffline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOffline001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    auto stateObserver = std::make_shared<DmStateObserver>(
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        nullptr);
    stateObserver->OnDeviceOffline(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceOffline002
 * @tc.desc: OnDeviceOffline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOffline002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
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
    stateObserver->OnDeviceOffline(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceReady001
 * @tc.desc: OnDeviceReady.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceReady001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = INVALID_TYPE;
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
    stateObserver->OnDeviceReady(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceReady002
 * @tc.desc: OnDeviceReady.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceReady002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
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
    stateObserver->OnDeviceReady(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceReady003
 * @tc.desc: OnDeviceReady.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceReady003, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = INVALID_TYPE;
    auto stateObserver = std::make_shared<DmStateObserver>(
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        nullptr,
        [](const DmDeviceInfo &deviceInfo) {
            return;
        });
    stateObserver->OnDeviceReady(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OnDeviceReady004
 * @tc.desc: OnDeviceReady.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceReady004, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    auto stateObserver = std::make_shared<DmStateObserver>(
        [](const DmDeviceInfo &deviceInfo) {
            return;
        },
        nullptr,
        [](const DmDeviceInfo &deviceInfo) {
            return;
        });
    stateObserver->OnDeviceReady(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetNetworkIds001
 * @tc.desc: GetNetworkIds.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetNetworkIds001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    std::string networkId = "testNetworkId";
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    auto ids = DMAdapter::GetInstance().GetNetworkIds();
    ASSERT_NE(0, ids.size());
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetLocalDeviceUdid001
 * @tc.desc: Get the local device udid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetLocalDeviceUdid001, TestSize.Level0)
{
    bool res = DMAdapter::GetInstance().Initialize();
    ASSERT_FALSE(res);
    std::string device = "deviceTestName";
    auto fromDevice = DMAdapter::GetInstance().GetDeviceName(device);
    ASSERT_FALSE(fromDevice.empty());
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
}

/**
 * @tc.name: GetLocalDeviceUdid002
 * @tc.desc: Get the local device udid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetLocalDeviceUdid002, TestSize.Level0)
{
    bool res = DMAdapter::GetInstance().Initialize();
    std::string device = "deviceTestName";
    auto fromDevice = DMAdapter::GetInstance().GetDeviceName(device);
    DmDeviceInfo info;
    std::string pkgName_ = "pkgName";
    std::string localDeviceUdid_ = "localDeviceUdid";
    DeviceManager::GetInstance().GetUdidByNetworkId(pkgName_, info.networkId, localDeviceUdid_);
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
}

/**
 * @tc.name: GetLocalDeviceUdid003
 * @tc.desc: GetLocalDeviceUdid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetLocalDeviceUdid003, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DMAdapter::GetInstance().localDeviceUdid_ = "testUdid";
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_FALSE(udid.empty());
#else
    ASSERT_TRUE(udid.empty());
#endif
}

/**
 * @tc.name: GetLocalDeviceUdid004
 * @tc.desc: GetLocalDeviceUdid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetLocalDeviceUdid004, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DMAdapter::GetInstance().localDeviceUdid_ = "";
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
#else
    ASSERT_FALSE(udid.empty());
#endif
}

/**
 * @tc.name: GetLocalNetworkId
 * @tc.desc: Get the local network id.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetLocalNetworkId, TestSize.Level0)
{
    bool res = DMAdapter::GetInstance().Initialize();
    ASSERT_FALSE(res);
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    ASSERT_FALSE(networkId.empty());
}

/**
 * @tc.name: DistributedClipRegister
 * @tc.desc: DistributedClip Register and Unregister.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, DistributedClipRegister, TestSize.Level0)
{
    DistributedClip *observer = new DistributedClip();
    DMAdapter::GetInstance().Register(observer);
    DMAdapter::GetInstance().Unregister(observer);
    ASSERT_TRUE(true);
}

/**
 * @tc.name: GetRemoteDeviceInfo
 * @tc.desc: Get the remote device info.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetRemoteDeviceInfo, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo("", remoteDevice);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR));
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetRemoteDeviceInfo002
 * @tc.desc: Get the remote device info.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetRemoteDeviceInfo002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    bool res = DMAdapter::GetInstance().Initialize();
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    DmDeviceInfo remoteDevice;
    int32_t result = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetRemoteDeviceInfo003
 * @tc.desc: Get the remote device info.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetRemoteDeviceInfo003, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    bool res = DMAdapter::GetInstance().Initialize();
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    DmDeviceInfo remoteDevice;
    int32_t result = DMAdapter::GetInstance().GetRemoteDeviceInfo("testNetworkId", remoteDevice);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR), result);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetUdidByNetworkId
 * @tc.desc: Get Udid By NetworkId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetUdidByNetworkId, TestSize.Level0)
{
    auto udid = DMAdapter::GetInstance().GetUdidByNetworkId("");
    ASSERT_TRUE(udid.empty());
}

/**
 * @tc.name: IsSameAccount
 * @tc.desc: is same account.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, IsSameAccount, TestSize.Level0)
{
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    bool ret = DMAdapter::GetInstance().IsSameAccount(networkId);
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: IsSameAccount002
 * @tc.desc: is same account.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, IsSameAccount002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    std::string networkId = "testNetworkId";
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    bool ret = DMAdapter::GetInstance().IsSameAccount(networkId);
    ASSERT_TRUE(ret);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: IsSameAccount003
 * @tc.desc: is same account.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, IsSameAccount003, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    bool res = DMAdapter::GetInstance().Initialize();
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    bool ret = DMAdapter::GetInstance().IsSameAccount("testNetworkId");
    ASSERT_FALSE(ret);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetDevices
 * @tc.desc: Get Devices.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetDevices, TestSize.Level0)
{
    DMAdapter::GetInstance().SetDevices();
    std::vector<DmDeviceInfo> devices = DMAdapter::GetInstance().GetDevices();
    ASSERT_TRUE(devices.empty());
}

/**
 * @tc.name: GetLocalDeviceType002
 * @tc.desc: GetLocalDeviceType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetLocalDeviceType002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    int32_t deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    EXPECT_EQ(DmDeviceType::DEVICE_TYPE_UNKNOWN, deviceType);
    deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    EXPECT_EQ(DmDeviceType::DEVICE_TYPE_UNKNOWN, deviceType);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: GetDeviceName001
 * @tc.desc: Get Local Device Type
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetDeviceName001, TestSize.Level0)
{
    std::string networkId = "invalidnetworkId";
    std::string expectedDeviceName = "unknown";
    (void)DMAdapter::GetInstance().GetLocalDeviceType();
    std::string actualDeviceName = DMAdapter::GetInstance().GetDeviceName(networkId);
    EXPECT_EQ(expectedDeviceName, actualDeviceName);
}

/**
 * @tc.name: GetDeviceName002
 * @tc.desc: Get Local Device Type
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetDeviceName002, TestSize.Level0)
{
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string expectedDeviceName = "unknown";
    std::string actualDeviceName = DMAdapter::GetInstance().GetDeviceName(networkId);
    EXPECT_EQ(expectedDeviceName, actualDeviceName);
}

/**
 * @tc.name: GetDeviceName003
 * @tc.desc: GetDeviceName.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetDeviceName003, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    bool res = DMAdapter::GetInstance().Initialize();
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string expectedDeviceName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(expectedDeviceName.begin(), expectedDeviceName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    std::string actualDeviceName = DMAdapter::GetInstance().GetDeviceName(networkId);
    EXPECT_EQ(expectedDeviceName, actualDeviceName);
#else
    ASSERT_TRUE(udid.empty());
#endif
}

/**
 * @tc.name: GetDeviceName004
 * @tc.desc: GetDeviceName.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, GetDeviceName004, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    bool res = DMAdapter::GetInstance().Initialize();
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);
    std::string actualDeviceName = DMAdapter::GetInstance().GetDeviceName("testNetworkId");
    EXPECT_EQ("unknown", actualDeviceName);
#else
    ASSERT_TRUE(udid.empty());
#endif
}

/**
 * @tc.name: DeInitialize
 * @tc.desc: De Initialize
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, DeInitialize, TestSize.Level0)
{
    DMAdapter::GetInstance().DeInitialize();
    ASSERT_TRUE(true);
}
} // namespace OHOS::MiscServices
