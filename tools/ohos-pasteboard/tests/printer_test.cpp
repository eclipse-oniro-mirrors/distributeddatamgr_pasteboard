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
#include "printer.h"
#include <nlohmann/json.hpp>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Pasteboard;
using json = nlohmann::json;

class PrinterTest : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PrinterTest, PrintSuccess_ValidJson, TestSize.Level1)
{
    json data;
    data["message"] = "success";
    std::string result = OutputPrinter::PrintSuccess(data);
    
    json parsed = json::parse(result);
    EXPECT_EQ(parsed["type"], "result");
    EXPECT_EQ(parsed["status"], "success");
    EXPECT_TRUE(parsed.contains("data"));
}

HWTEST_F(PrinterTest, PrintError_ValidJson, TestSize.Level1)
{
    std::string result = OutputPrinter::PrintError("ERR_TEST", "Test error", "Try again");
    
    json parsed = json::parse(result);
    EXPECT_EQ(parsed["type"], "result");
    EXPECT_EQ(parsed["status"], "failed");
    EXPECT_EQ(parsed["errCode"], "ERR_TEST");
    EXPECT_EQ(parsed["errMsg"], "Test error");
    EXPECT_EQ(parsed["suggestion"], "Try again");
    EXPECT_FALSE(parsed.contains("data"));
}

HWTEST_F(PrinterTest, PrintError_NoSuggestion_Success, TestSize.Level1)
{
    std::string result = OutputPrinter::PrintError("ERR_TEST", "Test error", "No suggestion available");
    
    json parsed = json::parse(result);
    EXPECT_EQ(parsed["type"], "result");
    EXPECT_EQ(parsed["status"], "failed");
    EXPECT_TRUE(parsed.contains("suggestion"));
}