/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "pasteboard_service.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "paste_data_record.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
constexpr uid_t ANCO_SERVICE_BROKER_UID = 5557;
}

class PasteboardServiceUriTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceUriTest SetUpTestCase");
    }

    static void TearDownTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceUriTest TearDownTestCase");
    }

    void SetUp()
    {
        service_ = std::make_shared<PasteboardService>();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceUriTest SetUp");
    }

    void TearDown()
    {
        service_ = nullptr;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceUriTest TearDown");
    }

protected:
    std::shared_ptr<PasteboardService> service_ = nullptr;
};

/**
 * @tc.name: GrantUriPermissionTest002
 * @tc.desc: test GrantUriPermission with readUris empty and writeUris empty
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GrantUriPermissionTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest002 start");
    std::map<uint32_t, std::vector<Uri>> grantUris;
    uint32_t targetTokenId = 123456;
    bool isRemoteData = false;
    
    int32_t ret = service_->GrantUriPermission(grantUris, targetTokenId, isRemoteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest002 end");
}

/**
 * @tc.name: GrantUriPermissionTest003
 * @tc.desc: test GrantUriPermission with readUris only
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GrantUriPermissionTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest003 start");
    std::map<uint32_t, std::vector<Uri>> grantUris;
    std::vector<Uri> readUris;
    readUris.emplace_back(Uri("file://com.example.test/path1"));
    grantUris[PasteDataRecord::READ_PERMISSION] = readUris;
    
    uint32_t targetTokenId = 123456;
    bool isRemoteData = false;
    
    int32_t ret = service_->GrantUriPermission(grantUris, targetTokenId, isRemoteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest003 end");
}

/**
 * @tc.name: GrantUriPermissionTest004
 * @tc.desc: test GrantUriPermission with writeUris only
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GrantUriPermissionTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest004 start");
    std::map<uint32_t, std::vector<Uri>> grantUris;
    std::vector<Uri> writeUris;
    writeUris.emplace_back(Uri("file://com.example.test/path2"));
    grantUris[PasteDataRecord::READ_WRITE_PERMISSION] = writeUris;
    
    uint32_t targetTokenId = 123456;
    bool isRemoteData = false;
    
    int32_t ret = service_->GrantUriPermission(grantUris, targetTokenId, isRemoteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest004 end");
}

/**
 * @tc.name: GrantUriPermissionTest005
 * @tc.desc: test GrantUriPermission with both read and write uris
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GrantUriPermissionTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest005 start");
    std::map<uint32_t, std::vector<Uri>> grantUris;
    std::vector<Uri> readUris;
    readUris.emplace_back(Uri("file://com.example.test/path1"));
    std::vector<Uri> writeUris;
    writeUris.emplace_back(Uri("file://com.example.test/path2"));
    
    grantUris[PasteDataRecord::READ_PERMISSION] = readUris;
    grantUris[PasteDataRecord::READ_WRITE_PERMISSION] = writeUris;
    
    uint32_t targetTokenId = 123456;
    bool isRemoteData = false;
    
    int32_t ret = service_->GrantUriPermission(grantUris, targetTokenId, isRemoteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest005 end");
}

/**
 * @tc.name: GrantUriPermissionTest006
 * @tc.desc: test GrantUriPermission with isRemoteData true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GrantUriPermissionTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest006 start");
    std::map<uint32_t, std::vector<Uri>> grantUris;
    std::vector<Uri> readUris;
    readUris.emplace_back(Uri("file://com.example.test/path1?networkid=device123"));
    grantUris[PasteDataRecord::READ_PERMISSION] = readUris;
    
    uint32_t targetTokenId = 123456;
    bool isRemoteData = true;
    
    int32_t ret = service_->GrantUriPermission(grantUris, targetTokenId, isRemoteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest006 end");
}

/**
 * @tc.name: RemoveInvalidRemoteUriTest001
 * @tc.desc: test RemoveInvalidRemoteUri with empty vector
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, RemoveInvalidRemoteUriTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveInvalidRemoteUriTest001 start");
    std::vector<Uri> grantUris;
    
    service_->RemoveInvalidRemoteUri(grantUris);
    EXPECT_TRUE(grantUris.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveInvalidRemoteUriTest001 end");
}

/**
 * @tc.name: RemoveInvalidRemoteUriTest002
 * @tc.desc: test RemoveInvalidRemoteUri with valid remote URI containing networkid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, RemoveInvalidRemoteUriTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveInvalidRemoteUriTest002 start");
    std::vector<Uri> grantUris;
    grantUris.emplace_back(Uri("file://com.example.test/path?networkid=device123"));
    grantUris.emplace_back(Uri("file://com.example.test/path2"));
    
    service_->RemoveInvalidRemoteUri(grantUris);
    EXPECT_EQ(grantUris.size(), 1u);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveInvalidRemoteUriTest002 end");
}

/**
 * @tc.name: RemoveInvalidRemoteUriTest003
 * @tc.desc: test RemoveInvalidRemoteUri with all URIs containing networkid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, RemoveInvalidRemoteUriTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveInvalidRemoteUriTest003 start");
    std::vector<Uri> grantUris;
    grantUris.emplace_back(Uri("file://com.example.test/path1?networkid=device1"));
    grantUris.emplace_back(Uri("file://com.example.test/path2?networkid=device2"));
    
    service_->RemoveInvalidRemoteUri(grantUris);
    EXPECT_EQ(grantUris.size(), 2u);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveInvalidRemoteUriTest003 end");
}

/**
 * @tc.name: CheckUriPermissionTest001
 * @tc.desc: test CheckUriPermission with empty PasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, CheckUriPermissionTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CheckUriPermissionTest001 start");
    PasteData data;
    std::pair<std::string, int32_t> targetBundleAndIndex = {"com.example.test", 0};
    
    auto result = service_->CheckUriPermission(data, targetBundleAndIndex);
    EXPECT_TRUE(result.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CheckUriPermissionTest001 end");
}

/**
 * @tc.name: CheckUriPermissionTest002
 * @tc.desc: test CheckUriPermission with PasteData containing text record
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, CheckUriPermissionTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CheckUriPermissionTest002 start");
    PasteData data;
    data.AddTextRecord("test");
    std::pair<std::string, int32_t> targetBundleAndIndex = {"com.example.test", 0};
    
    auto result = service_->CheckUriPermission(data, targetBundleAndIndex);
    EXPECT_TRUE(result.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CheckUriPermissionTest002 end");
}

/**
 * @tc.name: GenerateDistributedUriTest001
 * @tc.desc: test GenerateDistributedUri with empty PasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GenerateDistributedUriTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest001 start");
    PasteData pasteData;
    
    service_->GenerateDistributedUri(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest001 end");
}

/**
 * @tc.name: GenerateDistributedUriTest002
 * @tc.desc: test GenerateDistributedUri with PasteData containing URI record
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, GenerateDistributedUriTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest002 start");
    PasteData pasteData;
    OHOS::Uri uri("file://com.example.test/path");
    pasteData.AddUriRecord(uri);
    
    service_->GenerateDistributedUri(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest002 end");
}

/**
 * @tc.name: HasRemoteUriTest001
 * @tc.desc: test HasRemoteUri with null data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, HasRemoteUriTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasRemoteUriTest001 start");
    auto data = std::make_shared<PasteData>();
    data->AddTextRecord("test");
    
    bool ret = service_->HasRemoteUri(data);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasRemoteUriTest001 end");
}

/**
 * @tc.name: HasRemoteUriTest002
 * @tc.desc: test HasRemoteUri with data containing local URI
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, HasRemoteUriTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasRemoteUriTest002 start");
    auto data = std::make_shared<PasteData>();
    OHOS::Uri uri("file://com.example.test/localpath");
    data->AddUriRecord(uri);
    
    bool ret = service_->HasRemoteUri(data);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasRemoteUriTest002 end");
}

/**
 * @tc.name: HasRemoteDataTest001
 * @tc.desc: test HasRemoteData with no data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceUriTest, HasRemoteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasRemoteDataTest001 start");
    bool result = false;
    
    int32_t ret = service_->HasRemoteData(result);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasRemoteDataTest001 end");
}
}