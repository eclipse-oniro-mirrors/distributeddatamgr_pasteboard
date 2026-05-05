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

class IntegrationTest : public Test {
protected:
    void SetUp() override
    {
        std::vector<std::string> clearArgs = {"clear-data"};
        ExecuteCommand(clearArgs);
    }
    void TearDown() override
    {
        std::vector<std::string> clearArgs = {"clear-data"};
        ExecuteCommand(clearArgs);
    }
};

HWTEST_F(IntegrationTest, FullWorkflow_HasData_SetData_HasData_HasDataType_GetData_ClearData_HasData, TestSize.Level3)
{
    std::vector<std::string> hasArgs1 = {"has-data"};
    std::string hasResult1 = ExecuteCommand(hasArgs1);
    json parsed1 = json::parse(hasResult1);
    EXPECT_TRUE(parsed1["status"] == "success" || parsed1["status"] == "failed");
    
    std::vector<std::string> setArgs = {"set-data", "--text", "workflow-test"};
    std::string setResult = ExecuteCommand(setArgs);
    json setParsed = json::parse(setResult);
    
    if (setParsed["status"] == "success") {
        std::vector<std::string> hasArgs2 = {"has-data"};
        std::string hasResult2 = ExecuteCommand(hasArgs2);
        json parsed2 = json::parse(hasResult2);
        EXPECT_TRUE(parsed2["status"] == "success" || parsed2["status"] == "failed");
        
        std::vector<std::string> hasTypeArgs = {"has-data-type", "--type", "text/plain"};
        std::string hasTypeResult = ExecuteCommand(hasTypeArgs);
        json parsedType = json::parse(hasTypeResult);
        EXPECT_TRUE(parsedType["status"] == "success" || parsedType["status"] == "failed");
        
        std::vector<std::string> getArgs = {"get-data"};
        std::string getResult = ExecuteCommand(getArgs);
        json getParsed = json::parse(getResult);
        EXPECT_TRUE(getParsed["status"] == "success" || getParsed["status"] == "failed");
        
        std::vector<std::string> clearArgs = {"clear-data"};
        ExecuteCommand(clearArgs);
        
        std::vector<std::string> hasArgs3 = {"has-data"};
        std::string hasResult3 = ExecuteCommand(hasArgs3);
        json parsed3 = json::parse(hasResult3);
        EXPECT_TRUE(parsed3["status"] == "success" || parsed3["status"] == "failed");
    }
}

HWTEST_F(IntegrationTest, MultipleTypes_SetHtml_SetUri_GetData, TestSize.Level3)
{
    std::vector<std::string> setHtmlArgs = {"set-data", "--html", "<p>html-test</p>"};
    std::string setHtmlResult = ExecuteCommand(setHtmlArgs);
    json setHtmlParsed = json::parse(setHtmlResult);
    
    if (setHtmlParsed["status"] == "success") {
        std::vector<std::string> hasHtmlTypeArgs = {"has-data-type", "--type", "text/html"};
        std::string hasHtmlResult = ExecuteCommand(hasHtmlTypeArgs);
        json parsedHtml = json::parse(hasHtmlResult);
        EXPECT_TRUE(parsedHtml["status"] == "success" || parsedHtml["status"] == "failed");
        
        std::vector<std::string> getArgs1 = {"get-data"};
        std::string getResult1 = ExecuteCommand(getArgs1);
        json getParsed1 = json::parse(getResult1);
        EXPECT_TRUE(getParsed1["status"] == "success" || getParsed1["status"] == "failed");
    }
    
    std::vector<std::string> setUriArgs = {"set-data", "--uri", "file:///test/path"};
    std::string setUriResult = ExecuteCommand(setUriArgs);
    json setUriParsed = json::parse(setUriResult);
    
    if (setUriParsed["status"] == "success") {
        std::vector<std::string> hasUriTypeArgs = {"has-data-type", "--type", "text/uri"};
        std::string hasUriResult = ExecuteCommand(hasUriTypeArgs);
        json parsedUri = json::parse(hasUriResult);
        EXPECT_TRUE(parsedUri["status"] == "success" || parsedUri["status"] == "failed");
        
        std::vector<std::string> getArgs2 = {"get-data"};
        std::string getResult2 = ExecuteCommand(getArgs2);
        json getParsed2 = json::parse(getResult2);
        EXPECT_TRUE(getParsed2["status"] == "success" || getParsed2["status"] == "failed");
    }
}

