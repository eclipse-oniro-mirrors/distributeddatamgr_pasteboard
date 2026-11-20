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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "errors.h"
#include "ipasteboard_fuse_getter.h"
#include "iremote_object.h"
#include "pasteboard_error.h"
#include "pasteboard_fuse_getter_client.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace {
constexpr uint32_t TEST_RECORD_ID = 10;
constexpr uint32_t ABSENT_RECORD_ID = 99;
const std::string TEST_FILE_ID = "file_001";
const std::string TEST_FILE_NAME = "test.txt";
constexpr int64_t TEST_OFFSET = 0;
constexpr int32_t TEST_LENGTH = 16;
// Arbitrary non-zero value, distinct from ERR_INVALID_VALUE, used to verify the
// client forwards the provider's return code verbatim instead of remapping it.
constexpr int32_t DELEGATED_ERROR = static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID);

// Hand-written mock implementing IPasteboardFuseGetter. AsObject returns nullptr
// (no real IPC object is needed because the client under test calls the virtual
// methods directly). Each OnXxx records that it was called, captures the
// forwarded arguments, and returns / fills out-parameters with configurable
// values, so tests can assert both the delegation path and argument forwarding.
class MockFuseGetter : public IPasteboardFuseGetter {
public:
    OHOS::sptr<OHOS::IRemoteObject> AsObject() override { return nullptr; }

    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override
    {
        readCalled_ = true;
        lastRecordId_ = recordId;
        lastId_ = id;
        lastOffset_ = offset;
        lastLength_ = length;
        data = readData_;
        return readRet_;
    }

    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override
    {
        statCalled_ = true;
        lastRecordId_ = recordId;
        lastId_ = id;
        stat = stat_;
        return statRet_;
    }

    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override
    {
        cancelCalled_ = true;
        lastRecordId_ = recordId;
        lastIdList_ = idList;
        return cancelRet_;
    }

    // Configurable return data.
    std::vector<uint8_t> readData_{ 1, 2, 3, 4, 5 };
    int32_t readRet_ = 0;
    FuseFileStat stat_{};
    int32_t statRet_ = 0;
    int32_t cancelRet_ = 0;

    // Call capture.
    bool readCalled_ = false;
    bool statCalled_ = false;
    bool cancelCalled_ = false;
    uint32_t lastRecordId_ = 0;
    std::string lastId_;
    int64_t lastOffset_ = 0;
    int32_t lastLength_ = 0;
    std::vector<std::string> lastIdList_;
};

class PasteboardFuseGetterClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void PasteboardFuseGetterClientTest::SetUpTestCase(void) {}
void PasteboardFuseGetterClientTest::TearDownTestCase(void) {}
void PasteboardFuseGetterClientTest::SetUp(void) {}
void PasteboardFuseGetterClientTest::TearDown(void) {}
} // namespace

// ===== OnRead =====

