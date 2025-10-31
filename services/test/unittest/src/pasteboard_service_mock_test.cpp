/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

#include "accesstoken_kit.h"
#include "default_clip.h"
#include "dev_profile.h"
#include "distributed_file_daemon_manager.h"
#include "input_method_controller.h"
#include "input_method_property.h"
#include "ipc_skeleton.h"
#include "parameters.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service.h"
#include "remote_file_share.h"
#include "eventcenter/pasteboard_event.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;
using namespace std::chrono;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
using namespace OHOS::Storage::DistributedFile;
namespace OHOS {
namespace {
    const int32_t INT32_TEN = 10;
    const int32_t ACCOUNT_IDS_RANDOM = 1121;
    const int INT_ONE = 1;
    const uint8_t UINT8_ONE = 1;
    const int32_t INT32_NEGATIVE_NUMBER = -1;
    const pid_t TEST_SERVER_UID = 3500;
    const int64_t INT64_NEGATIVE_NUMBER = -1;
    const uint32_t UINT32_EXCEPTION_APPID = 9999985;
    const int INT_THREETHREETHREE = 333;
    const int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
    const int32_t CONTROL_TYPE_ALLOW_SEND_RECEIVE = 1;
    const int32_t DEVICE_COLLABORATION_UID = 5521;
    const std::string RANDOM_STRING = "TEST_string_111";
    const std::string URI_STRING = "https://www.a.com/";
    const std::string HTML_STRING =
        R"(<!DOCTYPE html><html lang="en"><head><title>S</title></head><body><h1>H</h1></body></html>)";
    const char *MIMETYPE_TEXT_HTML = "text/html";
    const std::u16string RANDOM_U16STRING = u"TEST_string_111";
    const std::string UTDID_PLAIN_TEXT = "general.plain-text";
    const std::string UTDID_PIXEL_MAP = "openharmony.pixel-map";
    using TestEvent = ClipPlugin::GlobalEvent;
}

class MyTestPasteboardChangedObserver : public IPasteboardChangedObserver {
    void OnPasteboardChanged()
    {
        return;
    }
    void OnPasteboardEvent(const PasteboardChangedEvent &event)
    {
        return;
    }
    sptr<IRemoteObject> AsObject()
    {
        return nullptr;
    }
};

static int g_recordValueByType = static_cast<int32_t>(PasteboardError::E_OK);
class PasteboardEntryGetterImpl : public IPasteboardEntryGetter {
    public:
        PasteboardEntryGetterImpl() {};
        ~PasteboardEntryGetterImpl() {};
        int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &value)
        {
            return g_recordValueByType;
        };
        sptr<IRemoteObject> AsObject()
        {
            return nullptr;
        };
    };

class PasteboardDelayGetterImpl : public IPasteboardDelayGetter {
public:
    PasteboardDelayGetterImpl() {}
    ~PasteboardDelayGetterImpl() {}
    void GetPasteData(const std::string &type, PasteData &data) {};
    void GetUnifiedData(const std::string &type, UDMF::UnifiedData &data) {};
    sptr<IRemoteObject> AsObject()
    {
        return nullptr;
    }
};

class DistributedFileDaemonManager {
public:
    DistributedFileDaemonManager() {}
    ~DistributedFileDaemonManager() {}
    int CloseP2PConnection(DmDeviceInfo &remoteDevice);
};

class PasteboardServiceInterface {
public:
    PasteboardServiceInterface(){};
    virtual ~PasteboardServiceInterface(){};

    virtual OHOS::ErrCode QueryActiveOsAccountIds(std::vector<int32_t> &ids);
    virtual bool Decode(const std::vector<std::uint8_t> &buffer) = 0;
    virtual bool Encode(std::vector<uint8_t> &buffer) const = 0;
    virtual pid_t GetCallingUid() = 0;
    virtual uint32_t GetCallingTokenID() = 0;
    virtual uint64_t GetCallingFullTokenID() = 0;
    virtual pid_t GetCallingPid() = 0;
    virtual bool HasContent(const std::string &utdId) const = 0;
    virtual std::shared_ptr<PasteDataRecord> GetRecordById(uint32_t recordId) const = 0;
    virtual std::shared_ptr<PasteDataEntry> GetEntry(const std::string &utdType) = 0;
    virtual bool IsOn() const = 0;
    virtual bool IsDelayData() const = 0;
    virtual bool IsDelayRecord() const = 0;
    virtual int GetRemoteDeviceInfo(const std::string &networkId, DmDeviceInfo &remoteDevice) = 0;
    virtual int CloseP2PConnection(DmDeviceInfo &remoteDevice) = 0;
    virtual int VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName) = 0;
    virtual ATokenTypeEnum GetTokenTypeFlag(AccessTokenID tokenId) = 0;
    virtual std::vector<std::string> GetMimeTypes() = 0;
    virtual std::string GetNetworkId() = 0;
    virtual int GetIntParameter(const std::string &key, int def, int min, int max) = 0;
    virtual uint32_t GetRemoteDeviceMinVersion() = 0;
    virtual int32_t GetDfsUriFromLocal(
        const std::string &uriStr, const int32_t &userId, struct HmdfsUriInfo &hui) = 0;
    virtual std::vector<std::shared_ptr<PasteDataRecord>> AllRecords() = 0;
    virtual bool Dump(int fd, const std::vector<std::string> &args) const = 0;
    virtual sptr<InputMethodController> GetInstance() = 0;
    virtual void RegisterCommand(std::shared_ptr<Command> &cmd) = 0;
    virtual int32_t GetDefaultInputMethod(std::shared_ptr<MiscServices::Property> &property) = 0;
    virtual bool GetBoolParameter(const std::string &key, bool defaultValue) = 0;
};

class PasteboardServiceInterfaceMock : public PasteboardServiceInterface {
public:
    PasteboardServiceInterfaceMock();
    ~PasteboardServiceInterfaceMock() override;

    MOCK_METHOD1(QueryActiveOsAccountIds, OHOS::ErrCode(std::vector<int32_t> &ids));
    MOCK_METHOD1(Decode, bool(const std::vector<std::uint8_t> &buffer));
    MOCK_CONST_METHOD1(Encode, bool(std::vector<uint8_t> &buffer));
    MOCK_METHOD0(GetCallingUid, pid_t());
    MOCK_METHOD0(GetCallingTokenID, uint32_t());
    MOCK_METHOD0(GetCallingFullTokenID, uint64_t());
    MOCK_METHOD0(GetCallingPid, pid_t());
    MOCK_CONST_METHOD1(HasContent, bool(const std::string &utdId));
    MOCK_METHOD1(GetEntry, std::shared_ptr<PasteDataEntry>(const std::string &utdType));
    MOCK_CONST_METHOD0(IsOn, bool());
    MOCK_CONST_METHOD0(IsDelayData, bool());
    MOCK_CONST_METHOD0(IsDelayRecord, bool());
    MOCK_METHOD1(CloseP2PConnection, int(DmDeviceInfo &remoteDevice));
    MOCK_METHOD2(GetRemoteDeviceInfo, int(const std::string &networkId, DmDeviceInfo &remoteDevice));
    MOCK_CONST_METHOD1(GetRecordById, std::shared_ptr<PasteDataRecord>(uint32_t recordId));
    MOCK_METHOD2(VerifyAccessToken, int(AccessTokenID tokenID, const std::string &permissionName));
    MOCK_METHOD1(GetTokenTypeFlag, ATokenTypeEnum(AccessTokenID tokenId));
    MOCK_METHOD0(GetMimeTypes, std::vector<std::string>());
    MOCK_METHOD1(RegisterCommand, void(std::shared_ptr<Command> &cmd));
    MOCK_METHOD0(GetNetworkId, std::string());
    MOCK_METHOD4(GetIntParameter, int(const std::string &key, int def, int min, int max));
    MOCK_METHOD0(GetRemoteDeviceMinVersion, uint32_t());
    MOCK_METHOD3(GetDfsUriFromLocal, int32_t(
        const std::string &uriStr, const int32_t &userId, struct HmdfsUriInfo &hui));
    MOCK_METHOD0(AllRecords, std::vector<std::shared_ptr<PasteDataRecord>>());
    MOCK_METHOD0(GetInstance, sptr<InputMethodController>());
    MOCK_CONST_METHOD2(Dump, bool(int fd, const std::vector<std::string> &args));
    MOCK_METHOD1(GetDefaultInputMethod, int32_t(std::shared_ptr<MiscServices::Property> &property));
    MOCK_METHOD2(GetBoolParameter, bool(const std::string &key, bool defaultValue));
};

static void *g_interface = nullptr;
static bool g_accountIds = false;
static bool g_encodeInsert = true;
static int g_encodeInsertCount = 0;

PasteboardServiceInterfaceMock::PasteboardServiceInterfaceMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardServiceInterfaceMock::~PasteboardServiceInterfaceMock()
{
    g_interface = nullptr;
}

static PasteboardServiceInterface *GetPasteboardServiceInterface()
{
    return reinterpret_cast<PasteboardServiceInterface *>(g_interface);
}

extern "C" {
int32_t InputMethodController::GetDefaultInputMethod(std::shared_ptr<Property> &property)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return 0;
    }
    if (g_encodeInsert) {
        property = std::make_shared<Property>();
    }
    return interface->GetDefaultInputMethod(property);
}

PasteboardDumpHelper &PasteboardDumpHelper::GetInstance()
{
    static PasteboardDumpHelper instance;
    return instance;
}

void PasteboardDumpHelper::RegisterCommand(std::shared_ptr<Command> &cmd)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return;
    }
    return interface->RegisterCommand(cmd);
}

sptr<InputMethodController> InputMethodController::GetInstance()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return nullptr;
    }
    return interface->GetInstance();
}

bool PasteboardDumpHelper::Dump(int fd, const std::vector<std::string> &args) const
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->Dump(fd, args);
}

std::vector<std::string> PasteData::GetMimeTypes()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return std::vector<std::string>();
    }
    return interface->GetMimeTypes();
}

bool DistributedModuleConfig::IsOn()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->IsOn();
}

bool PasteData::IsDelayData() const
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->IsDelayData();
}

bool PasteData::IsDelayRecord() const
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->IsDelayRecord();
}

int DMAdapter::GetRemoteDeviceInfo(const std::string &networkId, DmDeviceInfo &remoteDevice)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->GetRemoteDeviceInfo(networkId, remoteDevice);
}

int DistributedFileDaemonManager::CloseP2PConnection(DmDeviceInfo &remoteDevice)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->CloseP2PConnection(remoteDevice);
}

std::shared_ptr<PasteDataEntry> PasteDataRecord::GetEntry(const std::string &utdType)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return nullptr;
    }
    return interface->GetEntry(utdType);
}

