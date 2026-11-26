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
#include <gmock/gmock.h>

#include <memory>
#include <vector>

#include "accesstoken_kit.h"
#include "errors.h"
#include "fuse_getter_ipc_mock.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "pasteboard_error.h"
#include "pasteboard_fuse_getter_stub.h"
#include "pasteboard_hilog.h"
#include "pasteboard_serv_ipc_interface_code.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::Security::PasteboardServ;
namespace OHOS {
namespace {
constexpr uint32_t NATIVE_TOKEN_ID = 0x1001; // non-zero, maps to TOKEN_NATIVE
constexpr uint32_t HAP_TOKEN_ID = 0x1002; // non-zero, maps to TOKEN_HAP
constexpr uint32_t TEST_RECORD_ID = 7;
constexpr int64_t TEST_OFFSET = 0;
constexpr int32_t TEST_LENGTH = 16;
const std::string TEST_ID = "fuse_record_id";
const std::u16string MISMATCH_DESCRIPTOR = u"ohos.miscservices.pasteboard.mismatch";
constexpr uint32_t UNKNOWN_CODE = 9999; // not present in PasteboardFuseGetterInterfaceCode

// Configure the IPC mock so IsNativeCallingToken() returns true: a non-zero
// token id whose token type flag is TOKEN_NATIVE. This exercises the branch
// that lets OnRemoteRequest proceed past the native-caller gate.
void SetupNativeToken(FuseGetterIpcMock &mock)
{
    EXPECT_CALL(mock, GetCallingTokenID()).WillRepeatedly(Return(NATIVE_TOKEN_ID));
    EXPECT_CALL(mock, GetTokenTypeFlag(NATIVE_TOKEN_ID)).WillRepeatedly(Return(ATokenTypeEnum::TOKEN_NATIVE));
}
} // namespace

// Concrete subclass of the stub under test. OnRead/OnStat/OnCancel are turned
// into gmock methods so each test can both steer the return value (and output
// params) of the IPasteboardFuseGetter implementation and assert that the
// Handle* dispatch reached the correct virtual method.
class TestFuseGetterStub : public PasteboardFuseGetterStub {
public:
    TestFuseGetterStub() = default;
    ~TestFuseGetterStub() override = default;
    MOCK_METHOD(int32_t, OnRead,
        (uint32_t recordId, const std::string &id, int64_t offset, int32_t length, std::vector<uint8_t> &data),
        (override));
    MOCK_METHOD(int32_t, OnStat, (uint32_t recordId, const std::string &id, FuseFileStat &stat), (override));
    MOCK_METHOD(int32_t, OnCancel, (uint32_t recordId, const std::vector<std::string> &idList), (override));
};

class PasteboardFuseGetterStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

protected:
    // NiceMock: the stub unconditionally logs IPCSkeleton::GetCallingPid() on
    // every OnRemoteRequest entry, so an uninteresting-call policy would be
    // noisy. Explicit Times(0)/WillOnce expectations still behave normally.
    NiceMock<FuseGetterIpcMock> ipcMock_;
};

void PasteboardFuseGetterStubTest::SetUpTestCase(void) {}

void PasteboardFuseGetterStubTest::TearDownTestCase(void) {}

void PasteboardFuseGetterStubTest::SetUp(void)
{
    constexpr int32_t testCallingPid = 1000;
    ON_CALL(ipcMock_, GetCallingPid()).WillByDefault(Return(testCallingPid));
}

void PasteboardFuseGetterStubTest::TearDown(void) {}

/* ===== IsNativeCallingToken / OnRemoteRequest gate branches ===== */

