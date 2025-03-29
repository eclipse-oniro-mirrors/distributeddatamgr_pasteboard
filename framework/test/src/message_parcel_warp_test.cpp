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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstring>
#include <sys/mman.h>
#include <iostream>

#include "message_parcel_warp.h"
using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {

constexpr size_t MIN_RAW_SIZE = 32 * 1024; // 32k
class MessageParcelWarpInterface {
public:
    MessageParcelWarpInterface() {};
    virtual ~MessageParcelWarpInterface() {};

    virtual bool WriteInt64(int64_t value) = 0;
    virtual int64_t ReadInt64() = 0;
    virtual bool WriteFileDescriptor(int fd) = 0;
    virtual int ReadFileDescriptor() = 0;
    virtual bool WriteUnpadBuffer(const void *data, size_t size) = 0;
    virtual uint8_t *ReadUnpadBuffer(size_t length) = 0;
};

class MessageParcelWarpMock : public MessageParcelWarpInterface {
public:
    MessageParcelWarpMock();
    ~MessageParcelWarpMock() override;

    MOCK_METHOD1(WriteInt64, bool(int64_t value));
    MOCK_METHOD0(ReadInt64, int64_t());
    MOCK_METHOD1(WriteFileDescriptor, bool(int fd));
    MOCK_METHOD0(ReadFileDescriptor, int());
    MOCK_METHOD2(WriteUnpadBuffer, bool(const void *data, size_t size));
    MOCK_METHOD1(ReadUnpadBuffer, uint8_t *(size_t length));
};

static void *g_interface = nullptr;

MessageParcelWarpMock::MessageParcelWarpMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

MessageParcelWarpMock::~MessageParcelWarpMock()
{
    g_interface = nullptr;
}

static MessageParcelWarpInterface *GetMessageParcelWarpInterface()
{
    return reinterpret_cast<MessageParcelWarpInterface *>(g_interface);
}

extern "C" {
    bool Parcel::WriteInt64(int64_t value)
    {
        if (GetMessageParcelWarpInterface() == nullptr) {
            return false;
        }
        return GetMessageParcelWarpInterface()->WriteInt64(value);
    }
    int64_t Parcel::ReadInt64()
    {
        if (GetMessageParcelWarpInterface() == nullptr) {
            return 0;
        }
        return GetMessageParcelWarpInterface()->ReadInt64();
    }
    bool MessageParcel::WriteFileDescriptor(int fd)
    {
        if (GetMessageParcelWarpInterface() == nullptr) {
            return false;
        }
        return GetMessageParcelWarpInterface()->WriteFileDescriptor(fd);
    }
    int MessageParcel::ReadFileDescriptor()
    {
        if (GetMessageParcelWarpInterface() == nullptr) {
            return 0;
        }
        return GetMessageParcelWarpInterface()->ReadFileDescriptor();
    }
    bool Parcel::WriteUnpadBuffer(const void *data, size_t size)
    {
        if (GetMessageParcelWarpInterface() == nullptr) {
            return false;
        }
        return GetMessageParcelWarpInterface()->WriteUnpadBuffer(data, size);
    }
    const uint8_t *Parcel::ReadUnpadBuffer(size_t length)
    {
        if (GetMessageParcelWarpInterface() == nullptr) {
            return nullptr;
        }
        return GetMessageParcelWarpInterface()->ReadUnpadBuffer(length);
    }
}

namespace MiscServices {
class MessageParcelWarpTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void MessageParcelWarpTest::SetUpTestCase(void) { }

void MessageParcelWarpTest::TearDownTestCase(void) { }

void MessageParcelWarpTest::SetUp(void) { }

void MessageParcelWarpTest::TearDown(void) { }

/**
 * @tc.name: MemcpyDataTest001
 * @tc.desc: Test MemcpyData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, MemcpyDataTest001, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    size_t size = 1024;
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    const char data[] = "Test data";
    size_t count = strlen(data) + 1;

    auto result = messageParcelWarp.MemcpyData(ptr, size, data, count);
    EXPECT_TRUE(result);

    char* resultData = static_cast<char*>(ptr);
    EXPECT_STREQ(resultData, data);
    munmap(ptr, size);
}

/**
 * @tc.name: MemcpyDataTest002
 * @tc.desc: Test MemcpyData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, MemcpyDataTest002, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    size_t size = 256 * 1024 * 1024 + 1;
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char* data = new char[size];
    std::fill(data, data + size, 'A');

    auto result = messageParcelWarp.MemcpyData(ptr, size, data, size);
    EXPECT_TRUE(result);

    char* resultData = static_cast<char*>(ptr);
    EXPECT_STREQ(resultData, data);
    munmap(ptr, size);
}

/**
 * @tc.name: WriteRawDataTest001
 * @tc.desc: Test WriteRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, WriteRawDataTest001, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    MessageParcel parcel;
    const char data[] = "Test data";
    size_t size = strlen(data) + 1;

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, WriteInt64).WillOnce(testing::Return(false));

    auto result = messageParcelWarp.WriteRawData(parcel, data, size);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: WriteRawDataTest002
 * @tc.desc: Test WriteRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, WriteRawDataTest002, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    MessageParcel parcel;
    const char data[] = "Test data";
    size_t size = strlen(data) + 1;

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, WriteInt64).WillOnce(testing::Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer).WillOnce(testing::Return(true));

    auto result = messageParcelWarp.WriteRawData(parcel, data, size);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: WriteRawDataTest003
 * @tc.desc: Test WriteRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, WriteRawDataTest003, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    MessageParcel parcel;
    size_t size = MIN_RAW_SIZE + 1;
    const char* data = new char[size];
    std::fill(const_cast<char*>(data), const_cast<char*>(data) + size, 'A');

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, WriteInt64).WillOnce(testing::Return(true));
    EXPECT_CALL(mock, WriteFileDescriptor).WillOnce(testing::Return(false));

    auto result = messageParcelWarp.WriteRawData(parcel, data, size);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: WriteRawDataTest004
 * @tc.desc: Test WriteRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, WriteRawDataTest004, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    MessageParcel parcel;
    size_t size = MIN_RAW_SIZE + 1;
    const char* data = new char[size];
    std::fill(const_cast<char*>(data), const_cast<char*>(data) + size, 'A');

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, WriteInt64).WillOnce(testing::Return(true));
    EXPECT_CALL(mock, WriteFileDescriptor).WillOnce(testing::Return(true));

    auto result = messageParcelWarp.WriteRawData(parcel, data, size);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: ReadRawDataTest001
 * @tc.desc: Test ReadRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, ReadRawDataTest001, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    MessageParcel parcel;
    size_t size = MIN_RAW_SIZE - 1;

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, ReadInt64).WillOnce(testing::Return(1));
    EXPECT_CALL(mock, ReadUnpadBuffer).WillRepeatedly(testing::Return(nullptr));

    auto result = messageParcelWarp.ReadRawData(parcel, size);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: ReadRawDataTest002
 * @tc.desc: Test ReadRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, ReadRawDataTest002, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    messageParcelWarp.rawData_ = std::make_shared<char>('A');
    messageParcelWarp.writeRawDataFd_ = 0;
    MessageParcel parcel;
    size_t size = MIN_RAW_SIZE + 1;

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, ReadInt64).WillOnce(testing::Return(size));
    EXPECT_CALL(mock, ReadFileDescriptor).WillRepeatedly(testing::Return(1));

    auto result = messageParcelWarp.ReadRawData(parcel, size);
    EXPECT_NE(result, messageParcelWarp.rawData_.get());
}

/**
 * @tc.name: ReadRawDataTest003
 * @tc.desc: Test ReadRawData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(MessageParcelWarpTest, ReadRawDataTest003, TestSize.Level0)
{
    MessageParcelWarp messageParcelWarp;
    messageParcelWarp.rawData_ = nullptr;
    MessageParcel parcel;
    size_t size = MIN_RAW_SIZE + 1;

    NiceMock<MessageParcelWarpMock> mock;
    EXPECT_CALL(mock, ReadInt64).WillOnce(testing::Return(size));
    EXPECT_CALL(mock, ReadFileDescriptor).WillRepeatedly(testing::Return(-1));

    auto result = messageParcelWarp.ReadRawData(parcel, size);
    EXPECT_EQ(result, nullptr);
}
}
}