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

#include "pasteboard_client_mock_test.h"

#include "ipc_object_stub.h"
#include "iremote_object.h"
#include "message_parcel.h"

#include "ipasteboard_service.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "pasteboard_fuse_getter_stub.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service_loader.h"
#include "pasteboard_service_proxy.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MiscServices {
namespace {
// IPC transaction codes declared in services/IPasteboardService.idl ([ipccode N]) and materialised
// in the generated enum IPasteboardServiceIpcCode (out/.../gen/.../ipasteboard_service.h).
//   SetPasteDataOnly            -> COMMAND_SET_PASTE_DATA_ONLY            = 101
//   SetPasteData                -> COMMAND_SET_PASTE_DATA                 = 104
//   SetPasteDataVirtualFile     -> COMMAND_SET_PASTE_DATA_VIRTUAL_FILE    = 107
const std::u16string DESCRIPTOR_TEST = u"pasteboard_virtual_file_test";

// Records the last SendRequest seen by RecordingRemoteObject and lets each test script the
// SendRequest return value plus the ErrCode the generated proxy reads back from the reply parcel.
struct RemoteCallRecord {
    uint32_t lastCode = 0;
    int32_t sendRequestRet = 0; // value returned from IRemoteObject::SendRequest
    int32_t replyErrCode = 0;   // value written into reply for proxy->reply.ReadInt32()
    int callCount = 0;
};

RemoteCallRecord g_record;

// Minimal concrete IRemoteObject: it never actually performs IPC. It only records the transaction
// code the generated PasteboardServiceProxy forwards via Remote()->SendRequest(...) and pre-seeds
// the reply parcel with the ErrCode the proxy reads back. This mirrors the TestIRemoteObject
// pattern used by the existing PasteboardEntryGetterProxy / PasteboardFuseGetterProxy tests.
class RecordingRemoteObject : public IRemoteObject {
public:
    RecordingRemoteObject() : IRemoteObject(DESCRIPTOR_TEST) {}

