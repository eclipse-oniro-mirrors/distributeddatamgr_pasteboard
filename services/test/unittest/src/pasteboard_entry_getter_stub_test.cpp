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

#include "message_parcel_warp.h"
#include "pasteboard_entry_getter_stub.h"
#include "pasteboard_hilog.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
    const int64_t INT64_NEGATIVE_NUMBER = -1;
    const uint32_t UINT32_NEGATIVE_NUMBER = -1;
    const int64_t INT64_POSITIVE_NUMBER = 1;
    const int64_t DEFAULT_MAX_RAW_DATA_SIZE_ADD = 128 * 1024 * 1024 + 1;
    const int64_t DEFAULT_MAX_RAW_DATA_SIZE_SUB = 128 * 1024 * 1024 - 1;
    const int INT_NEGATIVE_NUMBER = -1;
    const int INT_POSITIVE_NUMBER_THREE = 333;
    const int INT_POSITIVE_NUMBER = 1;
    const int64_t INT64_POSITIVE_TWO_NUMBER = 2;
    const int32_t INT32_POSITIVE_TWO_NUMBER = 22;
    const uint32_t UINT32_POSITIVE_NUMBER = 1;
    const uint8_t UINT8_DATA_ARRAY[] = {0x01, 0x02, 0x03, 0x04};
    const size_t UINT8_DATA_ARRAY_LENGTH = 4;
    const uint8_t UINT8_RANDOM = 0x01;
    const std::u16string U16STRING_RAMDOM_VALUE = u"aaa";
}

namespace MiscServices {
class PasteboardEntryGetterStubI : public PasteboardEntryGetterStub {
public:
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &value)
    {
        return 0;
    }
};
} // namespace MiscServices

class PasteboardEntryGetterStubInterface {
public:
    PasteboardEntryGetterStubInterface() {};
    virtual ~PasteboardEntryGetterStubInterface() {};

    virtual uint32_t ReadUint32() = 0;
    virtual int64_t ReadInt64() = 0;
    virtual bool Decode(const std::vector<std::uint8_t> &buffer) = 0;
    virtual bool WriteInt32(int32_t value) = 0;
    virtual bool Encode(std::vector<uint8_t> &buffer) = 0;
    virtual bool WriteInt64(int64_t data) = 0;
    virtual std::u16string ReadInterfaceToken() = 0;
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data,
        MessageParcel &reply, MessageOption &option) = 0;
    virtual uint8_t *ReadUnpadBuffer(size_t length) = 0;
};

class PasteboardEntryGetterStubMock : public PasteboardEntryGetterStubInterface {
public:
    PasteboardEntryGetterStubMock();
    ~PasteboardEntryGetterStubMock() override;

    MOCK_METHOD0(ReadUint32, uint32_t());
    MOCK_METHOD0(ReadInt64, int64_t());
    MOCK_METHOD1(Decode, bool(const std::vector<std::uint8_t> &buffer));
    MOCK_METHOD1(WriteInt32, bool(int32_t value));
    MOCK_METHOD1(Encode, bool(std::vector<uint8_t> &buffer));
    MOCK_METHOD1(WriteInt64, bool(int64_t data));
    MOCK_METHOD0(ReadInterfaceToken, std::u16string());
    MOCK_METHOD1(ReadUnpadBuffer, uint8_t *(size_t length));
    MOCK_METHOD4(OnRemoteRequest, int(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option));
};

static void *g_interface = nullptr;
static bool g_encodeSwitch = false;

PasteboardEntryGetterStubMock::PasteboardEntryGetterStubMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardEntryGetterStubMock::~PasteboardEntryGetterStubMock()
{
    g_interface = nullptr;
}

static PasteboardEntryGetterStubInterface *GetPasteboardEntryGetterStubInterface()
{
    return reinterpret_cast<PasteboardEntryGetterStubInterface *>(g_interface);
}

extern "C" {
    const uint8_t *Parcel::ReadUnpadBuffer(size_t length)
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return nullptr;
        }
        return GetPasteboardEntryGetterStubInterface()->ReadUnpadBuffer(length);
    }

    int IPCObjectStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return INT_NEGATIVE_NUMBER;
        }
        return GetPasteboardEntryGetterStubInterface()->OnRemoteRequest(code, data, reply, option);
    }

    std::u16string MessageParcel::ReadInterfaceToken()
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return std::u16string();
        }
        return GetPasteboardEntryGetterStubInterface()->ReadInterfaceToken();
    }

    bool Parcel::WriteInt64(int64_t data)
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterStubInterface()->WriteInt64(data);
    }

    bool TLVWriteable::Encode(std::vector<uint8_t> &buffer, bool isRemote) const
    {
        (void)isRemote;
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return false;
        }
        buffer.clear();
        if (g_encodeSwitch) {
            buffer.push_back(UINT8_RANDOM);
        }

        return GetPasteboardEntryGetterStubInterface()->Encode(buffer);
    }

    uint32_t Parcel::ReadUint32()
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return 0;
        }
        return GetPasteboardEntryGetterStubInterface()->ReadUint32();
    }

    int64_t Parcel::ReadInt64()
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return 0;
        }
        return GetPasteboardEntryGetterStubInterface()->ReadInt64();
    }


    bool TLVReadable::Decode(const std::vector<std::uint8_t> &buffer)
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterStubInterface()->Decode(buffer);
    }

    bool Parcel::WriteInt32(int32_t value)
    {
        if (GetPasteboardEntryGetterStubInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterStubInterface()->WriteInt32(value);
    }
}

