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

#include "cJSON.h"
#include "clip/clip_plugin.h"
#include "serializable/serializable.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
class ClipPluginTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ClipPluginTest::SetUpTestCase(void) {}

void ClipPluginTest::TearDownTestCase(void) {}

void ClipPluginTest::SetUp(void) {}

void ClipPluginTest::TearDown(void) {}

class CustomClipPlugin : public ClipPlugin {
public:
    int32_t SetPasteData(const GlobalEvent &event, const std::vector<uint8_t> &data, uint32_t version,
        const std::vector<uint8_t> &mimeTypes) override
    {
        (void)event;
        (void)data;
        (void)version;
        (void)mimeTypes;
        return 0;
    }

    std::pair<int32_t, int32_t> GetPasteData(const GlobalEvent &event, std::vector<uint8_t> &data) override
    {
        (void)event;
        (void)data;
        return std::make_pair(0, 0);
    }
};

/**
 * @tc.name: MarshalTest001
 * @tc.desc: Marshal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, MarshalTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshalTest001 start");
    std::string PLUGIN_NAME_VAL = "distributed_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    std::shared_ptr<ClipPlugin> clipPlugin;
    clipPlugin = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    DistributedData::Serializable::json node;
    ClipPlugin::GlobalEvent globalEvent;
    std::string networkId = "networkId";
    int32_t result = clipPlugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
    ASSERT_TRUE(globalEvent.Marshal(node));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshalTest001 end");
}

/**
 * @tc.name: UnmarshalTest001
 * @tc.desc: Unmarshal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, UnmarshalTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshalTest001 start");
    std::string str = R"({
        {"version", 1},
        {"frameNum", 100},
        {"user", "testUser"},
        {"seqId", 12345},
        {"expiration", 1000},
        {"status", "active"},
        {"deviceId", "123456"},
        {"account", "testAcount"},
        {"dataType", "image"},
        {"syncTime", 2000},
    })";
    DistributedData::Serializable::json node = cJSON_Parse(str.c_str());
    ClipPlugin::GlobalEvent globalEvent;
    ASSERT_FALSE(globalEvent.Unmarshal(node));
    cJSON_Delete(node);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshalTest001 end");
}

/**
 * @tc.name: PublishServiceStateTest
 * @tc.desc: PublishServiceState.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, PublishServiceStateTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PublishServiceStateTest start");
    CustomClipPlugin clipPlugin;
    std::string networkId = "testNetworkId";
    int32_t result = clipPlugin.PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
    ASSERT_EQ(0, result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PublishServiceStateTest end");
}

/**
 * @tc.name: ApplyAdvancedResourceTest
 * @tc.desc: ApplyAdvancedResource
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, ApplyAdvancedResourceTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ApplyAdvancedResourceTest001 start");
    CustomClipPlugin clipPlugin;
    std::string networkId = "testNetworkId";

    int32_t result = clipPlugin.ApplyAdvancedResource(networkId);
    ASSERT_EQ(0, result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ApplyAdvancedResourceTest001 end");
}

/**
 * @tc.name: CloseTest
 * @tc.desc: Close.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, CloseTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CloseTest start");
    CustomClipPlugin clipPlugin;
    int32_t user = 0;
    bool isNeedClear = true;
    int32_t result = clipPlugin.Close(user);
    ASSERT_EQ(0, result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CloseTest end");
}

/**
 * @tc.name: RegisterDelayCallbackTest
 * @tc.desc: RegisterDelayCallback.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, RegisterDelayCallbackTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RegisterDelayCallbackTest start");
    CustomClipPlugin clipPlugin;
    ClipPlugin::DelayDataCallback dataCallback = nullptr;
    ClipPlugin::DelayEntryCallback entryCallback = nullptr;
    clipPlugin.RegisterDelayCallback(dataCallback, entryCallback);
    ASSERT_TRUE(true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RegisterDelayCallbackTest end");
}

/**
 * @tc.name: GetPasteDataEntryTest
 * @tc.desc: GetPasteDataEntry.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, GetPasteDataEntryTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteDataEntryTest start");
    CustomClipPlugin clipPlugin;
    ClipPlugin::GlobalEvent event;
    uint32_t recordId = 0;
    std::string utdId = "";
    std::vector<uint8_t> rawData;
    int32_t result = clipPlugin.GetPasteDataEntry(event, recordId, utdId, rawData);
    ASSERT_EQ(0, result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteDataEntryTest end");
}

/**
 * @tc.name: ChangeStoreStatusTest
 * @tc.desc: ChangeStoreStatus.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, ChangeStoreStatusTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ChangeStoreStatusTest start");
    CustomClipPlugin clipPlugin;
    uint32_t userId = 0;
    clipPlugin.ChangeStoreStatus(userId);
    ASSERT_TRUE(true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ChangeStoreStatusTest end");
}

/**
 * @tc.name: RegCreatorTest001
 * @tc.desc: RegCreator.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, RegCreatorTestTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RegCreatorTestTest001 start");
    std::string name = "testFactory";
    ClipPlugin::Factory *factory = nullptr;
    bool result = ClipPlugin::RegCreator(name, factory);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RegCreatorTestTest001 end");
}

/**
 * @tc.name: DestroyPluginTest001
 * @tc.desc: DestroyPlugin.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, DestroyPluginTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DestroyPluginTest001 start");
    std::string name = "testPlugin";
    ClipPlugin *plugin = nullptr;
    EXPECT_FALSE(ClipPlugin::DestroyPlugin(name, plugin));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DestroyPluginTest001 end");
}

/**
 * @tc.name: NeedSyncTopEventTest
 * @tc.desc: NeedSyncTopEvent.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, NeedSyncTopEventTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "NeedSyncTopEventTest start");
    CustomClipPlugin clipPlugin;
    bool result = clipPlugin.NeedSyncTopEvent();
    ASSERT_EQ(result, false);
    ClipPlugin::PreSyncCallback preSyncCB;
    clipPlugin.RegisterPreSyncCallback(preSyncCB);
    ClipPlugin::PreSyncMonitorCallback preSyncMonitorCB;
    clipPlugin.RegisterPreSyncMonitorCallback(preSyncMonitorCB);
    clipPlugin.SendPreSyncEvent(0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "NeedSyncTopEventTest end");
}

/**
 * @tc.name: ApplyAdvancedResourceTest
 * @tc.desc: ApplyAdvancedResource.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, ApplyAdvancedResourceTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ApplyAdvancedResourceTest start");
    CustomClipPlugin clipPlugin;
    std::string deviceId = "test";
    int32_t result = clipPlugin.ApplyAdvancedResource(deviceId);
    ASSERT_EQ(0, result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ApplyAdvancedResourceTest end");
}

/**
 * @tc.name: SetMaxLocalCapacityTest
 * @tc.desc: SetMaxLocalCapacity.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, SetMaxLocalCapacityTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMaxLocalCapacityTest start");
    auto clipPlugin = std::make_shared<CustomClipPlugin>();
    ASSERT_NE(clipPlugin, nullptr);
    int64_t maxLocalCapacity = 0;
    clipPlugin->SetMaxLocalCapacity(maxLocalCapacity);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMaxLocalCapacityTest end");
}

/**
 * @tc.name: GetMimeTypesTest
 * @tc.desc: GetMimeTypes.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, GetMimeTypesTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetMimeTypesTest start");
    auto clipPlugin = std::make_shared<CustomClipPlugin>();
    ASSERT_NE(clipPlugin, nullptr);
    std::vector<uint8_t> mimeTypes;
    ClipPlugin::GlobalEvent event;
    auto ret = clipPlugin->GetMimeTypes(mimeTypes, event);
    ASSERT_EQ(0, ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetMimeTypesTest end");
}

/**
 * @tc.name: IsWiFiEnableTest
 * @tc.desc: IsWiFiEnable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ClipPluginTest, IsWiFiEnableTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsWiFiEnableTest start");
    auto clipPlugin = std::make_shared<CustomClipPlugin>();
    ASSERT_NE(clipPlugin, nullptr);
    bool result = clipPlugin->IsWiFiEnable();
    ASSERT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsWiFiEnableTest end");
}
} // namespace OHOS::MiscServices