std::shared_ptr<PasteDataRecord> PasteData::GetRecordById(uint32_t recordId) const
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return nullptr;
    }
    return interface->GetRecordById(recordId);
}

bool PasteDataEntry::HasContent(const std::string &utdId) const
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->HasContent(utdId);
}

bool TLVWriteable::Encode(std::vector<uint8_t> &buffer, bool isRemote) const
{
    (void)isRemote;
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->Encode(buffer);
}

bool TLVReadable::Decode(const std::vector<uint8_t> &buffer)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->Decode(buffer);
}

pid_t IPCSkeleton::GetCallingUid()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->GetCallingUid();
}

uint32_t IPCSkeleton::GetCallingTokenID()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->GetCallingTokenID();
}

uint64_t IPCSkeleton::GetCallingFullTokenID()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->GetCallingFullTokenID();
}

pid_t IPCSkeleton::GetCallingPid()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->GetCallingPid();
}

int AccessTokenKit::VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->VerifyAccessToken(tokenID, permissionName);
}

ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(AccessTokenID tokenId)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return ATokenTypeEnum::TOKEN_INVALID;
    }
    return interface->GetTokenTypeFlag(tokenId);
}

int GetIntParameter(const std::string &key, int def, int min, int max)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return INT32_NEGATIVE_NUMBER;
    }
    return interface->GetIntParameter(key, def, min, max);
}

uint32_t DistributedModuleConfig::GetRemoteDeviceMinVersion()
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return UINT_MAX;
    }
    return interface->GetRemoteDeviceMinVersion();
}

int32_t RemoteFileShare::GetDfsUriFromLocal(
    const std::string &uriStr, const int32_t &userId, struct HmdfsUriInfo &hui)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return UINT_MAX;
    }
    return interface->GetDfsUriFromLocal(uriStr, userId, hui);
}

std::vector<std::shared_ptr<PasteDataRecord>> PasteData::AllRecords() const
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return std::vector<std::shared_ptr<PasteDataRecord>>();
    }
    return interface->AllRecords();
}
}

bool OHOS::system::GetBoolParameter(const std::string &key, bool defaultValue)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return false;
    }
    return interface->GetBoolParameter(key, defaultValue);
}

namespace MiscServices {
    int32_t InputMethodController::SendPrivateCommand(
        const std::unordered_map<std::string, PrivateDataValue> &privateCommand)
    {
        return 0;
    }

    int32_t InputMethodController::ReceivePrivateCommand(
        const std::unordered_map<std::string, PrivateDataValue> &privateCommand)
    {
        return 0;
    }

    InputMethodController::InputMethodController() {}

    InputMethodController::~InputMethodController() {}

    std::string PasteboardEvent::GetNetworkId() const
    {
        PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
        if (interface == nullptr) {
            return "";
        }
        return interface->GetNetworkId();
    }
}

namespace AccountSA {
OHOS::ErrCode OsAccountManager::QueryActiveOsAccountIds(std::vector<int32_t> &ids)
{
    PasteboardServiceInterface *interface = GetPasteboardServiceInterface();
    if (interface == nullptr) {
        return -1;
    }
    ids.clear();
    if (g_accountIds) {
        ids.push_back(ACCOUNT_IDS_RANDOM);
    }
    return interface->QueryActiveOsAccountIds(ids);
}
}

class PasteboardServiceMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardServiceMockTest::SetUpTestCase(void) { }

void PasteboardServiceMockTest::TearDownTestCase(void) { }

void PasteboardServiceMockTest::SetUp(void) { }

void PasteboardServiceMockTest::TearDown(void) { }