namespace MiscServices {
class PasteboardEntryGetterStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardEntryGetterStubTest::SetUpTestCase(void) { }

void PasteboardEntryGetterStubTest::TearDownTestCase(void) { }

void PasteboardEntryGetterStubTest::SetUp(void) { }

void PasteboardEntryGetterStubTest::TearDown(void) { }

/**
 * @tc.name: OnGetRecordValueByTypeTest001
 * @tc.desc: Test function OnGetRecordValueByType when rawDataSize == -1
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest001 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(INT64_NEGATIVE_NUMBER));
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    std::cout << "OnGetRecordValueByType aaaaa" << std::endl;
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest001 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest002
 * @tc.desc: Test function OnGetRecordValueByType when rawDataSize == 1 and rawDataSize > messageData.GetRawDataSize()
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest002 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillRepeatedly(Return(DEFAULT_MAX_RAW_DATA_SIZE_ADD));
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest002 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest003
 * @tc.desc: Test function OnGetRecordValueByType when rawData == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest003 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH))
        .WillOnce(Return(UINT8_DATA_ARRAY_LENGTH + UINT8_DATA_ARRAY_LENGTH));
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest003 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest004
 * @tc.desc: Test function OnGetRecordValueByType when Decode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest004 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    data.WriteRawData(UINT8_DATA_ARRAY, UINT8_DATA_ARRAY_LENGTH);

    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH)).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(const_cast<uint8_t*>(UINT8_DATA_ARRAY)));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(false));
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest004 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest005
 * @tc.desc: Test function OnGetRecordValueByType when WriteInt32 return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest005 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    data.WriteRawData(UINT8_DATA_ARRAY, UINT8_DATA_ARRAY_LENGTH);
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH)).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(const_cast<uint8_t*>(UINT8_DATA_ARRAY)));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(false));
    
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest005 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest006
 * @tc.desc: Test function OnGetRecordValueByType when Encode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest006 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    data.WriteRawData(UINT8_DATA_ARRAY, UINT8_DATA_ARRAY_LENGTH);
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH)).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(const_cast<uint8_t*>(UINT8_DATA_ARRAY)));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(false));
    
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest006 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest007
 * @tc.desc: Test function OnGetRecordValueByType when WriteInt64 return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest007 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    g_encodeSwitch = false;
    data.WriteRawData(UINT8_DATA_ARRAY, UINT8_DATA_ARRAY_LENGTH);
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH)).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(const_cast<uint8_t*>(UINT8_DATA_ARRAY)));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(false));
    
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest007 end");
}

/**
 * @tc.name: OnGetRecordValueByTypeTest008
 * @tc.desc: Test function OnGetRecordValueByType when done
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnGetRecordValueByTypeTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest008 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardEntryGetterStubMock> mock;
    g_encodeSwitch = true;
    data.WriteRawData(UINT8_DATA_ARRAY, UINT8_DATA_ARRAY_LENGTH);
    EXPECT_CALL(mock, ReadUint32()).WillOnce(Return(UINT32_NEGATIVE_NUMBER));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH)).WillOnce(Return(UINT8_DATA_ARRAY_LENGTH));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(const_cast<uint8_t*>(UINT8_DATA_ARRAY)));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    
    int32_t result = stub.OnGetRecordValueByType(data, reply);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetRecordValueByTypeTest008 end");
}

/**
 * @tc.name: OnRemoteRequestTest001
 * @tc.desc: Test function OnRemoteRequest when remoteDescriptor != localDescriptor
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnRemoteRequestTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest001 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::u16string localDescriptor = PasteboardEntryGetterStub::GetDescriptor();
    NiceMock<PasteboardEntryGetterStubMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(localDescriptor + U16STRING_RAMDOM_VALUE));
    
    int32_t result = stub.OnRemoteRequest(UINT32_POSITIVE_NUMBER, data, reply, option);
    EXPECT_EQ(result, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest001 end");
}

/**
 * @tc.name: OnRemoteRequestTest002
 * @tc.desc: Test function OnRemoteRequest when itFunc != memberFuncMap_.end()
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnRemoteRequestTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest002 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::u16string localDescriptor = PasteboardEntryGetterStub::GetDescriptor();
    stub.memberFuncMap_[UINT32_POSITIVE_NUMBER] = &PasteboardEntryGetterStub::OnGetRecordValueByType;
    g_encodeSwitch = true;
    data.WriteRawData(UINT8_DATA_ARRAY, UINT8_DATA_ARRAY_LENGTH);
    NiceMock<PasteboardEntryGetterStubMock> mock;
    int32_t result = stub.OnRemoteRequest(UINT32_POSITIVE_NUMBER, data, reply, option);
    EXPECT_EQ(result, INT32_POSITIVE_TWO_NUMBER);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest002 end");
}

/**
 * @tc.name: OnRemoteRequestTest003
 * @tc.desc: Test function OnRemoteRequest when memberFuncMap_ not find code
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterStubTest, OnRemoteRequestTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest003 start");
    PasteboardEntryGetterStubI stub;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::u16string localDescriptor = PasteboardEntryGetterStub::GetDescriptor();
    stub.memberFuncMap_.erase(UINT32_POSITIVE_NUMBER);
    NiceMock<PasteboardEntryGetterStubMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(localDescriptor));
    EXPECT_CALL(mock, OnRemoteRequest(testing::_, testing::_, testing::_, testing::_))
        .WillOnce(Return(INT_POSITIVE_NUMBER_THREE));
    
    int32_t result = stub.OnRemoteRequest(UINT32_POSITIVE_NUMBER, data, reply, option);
    EXPECT_EQ(result, INT_POSITIVE_NUMBER_THREE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest003 end");
}
}
} // namespace OHOS::MiscServices