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
#include "pasteboard_delay_getter_stub.h"
#include "pasteboard_hilog.h"
#include "pasteboard_observer_stub.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
namespace OHOS {
class PasteboardDelayStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

class PasteboardObserverImpl : public PasteboardObserverStub {
public:
    PasteboardObserverImpl();
    ~PasteboardObserverImpl();
    void OnPasteboardChanged() override;
    void OnPasteboardEvent(const PasteboardChangedEvent &event) override;
};

PasteboardObserverImpl::PasteboardObserverImpl()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "New Pasteboard Observer.");
}

PasteboardObserverImpl::~PasteboardObserverImpl()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "delete Pasteboard Observer.");
}

void PasteboardObserverImpl::OnPasteboardChanged()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "changed callback.");
}

void PasteboardObserverImpl::OnPasteboardEvent(const PasteboardChangedEvent &event)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "event callback.");
}

class PasteboardDelayGetterStubTest : public PasteboardDelayGetterStub {
public:
    PasteboardDelayGetterStubTest() = default;
    virtual ~PasteboardDelayGetterStubTest() = default;
    MOCK_METHOD(void, GetPasteData, (const std::string &type, PasteData &data), (override));
    MOCK_METHOD(void, GetUnifiedData, (const std::string &type, UDMF::UnifiedData &data), (override));
};

void PasteboardDelayStubTest::SetUpTestCase(void) {}

void PasteboardDelayStubTest::TearDownTestCase(void) {}

void PasteboardDelayStubTest::SetUp(void) {}

void PasteboardDelayStubTest::TearDown(void) {}

class PasteboardDelayStubInterface : public MessageParcel {
public:
    PasteboardDelayStubInterface(){};
    virtual ~PasteboardDelayStubInterface(){};
    virtual std::u16string ReadInterfaceToken() = 0;
    virtual bool Encode(std::vector<uint8_t> &buffer) const = 0;
    virtual bool WriteRawData(MessageParcel &parcelPata, const void *data, size_t size) = 0;
    virtual bool WriteInt64(int64_t value) = 0;
};

class PasteboardDelayStubInterfaceMock : public PasteboardDelayStubInterface {
public:
    PasteboardDelayStubInterfaceMock();
    ~PasteboardDelayStubInterfaceMock() override;
    MOCK_METHOD0(ReadInterfaceToken, std::u16string());
    MOCK_CONST_METHOD1(Encode, bool(std::vector<uint8_t> &buffer));
    MOCK_METHOD3(WriteRawData, bool(MessageParcel &parcelPata, const void *data, size_t size));
    MOCK_METHOD1(WriteInt64, bool(int64_t value));
};

static void *g_interface = nullptr;

PasteboardDelayStubInterfaceMock::PasteboardDelayStubInterfaceMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardDelayStubInterfaceMock::~PasteboardDelayStubInterfaceMock()
{
    g_interface = nullptr;
}

static PasteboardDelayStubInterface *GetPasteboardDelayStubInterface()
{
    return reinterpret_cast<PasteboardDelayStubInterface *>(g_interface);
}

extern "C" {
std::u16string MessageParcel::ReadInterfaceToken()
{
    PasteboardDelayStubInterface *interface = GetPasteboardDelayStubInterface();
    if (interface == nullptr) {
        return u"";
    }
    return interface->ReadInterfaceToken();
}

bool TLVWriteable::Encode(std::vector<uint8_t> &buffer, bool isRemote) const
{
    (void)isRemote;
    PasteboardDelayStubInterface *interface = GetPasteboardDelayStubInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->Encode(buffer);
}

bool Parcel::WriteInt64(int64_t data)
{
    PasteboardDelayStubInterface *interface = GetPasteboardDelayStubInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->WriteInt64(data);
}
}

