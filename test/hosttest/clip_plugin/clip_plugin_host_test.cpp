/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Host-only unit test for OHOS::MiscServices::ClipPlugin / DefaultClip
// (framework/framework/clip/{clip_plugin,default_clip}.cpp).
//
// Covers the plugin registry (RegCreator/CreatePlugin/DestroyPlugin), the
// ClipPlugin base-class default-virtual implementations (exercised through
// DefaultClip, which does not override most of them), and GlobalEvent's
// Serializable round-trip. Links the real serializable.cpp; shims only
// pasteboard_hilog.h and pasteboard_event_dfx.h (device logging / hisysevent).

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "clip/default_clip.h"

using namespace testing::ext;

namespace OHOS::MiscServices {
namespace {
constexpr uint32_t TOP_N_DEFAULT = 10;
constexpr uint32_t TOP_N_SMALL = 5;
constexpr int32_t USER_ZERO = 0;
constexpr int32_t USER_ONE = 1;
constexpr uint32_t MAX_LOCAL_CAPACITY_1K = 1024;
constexpr uint32_t MAX_LOCAL_CAPACITY_2K = 2048;
constexpr uint16_t EVT_FRAME_NUM = 2;
constexpr int32_t EVT_USER = 3;
constexpr uint32_t EVT_SEQ_ID = 4;
constexpr int64_t EVT_SYNC_TIME = -5;
constexpr uint64_t EVT_EXPIRATION = 123456789;
} // namespace

class ClipPluginHostTest : public testing::Test {
protected:
    // The registry is static/global; keep each test's factory name unique so
    // RegCreator's "already registered" branch isn't tripped by test order.
};

class FakeClip : public ClipPlugin {
public:
    int32_t SetPasteData(const GlobalEvent &, const std::vector<uint8_t> &, uint32_t,
        const std::vector<uint8_t> &) override
    {
        return 0;
    }
    std::pair<int32_t, int32_t> GetPasteData(const GlobalEvent &, std::vector<uint8_t> &) override
    {
        return {0, 0};
    }
};

class FakeFactory : public ClipPlugin::Factory {
public:
    bool destroyCalled = false;
    ClipPlugin *Create() override
    {
        created = new FakeClip();
        return created;
    }
    bool Destroy(ClipPlugin *plugin) override
    {
        destroyCalled = true;
        delete plugin;
        return true;
    }
    ClipPlugin *created = nullptr;
};

/**
 * @tc.name: RegCreatorRejectsNullFactory
 * @tc.desc: RegCreator rejects a null factory.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, RegCreatorRejectsNullFactory, TestSize.Level0)
{
    EXPECT_FALSE(ClipPlugin::RegCreator("null_factory_plugin", nullptr));
}

/**
 * @tc.name: RegCreatorRejectsDuplicateName
 * @tc.desc: RegCreator succeeds once, then rejects a duplicate name.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, RegCreatorRejectsDuplicateName, TestSize.Level0)
{
    FakeFactory factory;
    EXPECT_TRUE(ClipPlugin::RegCreator("dup_name_plugin", &factory));
    EXPECT_FALSE(ClipPlugin::RegCreator("dup_name_plugin", &factory));
}

/**
 * @tc.name: CreatePluginUnknownFallsBackToDefault
 * @tc.desc: CreatePlugin for an unknown name falls back to the default clip.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, CreatePluginUnknownFallsBackToDefault, TestSize.Level0)
{
    ClipPlugin *p1 = ClipPlugin::CreatePlugin("no_such_plugin_xyz");
    ClipPlugin *p2 = ClipPlugin::CreatePlugin("no_such_plugin_xyz");
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2); // same global default-clip instance every time
}

/**
 * @tc.name: CreatePluginUsesRegisteredFactory
 * @tc.desc: CreatePlugin for a registered name uses the factory.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, CreatePluginUsesRegisteredFactory, TestSize.Level0)
{
    FakeFactory factory;
    ASSERT_TRUE(ClipPlugin::RegCreator("create_uses_factory_plugin", &factory));

    ClipPlugin *p = ClipPlugin::CreatePlugin("create_uses_factory_plugin");
    EXPECT_EQ(p, factory.created);
    ClipPlugin::DestroyPlugin("create_uses_factory_plugin", p);
}

/**
 * @tc.name: DestroyDefaultClipIsNoop
 * @tc.desc: DestroyPlugin on the default clip instance is a no-op success.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DestroyDefaultClipIsNoop, TestSize.Level0)
{
    ClipPlugin *p = ClipPlugin::CreatePlugin("no_such_plugin_for_destroy_default");
    EXPECT_TRUE(ClipPlugin::DestroyPlugin("no_such_plugin_for_destroy_default", p));
}

/**
 * @tc.name: DestroyPluginRejectsNull
 * @tc.desc: DestroyPlugin rejects a null plugin.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DestroyPluginRejectsNull, TestSize.Level0)
{
    EXPECT_FALSE(ClipPlugin::DestroyPlugin("anything", nullptr));
}

/**
 * @tc.name: DestroyPluginUsesFactory
 * @tc.desc: DestroyPlugin for a registered name delegates to the factory.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DestroyPluginUsesFactory, TestSize.Level0)
{
    FakeFactory factory;
    ASSERT_TRUE(ClipPlugin::RegCreator("destroy_uses_factory_plugin", &factory));
    ClipPlugin *p = ClipPlugin::CreatePlugin("destroy_uses_factory_plugin");

    EXPECT_TRUE(ClipPlugin::DestroyPlugin("destroy_uses_factory_plugin", p));
    EXPECT_TRUE(factory.destroyCalled);
}

// ---- ClipPlugin base default GetTopEvents(topN) returns empty ----
// DefaultClip overrides the (topN,user) overload, which hides the base
// (topN) overload by C++ name-hiding rules; reach it via a ClipPlugin&.
/**
 * @tc.name: DefaultGetTopEventsIsEmpty
 * @tc.desc: ClipPlugin base GetTopEvents(topN) is reachable via a ClipPlugin& despite DefaultClip's name-hiding.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DefaultGetTopEventsIsEmpty, TestSize.Level0)
{
    DefaultClip clip;
    ClipPlugin &base = clip;
    EXPECT_TRUE(base.GetTopEvents(TOP_N_DEFAULT).empty());
}

/**
 * @tc.name: DefaultClipGetTopEventsWithUserIsEmpty
 * @tc.desc: DefaultClip overrides GetTopEvents(topN, user) but still returns empty.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DefaultClipGetTopEventsWithUserIsEmpty, TestSize.Level0)
{
    DefaultClip clip;
    EXPECT_TRUE(clip.GetTopEvents(TOP_N_DEFAULT, USER_ZERO).empty());
}

/**
 * @tc.name: BaseDefaultVirtualsReturnNoOpValues
 * @tc.desc: Base-class default virtuals return their documented no-op values.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, BaseDefaultVirtualsReturnNoOpValues, TestSize.Level0)
{
    DefaultClip clip;
    ClipPlugin &base = clip; // reach base overloads hidden by DefaultClip
    EXPECT_EQ(clip.ApplyAdvancedResource("dev1"), 0);
    EXPECT_EQ(clip.PublishServiceState("net1", ClipPlugin::ServiceStatus::IDLE), 0);
    EXPECT_EQ(base.Close(0), 0);
    EXPECT_FALSE(base.NeedSyncTopEvent());
    EXPECT_FALSE(base.IsWiFiEnable());
    base.Clear();
    clip.Clear(0);
    base.SetMaxLocalCapacity(MAX_LOCAL_CAPACITY_1K);
    base.SendPreSyncEvent(0);
    base.ChangeStoreStatus(0);
}

/**
 * @tc.name: GlobalEventRoundTrip
 * @tc.desc: GlobalEvent Marshal/Unmarshal round-trips every field.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, GlobalEventRoundTrip, TestSize.Level0)
{
    ClipPlugin::GlobalEvent src;
    src.version = 1;
    src.frameNum = EVT_FRAME_NUM;
    src.user = EVT_USER;
    src.seqId = EVT_SEQ_ID;
    src.status = ClipPlugin::EVT_NORMAL;
    src.syncTime = EVT_SYNC_TIME;
    src.expiration = EVT_EXPIRATION;
    src.deviceId = "dev-1";
    src.account = "acct-1";
    src.dataType = {"text/plain", "image/png"};

    std::string json = src.Marshall();
    EXPECT_FALSE(json.empty());

    ClipPlugin::GlobalEvent dst;
    ASSERT_TRUE(dst.Unmarshall(json));
    EXPECT_EQ(dst.version, src.version);
    EXPECT_EQ(dst.frameNum, src.frameNum);
    EXPECT_EQ(dst.user, src.user);
    EXPECT_EQ(dst.seqId, src.seqId);
    EXPECT_EQ(dst.status, src.status);
    EXPECT_EQ(dst.syncTime, src.syncTime);
    EXPECT_EQ(dst.expiration, src.expiration);
    EXPECT_EQ(dst.deviceId, src.deviceId);
    EXPECT_EQ(dst.account, src.account);
    EXPECT_EQ(dst.dataType, src.dataType);
    EXPECT_TRUE(dst == src);
}

/**
 * @tc.name: GlobalEventEqualityIsSeqIdAndDeviceIdOnly
 * @tc.desc: GlobalEvent::operator== compares seqId + deviceId only.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, GlobalEventEqualityIsSeqIdAndDeviceIdOnly, TestSize.Level0)
{
    ClipPlugin::GlobalEvent a;
    a.seqId = 1;
    a.deviceId = "dev-x";
    a.account = "acct-a";

    ClipPlugin::GlobalEvent b;
    b.seqId = 1;
    b.deviceId = "dev-x";
    b.account = "acct-different"; // differs, but operator== doesn't look at it

    EXPECT_TRUE(a == b);
}

/**
 * @tc.name: GlobalEventUnmarshalMalformedJson
 * @tc.desc: GlobalEvent Unmarshal fails cleanly on malformed JSON.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, GlobalEventUnmarshalMalformedJson, TestSize.Level0)
{
    ClipPlugin::GlobalEvent dst;
    EXPECT_FALSE(dst.Unmarshall("not json"));
}

// ---- DestroyPlugin returns false when the plugin isn't the default and its
// name isn't registered (covers the "registered name not found" branch) ----
/**
 * @tc.name: DestroyPluginUnregisteredNonDefaultReturnsFalse
 * @tc.desc: DestroyPlugin returns false when the plugin is not the default and its name is not registered.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DestroyPluginUnregisteredNonDefaultReturnsFalse, TestSize.Level0)
{
    FakeClip orphan; // not the global default clip, name never registered
    EXPECT_FALSE(ClipPlugin::DestroyPlugin("never_registered_name", &orphan));
}

// ---- The ClipPlugin base default virtuals (not overridden by DefaultClip)
// return their no-op values; exercised on a FakeClip which inherits them ----
/**
 * @tc.name: BaseVirtualsOnFakeClip
 * @tc.desc: The ClipPlugin base default virtuals return their no-op values, exercised via a FakeClip.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, BaseVirtualsOnFakeClip, TestSize.Level0)
{
    FakeClip clip;
    EXPECT_TRUE(clip.GetTopEvents(TOP_N_SMALL).empty());
    EXPECT_TRUE(clip.GetTopEvents(TOP_N_SMALL, USER_ONE).empty()); // base (topN,user) no-op
    EXPECT_EQ(clip.ApplyAdvancedResource("d"), 0);
    EXPECT_EQ(clip.PublishServiceState("n", ClipPlugin::ServiceStatus::CONNECT_SUCC), 0);
    EXPECT_EQ(clip.Close(0), 0);
    EXPECT_FALSE(clip.NeedSyncTopEvent());
    EXPECT_FALSE(clip.IsWiFiEnable());
    clip.Clear();
    clip.Clear(1);
    clip.SetMaxLocalCapacity(MAX_LOCAL_CAPACITY_2K);
    clip.SendPreSyncEvent(1);
    clip.ChangeStoreStatus(1);

    // callback registration no-ops
    clip.RegisterDelayCallback(nullptr, nullptr);
    clip.RegisterPreSyncCallback(nullptr);
    clip.RegisterPreSyncMonitorCallback(nullptr);

    // entry / mimetype no-op getters
    ClipPlugin::GlobalEvent evt;
    std::vector<uint8_t> raw;
    EXPECT_EQ(clip.GetPasteDataEntry(evt, 1, "utd", raw), 0);
    std::vector<uint8_t> mimeTypes;
    EXPECT_EQ(clip.GetMimeTypes(mimeTypes, evt), 0);
}

/**
 * @tc.name: DefaultClipSetGetPasteData
 * @tc.desc: DefaultClip's own SetPasteData / GetPasteData return their no-op values.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ClipPluginHostTest, DefaultClipSetGetPasteData, TestSize.Level0)
{
    DefaultClip clip;
    ClipPlugin::GlobalEvent evt;
    std::vector<uint8_t> data = {1, 2, 3};
    std::vector<uint8_t> mimeTypes;
    EXPECT_EQ(clip.SetPasteData(evt, data, 1, mimeTypes), 0);

    std::vector<uint8_t> out;
    auto pr = clip.GetPasteData(evt, out);
    EXPECT_EQ(pr.first, 0);
    EXPECT_EQ(pr.second, 0);
}
} // namespace OHOS::MiscServices
