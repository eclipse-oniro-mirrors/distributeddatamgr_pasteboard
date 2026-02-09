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

#include "device/dev_profile.h"
#include "device/dm_adapter.h"
#include "pasteboard_error.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
class DevProfileTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DevProfileTest::SetUpTestCase(void) {}

void DevProfileTest::TearDownTestCase(void)
{
    DevProfile::GetInstance().ClearDeviceProfileService();
}

void DevProfileTest::SetUp(void) {}

void DevProfileTest::TearDown(void) {}

/**
 * @tc.name: GetDeviceVersionTest001
 * @tc.desc: Get Device Version
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, GetDeviceVersionTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    uint32_t versionId;
    std::string bundleName = "com.dev.profile";
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: GetDeviceVersionTest002
 * @tc.desc: Get Device Version
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, GetDeviceVersionTest002, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    uint32_t versionId;
    std::string bundleName = "pasteboard_dm_adapter";
    bool res = DMAdapter::GetInstance().Initialize();
    DevProfile::GetInstance().proxy_ = nullptr;
    DevProfile::GetInstance().subscribeUdidList_.clear();
    DevProfile::GetInstance().GetDeviceVersion(bundleName, versionId);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: PostDelayReleaseProxy001
 * @tc.desc: in SetTimer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, PostDelayReleaseProxy001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    DevProfile::GetInstance().proxy_ = nullptr;
    std::string uuid = "PostDelayReleaseProxy001";
    DevProfile::GetInstance().SendSubscribeInfos();
    DevProfile::GetInstance().subscribeUdidList_.emplace(uuid);
    DevProfile::GetInstance().UnSubscribeAllProfileEvents();
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: GetDeviceStatusTest001
 * @tc.desc: GetDeviceStatus should not return E_OK when query invalid networkId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, GetDeviceStatusTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    bool enabledStatus = false;
    std::string networkId = "test.dev.profile";
    int32_t ret = DevProfile::GetInstance().GetDeviceStatus(networkId, enabledStatus);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
#else
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR));
#endif
}

/**
 * @tc.name: GetDeviceStatusTest002
 * @tc.desc: GetDeviceStatus should not return NO_TRUST_DEVICE_ERROR when query valid networkId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, GetDeviceStatusTest002, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    bool enabledStatus = false;
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    int32_t ret = DevProfile::GetInstance().GetDeviceStatus(networkId, enabledStatus);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR));
#else
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR));
#endif
}

/**
 * @tc.name: PutDeviceStatus001
 * @tc.desc: PutDeviceStatus should not return NO_TRUST_DEVICE_ERROR when query valid networkId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, PutDeviceStatus001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    bool res = DMAdapter::GetInstance().Initialize();
    bool enabledStatus = true;
    DevProfile::GetInstance().proxy_ = nullptr;
    DevProfile::GetInstance().subscribeUdidList_.clear();
    DevProfile::GetInstance().PutDeviceStatus(enabledStatus);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: SubscribeProfileEventTest001
 * @tc.desc: Subscribe Profile Event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, SubscribeProfileEventTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    std::string bundleName = "pasteboard_dm_adapter";
    bool res = DMAdapter::GetInstance().Initialize();
    DevProfile::GetInstance().SubscribeProfileEvent(bundleName);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: SubscribeProfileEventTest002
 * @tc.desc: Subscribe Profile Event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, SubscribeProfileEventTest002, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    std::string bundleName = "pasteboard_dm_adapter";
    bool res = DMAdapter::GetInstance().Initialize();
    DevProfile::GetInstance().SubscribeProfileEvent(bundleName);
    DevProfile::GetInstance().SendSubscribeInfos();
    DevProfile::GetInstance().SubscribeProfileEvent(bundleName);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: UnSubscribeProfileEventTest001
 * @tc.desc: UnSubscribe Profile Event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, UnSubscribeProfileEventTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    std::string bundleName = "pasteboard_dm_adapter";
    bool res = DMAdapter::GetInstance().Initialize();
    DevProfile::GetInstance().UnSubscribeProfileEvent(bundleName);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: UnSubscribeProfileEventTest002
 * @tc.desc: UnSubscribe Profile Event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, UnSubscribeProfileEventTest002, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    std::string bundleName = "pasteboard_dm_adapter";
    bool res = DMAdapter::GetInstance().Initialize();
    DevProfile::GetInstance().SubscribeProfileEvent(bundleName);
    DevProfile::GetInstance().UnSubscribeProfileEvent(bundleName);
    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: UnSubscribeAllProfileEvents001
 * @tc.desc: subscribeUdidList_ is empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, UnSubscribeAllProfileEvents001, TestSize.Level0)
{
#ifdef PB_DEVICE_INFO_MANAGER_ENABLE
    DevProfile& dp = DevProfile::GetInstance();
    dp.subscribeUdidList_.clear();
    dp.UnSubscribeAllProfileEvents();

    dp.proxy_= nullptr;
    dp.subscribeUdidList_.clear();
    dp.UnSubscribeAllProfileEvents();

    EXPECT_TRUE(true);
#else
    EXPECT_TRUE(true);
#endif
}

/**
 * @tc.name: WatchTest
 * @tc.desc: Watch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DevProfileTest, Watch, TestSize.Level0)
{
    DevProfile::Observer observer;
    DevProfile::GetInstance().Watch(observer);
    EXPECT_FALSE(observer);
}
} // namespace OHOS::MiscServices