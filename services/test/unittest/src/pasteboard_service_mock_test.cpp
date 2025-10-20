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
    std::string expectdStr;
    expectdStr.append("No copy data.").append("\n");
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    auto ret = tempPasteboard->DumpData();
    ASSERT_EQ(ret, expectdStr);
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
    std::string expectdStr;
    expectdStr.append("No copy data.").append("\n");
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    tempPasteboard->clips_.Insert(ACCOUNT_IDS_RANDOM, nullptr);
    NiceMock<PasteboardServiceInterfaceMock> mock;
    EXPECT_CALL(mock, QueryActiveOsAccountIds(testing::_)).WillRepeatedly(Return(ERR_OK));
    auto ret = tempPasteboard->DumpData();
    ASSERT_EQ(ret, expectdStr);
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
    std::string expectdStr;
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
    expectdStr.append("|Owner       :  ")
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
    ASSERT_EQ(ret, expectdStr);
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
    std::string expectdStr;
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
    expectdStr.append("|Owner       :  ")
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
    ASSERT_EQ(ret, expectdStr);
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
* @tc.name: GetDataSize001
* @tc.desc: GetDataSize001 function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, GetDataSize001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSize001 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    auto pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);
    size_t recordCounts = 1;
    auto tempRecord = std::make_shared<PasteDataRecord>();
    EXPECT_NE(tempRecord, nullptr);
    pasteData->records_.push_back(tempRecord);
    auto ret = tempPasteboard->GetDataSize(*pasteData);
    ASSERT_EQ(ret, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSize001 end.");
}

/**
* @tc.name: GetDataSize002
* @tc.desc: GetDataSize002 function test
* @tc.type: FUNC
*/
HWTEST_F(PasteboardServiceMockTest, GetDataSize002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSize002 start.");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    auto pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);
    auto ret = tempPasteboard->GetDataSize(*pasteData);
    ASSERT_EQ(ret, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSize002 end.");
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
}
} // namespace OHOS::MiscServices