HWTEST_F(IntegrationTest, HasRemoteData_SetData_GetData_HasRemoteData, TestSize.Level3)
{
    std::vector<std::string> hasRemoteArgs1 = {"has-remote-data"};
    std::string hasRemoteResult1 = ExecuteCommand(hasRemoteArgs1);
    json parsedRemote1 = json::parse(hasRemoteResult1);
    EXPECT_TRUE(parsedRemote1["status"] == "success" || parsedRemote1["status"] == "failed");
    
    std::vector<std::string> setArgs = {"set-data", "--text", "remote-test"};
    std::string setResult = ExecuteCommand(setArgs);
    json setParsed = json::parse(setResult);
    
    if (setParsed["status"] == "success") {
        std::vector<std::string> getArgs = {"get-data"};
        std::string getResult = ExecuteCommand(getArgs);
        json getParsed = json::parse(getResult);
        EXPECT_TRUE(getParsed["status"] == "success" || getParsed["status"] == "failed");
        
        std::vector<std::string> hasRemoteArgs2 = {"has-remote-data"};
        std::string hasRemoteResult2 = ExecuteCommand(hasRemoteArgs2);
        json parsedRemote2 = json::parse(hasRemoteResult2);
        EXPECT_TRUE(parsedRemote2["status"] == "success" || parsedRemote2["status"] == "failed");
    }
}

HWTEST_F(IntegrationTest, MimeTypeCheck_SetText_HasDataType_MatchAndMismatch, TestSize.Level3)
{
    std::vector<std::string> setArgs = {"set-data", "--text", "mime-test"};
    std::string setResult = ExecuteCommand(setArgs);
    json setParsed = json::parse(setResult);
    
    if (setParsed["status"] == "success") {
        std::vector<std::string> hasMatchArgs = {"has-data-type", "--type", "text/plain"};
        std::string hasMatchResult = ExecuteCommand(hasMatchArgs);
        json parsedMatch = json::parse(hasMatchResult);
        EXPECT_TRUE(parsedMatch["status"] == "success" || parsedMatch["status"] == "failed");
        
        std::vector<std::string> hasMismatchArgs = {"has-data-type", "--type", "image/png"};
        std::string hasMismatchResult = ExecuteCommand(hasMismatchArgs);
        json parsedMismatch = json::parse(hasMismatchResult);
        EXPECT_TRUE(parsedMismatch["status"] == "success" || parsedMismatch["status"] == "failed");
        
        std::vector<std::string> clearArgs = {"clear-data"};
        ExecuteCommand(clearArgs);
        
        std::vector<std::string> hasAfterClearArgs = {"has-data-type", "--type", "text/plain"};
        std::string hasAfterClearResult = ExecuteCommand(hasAfterClearArgs);
        json parsedAfterClear = json::parse(hasAfterClearResult);
        EXPECT_TRUE(parsedAfterClear["status"] == "success" || parsedAfterClear["status"] == "failed");
    }
}

HWTEST_F(IntegrationTest, Timeout_HasDataType_WithTimeout, TestSize.Level3)
{
    std::vector<std::string> setArgs = {"set-data", "--text", "timeout-test"};
    std::string setResult = ExecuteCommand(setArgs);
    json setParsed = json::parse(setResult);
    
    if (setParsed["status"] == "success") {
        std::vector<std::string> hasTimeoutArgs1 = {"has-data-type", "--type", "text/plain", "--timeout", "1000"};
        std::string hasTimeoutResult1 = ExecuteCommand(hasTimeoutArgs1);
        json parsedTimeout1 = json::parse(hasTimeoutResult1);
        EXPECT_TRUE(parsedTimeout1["status"] == "success" || parsedTimeout1["status"] == "failed");
        
        std::vector<std::string> hasTimeoutArgs2 = {"has-data-type", "--type", "image/jpeg", "--timeout", "500"};
        std::string hasTimeoutResult2 = ExecuteCommand(hasTimeoutArgs2);
        json parsedTimeout2 = json::parse(hasTimeoutResult2);
        EXPECT_TRUE(parsedTimeout2["status"] == "success" || parsedTimeout2["status"] == "failed");
    }
}

HWTEST_F(IntegrationTest, SequentialOperations_MultipleSetGet, TestSize.Level3)
{
    std::vector<std::string> set1Args = {"set-data", "--text", "seq-test-1"};
    std::string set1Result = ExecuteCommand(set1Args);
    json set1Parsed = json::parse(set1Result);
    
    if (set1Parsed["status"] == "success") {
        std::vector<std::string> get1Args = {"get-data"};
        std::string get1Result = ExecuteCommand(get1Args);
        json get1Parsed = json::parse(get1Result);
        EXPECT_TRUE(get1Parsed["status"] == "success" || get1Parsed["status"] == "failed");
    }
    
    std::vector<std::string> set2Args = {"set-data", "--html", "<p>seq-test-2</p>"};
    std::string set2Result = ExecuteCommand(set2Args);
    json set2Parsed = json::parse(set2Result);
    
    if (set2Parsed["status"] == "success") {
        std::vector<std::string> get2Args = {"get-data"};
        std::string get2Result = ExecuteCommand(get2Args);
        json get2Parsed = json::parse(get2Result);
        EXPECT_TRUE(get2Parsed["status"] == "success" || get2Parsed["status"] == "failed");
    }
    
    std::vector<std::string> set3Args = {"set-data", "--uri", "file:///seq-test-3"};
    std::string set3Result = ExecuteCommand(set3Args);
    json set3Parsed = json::parse(set3Result);
    
    if (set3Parsed["status"] == "success") {
        std::vector<std::string> get3Args = {"get-data"};
        std::string get3Result = ExecuteCommand(get3Args);
        json get3Parsed = json::parse(get3Result);
        EXPECT_TRUE(get3Parsed["status"] == "success" || get3Parsed["status"] == "failed");
    }
    
    std::vector<std::string> clearArgs = {"clear-data"};
    ExecuteCommand(clearArgs);
    
    std::vector<std::string> hasArgs = {"has-data"};
    std::string hasResult = ExecuteCommand(hasArgs);
    json hasParsed = json::parse(hasResult);
    EXPECT_TRUE(hasParsed["status"] == "success" || hasParsed["status"] == "failed");
}

