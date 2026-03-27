/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#include "device/distributed_module_config.h"
#include "pasteboard_error.h"

namespace OHOS::MiscServices {
using namespace testing;
using namespace testing::ext;

class DistributedModuleConfigTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedModuleConfigTest::SetUpTestCase(void) { }

void DistributedModuleConfigTest::TearDownTestCase(void) { }

void DistributedModuleConfigTest::SetUp(void) { }

void DistributedModuleConfigTest::TearDown(void) { }

/**
 * @tc.name: IsOnTest001
 * @tc.desc: Is On.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedModuleConfigTest, IsOnTest001, TestSize.Level0)
{
    DistributedModuleConfig config;
    config.Init();
    bool result = config.IsOn();
    config.DeInit();
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsOnTest
 * @tc.desc: Is On.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedModuleConfigTest, IsOnTest002, TestSize.Level0)
{
    DistributedModuleConfig config;
    std::string device = "validDevice";
    bool result = config.IsOn();
    config.OnReady(device);
    config.Online(device);
    config.Offline(device);
    EXPECT_FALSE(result);
    config.DeInit();
}

/**
 * @tc.name: IsOnTest003
 * @tc.desc: Is On.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedModuleConfigTest, IsOnTest003, TestSize.Level0)
{
    std::string networkId = "testNetworkId";
    std::string testName = "testDeviceName";
    DmDeviceInfo info;
    info.authForm = IDENTICAL_ACCOUNT;
    std::copy(networkId.begin(), networkId.end(), info.networkId);
    std::copy(testName.begin(), testName.end(), info.deviceName);
    DMAdapter::GetInstance().devices_.emplace_back(info);

    DistributedModuleConfig config;
    bool result = config.IsOn();
    EXPECT_FALSE(result);
    config.DeInit();
}

/**
 * @tc.name: WatchTest
 * @tc.desc: Watch.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedModuleConfigTest, WatchTest, TestSize.Level0)
{
    DistributedModuleConfig::Observer observer;
    DistributedModuleConfig config;
    config.Notify();
    config.Watch(observer);
    EXPECT_FALSE(observer);
    config.DeInit();
}

/**
 * @tc.name: GetRemoteDeviceMaxVersionTest001
 * @tc.desc: GetRemoteDeviceMaxVersion
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedModuleConfigTest, GetRemoteDeviceMaxVersionTest001, TestSize.Level0)
{
    DistributedModuleConfig config;
    uint32_t maxVersion = config.GetRemoteDeviceMaxVersion();
    EXPECT_GE(maxVersion, 0U);
}
} // namespace OHOS::MiscServices