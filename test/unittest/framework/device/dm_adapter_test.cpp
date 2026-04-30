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
#include "pasteboard_hilog.h"

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

void DMAdapterTest::SetUpTestCase(void) {}

void DMAdapterTest::TearDownTestCase(void) {}

void DMAdapterTest::SetUp(void) {}

void DMAdapterTest::TearDown(void) {}

/**
 * @tc.name: OnDeviceOnline001
 * @tc.desc: OnDeviceOnline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, OnDeviceOnline001, TestSize.Level0)
{
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline001 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline001 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline002 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline003 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline003 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline004 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOnline004 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOffline001 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOffline001 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOffline002 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceOffline002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady001 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady001 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady002 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady003 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady003 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady004 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnDeviceReady004 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNetworkIds001 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNetworkIds001 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid001 start");
    bool res = DMAdapter::GetInstance().Initialize();
    ASSERT_FALSE(res);
    std::string device = "deviceTestName";
    auto fromDevice = DMAdapter::GetInstance().GetDeviceName(device);
    ASSERT_FALSE(fromDevice.empty());
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid001 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid002 start");
    bool res = DMAdapter::GetInstance().Initialize();
    std::string device = "deviceTestName";
    auto fromDevice = DMAdapter::GetInstance().GetDeviceName(device);
    DmDeviceInfo info;
    std::string pkgName_ = "pkgName";
    std::string localDeviceUdid_ = "localDeviceUdid";
    DeviceManager::GetInstance().GetUdidByNetworkId(pkgName_, info.networkId, localDeviceUdid_);
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid003 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    DMAdapter::GetInstance().localDeviceUdid_ = "testUdid";
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_FALSE(udid.empty());
#else
    ASSERT_TRUE(udid.empty());
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid003 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid004 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    DMAdapter::GetInstance().localDeviceUdid_ = "";
    auto &udid = DMAdapter::GetInstance().GetLocalDeviceUdid();
    ASSERT_TRUE(udid.empty());
#else
    ASSERT_FALSE(udid.empty());
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceUdid004 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalNetworkId start");
    bool res = DMAdapter::GetInstance().Initialize();
    ASSERT_FALSE(res);
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    ASSERT_FALSE(networkId.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalNetworkId end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DistributedClipRegister start");
    DistributedClip *observer = new DistributedClip();
    DMAdapter::GetInstance().Register(observer);
    DMAdapter::GetInstance().Unregister(observer);
    ASSERT_TRUE(true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DistributedClipRegister end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRemoteDeviceInfo start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo("", remoteDevice);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR));
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRemoteDeviceInfo end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRemoteDeviceInfo002 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRemoteDeviceInfo002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRemoteDeviceInfo003 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetRemoteDeviceInfo003 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId start");
    auto udid = DMAdapter::GetInstance().GetUdidByNetworkId("");
    ASSERT_TRUE(udid.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetUdidByNetworkId end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsSameAccount start");
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    bool ret = DMAdapter::GetInstance().IsSameAccount(networkId);
    ASSERT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsSameAccount end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsSameAccount002 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsSameAccount002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsSameAccount003 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsSameAccount003 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDevices start");
    DMAdapter::GetInstance().SetDevices();
    std::vector<DmDeviceInfo> devices = DMAdapter::GetInstance().GetDevices();
    ASSERT_TRUE(devices.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDevices end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceType002 start");
#ifdef PB_DEVICE_MANAGER_ENABLE
    int32_t deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    EXPECT_EQ(DmDeviceType::DEVICE_TYPE_UNKNOWN, deviceType);
    deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    EXPECT_EQ(DmDeviceType::DEVICE_TYPE_UNKNOWN, deviceType);
#else
    ASSERT_TRUE(true);
#endif
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetLocalDeviceType002 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName001 start");
    std::string networkId = "invalidnetworkId";
    std::string expectedDeviceName = "unknown";
    (void)DMAdapter::GetInstance().GetLocalDeviceType();
    std::string actualDeviceName = DMAdapter::GetInstance().GetDeviceName(networkId);
    EXPECT_EQ(expectedDeviceName, actualDeviceName);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName001 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName002 start");
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string expectedDeviceName = "unknown";
    std::string actualDeviceName = DMAdapter::GetInstance().GetDeviceName(networkId);
    EXPECT_EQ(expectedDeviceName, actualDeviceName);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName002 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName003 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName003 end");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName004 start");
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
PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetDeviceName004 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DeInitialize start");
    DMAdapter::GetInstance().DeInitialize();
    ASSERT_TRUE(true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DeInitialize end");
}

/**
 * @tc.name: DmDeathTest
 * @tc.desc: De Initialize
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DMAdapterTest, DmDeathTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DmDeathTest start");
    auto dmDeath = std::make_shared<DmDeath>();
    ASSERT_TRUE(dmDeath != nullptr);
    dmDeath->OnRemoteDied();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DmDeathTest end");
}
} // namespace OHOS::MiscServices