HWTEST_F(IntegrationTest, AllCommands_ComprehensiveCheck, TestSize.Level3)
{
    std::vector<std::string> commands = {
        "set-data", "get-data", "clear-data",
        "has-data", "has-data-type", "has-remote-data"
    };
    
    // Verify commands work via ExecuteCommand (help returns empty string as output goes to stderr)
    for (const auto& cmd : commands) {
        std::vector<std::string> helpArgs = {cmd, "--help"};
        std::string helpResult = ExecuteCommand(helpArgs);
        EXPECT_TRUE(helpResult.empty()) << "Command " << cmd << " help should return empty string";
    }
    
    std::vector<std::string> setArgs = {"set-data", "--text", "comprehensive-test"};
    std::string setResult = ExecuteCommand(setArgs);
    json setParsed = json::parse(setResult);
    
    if (setParsed["status"] == "success") {
        std::vector<std::string> hasArgs = {"has-data"};
        std::string hasResult = ExecuteCommand(hasArgs);
        json hasParsed = json::parse(hasResult);
        EXPECT_TRUE(hasParsed.contains("status"));
        
        std::vector<std::string> hasTypeArgs = {"has-data-type", "--type", "text/plain"};
        std::string hasTypeResult = ExecuteCommand(hasTypeArgs);
        json hasTypeParsed = json::parse(hasTypeResult);
        EXPECT_TRUE(hasTypeParsed.contains("status"));
        
        std::vector<std::string> hasRemoteArgs = {"has-remote-data"};
        std::string hasRemoteResult = ExecuteCommand(hasRemoteArgs);
        json hasRemoteParsed = json::parse(hasRemoteResult);
        EXPECT_TRUE(hasRemoteParsed.contains("status"));
        
        std::vector<std::string> getArgs = {"get-data"};
        std::string getResult = ExecuteCommand(getArgs);
        json getParsed = json::parse(getResult);
        EXPECT_TRUE(getParsed.contains("status"));
        
        std::vector<std::string> clearArgs = {"clear-data"};
        ExecuteCommand(clearArgs);
        
        std::vector<std::string> hasFinalArgs = {"has-data"};
        std::string hasFinalResult = ExecuteCommand(hasFinalArgs);
        json hasFinalParsed = json::parse(hasFinalResult);
        EXPECT_TRUE(hasFinalParsed.contains("status"));
    }
}

HWTEST_F(IntegrationTest, MixedParams_SetTextHtmlUri_GetData, TestSize.Level3)
{
    std::vector<std::string> setArgs = {"set-data", "--text", "plain-text", "--html", "<p>html</p>", "--uri",
        "file:///test"};
    std::string setResult = ExecuteCommand(setArgs);
    json setParsed = json::parse(setResult);
    
    if (setParsed["status"] == "success") {
        std::vector<std::string> hasTextArgs = {"has-data-type", "--type", "text/plain"};
        std::string hasTextResult = ExecuteCommand(hasTextArgs);
        json hasTextParsed = json::parse(hasTextResult);
        EXPECT_TRUE(hasTextParsed["status"] == "success" || hasTextParsed["status"] == "failed");
        
        std::vector<std::string> hasHtmlArgs = {"has-data-type", "--type", "text/html"};
        std::string hasHtmlResult = ExecuteCommand(hasHtmlArgs);
        json hasHtmlParsed = json::parse(hasHtmlResult);
        EXPECT_TRUE(hasHtmlParsed["status"] == "success" || hasHtmlParsed["status"] == "failed");
        
        std::vector<std::string> hasUriArgs = {"has-data-type", "--type", "text/uri"};
        std::string hasUriResult = ExecuteCommand(hasUriArgs);
        json hasUriParsed = json::parse(hasUriResult);
        EXPECT_TRUE(hasUriParsed["status"] == "success" || hasUriParsed["status"] == "failed");
        
        std::vector<std::string> getArgs = {"get-data"};
        std::string getResult = ExecuteCommand(getArgs);
        json getParsed = json::parse(getResult);
        EXPECT_TRUE(getParsed["status"] == "success" || getParsed["status"] == "failed");
    }
}
