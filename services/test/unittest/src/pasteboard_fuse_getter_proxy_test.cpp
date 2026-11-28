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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ipc_object_stub.h"
#include "message_parcel.h"
#include "pasteboard_error.h"
#include "pasteboard_fuse_getter_proxy.h"
#include "pasteboard_hilog.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
    const uint32_t UINT32_RANDOM = 333;
    const std::string STRING_ID = "fuse_record_id";
    const int64_t INT64_OFFSET = 1024;
    const int32_t INT32_LENGTH = 4096;
    const int32_t SENDREQUEST_FAIL = 1;
    const int32_t REMOTE_RESULT_ERROR = 12900020;
    const int32_t DATA_SIZE_POSITIVE = 4;
    const int32_t DATA_SIZE_ZERO = 0;
    const int64_t STAT_SIZE = 2048;
    const int64_t STAT_MODIFY_TIME = 1700000000;
    const int32_t STAT_TYPE_FILE = 0;
    const std::u16string DESCRIPTOR_TEST = u"test_descriptor";
}

// Pure-virtual interface that mirrors the Parcel/MessageParcel methods the
// PasteboardFuseGetterProxy touches. Routing the real Parcel member functions
// through this interface (see the extern "C" block below) lets us force write
// failures and control reply read values with gmock, exactly like the existing
// PasteboardEntryGetterProxy / PasteboardDelayGetterProxy tests do.
class PasteboardFuseGetterProxyInterface {
public:
    PasteboardFuseGetterProxyInterface() {};
    virtual ~PasteboardFuseGetterProxyInterface() {};

    virtual bool WriteInterfaceToken(std::u16string name) = 0;
    virtual bool WriteUint32(uint32_t value) = 0;
    virtual bool WriteString(const std::string &value) = 0;
    virtual bool WriteInt64(int64_t value) = 0;
    virtual bool WriteInt32(int32_t value) = 0;
    virtual bool WriteStringVector(const std::vector<std::string> &val) = 0;
    virtual int32_t ReadInt32() = 0;
    virtual const std::string ReadString() = 0;
    virtual int64_t ReadInt64() = 0;
    virtual bool ReadUInt8Vector(std::vector<uint8_t> *val) = 0;
    virtual const uint8_t *ReadUnpadBuffer(size_t length) = 0;
};

class PasteboardFuseGetterProxyMock : public PasteboardFuseGetterProxyInterface {
public:
    PasteboardFuseGetterProxyMock();
    ~PasteboardFuseGetterProxyMock() override;

    MOCK_METHOD1(WriteInterfaceToken, bool(std::u16string name));
    MOCK_METHOD1(WriteUint32, bool(uint32_t value));
    MOCK_METHOD1(WriteString, bool(const std::string &value));
    MOCK_METHOD1(WriteInt64, bool(int64_t value));
    MOCK_METHOD1(WriteInt32, bool(int32_t value));
    MOCK_METHOD1(WriteStringVector, bool(const std::vector<std::string> &val));
    MOCK_METHOD0(ReadInt32, int32_t());
    MOCK_METHOD0(ReadString, const std::string());
    MOCK_METHOD0(ReadInt64, int64_t());
    MOCK_METHOD1(ReadUInt8Vector, bool(std::vector<uint8_t> *val));
    MOCK_METHOD1(ReadUnpadBuffer, const uint8_t *(size_t length));
};

static void *g_interface = nullptr;
// Controls TestIRemoteObject::SendRequest return value: true -> ERR_OK, false -> non-zero.
static bool g_sendrequest = true;

PasteboardFuseGetterProxyMock::PasteboardFuseGetterProxyMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardFuseGetterProxyMock::~PasteboardFuseGetterProxyMock()
{
    g_interface = nullptr;
}

static PasteboardFuseGetterProxyInterface *GetFuseGetterProxyInterface()
{
    return reinterpret_cast<PasteboardFuseGetterProxyInterface *>(g_interface);
}

class TestIRemoteObject : public IRemoteObject {
    TestIRemoteObject(): IRemoteObject(DESCRIPTOR_TEST) {}

    int32_t GetObjectRefCount()
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return g_sendrequest ? 0 : SENDREQUEST_FAIL;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }

    int Dump(int fd, const std::vector<std::u16string> &args)
    {
        return 0;
    }
};