namespace MiscServices {
/**
 * @tc.name: ClearTest002
 * @tc.desc: test Func Clear
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ClearTest002, TestSize.Level0)
{
    PasteboardService service;
    service.currentUserId_ = ERROR_USERID;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillOnce(Return(INT_ONE));
    int32_t result = service.Clear();
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR));
}

/**
 * @tc.name: GetRecordValueByTypeTest001
 * @tc.desc: test Func GetRecordValueByType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest001, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    PasteDataEntry value;
    value.SetUtdId(RANDOM_STRING);
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    auto record = PasteDataRecord::NewPlainTextRecord(RANDOM_STRING);
    auto entry = std::make_shared<PasteDataEntry>();
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(true);
    entry->SetMimeType(MIMETYPE_TEXT_HTML);
    entry->SetValue(HTML_STRING);

    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingTokenID()).WillOnce(Return(service.delayTokenId_.load()));
    EXPECT_CALL(mock, HasContent(testing::_)).WillOnce(Return(false));
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry));

    int32_t result = service.GetRecordValueByType(service.delayDataId_, recordId, value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL));
}

/**
 * @tc.name: GetRecordValueByTypeTest002
 * @tc.desc: test Func GetRecordValueByType when paste data is html
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest002, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    PasteDataEntry value;
    value.SetUtdId(RANDOM_STRING);
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    auto record = PasteDataRecord::NewPlainTextRecord(RANDOM_STRING);
    auto entry = std::make_shared<PasteDataEntry>();
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(false);
    entry->SetMimeType(MIMETYPE_TEXT_HTML);
    entry->SetValue(HTML_STRING);

    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingTokenID()).WillOnce(Return(service.delayTokenId_.load()));
    EXPECT_CALL(mock, HasContent(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry)).WillOnce(Return(entry));

    int32_t result = service.GetRecordValueByType(service.delayDataId_, recordId, value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetRecordValueByTypeTest003
 * @tc.desc: test Func GetRecordValueByType when paste data is url
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest003, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    PasteDataEntry value;
    value.SetUtdId(RANDOM_STRING);
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    auto record = PasteDataRecord::NewPlainTextRecord(RANDOM_STRING);
    auto entry = std::make_shared<PasteDataEntry>();
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(false);
    entry->SetMimeType(MIMETYPE_TEXT_URI);
    

    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingTokenID()).WillOnce(Return(service.delayTokenId_.load()));
    EXPECT_CALL(mock, HasContent(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry)).WillOnce(Return(entry));

    int32_t result = service.GetRecordValueByType(service.delayDataId_, recordId, value);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetRecordValueByTypeTest004
 * @tc.desc: test Func GetRecordValueByType when paste Encode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest004, TestSize.Level0)
{
    PasteboardService service;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    int32_t fd = 0;
    PasteDataEntry entryValue;

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(false));

    int32_t result = service.GetRecordValueByType(rawDataSize, buffer, fd, entryValue);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
}

/**
 * @tc.name: GetRecordValueByTypeTest005
 * @tc.desc: test Func GetRecordValueByType when paste rawDataSize < MIN_ASHMEM_DATA_SIZE
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest005, TestSize.Level0)
{
    PasteboardService service;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    int32_t fd = 0;
    PasteDataEntry entryValue;
    g_encodeInsert = true;
    g_encodeInsertCount = MIN_ASHMEM_DATA_SIZE + 1;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));

    int32_t result = service.GetRecordValueByType(rawDataSize, buffer, fd, entryValue);
    EXPECT_EQ(result, ERR_OK);
}

/**
 * @tc.name: GetRecordValueByTypeTest006
 * @tc.desc: test Func GetRecordValueByType when paste Encode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest006, TestSize.Level0)
{
    PasteboardService service;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    int32_t fd = 0;
    PasteDataEntry entryValue;
    g_encodeInsert = true;
    g_encodeInsertCount = INT_THREETHREETHREE;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));

    int32_t result = service.GetRecordValueByType(rawDataSize, buffer, fd, entryValue);
    EXPECT_EQ(result, ERR_OK);
}

/**
 * @tc.name: GetRecordValueByTypeTest007
 * @tc.desc: test Func GetRecordValueByType rawDataSize == 0
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest007, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    int64_t rawDataSize = INT64_NEGATIVE_NUMBER;
    std::vector<uint8_t> buffer;
    int fd = 0;

    int32_t result = service.GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}

/**
 * @tc.name: GetRecordValueByTypeTest008
 * @tc.desc: test Func GetRecordValueByType rawDataSize > DEFAULT_MAX_RAW_DATA_SIZE
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest008, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    int64_t rawDataSize = INT64_MAX;
    std::vector<uint8_t> buffer;
    int fd = 0;

    int32_t result = service.GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}

/**
 * @tc.name: GetRecordValueByTypeTest009
 * @tc.desc: test Func GetRecordValueByType rawDataSize > MIN_ASHMEM_DATA_SIZE
 *      Decode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest009, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    int64_t rawDataSize = MIN_ASHMEM_DATA_SIZE + 1;
    std::vector<uint8_t> buffer;
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    int fd = open("/dev/zero", O_RDWR);
    std::vector<char> data(rawDataSize, 'a');
    auto pasteData = std::make_shared<PasteData>();
    auto record = PasteDataRecord::NewPlainTextRecord(RANDOM_STRING);
    auto entry = std::make_shared<PasteDataEntry>();
    pasteData->SetDataId(service.delayDataId_);
    pasteData->AddRecord(record);
    pasteData->SetRemote(false);
    entry->SetMimeType(MIMETYPE_TEXT_URI);
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, pasteData);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, Decode(testing::_)).WillRepeatedly(Return(false));
    int32_t result = service.GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    close(fd);
}

/**
 * @tc.name: GetRecordValueByTypeTest010
 * @tc.desc: test Func GetRecordValueByType Decode return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest010, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    int64_t rawDataSize = 1024;
    std::vector<uint8_t> buffer;
    int fd;
    buffer.insert(buffer.end(), rawDataSize, UINT8_ONE);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, Decode(testing::_)).WillRepeatedly(Return(false));
    int32_t result = service.GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
}

/**
 * @tc.name: GetRecordValueByTypeTest011
 * @tc.desc: test Func GetRecordValueByType second child GetRecordValueByType return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetRecordValueByTypeTest011, TestSize.Level0)
{
    PasteboardService service;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    int64_t rawDataSize = 1024;
    std::vector<uint8_t> buffer;
    int fd;
    buffer.insert(buffer.end(), rawDataSize, UINT8_ONE);
    NiceMock<PasteboardServiceInterfaceMock> mock;

    PasteDataEntry value;
    value.SetUtdId(RANDOM_STRING);
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    auto record = PasteDataRecord::NewPlainTextRecord(RANDOM_STRING);
    auto entry = std::make_shared<PasteDataEntry>();
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(false);
    entry->SetMimeType(MIMETYPE_TEXT_URI);
    
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    EXPECT_CALL(mock, Decode(testing::_)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, GetCallingTokenID()).WillOnce(Return(service.delayTokenId_.load()));
    EXPECT_CALL(mock, HasContent(testing::_)).WillOnce(Return(true));
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry)).WillOnce(Return(entry));

    int32_t result = service.GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
}

/**
 * @tc.name: ShowProgressTest001
 * @tc.desc: test Func ShowProgress HasPasteData return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ShowProgressTest001, TestSize.Level0)
{
    PasteboardService service;
    service.currentUserId_ = ERROR_USERID;
    const std::string progressKey;
    sptr<IRemoteObject> observer;
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, std::make_shared<PasteData>());
    int32_t result = service.ShowProgress(progressKey, observer);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
}

/**
 * @tc.name: ShowProgressTest002
 * @tc.desc: test Func ShowProgress IsFocusedApp return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ShowProgressTest002, TestSize.Level0)
{
    PasteboardService service;
    const std::string progressKey;
    sptr<IRemoteObject> observer;
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    g_accountIds = true;
    service.currentScreenStatus = ScreenEvent::ScreenUnlocked;
    service.copyTime_.InsertOrAssign(ACCOUNT_IDS_RANDOM,
        static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()));
    auto tokenId = IPCSkeleton::GetCallingTokenID();

    data->SetScreenStatus(ScreenEvent::ScreenUnlocked);
    data->SetShareOption(ShareOption::LocalDevice);
    data->SetTokenId(IPCSkeleton::GetCallingTokenID());
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    EXPECT_CALL(mock, GetCallingTokenID()).WillRepeatedly(Return(tokenId + UINT32_EXCEPTION_APPID));

    int32_t result = service.ShowProgress(progressKey, observer);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
}

/**
 * @tc.name: ShowProgressTest003
 * @tc.desc: test Func ShowProgress
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ShowProgressTest003, TestSize.Level0)
{
    PasteboardService service;
    const std::string progressKey;
    sptr<IRemoteObject> observer;
    service.currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    g_accountIds = true;
    service.currentScreenStatus = ScreenEvent::ScreenUnlocked;
    service.copyTime_.InsertOrAssign(ACCOUNT_IDS_RANDOM,
        static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()));

    data->SetScreenStatus(ScreenEvent::ScreenUnlocked);
    data->SetShareOption(ShareOption::LocalDevice);
    data->SetTokenId(IPCSkeleton::GetCallingTokenID());
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE));
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));

    int32_t result = service.ShowProgress(progressKey, observer);
    EXPECT_EQ(result, static_cast<int32_t>(ERR_OK));
}

/**
 * @tc.name: HasPasteDataTest001
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, HasPasteDataTest001, TestSize.Level0)
{
    PasteboardService service;
    bool flag = false;
    int32_t result = service.HasPasteData(flag);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_EQ(flag, false);
}

/**
 * @tc.name: HasDataTypeTest003
 * @tc.desc: currentScreenStatus is ScreenEvent::ScreenUnlocked.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, HasDataTypeTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);
    tempPasteboard->currentScreenStatus = ScreenEvent::ScreenUnlocked;
    tempPasteboard->currentUserId_ = 1;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));
    bool funcResult;
    int32_t res = tempPasteboard->HasDataType(MIMETYPE_TEXT_PLAIN, funcResult);
    EXPECT_EQ(res, ERR_OK);
}

/**
 * @tc.name: HasDataTypeTest004
 * @tc.desc: clipPlugin_ is not nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, HasDataTypeTest004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);
    tempPasteboard->currentScreenStatus = ScreenEvent::ScreenUnlocked;
    tempPasteboard->currentUserId_ = 1;
    std::string PLUGIN_NAME_VAL = "distributed_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    tempPasteboard->clipPlugin_ =
        std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(false));
    bool funcResult;
    int32_t res = tempPasteboard->HasDataType(MIMETYPE_TEXT_PLAIN, funcResult);
    EXPECT_EQ(res, ERR_OK);
}

/**
 * @tc.name: GetPasteDataTest001
 * @tc.desc: test Func GetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetPasteDataTest001, TestSize.Level0)
{
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE));
    EXPECT_CALL(mock, VerifyAccessToken).WillOnce(Return(PermissionState::PERMISSION_GRANTED))
        .WillOnce(Return(PermissionState::PERMISSION_GRANTED));
    PasteboardService service;
    int fd;
    int64_t size;
    std::vector<uint8_t> rawData;
    int32_t syncTime;
    int32_t realErrCode;
    int32_t result = service.GetPasteData(fd, size, rawData, "", syncTime, realErrCode);
    if (fd >= 0) {
        close(fd);
    }
    EXPECT_EQ(realErrCode, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
}

/**
 * @tc.name: GetPasteDataTest002
 * @tc.desc: test Func GetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetPasteDataTest002, TestSize.Level0)
{
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillOnce(Return(ATokenTypeEnum::TOKEN_HAP))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_HAP));
    PasteboardService service;
    service.setPasteDataUId_ = ERROR_USERID;
    int fd;
    int64_t size;
    std::vector<uint8_t> rawData;
    int32_t syncTime;
    int32_t realErrCode;
    int32_t result = service.GetPasteData(fd, size, rawData, "", syncTime, realErrCode);
    if (fd >= 0) {
        close(fd);
    }
    EXPECT_EQ(realErrCode, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
}

/**
 * @tc.name: GetPasteDataTest003
 * @tc.desc: test Func GetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetPasteDataTest003, TestSize.Level0)
{
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillOnce(Return(ATokenTypeEnum::TOKEN_HAP))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_HAP))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_HAP));
    PasteboardService service;
    service.setPasteDataUId_ = TEST_SERVER_UID;
    int fd;
    int64_t size;
    std::vector<uint8_t> rawData;
    int32_t syncTime;
    int32_t realErrCode;
    int32_t result = service.GetPasteData(fd, size, rawData, "", syncTime, realErrCode);
    if (fd >= 0) {
        close(fd);
    }
    EXPECT_EQ(realErrCode, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
}

/**
 * @tc.name: GetPasteDataInnerTest001
 * @tc.desc: test Func GetPasteDataInner
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetPasteDataInnerTest001, TestSize.Level0)
{
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE));
    EXPECT_CALL(mock, VerifyAccessToken).WillOnce(Return(PermissionState::PERMISSION_GRANTED))
        .WillOnce(Return(PermissionState::PERMISSION_GRANTED));
    PasteboardService service;
    int fd;
    int64_t size;
    std::vector<uint8_t> rawData;
    int32_t syncTime;
    UeReportInfo ueReportInfo;
    int32_t result = service.GetPasteDataInner(fd, size, rawData, "", syncTime, ueReportInfo);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
}

/**
 * @tc.name: SetAppShareOptionsTest001
 * @tc.desc: test Func SetAppShareOptions, it will be return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetAppShareOptionsTest001, TestSize.Level0)
{
    PasteboardService service;
    int32_t shareOptions = 1;

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingFullTokenID).WillOnce(testing::Return(123));
    int32_t result = service.SetAppShareOptions(shareOptions);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}

/**
 * @tc.name: SetAppShareOptionsTest002
 * @tc.desc: test Func SetAppShareOptions, it will be return PERMISSION_VERIFICATION_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetAppShareOptionsTest002, TestSize.Level0)
{
    PasteboardService service;
    int32_t shareOptions = 0;

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, VerifyAccessToken).WillOnce(Return(PermissionState::PERMISSION_DENIED));
    EXPECT_CALL(mock, GetCallingFullTokenID).WillOnce(testing::Return(123));
    EXPECT_CALL(mock, GetCallingTokenID).WillOnce(testing::Return(1000));
    int32_t result = service.SetAppShareOptions(shareOptions);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
}

/**
 * @tc.name: SetAppShareOptionsTest003
 * @tc.desc: test Func SetAppShareOptions, it will be return PERMISSION_VERIFICATION_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetAppShareOptionsTest003, TestSize.Level0)
{
    PasteboardService service;
    int32_t shareOptions = 0;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, VerifyAccessToken).WillOnce(Return(PermissionState::PERMISSION_DENIED));
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(4294967295));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    int32_t result = service.SetAppShareOptions(shareOptions);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
}

/**
 * @tc.name: GetGlobalShareOptionTest001
 * @tc.desc: test Func GetGlobalShareOption, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetGlobalShareOptionTest001, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));

    std::vector<uint32_t> tokenIds;
    std::unordered_map<uint32_t, int32_t> funcResult;
    int32_t result = service.GetGlobalShareOption(tokenIds, funcResult);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: GetGlobalShareOptionTest002
 * @tc.desc: test Func GetGlobalShareOption, it will be return PERMISSION_VERIFICATION_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetGlobalShareOptionTest002, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(0));

    std::vector<uint32_t> tokenIds;
    std::unordered_map<uint32_t, int32_t> funcResult;
    int32_t result = service.GetGlobalShareOption(tokenIds, funcResult);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
}

/**
 * @tc.name: GetGlobalShareOptionTest003
 * @tc.desc: test Func GetGlobalShareOption, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetGlobalShareOptionTest003, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));

    std::vector<uint32_t> tokenIds = {1, 2, 3};
    std::unordered_map<uint32_t, int32_t> funcResult;
    int32_t result = service.GetGlobalShareOption(tokenIds, funcResult);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: SetGlobalShareOptionTest001
 * @tc.desc: test Func SetGlobalShareOption, it will be return PERMISSION_VERIFICATION_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetGlobalShareOptionTest001, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(0));

    std::unordered_map<uint32_t, int32_t> globalShareOptions = {{1, static_cast<int32_t>(InApp)}};
    int32_t result = service.SetGlobalShareOption(globalShareOptions);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
}

/**
 * @tc.name: SetGlobalShareOptionTest002
 * @tc.desc: test Func SetGlobalShareOption, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetGlobalShareOptionTest002, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));

    std::unordered_map<uint32_t, int32_t> globalShareOptions =
        {{1, static_cast<int32_t>(InApp)}, {2, static_cast<int32_t>(CrossDevice)}};
    int32_t result = service.SetGlobalShareOption(globalShareOptions);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: SetGlobalShareOptionTest003
 * @tc.desc: test Func SetGlobalShareOption, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetGlobalShareOptionTest003, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));

    std::unordered_map<uint32_t, int32_t> globalShareOptions = {{1, static_cast<int32_t>(InApp)}, {2, -1}};
    int32_t result = service.SetGlobalShareOption(globalShareOptions);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeAllObserverTest001
 * @tc.desc: test Func UnsubscribeAllObserver, it will be return 0.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeAllObserverTest001, TestSize.Level0)
{
    PasteboardService service;
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    int32_t result = service.UnsubscribeAllObserver(type);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: UnsubscribeAllObserverTest002
 * @tc.desc: test Func UnsubscribeAllObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeAllObserverTest002, TestSize.Level0)
{
    PasteboardService service;
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    int32_t result = service.UnsubscribeAllObserver(type);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeAllObserverTest003
 * @tc.desc: test Func UnsubscribeAllObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeAllObserverTest003, TestSize.Level0)
{
    PasteboardService service;
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_REMOTE;

    int32_t result = service.UnsubscribeAllObserver(type);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeAllObserverTest004
 * @tc.desc: test Func UnsubscribeAllObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeAllObserverTest004, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;

    int32_t result = service.UnsubscribeAllObserver(type);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeObserverTest001
 * @tc.desc: test Func UnsubscribeObserver, it will be return 0.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeObserverTest001, TestSize.Level0)
{
    PasteboardService service;

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.UnsubscribeObserver(type, observer);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: UnsubscribeObserverTest002
 * @tc.desc: test Func UnsubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeObserverTest002, TestSize.Level0)
{
    PasteboardService service;
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.UnsubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeObserverTest003
 * @tc.desc: test Func UnsubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeObserverTest003, TestSize.Level0)
{
    PasteboardService service;
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_REMOTE;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.UnsubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeObserverTest004
 * @tc.desc: test Func UnsubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeObserverTest004, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.UnsubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: UnsubscribeObserverTest005
 * @tc.desc: test Func UnsubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnsubscribeObserverTest005, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(0));

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.UnsubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: SubscribeObserverTest001
 * @tc.desc: test Func SubscribeObserver, it will be return 0.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SubscribeObserverTest001, TestSize.Level0)
{
    PasteboardService service;

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.SubscribeObserver(type, observer);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: SubscribeObserverTest002
 * @tc.desc: test Func SubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SubscribeObserverTest002, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingPid()).WillOnce(testing::Return(1234));

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_LOCAL;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.SubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: SubscribeObserverTest003
 * @tc.desc: test Func SubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SubscribeObserverTest003, TestSize.Level0)
{
    PasteboardService service;
    PasteboardObserverType type = PasteboardObserverType::OBSERVER_REMOTE;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.SubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: SubscribeObserverTest004
 * @tc.desc: test Func SubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SubscribeObserverTest004, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(PasteboardService::EDM_UID));
    EXPECT_CALL(ipcMock, GetCallingPid()).WillOnce(testing::Return(1234));

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.SubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: SubscribeObserverTest005
 * @tc.desc: test Func SubscribeObserver, it will be return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SubscribeObserverTest005, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(0));
    EXPECT_CALL(ipcMock, GetCallingPid()).WillOnce(testing::Return(1234));

    PasteboardObserverType type = PasteboardObserverType::OBSERVER_EVENT;
    sptr<IPasteboardChangedObserver> observer = nullptr;
    int32_t result = service.SubscribeObserver(type, observer);
    ASSERT_EQ(result, ERR_OK);
}

/**
 * @tc.name: RemoveGlobalShareOptionTest001
 * @tc.desc: GetAllObserversSize function test
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RemoveGlobalShareOptionTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAppShareOptionsTest001 start.");

    std::vector<uint32_t> tokenIds = { 1001, 1002, 1003 };
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(-1));
    auto ret = tempPasteboard->RemoveGlobalShareOption(tokenIds);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAppShareOptionsTest001 end.");
}

/**
* @tc.name: RemoveGlobalShareOptionTest002
* @tc.desc: RemoveGlobalShareOption function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, RemoveGlobalShareOptionTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveGlobalShareOptionTest002 start.");
    std::vector<uint32_t> tokenIds = { 1001, 1002, 1003 };
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(PasteboardService::EDM_UID));
    auto ret = tempPasteboard->RemoveGlobalShareOption(tokenIds);
    ASSERT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveGlobalShareOptionTest002 end.");
}

/**
* @tc.name: RemoveAppShareOptionsTest003
* @tc.desc: RemoveAppShareOptions function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, RemoveAppShareOptionsTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAppShareOptionsTest003 start.");
    std::vector<uint32_t> tokenIds = {};
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(PasteboardService::EDM_UID));
    auto ret = tempPasteboard->RemoveGlobalShareOption(tokenIds);
    ASSERT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAppShareOptionsTest003 end.");
}

/**
* @tc.name: DumpDataTest001
* @tc.desc: DumpData function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, DumpDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest001 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    tempPasteboard->currentUserId_ = ERROR_USERID;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(INT_ONE));
    auto ret = tempPasteboard->DumpData();
    ASSERT_EQ(ret, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest001 end.");
}

/**
* @tc.name: DumpDataTest002
* @tc.desc: DumpData function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, DumpDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest002 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string expectStr;
    expectStr.append("No copy data.").append("\n");
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    auto ret = tempPasteboard->DumpData();
    ASSERT_EQ(ret, expectStr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest002 end.");
}

/**
* @tc.name: DumpDataTest003
* @tc.desc: DumpData function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, DumpDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest003 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string expectStr;
    expectStr.append("No copy data.").append("\n");
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    tempPasteboard->clips_.Insert(ACCOUNT_IDS_RANDOM, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    auto ret = tempPasteboard->DumpData();
    ASSERT_EQ(ret, expectStr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest003 end.");
}

/**
* @tc.name: DumpDataTest004
* @tc.desc: DumpData function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, DumpDataTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest004 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string expectStr;
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);
    size_t recordCounts = 1;
    auto tempRecord = std::make_shared<PasteDataRecord>();
    EXPECT_NE(tempRecord, nullptr);
    pasteData->records_.push_back(tempRecord);
    PasteDataProperty* pData = new PasteDataProperty();
    std::string sourceDevice = "remote";
    pData->isRemote = true;
    std::string bundleName = "pasteboard";
    pData->bundleName = bundleName;
    std::string setTime = "111111";
    pData->setTime = setTime;
    std::string shareOption = "InAPP";
    pData->shareOption = ShareOption::InApp;
    std::string mimeType = "text/plain";
    pData->mimeTypes.push_back(mimeType);
    pasteData->SetProperty(*pData);
    tempPasteboard->clips_.Insert(ACCOUNT_IDS_RANDOM, pasteData);
    expectStr.append("|Owner       :  ")
            .append(bundleName)
            .append("\n")
            .append("|Timestamp   :  ")
            .append(setTime)
            .append("\n")
            .append("|Share Option:  ")
            .append(shareOption)
            .append("\n")
            .append("|Record Count:  ")
            .append(std::to_string(recordCounts))
            .append("\n")
            .append("|Mime types  :  {")
            .append(mimeType)
            .append(",")
            .append("}")
            .append("\n")
            .append("|source device:  ")
            .append(sourceDevice);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    auto ret = tempPasteboard->DumpData();
    delete pData;
    ASSERT_EQ(ret, expectStr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest004 end.");
}

/**
 * @tc.name: DumpDataTest005
 * @tc.desc: DumpData function test
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, DumpDataTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest005 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string expectStr;
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    auto pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);
    size_t recordCounts = 1;
    auto tempRecord = std::make_shared<PasteDataRecord>();
    EXPECT_NE(tempRecord, nullptr);
    pasteData->records_.push_back(tempRecord);
    PasteDataProperty* pData = new PasteDataProperty();
    std::string sourceDevice = "local";
    std::string bundleName = "pasteboard";
    pData->bundleName = bundleName;
    std::string setTime = "111111";
    pData->setTime = setTime;
    std::string shareOption = "InAPP";
    pData->shareOption = ShareOption::InApp;
    pasteData->SetProperty(*pData);
    tempPasteboard->clips_.Insert(ACCOUNT_IDS_RANDOM, pasteData);
    expectStr.append("|Owner       :  ")
            .append(bundleName)
            .append("\n")
            .append("|Timestamp   :  ")
            .append(setTime)
            .append("\n")
            .append("|Share Option:  ")
            .append(shareOption)
            .append("\n")
            .append("|Record Count:  ")
            .append(std::to_string(recordCounts))
            .append("\n")
            .append("|Mime types  :  {")
            .append("}")
            .append("\n")
            .append("|source device:  ")
            .append(sourceDevice);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    auto ret = tempPasteboard->DumpData();
    delete pData;
    ASSERT_EQ(ret, expectStr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest005 end.");
}

/**
 * @tc.name: IsCallerUidValid001
 * @tc.desc: IsCallerUidValid001 function test
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, IsCallerUidValid001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid001 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(PasteboardService::EDM_UID));
    auto ret = tempPasteboard->IsCallerUidValid();
    ASSERT_EQ(ret, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid001 end.");
}

/**
* @tc.name: IsCallerUidValid002
* @tc.desc: IsCallerUidValid002 function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, IsCallerUidValid002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid002 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->uid_ = -1;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(-1));
    auto ret = tempPasteboard->IsCallerUidValid();
    ASSERT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid002 end.");
}

/**
* @tc.name: IsCallerUidValid003
* @tc.desc: IsCallerUidValid003 function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, IsCallerUidValid003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid003 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->uid_ = 1;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(-1));
    auto ret = tempPasteboard->IsCallerUidValid();
    ASSERT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid003 end.");
}

/**
* @tc.name: IsCallerUidValid004
* @tc.desc: IsCallerUidValid004 function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, IsCallerUidValid004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid004 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->uid_ = -1;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(1));
    auto ret = tempPasteboard->IsCallerUidValid();
    ASSERT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid004 end.");
}

/**
* @tc.name: IsCallerUidValid005
* @tc.desc: IsCallerUidValid005 function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, IsCallerUidValid005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid005 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->uid_ = 1;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid()).WillRepeatedly(testing::Return(1));
    auto ret = tempPasteboard->IsCallerUidValid();
    ASSERT_EQ(ret, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCallerUidValid005 end.");
}

/**
 * @tc.name: CloseDistributedStoreTest002
 * @tc.desc: test Func CloseDistributedStore
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CloseDistributedStoreTest002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t user = 1;
    bool isNeedClear = true;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    EXPECT_EQ(tempPasteboard->GetClipPlugin(), nullptr);
    tempPasteboard->CloseDistributedStore(user, isNeedClear);
}

/**
 * @tc.name: CloseDistributedStoreTest003
 * @tc.desc: test Func CloseDistributedStore
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CloseDistributedStoreTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t user = 1;
    bool isNeedClear = true;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    tempPasteboard->clipPlugin_ = nullptr;
    EXPECT_NE(tempPasteboard->GetClipPlugin(), nullptr);
    EXPECT_NE(tempPasteboard->GetClipPlugin(), nullptr);
    tempPasteboard->CloseDistributedStore(user, isNeedClear);
}

/**
 * @tc.name: CloseDistributedStoreTest004
 * @tc.desc: test Func CloseDistributedStore
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CloseDistributedStoreTest004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t user = 1;
    bool isNeedClear = true;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(false));
    EXPECT_EQ(tempPasteboard->GetClipPlugin(), nullptr);
    tempPasteboard->CloseDistributedStore(user, isNeedClear);
}

/**
 * @tc.name: OnConfigChangeTest001
 * @tc.desc: test Func OnConfigChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, OnConfigChangeTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    g_accountIds = false;
    tempPasteboard->currentUserId_ = 1;
    tempPasteboard->OnConfigChange(false);
    EXPECT_EQ(tempPasteboard->GetCurrentAccountId(), 1);
}

/**
 * @tc.name: OnConfigChangeTest003
 * @tc.desc: test Func OnConfigChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, OnConfigChangeTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
    tempPasteboard->OnConfigChange(true);
}

/**
 * @tc.name: OnConfigChangeTest004
 * @tc.desc: test Func OnConfigChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, OnConfigChangeTest004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL4;
    tempPasteboard->OnConfigChange(true);
}

/**
 * @tc.name: OnConfigChangeTest005
 * @tc.desc: test Func OnConfigChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, OnConfigChangeTest005, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL4;
    tempPasteboard->OnConfigChange(true);
}

/**
 * @tc.name: PasteboardEventSubscriberTest001
 * @tc.desc: test Func PasteboardEventSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PasteboardEventSubscriberTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetNetworkId()).WillRepeatedly(testing::Return(""));
    tempPasteboard->PasteboardEventSubscriber();
}

/**
 * @tc.name: PasteboardEventSubscriberTest002
 * @tc.desc: test Func PasteboardEventSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PasteboardEventSubscriberTest002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId = "networkId1";
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetNetworkId()).WillRepeatedly(testing::Return(networkId));
    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = 1234;
    p2pInfo.isSuccess = false;
    std::string key = "key1";
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(key, p2pInfo);
    tempPasteboard->p2pMap_.Insert(networkId, p2pMap);
    tempPasteboard->PasteboardEventSubscriber();
    EXPECT_NE(tempPasteboard->p2pMap_.Size(), 0);
}

/**
 * @tc.name: PasteboardEventSubscriberTest003
 * @tc.desc: test Func PasteboardEventSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PasteboardEventSubscriberTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetNetworkId()).WillRepeatedly(testing::Return("networkId1"));
    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = 1234;
    p2pInfo.isSuccess = false;
    std::string key = "key1";
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(key, p2pInfo);
    tempPasteboard->p2pMap_.Insert("networkId2", p2pMap);
    tempPasteboard->PasteboardEventSubscriber();
    EXPECT_NE(tempPasteboard->p2pMap_.Size(), 0);
}

/**
 * @tc.name: SetDistributedDataTest001
 * @tc.desc: test Func SetDistributedData, IsConstraintEnabled is false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetDistributedDataTest001, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetIntParameter(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(INT32_NEGATIVE_NUMBER));

    PasteData pasteData;
    int32_t result = service.SetDistributedData(ACCOUNT_IDS_RANDOM, pasteData);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SetDistributedDataTest002
 * @tc.desc: test Func SetDistributedData, IsDisallowDistributed is true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetDistributedDataTest002, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetIntParameter(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(CONTROL_TYPE_ALLOW_SEND_RECEIVE));
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(DEVICE_COLLABORATION_UID));

    PasteData pasteData;
    int32_t result = service.SetDistributedData(ACCOUNT_IDS_RANDOM, pasteData);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SetDistributedDataTest003
 * @tc.desc: test Func SetDistributedData, clipPlugin == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetDistributedDataTest003, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetIntParameter(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(CONTROL_TYPE_ALLOW_SEND_RECEIVE));
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(1234));

    service.securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    PasteData pasteData;
    int32_t result = service.SetDistributedData(ACCOUNT_IDS_RANDOM, pasteData);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SetDistributedDataTest004
 * @tc.desc: test Func SetDistributedData, return true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetDistributedDataTest004, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetIntParameter(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(CONTROL_TYPE_ALLOW_SEND_RECEIVE));
    EXPECT_CALL(ipcMock, GetCallingUid()).WillOnce(testing::Return(1234));

    PasteData pasteData;
    int32_t result = service.SetDistributedData(ACCOUNT_IDS_RANDOM, pasteData);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SetCurrentDataTest001
 * @tc.desc: test Func SetCurrentData, clipPlugin == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetCurrentDataTest001, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;

    service.securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    TestEvent event;
    PasteData data;
    bool result = service.SetCurrentData(event, data);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SetCurrentDataTest002
 * @tc.desc: test Func SetCurrentData, needFull is true, encode is false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetCurrentDataTest002, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    service.securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    std::string PLUGIN_NAME_VAL = "mock_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    std::shared_ptr<ClipPlugin> clipPlugin =
        std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    service.clipPlugin_ = clipPlugin;
    TestEvent event;
    PasteData data;
    data.AddTextRecord(RANDOM_STRING);
    data.SetDelayRecord(true);
    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));
    EXPECT_CALL(mock, GetRemoteDeviceMinVersion())
        .WillRepeatedly(testing::Return(DistributedModuleConfig::FIRST_VERSION));

    bool result = service.SetCurrentData(event, data);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SetCurrentDataTest003
 * @tc.desc: test Func SetCurrentData, data.IsDelayRecord() && !needFull
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetCurrentDataTest003, TestSize.Level0)
{
    PasteboardService service;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    service.securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    std::string PLUGIN_NAME_VAL = "mock_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    std::shared_ptr<ClipPlugin> clipPlugin =
        std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    service.clipPlugin_ = clipPlugin;
    TestEvent event;
    PasteData data;
    data.AddTextRecord(RANDOM_STRING);
    data.SetDelayRecord(true);
    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));
    EXPECT_CALL(mock, GetRemoteDeviceMinVersion())
        .WillRepeatedly(testing::Return(DistributedModuleConfig::FIRST_VERSION + 1));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    bool result = service.SetCurrentData(event, data);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: GetDistributedDelayEntryTest001
 * @tc.desc: test Func GetDistributedDelayEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetDistributedDelayEntryTest001, TestSize.Level0)
{
    PasteboardService service;
    TestEvent event;
    event.user = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    EXPECT_NE(data, nullptr);
    auto record = PasteDataRecord::NewPlainTextRecord(RANDOM_STRING);
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(true);
    data->records_.emplace_back(record);
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    event.dataId == data->GetDataId();
    std::vector<uint8_t> rawData;
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetMimeType(MIMETYPE_TEXT_PLAIN);

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, service);
    service.entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry));
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    service.GetDistributedDelayEntry(event, data->GetRecordId(), RANDOM_STRING, rawData);
}

/**
 * @tc.name: GetDistributedDelayEntryTest002
 * @tc.desc: test Func GetDistributedDelayEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetDistributedDelayEntryTest002, TestSize.Level0)
{
    PasteboardService service;
    TestEvent event;
    event.user = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    EXPECT_NE(data, nullptr);
    OHOS::Uri uri("uri");
    auto record = PasteDataRecord::NewUriRecord(uri);
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(true);
    data->records_.emplace_back(record);
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    event.dataId == data->GetDataId();
    std::vector<uint8_t> rawData;
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetMimeType(MIMETYPE_TEXT_URI);

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, service);
    service.entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry));
    service.GetDistributedDelayEntry(event, data->GetRecordId(), RANDOM_STRING, rawData);
}

/**
 * @tc.name: GetDistributedDelayEntryTest003
 * @tc.desc: test Func GetDistributedDelayEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetDistributedDelayEntryTest003, TestSize.Level0)
{
    PasteboardService service;
    TestEvent event;
    event.user = ACCOUNT_IDS_RANDOM;
    auto data = std::make_shared<PasteData>();
    EXPECT_NE(data, nullptr);
    auto record = PasteDataRecord::NewHtmlRecord(HTML_STRING);
    data->SetDataId(service.delayDataId_);
    data->AddRecord(record);
    data->SetRemote(true);
    data->records_.emplace_back(record);
    service.clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, data);
    event.dataId == data->GetDataId();
    std::vector<uint8_t> rawData;
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetMimeType(MIMETYPE_TEXT_HTML);

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, service);
    service.entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetRecordById(testing::_)).WillOnce(Return(record));
    EXPECT_CALL(mock, GetEntry(testing::_)).WillOnce(Return(entry));
    service.GetDistributedDelayEntry(event, data->GetRecordId(), RANDOM_STRING, rawData);
}

/**
 * @tc.name: ProcessDistributedDelayUriTest003
 * @tc.desc: test Func ProcessDistributedDelayUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ProcessDistributedDelayUriTest003, TestSize.Level0)
{
    PasteboardService service;

    PasteData data;
    OHOS::Uri uri("/");
    auto record = PasteDataRecord::NewUriRecord(uri);
    data.AddRecord(record);

    auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_URI);
    auto utdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, entry->GetMimeType());
    entry->SetUtdId(utdId);
    std::vector<uint8_t> rawData;

    NiceMock<PasteboardServiceInterfaceMock> mock;
    struct HmdfsUriInfo dfsUri;
    EXPECT_CALL(mock, GetDfsUriFromLocal(testing::_, testing::_, testing::_))
        .WillOnce([](auto, auto, struct HmdfsUriInfo &hui) {
            hui.uriStr = "uri";
            hui.fileSize = 0;
            return 0;
        });
    EXPECT_CALL(mock, Encode(testing::_)).WillOnce(Return(true));
    int32_t ret = service.ProcessDistributedDelayUri(ACCOUNT_IDS_RANDOM, data, *entry, rawData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetClipPluginTest001
 * @tc.desc: test Func GetClipPlugin
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetClipPluginTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;

    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));

    auto result = tempPasteboard->GetClipPlugin();
    EXPECT_EQ(result, nullptr);
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: GetClipPluginTest002
 * @tc.desc: test Func GetClipPlugin
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetClipPluginTest002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    tempPasteboard->clipPlugin_ = nullptr;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;

    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));

    auto result = tempPasteboard->GetClipPlugin();
    EXPECT_NE(result, nullptr);
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: GetClipPluginTest003
 * @tc.desc: test Func GetClipPlugin
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetClipPluginTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    tempPasteboard->clipPlugin_ = nullptr;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;

    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(false));

    auto result = tempPasteboard->GetClipPlugin();
    EXPECT_EQ(result, nullptr);
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: GetClipPluginTest004
 * @tc.desc: test Func GetClipPlugin
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetClipPluginTest004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    std::string PLUGIN_NAME_VAL = "distributed_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    std::shared_ptr<ClipPlugin> clipPlugin =
        std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    tempPasteboard->clipPlugin_ = clipPlugin;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;

    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));

    auto result = tempPasteboard->GetClipPlugin();
    EXPECT_EQ(result, clipPlugin);
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: CleanDistributedDataTest001
 * @tc.desc: test Func CleanDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CleanDistributedDataTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->clipPlugin_ = nullptr;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;

    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));

    tempPasteboard->CleanDistributedData(ACCOUNT_IDS_RANDOM);
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: ChangeStoreStatusTest001
 * @tc.desc: test Func ChangeStoreStatus
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ChangeStoreStatusTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->clipPlugin_ = nullptr;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;

    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));

    tempPasteboard->ChangeStoreStatus(ACCOUNT_IDS_RANDOM);
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}


/**
 * @tc.name: GetFullDelayPasteDataTest001
 * @tc.desc: test Func GetFullDelayPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetFullDelayPasteDataTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->clipPlugin_ = nullptr;

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, *tempPasteboard);
    tempPasteboard->entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    PasteData data;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    record->SetDelayRecordFlag(false);
    record->SetRecordId(11);
    record->mimeType_ = MIMETYPE_TEXT_HTML;
    std::vector<std::shared_ptr<PasteDataRecord>> records_;
    records_.push_back(record);

    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mock, AllRecords()).WillRepeatedly(testing::Return(records_));

    auto result = tempPasteboard->GetFullDelayPasteData(ACCOUNT_IDS_RANDOM, data);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: GetFullDelayPasteDataTest002
 * @tc.desc: test Func GetFullDelayPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetFullDelayPasteDataTest002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->clipPlugin_ = nullptr;

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, *tempPasteboard);
    tempPasteboard->entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    PasteData data;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    record->SetDelayRecordFlag(true);
    record->SetRecordId(11);
    record->mimeType_ = MIMETYPE_TEXT_HTML;
    std::vector<std::shared_ptr<PasteDataEntry>> entries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    auto newObject = std::make_shared<Object>();
    entry->SetValue(newObject);
    entries.emplace_back(entry);
    record->entries_.push_back(entry);
    std::vector<std::shared_ptr<PasteDataRecord>> records_;
    records_.push_back(record);

    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mock, AllRecords()).WillRepeatedly(testing::Return(records_));

    auto result = tempPasteboard->GetFullDelayPasteData(ACCOUNT_IDS_RANDOM, data);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
}

/**
 * @tc.name: GetFullDelayPasteDataTest003
 * @tc.desc: test Func GetFullDelayPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetFullDelayPasteDataTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->clipPlugin_ = nullptr;
    g_recordValueByType = static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, *tempPasteboard);
    tempPasteboard->entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    PasteData data;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    record->SetDelayRecordFlag(true);
    record->SetRecordId(11);
    record->mimeType_ = MIMETYPE_TEXT_HTML;
    std::vector<std::shared_ptr<PasteDataEntry>> entries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    EntryValue var;
    entry->SetValue(var);
    entries.emplace_back(entry);
    record->entries_.push_back(entry);
    std::vector<std::shared_ptr<PasteDataRecord>> records_;
    records_.push_back(record);

    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mock, AllRecords()).WillRepeatedly(testing::Return(records_));

    auto result = tempPasteboard->GetFullDelayPasteData(ACCOUNT_IDS_RANDOM, data);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
    g_recordValueByType = static_cast<int32_t>(PasteboardError::E_OK);
}

/**
 * @tc.name: GetFullDelayPasteDataTest004
 * @tc.desc: test Func GetFullDelayPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetFullDelayPasteDataTest004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->clipPlugin_ = nullptr;
    g_recordValueByType = static_cast<int32_t>(PasteboardError::E_OK);

    sptr<PasteboardEntryGetterImpl> pasteboardEntryGetterImpl = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient =
        sptr<PasteboardService::EntryGetterDeathRecipient>::MakeSptr(ACCOUNT_IDS_RANDOM, *tempPasteboard);
    tempPasteboard->entryGetters_.InsertOrAssign(
        ACCOUNT_IDS_RANDOM, std::make_pair(pasteboardEntryGetterImpl, deathRecipient));

    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    PasteData data;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    record->SetDelayRecordFlag(true);
    record->SetRecordId(11);
    record->mimeType_ = MIMETYPE_TEXT_HTML;
    std::vector<std::shared_ptr<PasteDataEntry>> entries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    EntryValue var;
    entry->SetValue(var);
    entry->SetMimeType(MIMETYPE_TEXT_HTML);
    entries.emplace_back(entry);
    record->entries_.push_back(entry);
    std::vector<std::shared_ptr<PasteDataRecord>> records_;
    records_.push_back(record);

    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mock, AllRecords()).WillRepeatedly(testing::Return(records_));

    auto result = tempPasteboard->GetFullDelayPasteData(ACCOUNT_IDS_RANDOM, data);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL0;
    g_recordValueByType = static_cast<int32_t>(PasteboardError::E_OK);
}

/**
 * @tc.name: GetDistributedData002
 * @tc.desc: test Func GetDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetDistributedData002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    TestEvent event;
    int32_t user = 0;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    std::pair<std::shared_ptr<PasteData>, PasteDateResult> result = tempPasteboard->GetDistributedData(event, user);
    EXPECT_EQ(result.second.errorCode, static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR));
}

/**
 * @tc.name: GetDistributedData003
 * @tc.desc: test Func GetDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetDistributedData003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    TestEvent event;
    int32_t user = 0;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    auto release = [](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(RANDOM_STRING, plugin);
    };
    tempPasteboard->clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(RANDOM_STRING), release);
    EXPECT_CALL(mock, IsOn()).WillOnce(testing::Return(true));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL4;
    std::pair<std::shared_ptr<PasteData>, PasteDateResult> result = tempPasteboard->GetDistributedData(event, user);
    EXPECT_EQ(result.second.errorCode, static_cast<int32_t>(PasteboardError::E_OK));
    tempPasteboard->clipPlugin_ = nullptr;
}

/**
 * @tc.name: GetPasteDataDot002
 * @tc.desc: test Func GetPasteDataDot
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetPasteDataDot002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData pasteData;
    pasteData.SetRemote(false);
    int32_t userId = 0;
    EXPECT_NO_FATAL_FAILURE(tempPasteboard->GetPasteDataDot(pasteData, RANDOM_STRING, userId));
}

/**
 * @tc.name: GetPasteDataDot003
 * @tc.desc: test Func GetPasteDataDot
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GetPasteDataDot003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData pasteData;
    pasteData.SetRemote(true);
    int32_t userId = 0;
    EXPECT_NO_FATAL_FAILURE(tempPasteboard->GetPasteDataDot(pasteData, RANDOM_STRING, userId));
}

/**
 * @tc.name: DumpHistory002
 * @tc.desc: test Func DumpHistory
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, DumpHistory002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->dataHistory_.clear();
    tempPasteboard->dataHistory_.push_back(RANDOM_STRING);
    std::string history = tempPasteboard->DumpHistory();
    EXPECT_TRUE(history.find("Access history last ten times: ") != std::string::npos);
    tempPasteboard->dataHistory_.clear();
}

/**
 * @tc.name: DumpHistory003
 * @tc.desc: test Func DumpHistory
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, DumpHistory003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->dataHistory_.clear();
    std::string history = tempPasteboard->DumpHistory();
    EXPECT_TRUE(history.find("Access history fail! dataHistory_ no data.") != std::string::npos);
}

/**
 * @tc.name: Dump002
 * @tc.desc: test Func Dump when uid > maxUid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, Dump002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int fd = INT_ONE;
    std::vector<std::u16string> args;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingUid).WillOnce(testing::Return(UINT32_EXCEPTION_APPID));
    int result = tempPasteboard->Dump(fd, args);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: Dump003
 * @tc.desc: test Func Dump when PasteboardDumpHelper::GetInstance().Dump return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, Dump003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int fd = INT_ONE;
    std::vector<std::u16string> args;
    args.push_back(RANDOM_U16STRING);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingUid).WillOnce(testing::Return(INT_THREETHREETHREE));
    EXPECT_CALL(mock, Dump).WillOnce(testing::Return(false));
    int result = tempPasteboard->Dump(fd, args);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: Dump004
 * @tc.desc: test Func Dump when PasteboardDumpHelper::GetInstance().Dump return true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, Dump004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int fd = INT_ONE;
    std::vector<std::u16string> args;
    args.push_back(RANDOM_U16STRING);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetCallingUid).WillOnce(testing::Return(INT_THREETHREETHREE));
    EXPECT_CALL(mock, Dump).WillOnce(testing::Return(true));
    int result = tempPasteboard->Dump(fd, args);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: IsNeedThaw002
 * @tc.desc: test Func IsNeedThaw when InputMethodController::GetInstance return nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, IsNeedThaw002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetInstance).WillOnce(testing::Return(nullptr));
    bool result = tempPasteboard->IsNeedThaw();
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsNeedThaw003
 * @tc.desc: test Func IsNeedThaw when imc->GetDefaultInputMethod is error
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, IsNeedThaw003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    sptr<InputMethodController> instance_ = new (std::nothrow) InputMethodController();
    g_encodeInsert = true;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetInstance).WillOnce(testing::Return(instance_));
    EXPECT_CALL(mock, GetDefaultInputMethod).WillOnce(testing::Return(INT32_NEGATIVE_NUMBER));
    bool result = tempPasteboard->IsNeedThaw();
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsNeedThaw004
 * @tc.desc: test Func IsNeedThaw when property == nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, IsNeedThaw004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    sptr<InputMethodController> instance_ = new (std::nothrow) InputMethodController();
    g_encodeInsert = false;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetInstance).WillOnce(testing::Return(instance_));
    EXPECT_CALL(mock, GetDefaultInputMethod).WillOnce(testing::Return(0));
    bool result = tempPasteboard->IsNeedThaw();
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsNeedThaw005
 * @tc.desc: test Func IsNeedThaw when validity
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, IsNeedThaw005, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    sptr<InputMethodController> instance_ = new (std::nothrow) InputMethodController();
    g_encodeInsert = true;
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetInstance).WillOnce(testing::Return(instance_));
    EXPECT_CALL(mock, GetDefaultInputMethod).WillOnce(testing::Return(0));
    bool result = tempPasteboard->IsNeedThaw();
    EXPECT_TRUE(result);
}

/**
 * @tc.name: GrantUriPermissionTest001
 * @tc.desc: test Func GrantUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GrantUriPermissionTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetBoolParameter(_, _)).WillRepeatedly(testing::Return(true));

    std::vector<Uri> grantUris;
    std::string targetBundleName = "com.example.app";
    int32_t appIndex = 1;
    std::string uriStr = URI_STRING;
    auto uri = OHOS::Uri(uriStr);
    grantUris.push_back(uri);
    std::map<uint32_t, std::vector<Uri>> uriMap;
    uriMap.insert(std::make_pair(PasteDataRecord::READ_PERMISSION, grantUris));

    auto result = tempPasteboard->GrantUriPermission(uriMap, targetBundleName, false, appIndex);
    EXPECT_NE(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GrantUriPermissionTest002
 * @tc.desc: test Func GrantUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GrantUriPermissionTest002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetBoolParameter(_, _)).WillRepeatedly(testing::Return(false));

    std::vector<Uri> grantUris;
    std::string targetBundleName = "com.example.app";
    int32_t appIndex = 1;
    std::string uriStr = URI_STRING;
    auto uri = OHOS::Uri(uriStr);
    grantUris.push_back(uri);
    std::map<uint32_t, std::vector<Uri>> uriMap;
    uriMap.insert(std::make_pair(PasteDataRecord::READ_PERMISSION, grantUris));

    auto result = tempPasteboard->GrantUriPermission(uriMap, targetBundleName, false, appIndex);
    EXPECT_NE(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GrantUriPermissionTest003
 * @tc.desc: test Func GrantUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GrantUriPermissionTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetBoolParameter(_, _)).WillRepeatedly(testing::Return(true));

    std::vector<Uri> grantUris;
    std::string targetBundleName = "com.example.app";
    int32_t appIndex = 1;
    std::string uriStr = URI_STRING;
    auto uri = OHOS::Uri(uriStr);
    grantUris.push_back(uri);
    std::map<uint32_t, std::vector<Uri>> uriMap;
    uriMap.insert(std::make_pair(PasteDataRecord::READ_PERMISSION, grantUris));

    auto result = tempPasteboard->GrantUriPermission(uriMap, targetBundleName, true, appIndex);
    EXPECT_NE(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GrantUriPermissionTest004
 * @tc.desc: test Func GrantUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, GrantUriPermissionTest004, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetBoolParameter(_, _)).WillRepeatedly(testing::Return(false));

    std::vector<Uri> grantUris;
    std::string targetBundleName = "com.example.app";
    int32_t appIndex = 1;
    std::string uriStr = URI_STRING;
    auto uri = OHOS::Uri(uriStr);
    grantUris.push_back(uri);
    std::map<uint32_t, std::vector<Uri>> uriMap;
    uriMap.insert(std::make_pair(PasteDataRecord::READ_PERMISSION, grantUris));

    auto result = tempPasteboard->GrantUriPermission(uriMap, targetBundleName, true, appIndex);
    EXPECT_NE(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: SetAppShareOptionsTest004
 * @tc.desc: test Func SetAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetAppShareOptionsTest004, TestSize.Level0)
{
    PasteboardService service;
    int32_t shareOptions = 0;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(4294967296));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    int32_t result = service.SetAppShareOptions(shareOptions);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: SetAppShareOptionsTest005
 * @tc.desc: test Func SetAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetAppShareOptionsTest005, TestSize.Level0)
{
    PasteboardService service;
    int32_t shareOptions = 0;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(4294967296));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    ShareOption testOption = InApp;
    PasteboardService::GlobalShareOption option = {.source = PasteboardService::GlobalShareOptionSource::APP,
        .shareOption = testOption};
    service.globalShareOptions_.InsertOrAssign(1000, option);
    int32_t result = service.SetAppShareOptions(shareOptions);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_OPERATION_ERROR));
}

/**
 * @tc.name: RemoveAppShareOptionsTest001
 * @tc.desc: test Func RemoveAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RemoveAppShareOptionsTest001, TestSize.Level0)
{
    PasteboardService service;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, VerifyAccessToken).WillOnce(Return(PermissionState::PERMISSION_DENIED));
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(123));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    int32_t result = service.RemoveAppShareOptions();
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
}

/**
 * @tc.name: RemoveAppShareOptionsTest002
 * @tc.desc: test Func RemoveAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RemoveAppShareOptionsTest002, TestSize.Level0)
{
    PasteboardService service;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(4294967296));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    int32_t result = service.RemoveAppShareOptions();
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: RemoveAppShareOptionsTest005
 * @tc.desc: test Func RemoveAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RemoveAppShareOptionsTest005, TestSize.Level0)
{
    PasteboardService service;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(4294967296));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    ShareOption testOption = InApp;
    PasteboardService::GlobalShareOption option = {.source = PasteboardService::GlobalShareOptionSource::APP,
        .shareOption = testOption};
    service.globalShareOptions_.InsertOrAssign(1000, option);
    int32_t result = service.RemoveAppShareOptions();
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: RemoveAppShareOptionsTest004
 * @tc.desc: test Func RemoveAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RemoveAppShareOptionsTest004, TestSize.Level0)
{
    PasteboardService service;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingFullTokenID).WillOnce(testing::Return(4294967296));
    EXPECT_CALL(ipcMock, GetCallingTokenID).WillOnce(testing::Return(1000));
    ShareOption testOption = InApp;
    PasteboardService::GlobalShareOption option = {.source = PasteboardService::GlobalShareOptionSource::MDM,
        .shareOption = testOption};
    service.globalShareOptions_.InsertOrAssign(1000, option);
    int32_t result = service.RemoveAppShareOptions();
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: CheckMdmShareOptionTest001
 * @tc.desc: test Func CheckMdmShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CheckMdmShareOptionTest001, TestSize.Level0)
{
    PasteboardService service;
    PasteData pasteData;

    pasteData.SetTokenId(1000);
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    ShareOption testOption = InApp;
    PasteboardService::GlobalShareOption option = {.source = PasteboardService::GlobalShareOptionSource::MDM,
        .shareOption = testOption};
    service.globalShareOptions_.InsertOrAssign(1000, option);
    int32_t result = service.CheckMdmShareOption(pasteData);
    ASSERT_EQ(result, true);
}

/**
 * @tc.name: CheckMdmShareOptionTest002
 * @tc.desc: test Func CheckMdmShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CheckMdmShareOptionTest002, TestSize.Level0)
{
    PasteboardService service;
    PasteData pasteData;

    NiceMock<PasteboardServiceInterfaceMock> ipcMock;
    int32_t result = service.CheckMdmShareOption(pasteData);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SetPasteboardHistoryTest001
 * @tc.desc: test Func SetPasteboardHistory
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, SetPasteboardHistoryTest001, TestSize.Level0)
{
    PasteboardService service;
    HistoryInfo info;
    for (int i = 1; i <= INT32_TEN; i++) {
        info.time = std::to_string(i);
        info.bundleName = "app" + std::to_string(i);
        info.state = "COPY";
        info.remote = "";
        info.userId = 0;
        service.SetPasteboardHistory(info);
    }

    ASSERT_EQ(service.dataHistory_.size(), INT32_TEN);
    info.time = "11";
    info.bundleName = "app11";
    NiceMock<PasteboardServiceInterfaceMock> ipcMock;

    int32_t result = service.SetPasteboardHistory(info);
    ASSERT_EQ(service.dataHistory_.size(), INT32_TEN);

    ASSERT_EQ(result, true);
    EXPECT_TRUE(service.dataHistory_.front().find("2 ") == 0);
    EXPECT_TRUE(service.dataHistory_.back().find("11 ") == 0);
}

/**
 * @tc.name: HandleDelayDataAndRecordTest001
 * @tc.desc: HandleDelayDataAndRecordTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, HandleDelayDataAndRecordTest001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    AppInfo appInfo;

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsDelayData()).WillOnce(Return(true));

    tempPasteboard->HandleDelayDataAndRecord(pasteData, delayGetter, entryGetter, appInfo);
}

/**
 * @tc.name: HandleDelayDataAndRecordTest002
 * @tc.desc: HandleDelayDataAndRecordTest002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, HandleDelayDataAndRecordTest002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;
    AppInfo appInfo;

    tempPasteboard->HandleDelayDataAndRecord(pasteData, delayGetter, entryGetter, appInfo);
}

/**
 * @tc.name: HandleDelayDataAndRecordTest003
 * @tc.desc: HandleDelayDataAndRecordTest003
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, HandleDelayDataAndRecordTest003, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    sptr<IPasteboardDelayGetter> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;
    AppInfo appInfo;

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsDelayRecord()).WillOnce(Return(true));

    tempPasteboard->HandleDelayDataAndRecord(pasteData, delayGetter, entryGetter, appInfo);
}

/**
 * @tc.name: EstablishP2PLinkTest001
 * @tc.desc: EstablishP2PLinkTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, EstablishP2PLinkTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteboardService service;
    std::string networkId = "network123";
    std::string pasteId = "paste123";

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetRemoteDeviceInfo(testing::_, testing::_))
        .WillOnce(Return(static_cast<int32_t>(PasteboardError::E_OK)));

    tempPasteboard->EstablishP2PLink(networkId, pasteId);
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: CloseP2PLinkTest001
 * @tc.desc: CloseP2PLinkTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CloseP2PLinkTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string networkId = "network123";

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetRemoteDeviceInfo(testing::_, testing::_))
        .WillOnce(Return(static_cast<int32_t>(PasteboardError::E_OK)));

    tempPasteboard->CloseP2PLink(networkId);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: PreSyncRemotePasteboardDataTest
 * @tc.desc: PreSyncRemotePasteboardData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PreSyncRemotePasteboardDataTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    testing::NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, IsOn()).WillRepeatedly(testing::Return(true));
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL1;
    tempPasteboard->PreSyncRemotePasteboardData();
    tempPasteboard->securityLevel_.securityLevel_ = DATA_SEC_LEVEL3;
    std::string PLUGIN_NAME_VAL = "distributed_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    std::shared_ptr<ClipPlugin> clipPlugin =
        std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    EXPECT_NE(clipPlugin, nullptr);
    tempPasteboard->clipPlugin_ = clipPlugin;
    EXPECT_NE(tempPasteboard->GetClipPlugin(), nullptr);
    tempPasteboard->PreSyncRemotePasteboardData();
}

/**
 * @tc.name: ClearP2PEstablishTaskInfoTest
 * @tc.desc: ClearP2PEstablishTaskInfo
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ClearP2PEstablishTaskInfoTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ClearP2PEstablishTaskInfo();
}

/**
 * @tc.name: RegisterPreSyncCallbackTest
 * @tc.desc: RegisterPreSyncCallback
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RegisterPreSyncCallbackTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->RegisterPreSyncCallback(nullptr);
    auto clipPlugin = std::make_shared<DefaultClip>();
    EXPECT_NE(clipPlugin, nullptr);
    tempPasteboard->RegisterPreSyncCallback(clipPlugin);
}

/**
 * @tc.name: PreEstablishP2PLinkCallbackTest
 * @tc.desc: PreEstablishP2PLinkCallback
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PreEstablishP2PLinkCallbackTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId;
    auto clipPlugin = std::make_shared<DefaultClip>();
    EXPECT_NE(clipPlugin, nullptr);
    tempPasteboard->PreEstablishP2PLinkCallback(networkId, clipPlugin.get());
    networkId = "TestNetworkId";
    tempPasteboard->PreEstablishP2PLinkCallback(networkId, clipPlugin.get());
}

/**
 * @tc.name: PreSyncSwitchMonitorCallbackTest
 * @tc.desc: PreSyncSwitchMonitorCallback
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PreSyncSwitchMonitorCallbackTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->PreSyncSwitchMonitorCallback();
}

/**
 * @tc.name: RegisterPreSyncMonitorTest
 * @tc.desc: RegisterPreSyncMonitor
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, RegisterPreSyncMonitorTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->RegisterPreSyncMonitor();
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    EXPECT_NE(tempPasteboard->ffrtTimer_, nullptr);
    tempPasteboard->RegisterPreSyncMonitor();
    tempPasteboard->subscribeActiveId_ = 0;
    tempPasteboard->RegisterPreSyncMonitor();
}

/**
 * @tc.name: UnRegisterPreSyncMonitorTest
 * @tc.desc: UnRegisterPreSyncMonitor
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, UnRegisterPreSyncMonitorTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->subscribeActiveId_ = -1;
    tempPasteboard->UnRegisterPreSyncMonitor();
}

/**
 * @tc.name: DeletePreSyncP2pFromP2pMapTest
 * @tc.desc: DeletePreSyncP2pFromP2pMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, DeletePreSyncP2pFromP2pMapTest, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId = "TestNetworkId";
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    EXPECT_NE(tempPasteboard->ffrtTimer_, nullptr);
    tempPasteboard->DeletePreSyncP2pFromP2pMap(networkId);
}

/**
 * @tc.name: CheckAndReuseP2PLinkTest001
 * @tc.desc: CheckAndReuseP2PLink
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CheckAndReuseP2PLinkTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId = "TestNetworkId";
    std::string pasteId = "P2pPreSyncId_";
    auto result = tempPasteboard->CheckAndReuseP2PLink(networkId, pasteId);
    EXPECT_EQ(result, nullptr);
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    EXPECT_NE(tempPasteboard->ffrtTimer_, nullptr);

    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = 123;
    p2pInfo.isSuccess = false;
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(pasteId, p2pInfo);
    tempPasteboard->p2pMap_.Insert(networkId, p2pMap);
    std::shared_ptr<BlockObject<int32_t>> block = std::make_shared<BlockObject<int32_t>>(2000, 0);
    EXPECT_NE(block, nullptr);
    tempPasteboard->preSyncP2pMap_.insert(std::make_pair(networkId, block));
    result = tempPasteboard->CheckAndReuseP2PLink(networkId, pasteId);
    EXPECT_NE(result, nullptr);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: EstablishP2PLinkTaskTest001
 * @tc.desc: EstablishP2PLinkTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, EstablishP2PLinkTaskTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    TestEvent event;
    std::string pasteId = "P2pPreSyncId_";
    auto result = tempPasteboard->EstablishP2PLinkTask(pasteId, event);
    EXPECT_EQ(result, nullptr);
    event.deviceId = DMAdapter::GetInstance().GetLocalNetworkId();
    result = tempPasteboard->EstablishP2PLinkTask(pasteId, event);
    EXPECT_EQ(result, nullptr);
    event.deviceId = "TestNetworkId";
    std::string uriType = "text/plain";
    event.dataType.push_back(uriType);
    result = tempPasteboard->EstablishP2PLinkTask(pasteId, event);
    EXPECT_EQ(result, nullptr);
    uriType = "text/uri";
    event.dataType.push_back(uriType);
    result = tempPasteboard->EstablishP2PLinkTask(pasteId, event);
    EXPECT_EQ(result, nullptr);
    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = 123;
    p2pInfo.isSuccess = false;
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(pasteId, p2pInfo);
    tempPasteboard->p2pMap_.Insert(event.deviceId, p2pMap);
    std::shared_ptr<BlockObject<int32_t>> block = std::make_shared<BlockObject<int32_t>>(2000, 0);
    EXPECT_NE(block, nullptr);
    tempPasteboard->preSyncP2pMap_.insert(std::make_pair(event.deviceId, block));
    result = tempPasteboard->EstablishP2PLinkTask(pasteId, event);
    EXPECT_NE(result, nullptr);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: OpenP2PLinkForPreEstablishTest001
 * @tc.desc: OpenP2PLinkForPreEstablish
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, OpenP2PLinkForPreEstablishTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId = "TestNetworkId";
    auto clipPlugin = std::make_shared<DefaultClip>();
    EXPECT_NE(clipPlugin, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetRemoteDeviceInfo(testing::_, testing::_))
        .WillOnce(Return(static_cast<int32_t>(PasteboardError::OTHER_ERROR)));
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    EXPECT_NE(tempPasteboard->ffrtTimer_, nullptr);
    auto result = tempPasteboard->OpenP2PLinkForPreEstablish(networkId, clipPlugin.get());
    EXPECT_EQ(result, false);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: PreEstablishP2PLinkTest001
 * @tc.desc: PreEstablishP2PLink
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, PreEstablishP2PLinkTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    EXPECT_NE(tempPasteboard->ffrtTimer_, nullptr);
    tempPasteboard->p2pEstablishInfo_.pasteBlock = std::make_shared<BlockObject<int32_t>>(2000, 0);
    EXPECT_NE(tempPasteboard->p2pEstablishInfo_.pasteBlock, nullptr);
    std::string networkId = "TestNetworkId";
    tempPasteboard->p2pEstablishInfo_.networkId = networkId;
    auto clipPlugin = std::make_shared<DefaultClip>();
    EXPECT_NE(clipPlugin, nullptr);
    tempPasteboard->PreEstablishP2PLink(networkId, clipPlugin.get());
    tempPasteboard->p2pEstablishInfo_.pasteBlock = nullptr;
    std::string p2pPresyncId = "P2pPreSyncId_";
    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = 123;
    p2pInfo.isSuccess = false;
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(p2pPresyncId, p2pInfo);
    tempPasteboard->p2pMap_.Insert(networkId, p2pMap);
    tempPasteboard->PreEstablishP2PLink(networkId, clipPlugin.get());
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: AddPreSyncP2pTimeoutTaskTest001
 * @tc.desc: AddPreSyncP2pTimeoutTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, AddPreSyncP2pTimeoutTaskTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId = "TestNetworkId";
    tempPasteboard->AddPreSyncP2pTimeoutTask(networkId);
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    EXPECT_NE(tempPasteboard->ffrtTimer_, nullptr);
    tempPasteboard->AddPreSyncP2pTimeoutTask(networkId);
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: CheckAndGrantRemoteUriTest001
 * @tc.desc: CheckAndGrantRemoteUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, CheckAndGrantRemoteUriTest001, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData data;
    data.SetRemote(true);
    AppInfo appInfo;
    std::string pasteId = "TestPasteId";
    std::string networkId = "TestNetworkId";
    std::shared_ptr<BlockObject<int32_t>> block = std::make_shared<BlockObject<int32_t>>(2000, 0);
    EXPECT_NE(block, nullptr);
    int32_t result = tempPasteboard->CheckAndGrantRemoteUri(data, appInfo, pasteId, networkId, block);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
#else
    ASSERT_TRUE(true);
#endif
}

/**
 * @tc.name: ResubscribeObserver001
 * @tc.desc: ResubscribeObserver001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ResubscribeObserver001, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto type = PasteboardObserverType::OBSERVER_LOCAL;
    const sptr<IPasteboardChangedObserver> observer = sptr<MyTestPasteboardChangedObserver>::MakeSptr();

    int32_t res = tempPasteboard->ResubscribeObserver(type, observer);
    EXPECT_EQ(res, ERR_OK);
}

/**
 * @tc.name: ResubscribeObserver002
 * @tc.desc: ResubscribeObserver002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceMockTest, ResubscribeObserver002, TestSize.Level0)
{
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto type = PasteboardObserverType::OBSERVER_LOCAL;
    const sptr<IPasteboardChangedObserver> observer = sptr<MyTestPasteboardChangedObserver>::MakeSptr();

    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillRepeatedly(Return(ATokenTypeEnum::TOKEN_HAP));
    int32_t res = tempPasteboard->ResubscribeObserver(type, observer);
    EXPECT_EQ(res, ERR_OK);
}

class EntryGetterImpl : public IPasteboardEntryGetter {
public:
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &entry) override
    {
        (void)recordId;
        (void)entry;
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

/**
 * @tc.name: SyncDelayedData001
 * @tc.desc: should return NO_DATA when not set paste data
 *           should return INVALID_TOKEN_ID when paste data tokenId mismatch
 *           should return NO_DELAY_GETTER when entry getter not find
 *           else should return ERR_OK and remove empty entry
 */
HWTEST_F(PasteboardServiceMockTest, SyncDelayedData001, TestSize.Level0)
{
    g_accountIds = true;
    uint32_t tokenId = 1;
    int32_t userId = ACCOUNT_IDS_RANDOM;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, GetTokenTypeFlag).WillRepeatedly(Return(ATokenTypeEnum::TOKEN_NATIVE));
    EXPECT_CALL(mock, GetCallingTokenID).WillRepeatedly(Return(tokenId));
    EXPECT_CALL(mock, QueryActiveOsAccountIds).WillRepeatedly(Return(ERR_OK));