/* *
 * @tc.name: OnRemoteRequestTest001
 * @tc.desc: tokenId == 0 -> IsNativeCallingToken returns false -> reject with ERR_INVALID_VALUE.
 *           GetTokenTypeFlag must not be consulted when the token id is zero.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest001 start");
    EXPECT_CALL(ipcMock_, GetCallingTokenID()).WillRepeatedly(Return(0));
    EXPECT_CALL(ipcMock_, GetTokenTypeFlag(testing::_)).Times(0);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnRead(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest001 end");
}

/* *
 * @tc.name: OnRemoteRequestTest002
 * @tc.desc: tokenId != 0 but GetTokenTypeFlag returns TOKEN_HAP (non-native) ->
 *           IsNativeCallingToken returns false -> reject with ERR_INVALID_VALUE.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest002 start");
    EXPECT_CALL(ipcMock_, GetCallingTokenID()).WillRepeatedly(Return(HAP_TOKEN_ID));
    EXPECT_CALL(ipcMock_, GetTokenTypeFlag(HAP_TOKEN_ID)).WillRepeatedly(Return(ATokenTypeEnum::TOKEN_HAP));
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnRead(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest002 end");
}

/* *
 * @tc.name: OnRemoteRequestTest003
 * @tc.desc: TOKEN_NATIVE passes IsNativeCallingToken and proceeds to the descriptor check;
 *           a mismatched interface token is rejected with ERR_INVALID_VALUE. This also
 *           covers the "native token -> continue" branch (branch 3) since execution
 *           reaches the descriptor comparison only when IsNativeCallingToken is true.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest003 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnRead(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);

    MessageParcel data;
    data.WriteInterfaceToken(MISMATCH_DESCRIPTOR);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest003 end");
}

/* *
 * @tc.name: OnRemoteRequestTest004
 * @tc.desc: native caller + matching descriptor + code not in memberFuncMap_ ->
 *           falls through to IPCObjectStub::OnRemoteRequest which returns
 *           IPC_STUB_UNKNOW_TRANS_ERR. No Handle* method is dispatched.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest004 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnRead(testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(*stub, OnStat(testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(*stub, OnCancel(testing::_, testing::_)).Times(0);

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(UNKNOWN_CODE, data, reply, option);
    EXPECT_EQ(ret, IPC_STUB_UNKNOW_TRANS_ERR);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest004 end");
}

/* *
 * @tc.name: OnRemoteRequestTest005
 * @tc.desc: native caller + matching descriptor + ON_READ -> dispatches to HandleRead,
 *           which invokes OnRead with the parcel parameters. Reply carries result + dataSize.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest005 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnRead(TEST_RECORD_ID, TEST_ID, TEST_OFFSET, TEST_LENGTH, testing::_))
        .WillOnce(DoAll(SetArgReferee<4>(std::vector<uint8_t>{}), Return(0)));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    data.WriteInt64(TEST_OFFSET);
    data.WriteInt32(TEST_LENGTH);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0); // result
    EXPECT_EQ(reply.ReadInt32(), 0); // dataSize
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest005 end");
}

/* *
 * @tc.name: OnRemoteRequestTest006
 * @tc.desc: native caller + matching descriptor + ON_STAT -> dispatches to HandleStat,
 *           which invokes OnStat with the parcel parameters.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest006 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnStat(TEST_RECORD_ID, TEST_ID, testing::_))
        .WillOnce(DoAll(SetArgReferee<2>(FuseFileStat{}), Return(0)));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_STAT), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0); // result
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest006 end");
}

/* *
 * @tc.name: OnRemoteRequestTest007
 * @tc.desc: native caller + matching descriptor + ON_CANCEL -> dispatches to HandleCancel,
 *           which reads the id list and invokes OnCancel with it.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, OnRemoteRequestTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest007 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    std::vector<std::string> idList{"file_a", "file_b"};
    EXPECT_CALL(*stub, OnCancel(TEST_RECORD_ID, idList)).WillOnce(Return(0));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteStringVector(idList);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_CANCEL), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0); // result
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteRequestTest007 end");
}

/* ===== HandleRead branches ===== */

/* *
 * @tc.name: HandleReadTest001
 * @tc.desc: OnRead returns success and leaves file data empty -> reply holds result=0, dataSize=0,
 *           and no UInt8Vector is written.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleReadTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleReadTest001 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnRead(TEST_RECORD_ID, TEST_ID, TEST_OFFSET, TEST_LENGTH, testing::_))
        .WillOnce(DoAll(SetArgReferee<4>(std::vector<uint8_t>{}), Return(0)));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    data.WriteInt64(TEST_OFFSET);
    data.WriteInt32(TEST_LENGTH);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0); // result
    EXPECT_EQ(reply.ReadInt32(), 0); // dataSize
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleReadTest001 end");
}

/* *
 * @tc.name: HandleReadTest002
 * @tc.desc: OnRead returns success and populates non-empty file data -> reply holds result=0,
 *           dataSize>0 and the matching UInt8Vector payload.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleReadTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleReadTest002 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    std::vector<uint8_t> fileData{0x10, 0x20, 0x30, 0x40};
    EXPECT_CALL(*stub, OnRead(TEST_RECORD_ID, TEST_ID, TEST_OFFSET, TEST_LENGTH, testing::_))
        .WillOnce(DoAll(SetArgReferee<4>(fileData), Return(0)));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    data.WriteInt64(TEST_OFFSET);
    data.WriteInt32(TEST_LENGTH);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0); // result
    int32_t dataSize = reply.ReadInt32();
    EXPECT_EQ(dataSize, static_cast<int32_t>(fileData.size()));
    const void *rawData = reply.ReadRawData(dataSize);
    ASSERT_NE(rawData, nullptr);
    std::vector<uint8_t> out(static_cast<const uint8_t *>(rawData),
                             static_cast<const uint8_t *>(rawData) + dataSize);
    EXPECT_EQ(out, fileData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleReadTest002 end");
}

/* *
 * @tc.name: HandleReadTest003
 * @tc.desc: OnRead returns an error code -> HandleRead still writes result/dataSize and returns
 *           ERR_OK; the reply's first int32 is the OnRead error code.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleReadTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleReadTest003 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    int32_t errCode = static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID);
    EXPECT_CALL(*stub, OnRead(TEST_RECORD_ID, TEST_ID, TEST_OFFSET, TEST_LENGTH, testing::_))
        .WillOnce(DoAll(SetArgReferee<4>(std::vector<uint8_t>{}), Return(errCode)));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    data.WriteInt64(TEST_OFFSET);
    data.WriteInt32(TEST_LENGTH);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), errCode); // result carries the OnRead error
    EXPECT_EQ(reply.ReadInt32(), 0);       // dataSize still written (empty)
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleReadTest003 end");
}

/* ===== HandleStat branches ===== */

