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
#include "parser.h"
#include <vector>
#include <string>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Pasteboard;

class ParserTest : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ParserTest, ParseSetData_TextOnly_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--text", "Hello World"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.orderedParams.size(), 1);
    EXPECT_EQ(result.orderedParams[0].first, "text");
    EXPECT_EQ(result.orderedParams[0].second, "Hello World");
}

HWTEST_F(ParserTest, ParseSetData_HtmlOnly_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--html", "<p>Hello</p>"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.orderedParams.size(), 1);
    EXPECT_EQ(result.orderedParams[0].first, "html");
}

HWTEST_F(ParserTest, ParseSetData_UriOnly_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--uri", "file:///path/to/file"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.orderedParams.size(), 1);
    EXPECT_EQ(result.orderedParams[0].first, "uri");
}

HWTEST_F(ParserTest, ParseSetData_MixedParams_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--text", "plain", "--html", "<p>html</p>", "--uri", "file:///test"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.orderedParams.size(), 3);
}

HWTEST_F(ParserTest, ParseSetData_NoParams_Error, TestSize.Level1)
{
    std::vector<std::string> args = {};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errMsg.find("At least one") != std::string::npos);
}

HWTEST_F(ParserTest, ParseSetData_MissingValue_Error, TestSize.Level1)
{
    std::vector<std::string> args = {"--text"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_FALSE(result.success);
}

HWTEST_F(ParserTest, ParseSetData_InvalidParam_Error, TestSize.Level1)
{
    std::vector<std::string> args = {"--invalid", "value"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_FALSE(result.success);
}

HWTEST_F(ParserTest, ParseSetData_DuplicateParams_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--text", "first", "--text", "second"};
    auto result = SpecialParser::ParseSetData(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.orderedParams.size(), 2);
}

HWTEST_F(ParserTest, ParseHasDataType_NoTimeout_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--type", "text/plain"};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.hasTimeout);
    EXPECT_EQ(result.type, "text/plain");
}

HWTEST_F(ParserTest, ParseHasDataType_WithTimeout_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--type", "text/plain", "--timeout", "1000"};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.hasTimeout);
    EXPECT_EQ(result.timeout, 1000);
}

HWTEST_F(ParserTest, ParseHasDataType_MissingMimeType_Error, TestSize.Level1)
{
    std::vector<std::string> args = {};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_FALSE(result.success);
}

HWTEST_F(ParserTest, ParseHasDataType_InvalidTimeout_Error, TestSize.Level1)
{
    std::vector<std::string> args = {"--type", "text/plain", "--timeout", "0"};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_FALSE(result.success);
}

HWTEST_F(ParserTest, ParseHasDataType_OutOfRangeTimeout_Error, TestSize.Level1)
{
    std::vector<std::string> args = {"--type", "text/plain", "--timeout", "6000"};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_FALSE(result.success);
}

HWTEST_F(ParserTest, ParseHasDataType_MinTimeout_1_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--type", "text/plain", "--timeout", "1"};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.timeout, 1);
}

HWTEST_F(ParserTest, ParseHasDataType_MaxTimeout_5000_Success, TestSize.Level1)
{
    std::vector<std::string> args = {"--type", "text/plain", "--timeout", "5000"};
    auto result = SpecialParser::ParseHasDataType(args);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.timeout, 5000);
}