    int32_t GetObjectRefCount()
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        g_record.lastCode = code;
        g_record.callCount++;
        // The generated proxy reads a single ErrCode from reply via reply.ReadInt32() after
        // SendRequest returns ERR_OK. Seed it so the proxy can complete its success path.
        reply.WriteInt32(g_record.replyErrCode);
        return g_record.sendRequestRet;
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

// Concrete IPasteboardFuseGetter so SetPasteData can be given a non-empty fuseGetters map. Only
// AsObject() is ever reached on the Set path (the proxy marshals fuseGetter->AsObject() into the
// request parcel); the OnRead/OnStat/OnCancel callbacks are never invoked in this test.
class MockFuseGetter : public PasteboardFuseGetterStub {
public:
    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override
    {
        return ERR_OK;
    }
    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override
    {
        return ERR_OK;
    }
    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override
    {
        return ERR_OK;
    }
};

// Install a PasteboardServiceProxy backed by RecordingRemoteObject as the client's service proxy.
// GetPasteboardService() short-circuits and returns pasteboardServiceProxy_ directly when it is
// non-null, so no samgr interaction happens on the success path.
static sptr<PasteboardServiceProxy> InstallRecordingProxy()
{
    g_record = RemoteCallRecord{};
    g_record.replyErrCode = 0; // proxy reads ERR_OK from reply
    g_record.sendRequestRet = 0; // SendRequest reports success
    sptr<IRemoteObject> remote = sptr<RecordingRemoteObject>::MakeSptr();
    sptr<PasteboardServiceProxy> proxy = sptr<PasteboardServiceProxy>::MakeSptr(remote);
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = proxy;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    return proxy;
}

static std::shared_ptr<PasteData> MakePlainTextData()
{
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData("hello");
    return pasteData;
}

static std::map<uint32_t, sptr<IPasteboardFuseGetter>> MakeFuseGetters()
{
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> fuseGetters;
    fuseGetters[0] = sptr<MockFuseGetter>::MakeSptr();
    return fuseGetters;
}
} // namespace

/**
 * @tc.name: SetPasteDataVirtualFileBranch_001
 * @tc.desc: SetPasteData with non-empty fuseGetters must take the new virtual-file branch and
 *           call SetPasteDataVirtualFile (ipccode 107) on the service proxy.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SetPasteDataVirtualFileBranch_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager())
        .WillRepeatedly(testing::Return(nullptr));

    InstallRecordingProxy();
    auto pasteData = MakePlainTextData();
    ASSERT_NE(pasteData, nullptr);
    auto fuseGetters = MakeFuseGetters();
    ASSERT_NE(fuseGetters[0], nullptr);

    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData, nullptr, {}, fuseGetters);

    // The new branch must dispatch to SetPasteDataVirtualFile (ipccode 107), not the legacy
    // SetPasteDataOnly/SetPasteData path (101/104).
    EXPECT_EQ(g_record.callCount, 1);
    EXPECT_EQ(g_record.lastCode,
        static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_SET_PASTE_DATA_VIRTUAL_FILE));
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_001 end");
}

/**
 * @tc.name: SetPasteDataVirtualFileBranch_002
 * @tc.desc: SetPasteData with empty fuseGetters must keep taking the legacy DispatchSetProxy
 *           branch (SetPasteDataOnly, ipccode 101) and must NOT call SetPasteDataVirtualFile.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SetPasteDataVirtualFileBranch_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_002 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager())
        .WillRepeatedly(testing::Return(nullptr));

    InstallRecordingProxy();
    auto pasteData = MakePlainTextData();
    ASSERT_NE(pasteData, nullptr);

    // No delay getter, no entry getters, no fuse getters -> DispatchSetProxy -> SetPasteDataOnly.
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData, nullptr, {}, {});

    EXPECT_EQ(g_record.callCount, 1);
    EXPECT_EQ(g_record.lastCode,
        static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_SET_PASTE_DATA_ONLY));
    EXPECT_NE(g_record.lastCode,
        static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_SET_PASTE_DATA_VIRTUAL_FILE));
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_002 end");
}

/**
 * @tc.name: SetPasteDataVirtualFileBranch_003
 * @tc.desc: SetPasteData with non-empty fuseGetters still routes to SetPasteDataVirtualFile
 *           (ipccode 107) when the remote SendRequest fails; the failure surfaces as a non-E_OK
 *           return after ConvertErrCode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SetPasteDataVirtualFileBranch_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_003 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager())
        .WillRepeatedly(testing::Return(nullptr));

    InstallRecordingProxy();
    // Force SendRequest itself to fail (non-zero return). The generated proxy forwards this code
    // directly, and SetPasteDataInner runs it through ConvertErrCode.
    g_record.sendRequestRet = 1;

    auto pasteData = MakePlainTextData();
    ASSERT_NE(pasteData, nullptr);
    auto fuseGetters = MakeFuseGetters();
    ASSERT_NE(fuseGetters[0], nullptr);

    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData, nullptr, {}, fuseGetters);

    EXPECT_EQ(g_record.callCount, 1);
    EXPECT_EQ(g_record.lastCode,
        static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_SET_PASTE_DATA_VIRTUAL_FILE));
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));

    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_003 end");
}

/**
 * @tc.name: SetPasteDataVirtualFileBranch_004
 * @tc.desc: SetPasteData must accept the fuseGetters parameter on the null-proxy path and return
 *           OBTAIN_SERVER_SA_ERROR before any IPC is attempted. This guards API-level acceptance of
 *           the new parameter even when the full virtual-file branch cannot be driven end-to-end.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SetPasteDataVirtualFileBranch_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_004 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager())
        .WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    g_record = RemoteCallRecord{};

    auto pasteData = MakePlainTextData();
    ASSERT_NE(pasteData, nullptr);
    auto fuseGetters = MakeFuseGetters();
    ASSERT_NE(fuseGetters[0], nullptr);

    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData, nullptr, {}, fuseGetters);

    // Proxy is null -> the branch is never reached and no SendRequest must be issued.
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    EXPECT_EQ(g_record.callCount, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteDataVirtualFileBranch_004 end");
}
} // namespace MiscServices
} // namespace OHOS
