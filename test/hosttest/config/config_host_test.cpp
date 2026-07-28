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

// Host-only unit test for OHOS::MiscServices::Config (services/load/config.cpp).
//
// A "composition" sample: Config is a Serializable, so this suite links the
// real config.cpp against the real serializable.cpp + cJSON -- no shim, no fake.
// It exercises the nested-object and vector<Serializable> paths of the base
// Serializable helpers through a concrete production type.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "config.h"

using namespace testing::ext;

namespace OHOS::MiscServices {
namespace {
constexpr int32_t SAMPLE_UID = 4321;
constexpr int COMPONENT_COUNT = 3;
constexpr size_t LAST_COMPONENT_INDEX = 2;
} // namespace

class ConfigHostTest : public testing::Test {
protected:
    static Config MakeSample()
    {
        Config c;
        c.processLabel = "pasteboard_service";
        c.version = "3.0";
        c.features = {"copy", "paste"};
        c.plugins = {"pluginA"};
        c.uid = SAMPLE_UID;

        Config::Component comp;
        comp.description = "clip";
        comp.lib = "libclip.z.so";
        comp.constructor = "Create";
        comp.destructor = "Destroy";
        comp.params = "{}";
        c.components.push_back(comp);
        return c;
    }
};

/**
 * @tc.name: FullRoundTrip
 * @tc.desc: A fully populated Config round-trips through Marshall/Unmarshall.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ConfigHostTest, FullRoundTrip, TestSize.Level0)
{
    Config src = MakeSample();
    std::string json = src.Marshall();
    EXPECT_FALSE(json.empty());

    Config dst;
    ASSERT_TRUE(dst.Unmarshall(json));
    EXPECT_EQ(dst.processLabel, src.processLabel);
    EXPECT_EQ(dst.version, src.version);
    EXPECT_EQ(dst.features, src.features);
    EXPECT_EQ(dst.plugins, src.plugins);
    EXPECT_EQ(dst.uid, src.uid);
    ASSERT_EQ(dst.components.size(), 1u);
    EXPECT_EQ(dst.components[0].lib, "libclip.z.so");
    EXPECT_EQ(dst.components[0].constructor, "Create");
}

/**
 * @tc.name: ComponentRoundTrip
 * @tc.desc: The nested Component type round-trips on its own.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ConfigHostTest, ComponentRoundTrip, TestSize.Level0)
{
    Config::Component src;
    src.description = "d";
    src.lib = "l";
    src.constructor = "c";
    src.destructor = "x";
    src.params = "p";

    std::string json = src.Marshall();
    EXPECT_FALSE(json.empty());

    Config::Component dst;
    ASSERT_TRUE(dst.Unmarshall(json));
    EXPECT_EQ(dst.description, "d");
    EXPECT_EQ(dst.params, "p");
}

/**
 * @tc.name: MultipleComponents
 * @tc.desc: A Config with several components preserves order and count.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ConfigHostTest, MultipleComponents, TestSize.Level0)
{
    Config src;
    src.processLabel = "svc";
    src.version = "1";
    for (int i = 0; i < COMPONENT_COUNT; ++i) {
        Config::Component comp;
        comp.lib = "lib" + std::to_string(i);
        src.components.push_back(comp);
    }

    Config dst;
    ASSERT_TRUE(dst.Unmarshall(src.Marshall()));
    ASSERT_EQ(dst.components.size(), static_cast<size_t>(COMPONENT_COUNT));
    EXPECT_EQ(dst.components[0].lib, "lib0");
    EXPECT_EQ(dst.components[LAST_COMPONENT_INDEX].lib, "lib2");
}

// ---- Unmarshal fails when the required processLabel/version are absent ----
// Config::Unmarshal returns the AND of processLabel & version reads.
/**
 * @tc.name: UnmarshalMissingRequiredFails
 * @tc.desc: Unmarshal fails when required processLabel/version are absent (result is the AND of both reads).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ConfigHostTest, UnmarshalMissingRequiredFails, TestSize.Level0)
{
    Config dst;
    // Valid JSON object, but without processLabel/version.
    EXPECT_FALSE(dst.Unmarshall(R"({"uid":1})"));
}

/**
 * @tc.name: UnmarshalMissingOptionalIsEmpty
 * @tc.desc: Unmarshal tolerates missing optional collections (empty, not failure).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ConfigHostTest, UnmarshalMissingOptionalIsEmpty, TestSize.Level0)
{
    Config dst;
    ASSERT_TRUE(dst.Unmarshall(R"({"processLabel":"svc","version":"2"})"));
    EXPECT_EQ(dst.processLabel, "svc");
    EXPECT_TRUE(dst.features.empty());
    EXPECT_TRUE(dst.components.empty());
}

/**
 * @tc.name: UnmarshalMalformedJson
 * @tc.desc: Malformed JSON is rejected cleanly.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ConfigHostTest, UnmarshalMalformedJson, TestSize.Level0)
{
    Config dst;
    EXPECT_FALSE(dst.Unmarshall("not json"));
    EXPECT_FALSE(dst.Unmarshall(""));
}
} // namespace OHOS::MiscServices