/* *
 * @tc.name: HandleStatTest001
 * @tc.desc: OnStat returns success and populates the FuseFileStat -> reply holds result=0 plus
 *           id/name/size/modifyTime/type fields exactly as produced by OnStat.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleStatTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleStatTest001 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    FuseFileStat stat;
    stat.id = "stat_id";
    stat.name = "stat_name";
    stat.size = 1234;
    stat.modifyTime = 5678;
    stat.type = 0;
    EXPECT_CALL(*stub, OnStat(TEST_RECORD_ID, TEST_ID, testing::_))
        .WillOnce(DoAll(SetArgReferee<2>(stat), Return(0)));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_STAT), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0);        // result
    EXPECT_EQ(reply.ReadString(), "stat_id");
    EXPECT_EQ(reply.ReadString(), "stat_name");
    EXPECT_EQ(reply.ReadInt64(), 1234);     // size
    EXPECT_EQ(reply.ReadInt64(), 5678);     // modifyTime
    EXPECT_EQ(reply.ReadInt32(), 0);        // type
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleStatTest001 end");
}

/* *
 * @tc.name: HandleStatTest002
 * @tc.desc: OnStat returns an error code -> HandleStat still writes result + default stat fields
 *           and returns ERR_OK; the reply's first int32 is the OnStat error code.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleStatTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleStatTest002 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    int32_t errCode = static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID);
    EXPECT_CALL(*stub, OnStat(TEST_RECORD_ID, TEST_ID, testing::_)).WillOnce(Return(errCode));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteString(TEST_ID);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_STAT), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), errCode); // result carries the OnStat error
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleStatTest002 end");
}

/* ===== HandleCancel branches ===== */

/* *
 * @tc.name: HandleCancelTest001
 * @tc.desc: ReadStringVector fails (corrupt vector count == -1) -> HandleCancel returns
 *           ERR_INVALID_VALUE without invoking OnCancel.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleCancelTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleCancelTest001 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    EXPECT_CALL(*stub, OnCancel(testing::_, testing::_)).Times(0);

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteInt32(-1); // negative count -> Parcel::ReadStringVector returns false
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_CANCEL), data, reply, option);
    EXPECT_EQ(ret, ERR_INVALID_VALUE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleCancelTest001 end");
}

/* *
 * @tc.name: HandleCancelTest002
 * @tc.desc: ReadStringVector succeeds + OnCancel returns success -> reply holds result=0 and
 *           HandleCancel returns ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleCancelTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleCancelTest002 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    std::vector<std::string> idList{"file_a", "file_b"};
    EXPECT_CALL(*stub, OnCancel(TEST_RECORD_ID, idList)).WillOnce(Return(0));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteStringVector(idList);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_CANCEL), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), 0); // result
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleCancelTest002 end");
}

/* *
 * @tc.name: HandleCancelTest003
 * @tc.desc: ReadStringVector succeeds + OnCancel returns an error code -> HandleCancel writes
 *           the result and returns ERR_OK; the reply's first int32 is the OnCancel error code.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseGetterStubTest, HandleCancelTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleCancelTest003 start");
    SetupNativeToken(ipcMock_);
    auto stub = std::make_shared<TestFuseGetterStub>();
    std::vector<std::string> idList{"file_x"};
    int32_t errCode = static_cast<int32_t>(PasteboardError::CANCELED);
    EXPECT_CALL(*stub, OnCancel(TEST_RECORD_ID, idList)).WillOnce(Return(errCode));

    MessageParcel data;
    data.WriteInterfaceToken(stub->GetDescriptor());
    data.WriteUint32(TEST_RECORD_ID);
    data.WriteStringVector(idList);
    MessageParcel reply;
    MessageOption option;
    int ret = stub->OnRemoteRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_CANCEL), data, reply, option);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(reply.ReadInt32(), errCode); // result carries the OnCancel error
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HandleCancelTest003 end");
}
} // namespace OHOS
