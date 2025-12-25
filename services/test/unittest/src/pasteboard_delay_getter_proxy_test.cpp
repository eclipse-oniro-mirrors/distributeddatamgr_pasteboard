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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ipc_skeleton.h"
#include "message_parcel_warp.h"
#include "pasteboard_hilog.h"
#include "pasteboard_delay_getter_proxy.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
namespace OHOS {
namespace {
int g_sendRequest = 0;
class RemoteObjectTest : public IRemoteObject {
public:
    explicit RemoteObjectTest(std::u16string descriptor) : IRemoteObject(descriptor) {}
    ~RemoteObjectTest() {}

    int32_t GetObjectRefCount() { return 0; }
    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                    MessageOption &option) { return g_sendRequest; }
    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) { return true; }
    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) { return true; }
    int Dump(int fd, const std::vector<std::u16string> &args) { return 0; }
};

constexpr int64_t TEST_ERROR_PAW_DATA_SIZE = -1;
constexpr int64_t TEST_MAX_RAW_DATA_SIZE = 256 * 1024 * 1024; // 256M
constexpr int64_t ERR_INVALID_PARAMETER = 401;
} // namespace

class PasteboardDelayProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void SetSendRequestResult(int status = 0);
};

void PasteboardDelayProxyTest::SetUpTestCase(void) { }

void PasteboardDelayProxyTest::TearDownTestCase(void) { }

void PasteboardDelayProxyTest::SetUp(void) { }

void PasteboardDelayProxyTest::TearDown(void) { }

void PasteboardDelayProxyTest::SetSendRequestResult(int status)
{
    g_sendRequest = status;
}

class PasteboardDelayProxyInterface : public MessageParcel {
public:
    PasteboardDelayProxyInterface() {};
    virtual ~PasteboardDelayProxyInterface() {};
    virtual bool WriteInterfaceToken(std::u16string name) = 0;
    virtual bool WriteString(const std::string &value) = 0;
    virtual int SendRequest(uint32_t code, MessageParcel &data,
                            MessageParcel &reply, MessageOption &option) = 0;
    virtual int64_t ReadInt64() = 0;
    virtual const void *ReadRawData(MessageParcel &parcelPata, size_t size) = 0;
    virtual bool Decode(const std::vector<std::uint8_t> &buffer) = 0;
    virtual const uint8_t *ReadUnpadBuffer(size_t length) = 0;
};

class PasteboardDelayProxyInterfaceMock : public PasteboardDelayProxyInterface {
public:
    PasteboardDelayProxyInterfaceMock();
    ~PasteboardDelayProxyInterfaceMock() override;
    MOCK_METHOD1(WriteInterfaceToken, bool(std::u16string name));
    MOCK_METHOD1(WriteString, bool(const std::string &value));
    MOCK_METHOD4(SendRequest, int(uint32_t code, MessageParcel &data,
                                  MessageParcel &reply, MessageOption &option));
    MOCK_METHOD0(ReadInt64, int64_t());
    MOCK_METHOD2(ReadRawData, const void*(MessageParcel &parcelPata, size_t size));
    MOCK_METHOD1(Decode, bool(const std::vector<std::uint8_t> &buffer));
    MOCK_METHOD1(ReadUnpadBuffer, const uint8_t *(size_t));
};

static void *g_interface = nullptr;

PasteboardDelayProxyInterfaceMock::PasteboardDelayProxyInterfaceMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardDelayProxyInterfaceMock::~PasteboardDelayProxyInterfaceMock()
{
    g_interface = nullptr;
}

static PasteboardDelayProxyInterface *GetPasteboardDelayProxyInterface()
{
    return reinterpret_cast<PasteboardDelayProxyInterface*>(g_interface);
}

extern "C" {
    bool MessageParcel::WriteInterfaceToken(std::u16string name)
    {
        PasteboardDelayProxyInterface *interface = GetPasteboardDelayProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteInterfaceToken(name);
    }

    bool Parcel::WriteString(const std::string &value)
    {
        PasteboardDelayProxyInterface *interface = GetPasteboardDelayProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->WriteString(value);
    }

    int IRemoteObject::SendRequest(uint32_t code, MessageParcel &data,
                                   MessageParcel &reply, MessageOption &option)
    {
        PasteboardDelayProxyInterface *interface = GetPasteboardDelayProxyInterface();
        if (interface == nullptr) {
            return -1;
        }
        return interface->SendRequest(code, data, reply, option);
    }

    int64_t Parcel::ReadInt64()
    {
        PasteboardDelayProxyInterface *interface = GetPasteboardDelayProxyInterface();
        if (interface == nullptr) {
            return -1;
        }
        return interface->ReadInt64();
    }

    bool TLVReadable::Decode(const std::vector<uint8_t> &buffer)
    {
        PasteboardDelayProxyInterface *interface = GetPasteboardDelayProxyInterface();
        if (interface == nullptr) {
            return false;
        }
        return interface->Decode(buffer);
    }

    const uint8_t *Parcel::ReadUnpadBuffer(size_t length)
    {
        PasteboardDelayProxyInterface *interface = GetPasteboardDelayProxyInterface();
        if (interface == nullptr) {
            return nullptr;
        }
        return interface->ReadUnpadBuffer(length);
    }
}

