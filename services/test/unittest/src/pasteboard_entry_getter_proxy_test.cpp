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

#include "ipc_object_stub.h"
#include "message_parcel_warp.h"
#include "pasteboard_entry_getter_proxy.h"
#include "pasteboard_error.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
    const int64_t INT64_NEGATIVE_NUMBER = -1;
    const int64_t INT64_POSITIVE_NUMBER = 1;
    const uint32_t UINT32_RANDOM = 333;
    const uint8_t UINT8_RANDOM = 0x01;
    const int HANDEL_AND_PROTO = 1;
    const int SENDREQUEST_ONE = 1;
    const int SENDREQUEST_TWO = 2;
    const int SENDREQUEST_ZERO = 0;
    const std::u16string DESCRIPTOR_TEST = u"test_descriptor";
}

class PasteboardEntryGetterProxyInterface {
public:
    PasteboardEntryGetterProxyInterface() {};
    virtual ~PasteboardEntryGetterProxyInterface() {};

    virtual bool WriteInterfaceToken(std::u16string name) = 0;
    virtual bool WriteUint32(uint32_t value) = 0;
    virtual bool Encode(std::vector<uint8_t> &buffer) = 0;
    virtual bool WriteInt64(int64_t value) = 0;
    virtual bool WriteRawData(MessageParcel &parcelPata, const void *data, size_t size) = 0;
    virtual bool WriteUnpadBuffer(const void *data, size_t size) = 0;
    virtual int64_t ReadInt64() = 0;
    virtual bool Decode(const std::vector<std::uint8_t> &buffer) = 0;
    virtual uint8_t *ReadUnpadBuffer(size_t length) = 0;
};

class PasteboardEntryGetterProxyMock : public PasteboardEntryGetterProxyInterface {
public:
    PasteboardEntryGetterProxyMock();
    ~PasteboardEntryGetterProxyMock() override;

    MOCK_METHOD1(WriteInterfaceToken, bool(std::u16string name));
    MOCK_METHOD1(WriteUint32, bool(uint32_t value));
    MOCK_METHOD1(Encode, bool(std::vector<uint8_t> &buffer));
    MOCK_METHOD1(Decode, bool(const std::vector<std::uint8_t> &buffer));
    MOCK_METHOD1(WriteInt64, bool(int64_t value));
    MOCK_METHOD3(WriteRawData, bool(MessageParcel &parcelPata, const void *data, size_t size));
    MOCK_METHOD2(WriteUnpadBuffer, bool(const void *data, size_t size));
    MOCK_METHOD0(ReadInt64, int64_t());
    MOCK_METHOD1(ReadUnpadBuffer, uint8_t *(size_t length));
};

static void *g_interface = nullptr;
static bool g_writeRawData = false;
static bool g_sendrequest = false;

PasteboardEntryGetterProxyMock::PasteboardEntryGetterProxyMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardEntryGetterProxyMock::~PasteboardEntryGetterProxyMock()
{
    g_interface = nullptr;
}

static PasteboardEntryGetterProxyInterface *GetPasteboardEntryGetterProxyInterface()
{
    return reinterpret_cast<PasteboardEntryGetterProxyInterface *>(g_interface);
}

class TestIRemoteObject : public IRemoteObject {
    TestIRemoteObject(): IRemoteObject(DESCRIPTOR_TEST) {}

    int32_t GetObjectRefCount()
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return g_sendrequest ? 0 : 1;
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
    const uint8_t *Parcel::ReadUnpadBuffer(size_t length)
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return nullptr;
        }
        return GetPasteboardEntryGetterProxyInterface()->ReadUnpadBuffer(length);
    }

    bool TLVReadable::Decode(const std::vector<std::uint8_t> &buffer)
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterProxyInterface()->Decode(buffer);
    }

    int64_t Parcel::ReadInt64()
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return 0;
        }
        return GetPasteboardEntryGetterProxyInterface()->ReadInt64();
    }

    bool Parcel::WriteUnpadBuffer(const void *data, size_t size)
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterProxyInterface()->WriteUnpadBuffer(data, size);
    }

    bool MessageParcel::WriteInterfaceToken(std::u16string name)
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterProxyInterface()->WriteInterfaceToken(name);
    }

    bool Parcel::WriteInt64(int64_t value)
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterProxyInterface()->WriteInt64(value);
    }

    bool Parcel::WriteUint32(uint32_t value)
    {
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return false;
        }
        return GetPasteboardEntryGetterProxyInterface()->WriteUint32(value);
    }

    bool TLVWriteable::Encode(std::vector<uint8_t> &buffer, bool isRemote) const
    {
        (void)isRemote;
        if (GetPasteboardEntryGetterProxyInterface() == nullptr) {
            return false;
        }
        buffer.clear();
        if (g_writeRawData) {
            buffer.push_back(UINT8_RANDOM);
        }
        return GetPasteboardEntryGetterProxyInterface()->Encode(buffer);
    }
};

class PasteboardEntryGetterProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardEntryGetterProxyTest::SetUpTestCase(void) { }

void PasteboardEntryGetterProxyTest::TearDownTestCase(void) { }

void PasteboardEntryGetterProxyTest::SetUp(void) { }

void PasteboardEntryGetterProxyTest::TearDown(void) { }

