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

#include <dlfcn.h>
#include "loader.h"
#include "config.h"
#include <string>
#include <gtest/gtest.h>

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;
namespace {
    const std::string LIB_STRING = "random_lib";
    const std::string CONSTRUCTOR_STRING = "random_constructor";
}

namespace OHOS {
namespace MiscServices {
bool Config::Component::Marshal(Serializable::json &node) const
{
    return true;
}
bool Config::Component::Unmarshal(const Serializable::json &node)
{
    return true;
}

bool Config::Marshal(Serializable::json &node) const
{
    return true;
}

static bool g_unmarshal = false;
static std::string g_description;
static std::string g_lib;
static std::string g_constructor;
static std::string g_destructor;
static std::string g_params;
bool Config::Unmarshal(const DistributedData::Serializable::json &node)
{
    if (g_unmarshal) {
        Component component;
        component.description = g_description;
        component.lib = g_lib;
        component.constructor = g_constructor;
        component.destructor = g_destructor;
        component.params = g_params;
        this->components.push_back(component);
    } else {
        auto ret = GetValue(node, GET_NAME(processLabel), processLabel);
        ret = GetValue(node, GET_NAME(version), version) && ret;
        GetValue(node, GET_NAME(features), features);
        GetValue(node, GET_NAME(plugins), plugins);
        GetValue(node, GET_NAME(components), components);
        GetValue(node, GET_NAME(uid), uid);
    }
    return true;
}
}

class PasteboardLoadTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardLoadTest::SetUpTestCase(void) { }

void PasteboardLoadTest::TearDownTestCase(void) { }

void PasteboardLoadTest::SetUp(void) { }

void PasteboardLoadTest::TearDown(void) { }

namespace MiscServices {
/**
 * @tc.name: LoadComponentsTest001
 * @tc.desc: LoadComponents when component.lib == ""
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadComponentsTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    g_lib.clear();
    g_unmarshal = true;
    EXPECT_NO_FATAL_FAILURE(loader.LoadComponents());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}
/**
 * @tc.name: LoadComponentsTest002
 * @tc.desc: LoadComponents when Loader::ComponentIsExist(component.lib) return true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadComponentsTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    std::string lib = LIB_STRING;
    loader.handleMap[LIB_STRING] = &lib;
    g_lib = lib;
    g_unmarshal = true;
    EXPECT_NO_FATAL_FAILURE(loader.LoadComponents());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}
/**
 * @tc.name: LoadComponentsTest003
 * @tc.desc: LoadComponents when dlopen return nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadComponentsTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    std::string lib = LIB_STRING;
    loader.handleMap.clear();
    g_lib = lib;
    g_unmarshal = false;
    EXPECT_NO_FATAL_FAILURE(loader.LoadComponents());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}
/**
 * @tc.name: LoadComponentsTest004
 * @tc.desc: LoadComponents when component.constructor.empty() return true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadComponentsTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    std::string lib = LIB_STRING;
    loader.handleMap.clear();
    g_lib = lib;
    g_constructor.clear();
    g_unmarshal = false;
    EXPECT_NO_FATAL_FAILURE(loader.LoadComponents());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}
/**
 * @tc.name: LoadComponentsTest005
 * @tc.desc: LoadComponents dlsym return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadComponentsTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    std::string lib = LIB_STRING;
    loader.handleMap.clear();
    g_lib = lib;
    g_constructor = CONSTRUCTOR_STRING;
    g_unmarshal = false;
    EXPECT_NO_FATAL_FAILURE(loader.LoadComponents());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: LoadUidTest
 * @tc.desc: LoadUid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadUidTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    EXPECT_NO_FATAL_FAILURE(loader.LoadUid());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: LoadConfigTest
 * @tc.desc: LoadConfig
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, LoadConfigTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    EXPECT_NO_FATAL_FAILURE(loader.LoadConfig());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: ComponentIsExistTest
 * @tc.desc: ComponentIsExist
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLoadTest, ComponentIsExistTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    Loader loader;
    EXPECT_NO_FATAL_FAILURE(loader.ComponentIsExist(LIB_STRING));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}
}
} // namespace OHOS::MiscServices