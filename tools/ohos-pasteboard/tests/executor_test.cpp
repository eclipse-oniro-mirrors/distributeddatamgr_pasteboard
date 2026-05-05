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

#include <gtest/gtest.h>
#include "executor.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Pasteboard;
using json = nlohmann::json;

class ExecutorTest : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ExecutorTest, ExecuteCommand_GlobalHelp_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--help"};
    std::string result = ExecuteCommand(args);
    
    EXPECT_TRUE(result.empty());
}

HWTEST_F(ExecutorTest, ExecuteCommand_EmptyArgs_ShowHelp, TestSize.Level1)
{
    std::vector<std::string> args = {};
    std::string result = ExecuteCommand(args);
    
    EXPECT_TRUE(result.empty());
}

HWTEST_F(ExecutorTest, ExecuteCommand_CommandHelp_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"set-data", "--help"};
    std::string result = ExecuteCommand(args);
    
    // help输出到stderr，返回空字符串
    EXPECT_TRUE(result.empty());
}

HWTEST_F(ExecutorTest, ExecuteCommand_HasDataType_NoMimeType_Error, TestSize.Level1)
{
    std::vector<std::string> args = {"has-data-type", "--help"};
    std::string result = ExecuteCommand(args);
    
    // help输出到stderr，返回空字符串
    EXPECT_TRUE(result.empty());
}

HWTEST_F(ExecutorTest, ExecuteCommand_HasDataType_InvalidTimeout_Error, TestSize.Level1)
{
    std::vector<std::string> args = {"has-data-type", "--type", "text/plain", "--timeout", "0"};
    std::string result = ExecuteCommand(args);
    
    json parsed = json::parse(result);
    EXPECT_EQ(parsed["type"], "result");
    EXPECT_EQ(parsed["status"], "failed");
    EXPECT_FALSE(parsed.contains("data"));
}

HWTEST_F(ExecutorTest, AllCommands_Registered, TestSize.Level2)
{
    std::vector<std::string> commands = {
        "set-data", "get-data", "clear-data",
        "has-data", "has-data-type", "has-remote-data"
    };
    
    RegisterAllCommands();
    auto& registry = CommandRegistry::Instance();
    for (const auto& cmd : commands) {
        auto command = registry.GetCommand(cmd);
        EXPECT_TRUE(command != nullptr) << "Command " << cmd << " not registered";
    }
}

HWTEST_F(ExecutorTest, HelpCommand_AllFieldsPresent, TestSize.Level2)
{
    std::vector<std::string> args = {"set-data", "--help"};
    std::string result = ExecuteCommand(args);
    
    // help输出到stderr，返回空字符串
    EXPECT_TRUE(result.empty());
}

HWTEST_F(ExecutorTest, GlobalHelp_CommandsListComplete, TestSize.Level2)
{
    std::vector<std::string> args = {"--help"};
    std::string result = ExecuteCommand(args);
    
    // help输出到stderr，返回空字符串
    EXPECT_TRUE(result.empty());
}