extern "C" {
    bool MessageParcel::WriteInterfaceToken(std::u16string name)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteInterfaceToken(name);
    }

    bool Parcel::WriteUint32(uint32_t value)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteUint32(value);
    }

    bool Parcel::WriteString(const std::string &value)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteString(value);
    }

    bool Parcel::WriteInt64(int64_t value)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteInt64(value);
    }

    bool Parcel::WriteInt32(int32_t value)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteInt32(value);
    }

    bool Parcel::WriteStringVector(const std::vector<std::string> &val)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteStringVector(val);
    }

    int32_t Parcel::ReadInt32()
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return 0;
        }
        return interface->ReadInt32();
    }

    const std::string Parcel::ReadString()
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return "";
        }
        return interface->ReadString();
    }

    int64_t Parcel::ReadInt64()
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return 0;
        }
        return interface->ReadInt64();
    }

    bool Parcel::ReadUInt8Vector(std::vector<uint8_t> *val)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->ReadUInt8Vector(val);
    }

    const uint8_t *Parcel::ReadUnpadBuffer(size_t length)
    {
        auto *interface = GetFuseGetterProxyInterface();
        if (interface == nullptr) {
            return nullptr;
        }
        return interface->ReadUnpadBuffer(length);
    }
};

class PasteboardFuseGetterProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardFuseGetterProxyTest::SetUpTestCase(void) { }

void PasteboardFuseGetterProxyTest::TearDownTestCase(void) { }

void PasteboardFuseGetterProxyTest::SetUp(void)
{
    g_sendrequest = true;
}

void PasteboardFuseGetterProxyTest::TearDown(void) { }