/**
 * @tc.name: OnRead_001
 * @tc.desc: OnRead returns ERR_INVALID_VALUE and leaves data untouched when recordId is absent from providers.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnRead_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_001 start");
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<uint8_t> data;
    int32_t ret = client->OnRead(ABSENT_RECORD_ID, TEST_FILE_ID, TEST_OFFSET, TEST_LENGTH, data);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    EXPECT_TRUE(data.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_001 end");
}

/**
 * @tc.name: OnRead_002
 * @tc.desc: OnRead returns ERR_INVALID_VALUE when the provider mapped to recordId is null.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnRead_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_002 start");
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = sptr<IPasteboardFuseGetter>();
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<uint8_t> data;
    int32_t ret = client->OnRead(TEST_RECORD_ID, TEST_FILE_ID, TEST_OFFSET, TEST_LENGTH, data);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    EXPECT_TRUE(data.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_002 end");
}

/**
 * @tc.name: OnRead_003
 * @tc.desc: OnRead delegates to the matching provider on success, populating data and forwarding arguments.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnRead_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_003 start");
    auto mockSptr = sptr<MockFuseGetter>::MakeSptr();
    ASSERT_NE(mockSptr, nullptr);
    mockSptr->readRet_ = 0;
    mockSptr->readData_ = { 10, 20, 30 };

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = mockSptr;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<uint8_t> data;
    int32_t ret = client->OnRead(TEST_RECORD_ID, TEST_FILE_ID, TEST_OFFSET, TEST_LENGTH, data);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(data, mockSptr->readData_);
    EXPECT_TRUE(mockSptr->readCalled_);
    EXPECT_EQ(mockSptr->lastRecordId_, TEST_RECORD_ID);
    EXPECT_EQ(mockSptr->lastId_, TEST_FILE_ID);
    EXPECT_EQ(mockSptr->lastOffset_, TEST_OFFSET);
    EXPECT_EQ(mockSptr->lastLength_, TEST_LENGTH);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_003 end");
}

/**
 * @tc.name: OnRead_004
 * @tc.desc: OnRead forwards the provider error code verbatim when the provider fails.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnRead_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_004 start");
    auto mockSptr = sptr<MockFuseGetter>::MakeSptr();
    ASSERT_NE(mockSptr, nullptr);
    mockSptr->readRet_ = DELEGATED_ERROR;
    mockSptr->readData_.clear();

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = mockSptr;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<uint8_t> data;
    int32_t ret = client->OnRead(TEST_RECORD_ID, TEST_FILE_ID, TEST_OFFSET, TEST_LENGTH, data);
    EXPECT_EQ(ret, DELEGATED_ERROR);
    EXPECT_TRUE(mockSptr->readCalled_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRead_004 end");
}

// ===== OnStat =====

/**
 * @tc.name: OnStat_001
 * @tc.desc: OnStat returns ERR_INVALID_VALUE when recordId is absent from providers.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnStat_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_001 start");
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    FuseFileStat stat{};
    int32_t ret = client->OnStat(ABSENT_RECORD_ID, TEST_FILE_ID, stat);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    EXPECT_EQ(stat.size, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_001 end");
}

/**
 * @tc.name: OnStat_002
 * @tc.desc: OnStat returns ERR_INVALID_VALUE when the provider mapped to recordId is null.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnStat_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_002 start");
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = sptr<IPasteboardFuseGetter>();
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    FuseFileStat stat{};
    int32_t ret = client->OnStat(TEST_RECORD_ID, TEST_FILE_ID, stat);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    EXPECT_EQ(stat.size, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_002 end");
}

/**
 * @tc.name: OnStat_003
 * @tc.desc: OnStat delegates to the matching provider on success, populating stat and forwarding arguments.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnStat_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_003 start");
    auto mockSptr = sptr<MockFuseGetter>::MakeSptr();
    ASSERT_NE(mockSptr, nullptr);
    mockSptr->statRet_ = 0;
    mockSptr->stat_ = { TEST_FILE_ID, TEST_FILE_NAME, 1024, 123456789, 0 };

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = mockSptr;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    FuseFileStat stat{};
    int32_t ret = client->OnStat(TEST_RECORD_ID, TEST_FILE_ID, stat);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stat.id, TEST_FILE_ID);
    EXPECT_EQ(stat.name, TEST_FILE_NAME);
    EXPECT_EQ(stat.size, 1024);
    EXPECT_EQ(stat.modifyTime, 123456789);
    EXPECT_EQ(stat.type, 0);
    EXPECT_TRUE(mockSptr->statCalled_);
    EXPECT_EQ(mockSptr->lastRecordId_, TEST_RECORD_ID);
    EXPECT_EQ(mockSptr->lastId_, TEST_FILE_ID);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_003 end");
}

/**
 * @tc.name: OnStat_004
 * @tc.desc: OnStat forwards the provider error code verbatim when the provider fails.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnStat_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_004 start");
    auto mockSptr = sptr<MockFuseGetter>::MakeSptr();
    ASSERT_NE(mockSptr, nullptr);
    mockSptr->statRet_ = DELEGATED_ERROR;

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = mockSptr;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    FuseFileStat stat{};
    int32_t ret = client->OnStat(TEST_RECORD_ID, TEST_FILE_ID, stat);
    EXPECT_EQ(ret, DELEGATED_ERROR);
    EXPECT_TRUE(mockSptr->statCalled_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnStat_004 end");
}

// ===== OnCancel =====

/**
 * @tc.name: OnCancel_001
 * @tc.desc: OnCancel returns ERR_INVALID_VALUE when recordId is absent from providers.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnCancel_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_001 start");
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<std::string> idList = { "a", "b" };
    int32_t ret = client->OnCancel(ABSENT_RECORD_ID, idList);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_001 end");
}

/**
 * @tc.name: OnCancel_002
 * @tc.desc: OnCancel returns ERR_INVALID_VALUE when the provider mapped to recordId is null.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnCancel_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_002 start");
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = sptr<IPasteboardFuseGetter>();
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<std::string> idList = { "a", "b" };
    int32_t ret = client->OnCancel(TEST_RECORD_ID, idList);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_002 end");
}

/**
 * @tc.name: OnCancel_003
 * @tc.desc: OnCancel delegates to the matching provider on success, forwarding recordId and idList.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnCancel_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_003 start");
    auto mockSptr = sptr<MockFuseGetter>::MakeSptr();
    ASSERT_NE(mockSptr, nullptr);
    mockSptr->cancelRet_ = 0;

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = mockSptr;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<std::string> idList = { "file_001", "file_002", "file_003" };
    int32_t ret = client->OnCancel(TEST_RECORD_ID, idList);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(mockSptr->cancelCalled_);
    EXPECT_EQ(mockSptr->lastRecordId_, TEST_RECORD_ID);
    EXPECT_EQ(mockSptr->lastIdList_, idList);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_003 end");
}

/**
 * @tc.name: OnCancel_004
 * @tc.desc: OnCancel forwards the provider error code verbatim when the provider fails.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterClientTest, OnCancel_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_004 start");
    auto mockSptr = sptr<MockFuseGetter>::MakeSptr();
    ASSERT_NE(mockSptr, nullptr);
    mockSptr->cancelRet_ = DELEGATED_ERROR;

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers;
    providers[TEST_RECORD_ID] = mockSptr;
    auto client = sptr<PasteboardFuseGetterClient>::MakeSptr(providers);
    ASSERT_NE(client, nullptr);

    std::vector<std::string> idList = { "a", "b" };
    int32_t ret = client->OnCancel(TEST_RECORD_ID, idList);
    EXPECT_EQ(ret, DELEGATED_ERROR);
    EXPECT_TRUE(mockSptr->cancelCalled_);
    EXPECT_EQ(mockSptr->lastIdList_, idList);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnCancel_004 end");
}