/* *
 * @tc.name: OnRemoteRequestTest001
 * @tc.desc: remoteDescriptor != localDescriptor
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnRemoteRequestTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest001 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(u"testing"));
    uint32_t code = IPasteboardDelayGetter::TRANS_HEAD;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = tempDelayGetter->OnRemoteRequest(code, data, reply, option);
    ASSERT_EQ(ret, -1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest001 end");
}

/* *
 * @tc.name: OnRemoteRequestTest002
 * @tc.desc: code >= TRANS_BUTT
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnRemoteRequestTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest002 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    std::u16string localDescriptor = tempDelayGetter->GetDescriptor();
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(localDescriptor));
    uint32_t code = IPasteboardDelayGetter::TRANS_BUTT;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = tempDelayGetter->OnRemoteRequest(code, data, reply, option);
    ASSERT_EQ(ret, -1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest002 end");
}

/* *
 * @tc.name: OnRemoteRequestTest003
 * @tc.desc: code >= TRANS_BUTT
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnRemoteRequestTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest003 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    std::u16string localDescriptor = tempDelayGetter->GetDescriptor();
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(localDescriptor));
    uint32_t code = 1;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = tempDelayGetter->OnRemoteRequest(code, data, reply, option);
    ASSERT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest003 end");
}

/* *
 * @tc.name: OnRemoteRequestTest004
 * @tc.desc: code >= TRANS_BUTT
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnRemoteRequestTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest004 start");
    auto pasteboardObserverStub = std::make_shared<PasteboardObserverImpl>();
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(u"testing"));
    uint32_t code = 2;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = pasteboardObserverStub->OnRemoteRequest(code, data, reply, option);
    ASSERT_EQ(ret, IPC_STUB_UNKNOW_TRANS_ERR);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest004 end");
}

/* *
 * @tc.name: OnRemoteRequestTest005
 * @tc.desc: code >= TRANS_BUTT
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnRemoteRequestTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest005 start");
    auto pasteboardObserverStub = std::make_shared<PasteboardObserverImpl>();
    std::u16string myDescriptor = pasteboardObserverStub->GetDescriptor();
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(myDescriptor));
    uint32_t code = 2;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = pasteboardObserverStub->OnRemoteRequest(code, data, reply, option);
    ASSERT_EQ(ret, IPC_STUB_UNKNOW_TRANS_ERR);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest005 end");
}

/* *
 * @tc.name: OnRemoteRequestTest006
 * @tc.desc: code >= TRANS_BUTT
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnRemoteRequestTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest006 start");
    auto pasteboardObserverStub = std::make_shared<PasteboardObserverImpl>();
    std::u16string myDescriptor = pasteboardObserverStub->GetDescriptor();
    pasteboardObserverStub->memberFuncMap_[2] = nullptr;
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, ReadInterfaceToken()).WillOnce(Return(myDescriptor));
    uint32_t code = 2;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int32_t ret = pasteboardObserverStub->OnRemoteRequest(code, data, reply, option);
    ASSERT_EQ(ret, IPC_STUB_UNKNOW_TRANS_ERR);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteRequestTest006 end");
}

/* *
 * @tc.name: OnGetPasteDataTest001
 * @tc.desc: pasteData.Encode(pasteDataTlv) return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnGetPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetPasteDataTest001 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(false));
    int32_t ret = tempDelayGetter->OnGetPasteData(data, reply);
    ASSERT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetPasteDataTest001 end");
}

/* *
 * @tc.name: OnGetPasteDataTest002
 * @tc.desc: WriteInt64 return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnGetPasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetPasteDataTest002 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillRepeatedly(Return(false));
    int32_t ret = tempDelayGetter->OnGetPasteData(data, reply);
    ASSERT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetPasteDataTest002 end");
}

/* *
 * @tc.name: OnGetPasteDataTest003
 * @tc.desc: reply.WriteInt64 return error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnGetPasteDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetPasteDataTest003 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    MessageParcel data;
    MessageParcel reply;
    NiceMock<PasteboardDelayStubInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, WriteInt64(testing::_)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, WriteRawData(testing::_, testing::_, testing::_)).WillRepeatedly(Return(false));
    int32_t ret = tempDelayGetter->OnGetPasteData(data, reply);
    ASSERT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetPasteDataTest003 end");
}

/* *
 * @tc.name: OnGetUnifiedDataTest001
 * @tc.desc: OnGetUnifiedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayStubTest, OnGetUnifiedDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetUnifiedDataTest001 start");
    auto tempDelayGetter = std::make_shared<PasteboardDelayGetterStubTest>();
    MessageParcel data;
    MessageParcel reply;
    int32_t ret = tempDelayGetter->OnGetUnifiedData(data, reply);
    ASSERT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetUnifiedDataTest001 end");
}
} // namespace OHOS