namespace MiscServices {
/**
 * @tc.name: MakeRequestTest001
 * @tc.desc: Test function MakeRequest when WriteInterfaceToken return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, MakeRequestTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest001 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardEntryGetterProxy proxy(rObject);
    PasteDataEntry entry;
    MessageParcel parcel;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(false));
    int32_t result = proxy.MakeRequest(UINT32_RANDOM, entry, parcel);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest001 end");
}

/**
 * @tc.name: MakeRequestTest002
 * @tc.desc: Test function MakeRequest when WriteUint32 return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, MakeRequestTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest002 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardEntryGetterProxy proxy(rObject);
    PasteDataEntry entry;
    MessageParcel parcel;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(false));
    int32_t result = proxy.MakeRequest(UINT32_RANDOM, entry, parcel);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest002 end");
}

/**
 * @tc.name: MakeRequestTest003
 * @tc.desc: Test function MakeRequest when Encode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, MakeRequestTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest003 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardEntryGetterProxy proxy(rObject);
    PasteDataEntry entry;
    MessageParcel parcel;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(false));
    int32_t result = proxy.MakeRequest(UINT32_RANDOM, entry, parcel);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest003 end");
}

/**
 * @tc.name: MakeRequestTest004
 * @tc.desc: Test function MakeRequest when WriteInt64 return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, MakeRequestTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest004 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardEntryGetterProxy proxy(rObject);
    PasteDataEntry entry;
    MessageParcel parcel;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(false));
    int32_t result = proxy.MakeRequest(UINT32_RANDOM, entry, parcel);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest004 end");
}

/**
 * @tc.name: MakeRequestTest005
 * @tc.desc: Test function MakeRequest when sendEntryTLV.size() == 0
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, MakeRequestTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest005 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardEntryGetterProxy proxy(rObject);
    PasteDataEntry entry;
    MessageParcel parcel;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    g_writeRawData = false;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true));
    int32_t result = proxy.MakeRequest(UINT32_RANDOM, entry, parcel);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest005 end");
}

/**
 * @tc.name: MakeRequestTest006
 * @tc.desc: Test function MakeRequest when done
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, MakeRequestTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest006 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteboardEntryGetterProxy proxy(rObject);
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    PasteDataEntry entry;
    MessageParcel parcel;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(true));
    int32_t result = proxy.MakeRequest(UINT32_RANDOM, entry, parcel);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "MakeRequestTest006 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest001
 * @tc.desc: Test function GetRecordValueByType when MakeRequest return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, GetRecordValueByTypeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest001 start");
    sptr<IRemoteObject> rObject = nullptr;
    PasteDataEntry entry;
    PasteboardEntryGetterProxy proxy(rObject);
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    MessageParcel parcel;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(false));
    int32_t result = proxy.GetRecordValueByType(UINT32_RANDOM, entry);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest001 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest002
 * @tc.desc: Test function GetRecordValueByType when SendRequest return SENDREQUEST_ONE
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, GetRecordValueByTypeTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest002 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteDataEntry entry;
    PasteboardEntryGetterProxy proxy(rObject);
    g_sendrequest = false;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    MessageParcel parcel;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(true));
    int32_t result = proxy.GetRecordValueByType(UINT32_RANDOM, entry);
    EXPECT_EQ(result, SENDREQUEST_ONE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest002 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest003
 * @tc.desc: Test function GetRecordValueByType when rawDataSize <= 0
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, GetRecordValueByTypeTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest003 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteDataEntry entry;
    PasteboardEntryGetterProxy proxy(rObject);
    g_sendrequest = true;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    MessageParcel parcel;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(INT64_NEGATIVE_NUMBER));
    int32_t result = proxy.GetRecordValueByType(UINT32_RANDOM, entry);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest003 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest004
 * @tc.desc: Test function GetRecordValueByType when rawDataSize >
 *           DEFAULT_MAX_RAW_DATA_SIZE (128 * 1024 * 1024; // 128M)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, GetRecordValueByTypeTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest004 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteDataEntry entry;
    PasteboardEntryGetterProxy proxy(rObject);
    g_sendrequest = true;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    MessageParcel parcel;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt64()).WillRepeatedly(Return(DEFAULT_MAX_RAW_DATA_SIZE + DEFAULT_MAX_RAW_DATA_SIZE));
    int32_t result = proxy.GetRecordValueByType(UINT32_RANDOM, entry);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest004 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest005
 * @tc.desc: Test function GetRecordValueByType when Decode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, GetRecordValueByTypeTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest005 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteDataEntry entry;
    PasteboardEntryGetterProxy proxy(rObject);
    g_sendrequest = true;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    uint8_t randomValue = UINT8_RANDOM;
    MessageParcel parcel;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(INT64_POSITIVE_NUMBER)).WillOnce(Return(INT64_POSITIVE_NUMBER));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(false));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(&randomValue));
    int32_t result = proxy.GetRecordValueByType(UINT32_RANDOM, entry);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest005 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest006
 * @tc.desc: Test function GetRecordValueByType when done
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterProxyTest, GetRecordValueByTypeTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest006 start");
    sptr<IRemoteObject> rObject = sptr<TestIRemoteObject>::MakeSptr();
    PasteDataEntry entry;
    PasteboardEntryGetterProxy proxy(rObject);
    g_sendrequest = true;
    NiceMock<PasteboardEntryGetterProxyMock> mock;
    MessageParcel parcel;
    uint8_t randomValue = UINT8_RANDOM;
    g_writeRawData = true;
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUint32(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillOnce(Return(true)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer(testing::_, testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, ReadInt64()).WillOnce(Return(INT64_POSITIVE_NUMBER)).WillOnce(Return(INT64_POSITIVE_NUMBER));
    EXPECT_CALL(mock, ReadUnpadBuffer(testing::_)).WillOnce(Return(&randomValue));
    EXPECT_CALL(mock, Decode(testing::_)).WillOnce(Return(true));
    int32_t result = proxy.GetRecordValueByType(UINT32_RANDOM, entry);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest006 end");
}
}
} // namespace OHOS::MiscServices