    auto pbs = std::make_shared<PasteboardService>();
    int32_t ret = pbs->SyncDelayedData();
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));

    auto setData = std::make_shared<PasteData>();
    auto record = std::make_shared<PasteDataRecord>();
    auto entryText = std::make_shared<PasteDataEntry>(UTDID_PLAIN_TEXT, "plain text");
    auto entryPixelMap = std::make_shared<PasteDataEntry>();
    entryPixelMap->SetUtdId(UTDID_PIXEL_MAP);
    record->SetDelayRecordFlag(true);
    record->AddEntry(UTDID_PLAIN_TEXT, entryText);
    record->AddEntry(UTDID_PIXEL_MAP, entryPixelMap);
    setData->AddRecord(record);
    setData->SetTokenId(tokenId + 1);
    pbs->clips_.InsertOrAssign(userId, setData);
    ret = pbs->SyncDelayedData();
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_TOKEN_ID));

    setData->SetTokenId(tokenId);
    ret = pbs->SyncDelayedData();
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER));

    sptr<IPasteboardEntryGetter> entryGetter = sptr<EntryGetterImpl>::MakeSptr();
    pbs->entryGetters_.InsertOrAssign(userId, std::make_pair(entryGetter, nullptr));
    ret = pbs->SyncDelayedData();
    EXPECT_EQ(ret, ERR_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_FALSE(record->HasEmptyEntry());
    EXPECT_EQ(setData->GetMimeTypes().size(), 1);
    EXPECT_STREQ(setData->GetMimeTypes()[0].c_str(), MIMETYPE_TEXT_PLAIN);
}
}
} // namespace OHOS::MiscServices