namespace MiscServices {
// ============================ OnRead ============================

/**
 * @tc.name: OnReadTest001
 * @tc.desc: Test OnRead when WriteInterfaceToken returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest001 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(false));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest001 end");
}

/**
 * @tc.name: OnReadTest002
 * @tc.desc: Test OnRead when WriteUint32 returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest002 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(false));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest002 end");
}

/**
 * @tc.name: OnReadTest003
 * @tc.desc: Test OnRead when WriteString returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest003 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(false));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest003 end");
}

/**
 * @tc.name: OnReadTest004
 * @tc.desc: Test OnRead when WriteInt64 returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest004 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(false));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest004 end");
}

/**
 * @tc.name: OnReadTest005
 * @tc.desc: Test OnRead when WriteInt32 returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest005 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(false));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest005 end");
}

/**
 * @tc.name: OnReadTest006
 * @tc.desc: Test OnRead when Remote() returns null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest006 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest006 end");
}

/**
 * @tc.name: OnReadTest007
 * @tc.desc: Test OnRead when SendRequest returns non-ERR_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest007 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    g_sendrequest = false;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, SENDREQUEST_FAIL);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest007 end");
}

/**
 * @tc.name: OnReadTest008
 * @tc.desc: Test OnRead when reply result is not ERR_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest008 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt32()).WillOnce(Return(REMOTE_RESULT_ERROR));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, REMOTE_RESULT_ERROR);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest008 end");
}

/**
 * @tc.name: OnReadTest009
 * @tc.desc: Test OnRead when dataSize > 0 but ReadRawData fails (ReadUnpadBuffer returns null)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest009 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt32())
        .WillOnce(Return(ERR_OK))
        .WillOnce(Return(DATA_SIZE_POSITIVE))
        .WillOnce(Return(DATA_SIZE_POSITIVE));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(nullptr));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest009 end");
}

/**
 * @tc.name: OnReadTest010
 * @tc.desc: Test OnRead success when dataSize > 0 and ReadRawData succeeds
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest010 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    std::vector<uint8_t> expected = { 0x01, 0x02, 0x03, 0x04 };
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt32())
        .WillOnce(Return(ERR_OK))
        .WillOnce(Return(DATA_SIZE_POSITIVE))
        .WillOnce(Return(DATA_SIZE_POSITIVE));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(expected.data()));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_EQ(data, expected);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest010 end");
}

/**
 * @tc.name: OnReadTest011
 * @tc.desc: Test OnRead success when dataSize == 0 (no ReadRawData call)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest011, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest011 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt32()).WillOnce(Return(ERR_OK)).WillOnce(Return(DATA_SIZE_ZERO));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_TRUE(data.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest011 end");
}

/**
 * @tc.name: OnReadTest012
 * @tc.desc: Test OnRead when ReadRawData internal size check fails (WriteRawData size != expected size)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnReadTest012, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest012 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    // reply: result(OK), size(4), ReadRawData internal size(999 != 4) -> nullptr
    EXPECT_CALL(mock, ReadInt32())
        .WillOnce(Return(ERR_OK))
        .WillOnce(Return(DATA_SIZE_POSITIVE))
        .WillOnce(Return(999));
    std::vector<uint8_t> data;
    int32_t result = proxy.OnRead(UINT32_RANDOM, STRING_ID, INT64_OFFSET, INT32_LENGTH, data);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    EXPECT_TRUE(data.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReadTest012 end");
}

// ============================ OnStat ============================

/**
 * @tc.name: OnStatTest001
 * @tc.desc: Test OnStat when WriteInterfaceToken returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest001 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(false));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest001 end");
}

/**
 * @tc.name: OnStatTest002
 * @tc.desc: Test OnStat when WriteUint32 returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest002 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(false));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest002 end");
}

/**
 * @tc.name: OnStatTest003
 * @tc.desc: Test OnStat when WriteString returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest003 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(false));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest003 end");
}

/**
 * @tc.name: OnStatTest004
 * @tc.desc: Test OnStat when Remote() returns null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest004 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest004 end");
}

/**
 * @tc.name: OnStatTest005
 * @tc.desc: Test OnStat when SendRequest returns non-ERR_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest005 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    g_sendrequest = false;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, SENDREQUEST_FAIL);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest005 end");
}

/**
 * @tc.name: OnStatTest006
 * @tc.desc: Test OnStat when reply result is not ERR_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest006 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt32()).WillOnce(Return(REMOTE_RESULT_ERROR));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, REMOTE_RESULT_ERROR);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest006 end");
}

/**
 * @tc.name: OnStatTest007
 * @tc.desc: Test OnStat success with stat fields populated
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnStatTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest007 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    // reply layout: result(ERR_OK), id, name, size, modifyTime, type
    EXPECT_CALL(mock, ReadInt32())
        .WillOnce(Return(ERR_OK))
        .WillOnce(Return(STAT_TYPE_FILE));
    EXPECT_CALL(mock, ReadString())
        .WillOnce(Return(STRING_ID))
        .WillOnce(Return("fuse_file_name"));
    EXPECT_CALL(mock, ReadInt64())
        .WillOnce(Return(STAT_SIZE))
        .WillOnce(Return(STAT_MODIFY_TIME));
    FuseFileStat stat;
    int32_t result = proxy.OnStat(UINT32_RANDOM, STRING_ID, stat);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_EQ(stat.id, STRING_ID);
    EXPECT_EQ(stat.name, "fuse_file_name");
    EXPECT_EQ(stat.size, STAT_SIZE);
    EXPECT_EQ(stat.modifyTime, STAT_MODIFY_TIME);
    EXPECT_EQ(stat.type, STAT_TYPE_FILE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStatTest007 end");
}

// ============================ OnCancel ============================

/**
 * @tc.name: OnCancelTest001
 * @tc.desc: Test OnCancel when WriteInterfaceToken returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnCancelTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest001 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(false));
    std::vector<std::string> idList = { "id1", "id2" };
    int32_t result = proxy.OnCancel(UINT32_RANDOM, idList);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest001 end");
}

/**
 * @tc.name: OnCancelTest002
 * @tc.desc: Test OnCancel when WriteUint32 returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnCancelTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest002 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(false));
    std::vector<std::string> idList = { "id1", "id2" };
    int32_t result = proxy.OnCancel(UINT32_RANDOM, idList);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest002 end");
}

/**
 * @tc.name: OnCancelTest003
 * @tc.desc: Test OnCancel when WriteStringVector returns false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnCancelTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest003 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteStringVector(testing::_)).WillOnce(Return(false));
    std::vector<std::string> idList = { "id1", "id2" };
    int32_t result = proxy.OnCancel(UINT32_RANDOM, idList);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest003 end");
}

/**
 * @tc.name: OnCancelTest004
 * @tc.desc: Test OnCancel when Remote() returns null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnCancelTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest004 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteStringVector(testing::_)).WillOnce(Return(true));
    std::vector<std::string> idList = { "id1", "id2" };
    int32_t result = proxy.OnCancel(UINT32_RANDOM, idList);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest004 end");
}

/**
 * @tc.name: OnCancelTest005
 * @tc.desc: Test OnCancel when SendRequest returns non-ERR_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnCancelTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest005 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    g_sendrequest = false;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteStringVector(testing::_)).WillOnce(Return(true));
    std::vector<std::string> idList = { "id1", "id2" };
    int32_t result = proxy.OnCancel(UINT32_RANDOM, idList);
    EXPECT_EQ(result, SENDREQUEST_FAIL);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest005 end");
}

/**
 * @tc.name: OnCancelTest006
 * @tc.desc: Test OnCancel success returns the reply ReadInt32 value
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterProxyTest, OnCancelTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest006 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteboardFuseGetterProxy proxy(rObject);
    NiceMock<PasteboardFuseGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteStringVector(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt32()).WillOnce(Return(ERR_OK));
    std::vector<std::string> idList = { "id1", "id2" };
    int32_t result = proxy.OnCancel(UINT32_RANDOM, idList);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnCancelTest006 end");
}
} // namespace MiscServices
} // namespace OHOS
