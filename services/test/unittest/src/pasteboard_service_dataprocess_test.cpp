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
#include "paste_data_entry.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
}

class PasteboardServiceDataTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceDataTest SetUpTestCase");
    }

    static void TearDownTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceDataTest TearDownTestCase");
    }

    void SetUp()
    {
        service_ = std::make_shared<PasteboardService>();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceDataTest SetUp");
    }

    void TearDown()
    {
        service_ = nullptr;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceDataTest TearDown");
    }

protected:
    std::shared_ptr<PasteboardService> service_ = nullptr;
};

/**
 * @tc.name: DealDataTest002
 * @tc.desc: test DealData with small data size (<= MIN_ASHMEM_DATA_SIZE)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, DealDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest002 start");
    PasteData data;
    data.AddTextRecord("test");
    
    int fd = -1;
    int64_t size = 0;
    std::vector<uint8_t> rawData;
    
    int32_t ret = service_->DealData(fd, size, rawData, data);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_GT(size, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest002 end");
}

/**
 * @tc.name: DealDataTest003
 * @tc.desc: test DealData with large data size (> MIN_ASHMEM_DATA_SIZE)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, DealDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest003 start");
    PasteData data;
    std::string largeText(MIN_ASHMEM_DATA_SIZE + 1024, 'a');
    data.AddTextRecord(largeText);
    
    int fd = -1;
    int64_t size = 0;
    std::vector<uint8_t> rawData;
    
    int32_t ret = service_->DealData(fd, size, rawData, data);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_GT(size, MIN_ASHMEM_DATA_SIZE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest003 end");
}

/**
 * @tc.name: DealDataTest004
 * @tc.desc: test DealData with empty PasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, DealDataTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest004 start");
    PasteData data;
    
    int fd = -1;
    int64_t size = 0;
    std::vector<uint8_t> rawData;
    
    int32_t ret = service_->DealData(fd, size, rawData, data);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_GT(size, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest004 end");
}

/**
 * @tc.name: WriteRawDataTest001
 * @tc.desc: test WriteRawData with null data pointer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, WriteRawDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest001 start");
    int64_t size = 1024;
    int fd = -1;
    
    bool ret = service_->WriteRawData(nullptr, size, fd);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest001 end");
}

/**
 * @tc.name: WriteRawDataTest002
 * @tc.desc: test WriteRawData with invalid size (<= 0)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, WriteRawDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest002 start");
    std::vector<uint8_t> buffer = {1, 2, 3, 4, 5};
    int64_t size = 0;
    int fd = -1;
    
    bool ret = service_->WriteRawData(buffer.data(), size, fd);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest002 end");
}

/**
 * @tc.name: WriteRawDataTest003
 * @tc.desc: test WriteRawData with negative size
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, WriteRawDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest003 start");
    std::vector<uint8_t> buffer = {1, 2, 3, 4, 5};
    int64_t size = -100;
    int fd = -1;
    
    bool ret = service_->WriteRawData(buffer.data(), size, fd);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest003 end");
}

/**
 * @tc.name: HasPasteDataTest001
 * @tc.desc: test HasPasteData with no data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, HasPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest001 start");
    bool result = false;
    
    int32_t ret = service_->HasPasteData(result);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest001 end");
}

/**
 * @tc.name: HasPasteDataTest002
 * @tc.desc: test HasPasteData with valid data in clips_
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, HasPasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest002 start");
    int32_t userId = 100;
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddTextRecord("test");
    service_->clips_.InsertOrAssign(userId, pasteData);
    
    bool result = false;
    int32_t ret = service_->HasPasteData(result);
    EXPECT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest002 end");
}

/**
 * @tc.name: ClearTest001
 * @tc.desc: test Clear with valid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, ClearTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearTest001 start");
    int32_t userId = 100;
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddTextRecord("test");
    service_->clips_.InsertOrAssign(userId, pasteData);
    
    int32_t ret = service_->Clear();
    EXPECT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearTest001 end");
}

/**
 * @tc.name: ClearByUserTest001
 * @tc.desc: test ClearByUser with specific userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, ClearByUserTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByUserTest001 start");
    int32_t userId = 100;
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddTextRecord("test");
    service_->clips_.InsertOrAssign(userId, pasteData);
    
    int32_t ret = service_->ClearByUser(userId);
    EXPECT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByUserTest001 end");
}

/**
 * @tc.name: GetChangeCountTest001
 * @tc.desc: test GetChangeCount with no data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, GetChangeCountTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetChangeCountTest001 start");
    uint32_t changeCount = 0;
    
    int32_t ret = service_->GetChangeCount(changeCount);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(changeCount, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetChangeCountTest001 end");
}

/**
 * @tc.name: IncreaseChangeCountTest001
 * @tc.desc: test IncreaseChangeCount increments counter
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, IncreaseChangeCountTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest001 start");
    int32_t userId = 100;
    
    service_->IncreaseChangeCount(userId);
    
    uint32_t changeCount = 0;
    service_->clipChangeCount_.ComputeIfPresent(userId, [&changeCount](auto, auto &value) {
        changeCount = value;
        return true;
    });
    EXPECT_EQ(changeCount, 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest001 end");
}

/**
 * @tc.name: IncreaseChangeCountTest002
 * @tc.desc: test IncreaseChangeCount with UINT32_MAX value
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceDataTest, IncreaseChangeCountTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest002 start");
    int32_t userId = 200;
    service_->clipChangeCount_.InsertOrAssign(userId, UINT32_MAX);
    
    service_->IncreaseChangeCount(userId);
    
    uint32_t changeCount = 0;
    service_->clipChangeCount_.ComputeIfPresent(userId, [&changeCount](auto, auto &value) {
        changeCount = value;
        return true;
    });
    EXPECT_EQ(changeCount, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest002 end");
}
}