/**
 * @tc.name: GetPasteDataTest001
 * @tc.desc: Function GetPasteData when WriteInterfaceToken return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayProxyTest, GetPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest001 start");
    std::string testType = "text/plain";
    PasteData testData;
    NiceMock<PasteboardDelayProxyInterfaceMock> mock;
    sptr<RemoteObjectTest> remote = sptr<RemoteObjectTest>::MakeSptr(u"test");
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(false));

    PasteboardDelayGetterProxy proxy(remote);
    proxy.GetPasteData(testType, testData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest001 end");
}

/**
 * @tc.name: GetPasteDataTest002
 * @tc.desc: Function GetPasteData when WriteString return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayProxyTest, GetPasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest002 start");
    std::string testType = "text/plain";
    PasteData testData;
    NiceMock<PasteboardDelayProxyInterfaceMock> mock;
    sptr<RemoteObjectTest> remote = sptr<RemoteObjectTest>::MakeSptr(u"test");
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(false));

    PasteboardDelayGetterProxy proxy(remote);
    proxy.GetPasteData(testType, testData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest002 end");
}

/**
 * @tc.name: GetPasteDataTest003
 * @tc.desc: Function GetPasteData when SendRequest return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayProxyTest, GetPasteDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest003 start");
    std::string testType = "text/plain";
    PasteData testData;
    NiceMock<PasteboardDelayProxyInterfaceMock> mock;
    sptr<RemoteObjectTest> remote = sptr<RemoteObjectTest>::MakeSptr(u"test");
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    SetSendRequestResult(ERR_INVALID_PARAMETER);
    PasteboardDelayGetterProxy proxy(remote);
    proxy.GetPasteData(testType, testData);
    SetSendRequestResult(OHOS::ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest003 end");
}

/**
 * @tc.name: GetPasteDataTest004
 * @tc.desc: Function GetPasteData when ReadInt64 return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayProxyTest, GetPasteDataTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest004 start");
    std::string testType = "text/plain";
    PasteData testData;
    NiceMock<PasteboardDelayProxyInterfaceMock> mock;
    sptr<RemoteObjectTest> remote = sptr<RemoteObjectTest>::MakeSptr(u"test");
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, SendRequest(testing::_, testing::_, testing::_, testing::_))
               .WillRepeatedly(Return(OHOS::ERR_OK));
    EXPECT_CALL(mock, ReadInt64()).WillRepeatedly(Return(TEST_ERROR_PAW_DATA_SIZE));
    PasteboardDelayGetterProxy proxy(remote);
    proxy.GetPasteData(testType, testData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest004 end");
}

/**
 * @tc.name: GetPasteDataTest005
 * @tc.desc: Function GetPasteData when ReadInt64 return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayProxyTest, GetPasteDataTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest005 start");
    std::string testType = "text/plain";
    PasteData testData;
    NiceMock<PasteboardDelayProxyInterfaceMock> mock;
    sptr<RemoteObjectTest> remote = sptr<RemoteObjectTest>::MakeSptr(u"test");
    EXPECT_CALL(mock, WriteInterfaceToken(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteString(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, SendRequest(testing::_, testing::_, testing::_, testing::_))
               .WillRepeatedly(Return(OHOS::ERR_OK));
    EXPECT_CALL(mock, ReadInt64()).WillRepeatedly(Return(TEST_MAX_RAW_DATA_SIZE));
    PasteboardDelayGetterProxy proxy(remote);
    proxy.GetPasteData(testType, testData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest005 end");
}

/**
 * @tc.name: GetPasteDataTest006
 * @tc.desc: Function GetUnifiedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayProxyTest, GetPasteDataTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest006 start");
    std::string testType = "text/plain";
    UDMF::UnifiedData testData;
    sptr<RemoteObjectTest> remote = sptr<RemoteObjectTest>::MakeSptr(u"test");
    EXPECT_NE(remote, nullptr);

    PasteboardDelayGetterProxy proxy(remote);
    proxy.GetUnifiedData(testType, testData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest006 end");
}
} // namespace OHOS