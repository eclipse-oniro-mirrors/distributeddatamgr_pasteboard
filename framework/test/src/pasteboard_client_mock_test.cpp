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
#include <thread>

#include "entry_getter.h"
#include "errors.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service.h"
#include "pasteboard_service_loader.h"
#include "unistd.h"
#include "tlv_writeable.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace OHOS {
constexpr uint8_t TEST_DATA = 0x02;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024; // 32K
constexpr int32_t DEFAULT_MAX_RAW_DATA_SIZE = 128 * 1024 * 1024; // 128M
class PasteboardClientInterface {
public:
    PasteboardClientInterface() {};
    virtual ~PasteboardClientInterface() {};

    virtual bool Encode(std::vector<uint8_t> &buffer) = 0;
    virtual bool Decode(const std::vector<std::uint8_t> &buffer) = 0;
    virtual bool WriteInt64(int64_t value) = 0;
    virtual bool WriteUnpadBuffer(const void *data, size_t size) = 0;
};
class PasteboardClientMock : public PasteboardClientInterface {
public:
    PasteboardClientMock();
    ~PasteboardClientMock() override;

    MOCK_METHOD1(Encode, bool(std::vector<uint8_t> &buffer));
    MOCK_METHOD1(Decode, bool(const std::vector<std::uint8_t> &buffer));
    MOCK_METHOD1(WriteInt64, bool(int64_t value));
    MOCK_METHOD2(WriteUnpadBuffer, bool(const void *data, size_t size));
};
static void *g_interface = nullptr;

PasteboardClientMock::PasteboardClientMock()
{
    g_interface = reinterpret_cast<void *>(this);
}

PasteboardClientMock::~PasteboardClientMock()
{
    g_interface = nullptr;
}

static PasteboardClientInterface *GetPasteboardClientInterface()
{
    return reinterpret_cast<PasteboardClientInterface *>(g_interface);
}

bool TLVWriteable::Encode(size_t len, std::vector<uint8_t> &buffer, bool isRemote) const
{
    (void)isRemote;
    if (GetPasteboardClientInterface() == nullptr) {
        return false;
    }
    if (g_interface) {
        buffer.push_back(TEST_DATA);
    }
    return GetPasteboardClientInterface()->Encode(buffer);
}
bool TLVReadable::Decode(const std::vector<std::uint8_t> &buffer)
{
    if (GetPasteboardClientInterface() == nullptr) {
        return false;
    }
    return GetPasteboardClientInterface()->Decode(buffer);
}
bool Parcel::WriteInt64(int64_t value)
{
    if (GetPasteboardClientInterface() == nullptr) {
        return false;
    }
    return GetPasteboardClientInterface()->WriteInt64(value);
}
bool Parcel::WriteUnpadBuffer(const void *data, size_t size)
{
    if (GetPasteboardClientInterface() == nullptr) {
        return false;
    }
    return GetPasteboardClientInterface()->WriteUnpadBuffer(data, size);
}

class UDMFEntryGetterImpl : public UDMF::EntryGetter {
public:
    UDMF::ValueType GetValueByType(const std::string &utdId) override;
};

UDMF::ValueType UDMFEntryGetterImpl::GetValueByType(const std::string &utdId)
{
    return nullptr;
}

namespace MiscServices {
class PasteboardClientMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};
    
void PasteboardClientMockTest::SetUpTestCase(void)
{
}

void PasteboardClientMockTest::TearDownTestCase(void)
{
}

void PasteboardClientMockTest::SetUp(void) { }

void PasteboardClientMockTest::TearDown(void) { }

/**
 * @tc.name: WritePasteDataTest001
 * @tc.desc: WritePasteDataTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, WritePasteDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- WritePasteDataTest001  enter-----");
    PasteData pasteData;
    int64_t tlvSize = 1;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    std::vector<uint8_t> buffer = {0x01, 0x02, 0x03};
    int fd = 3;
    NiceMock<PasteboardClientMock> mock;

    EXPECT_CALL(mock, Encode).WillRepeatedly(testing::Return(false));
    auto result = PasteboardClient::GetInstance()->WritePasteData(
        pasteData, buffer, fd, tlvSize, messageData, parcelPata);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- WritePasteDataTest001  end-----");
}

/**
 * @tc.name: WritePasteDataTest002
 * @tc.desc: WritePasteDataTest002
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, WritePasteDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- WritePasteDataTest002  enter-----");
    PasteData pasteData;
    int64_t tlvSize = MIN_ASHMEM_DATA_SIZE + 1;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    std::vector<uint8_t> buffer = {0x01, 0x02, 0x03};
    int fd = 3;
    NiceMock<PasteboardClientMock> mock;

    EXPECT_CALL(mock, Encode).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mock, WriteInt64).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mock, WriteUnpadBuffer).WillRepeatedly(testing::Return(true));
    auto result = PasteboardClient::GetInstance()->WritePasteData(
        pasteData, buffer, fd, tlvSize, messageData, parcelPata);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- WritePasteDataTest002  end-----");
}

/**
 * @tc.name: GetProgressByProgressInfoTest001
 * @tc.desc: GetProgressByProgressInfoTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, GetProgressByProgressInfoTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetProgressByProgressInfoTest001  enter-----");
    auto params = std::make_shared<GetDataParams>();
    EXPECT_NE(params, nullptr);
    params->info = nullptr;

    PasteboardClient::GetInstance()->GetProgressByProgressInfo(params);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetProgressByProgressInfoTest001  end-----");
}

/**
 * @tc.name: ConvertErrCode001
 * @tc.desc: ConvertErrCode001 enter ERR_INVALID_VALUE and return SERIALIZATION_ERROR
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, ConvertErrCode001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode001  enter-----");
    int32_t code = PasteboardClient::GetInstance()->ConvertErrCode(ERR_INVALID_VALUE);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR), code);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode001  end-----");
}

/**
 * @tc.name: ConvertErrCode002
 * @tc.desc: ConvertErrCode002 enter ERR_INVALID_DATA and return SERIALIZATION_ERROR
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, ConvertErrCode002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode002  enter-----");
    int32_t code = PasteboardClient::GetInstance()->ConvertErrCode(ERR_INVALID_DATA);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR), code);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode002  end-----");
}

/**
 * @tc.name: ConvertErrCode003
 * @tc.desc: ConvertErrCode003 enter ERR_OK and return E_OK
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, ConvertErrCode003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode003  enter-----");
    int32_t code = PasteboardClient::GetInstance()->ConvertErrCode(ERR_OK);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::E_OK), code);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode003  end-----");
}

/**
 * @tc.name: ConvertErrCode004
 * @tc.desc: ConvertErrCode004 enter errCode and return errCode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, ConvertErrCode004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode004  enter-----");
    int32_t code = PasteboardClient::GetInstance()->ConvertErrCode(ERR_PERMISSION_DENIED);
    EXPECT_EQ(static_cast<int32_t>(ERR_PERMISSION_DENIED), code);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ConvertErrCode004  end-----");
}

/**
 * @tc.name: RemoveAppShareOptions001
 * @tc.desc: RemoveAppShareOptions001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, RemoveAppShareOptions001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- RemoveAppShareOptions001  enter-----");
    int32_t ret = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- RemoveAppShareOptions001  end-----");
}

/**
 * @tc.name: CreateGetterAgent001
 * @tc.desc: CreateGetterAgent001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */

HWTEST_F(PasteboardClientMockTest, CreateGetterAgent001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- CreateGetterAgent001  enter-----");
    sptr<PasteboardDelayGetterClient> delayGetterAgent;
    std::shared_ptr<PasteboardDelayGetter> delayGetter = nullptr;
    sptr<PasteboardEntryGetterClient> entryGetterAgent = nullptr;
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters;
    PasteData pasteData;

    auto udmfEntryGetterImpl = std::make_shared<UDMFEntryGetterImpl>();
    EXPECT_NE(udmfEntryGetterImpl, nullptr);
    entryGetters.emplace(1, udmfEntryGetterImpl);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(true);
    PasteboardClient::GetInstance()->CreateGetterAgent(delayGetterAgent,
        delayGetter, entryGetterAgent, entryGetters, pasteData);
    entryGetters.clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- CreateGetterAgent001  end-----");
}

/**
 * @tc.name: ProcessRadarReport001
 * @tc.desc: ProcessRadarReport001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, ProcessRadarReport001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ProcessRadarReport001  enter-----");
    int32_t ret;
    PasteData pasteData;
    PasteDataFromServiceInfo pasteDataFromServiceInfo;
    int32_t syncTime;
    std::string pasteDataInfoSummary = "";

    syncTime = 0;
    ret = static_cast<int32_t>(PasteboardError::E_OK);
    pasteData.deviceId_ = "";
    PasteboardClient::GetInstance()->ProcessRadarReport(
        ret, pasteData, pasteDataFromServiceInfo, syncTime);
    EXPECT_EQ(pasteData.deviceId_.length(), 0);
    pasteData.deviceId_ = "deviceId_";
    PasteboardClient::GetInstance()->ProcessRadarReport(
        ret, pasteData, pasteDataFromServiceInfo, syncTime);

    ret = static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    PasteboardClient::GetInstance()->ProcessRadarReport(
        ret, pasteData, pasteDataFromServiceInfo, syncTime);

    ret = static_cast<int32_t>(PasteboardError::TASK_PROCESSING);
    PasteboardClient::GetInstance()->ProcessRadarReport(
        ret, pasteData, pasteDataFromServiceInfo, syncTime);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ProcessRadarReport001  end-----");
}

/**
 * @tc.name: ProcessPasteData001
 * @tc.desc: ProcessPasteData001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, ProcessPasteData001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ProcessPasteData001  enter-----");
    PasteData pasteData;
    int64_t rawDataSize = 0;
    int fd = -1;
    const std::vector<uint8_t> recvTLV = {};
    int ret = -1;

    // fd < 0
    ret = PasteboardClient::GetInstance()->ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR), ret);

    // rawDataSize =< 0
    fd = 4;
    ret = PasteboardClient::GetInstance()->ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), ret);

    // rawDataSize > DEFAULT_MAX_RAW_DATA_SIZE
    rawDataSize = DEFAULT_MAX_RAW_DATA_SIZE * 2;
    ret = PasteboardClient::GetInstance()->ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), ret);

    // rawDataSize > MIN_ASHMEM_DATA_SIZE
    rawDataSize = MIN_ASHMEM_DATA_SIZE * 2;
    ret = PasteboardClient::GetInstance()->ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR), ret);

    // rawDataSize else
    rawDataSize = MIN_ASHMEM_DATA_SIZE / 2;
    ret = PasteboardClient::GetInstance()->ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- ProcessPasteData001  end-----");
}

HWTEST_F(PasteboardClientMockTest, GetDataReportTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest001  enter-----");
    PasteData pasteData;
    pasteData.deviceId_ = "";
    const int32_t syncTime = 0;
    const std::string currentId = "test_111";
    const std::string currentPid = "test.app";
    PasteboardClient::GetInstance()->GetDataReport(pasteData, syncTime, currentId, currentPid,
        static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_EQ(pasteData.deviceId_.length(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest001  end-----");
}

HWTEST_F(PasteboardClientMockTest, GetDataReportTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest002  enter-----");
    PasteData pasteData;
    const int32_t syncTime = 100;
    pasteData.deviceId_ = "test_222";
    const std::string currentPid = "test.app";
    PasteboardClient::GetInstance()->GetDataReport(
        pasteData, syncTime, "error_222", currentPid, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_NE(pasteData.deviceId_.length(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest002  end-----");
}

HWTEST_F(PasteboardClientMockTest, GetDataReportTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest003  enter-----");
    PasteData pasteData;
    const int32_t syncTime = 0;
    const std::string currentId = "test_333";
    const std::string currentPid = "test.app";
    PasteboardClient::GetInstance()->GetDataReport(pasteData, syncTime, currentId, currentPid,
        static_cast<int32_t>(PasteboardError::TASK_PROCESSING));
    EXPECT_EQ(pasteData.deviceId_.length(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest003  end-----");
}

HWTEST_F(PasteboardClientMockTest, GetDataReportTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest004  enter-----");
    PasteData pasteData;
    const int32_t syncTime = 0;
    const std::string currentId = "test_444";
    const std::string currentPid = "test.app";
    PasteboardClient::GetInstance()->GetDataReport(pasteData, syncTime, currentId, currentPid,
        static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR));
    EXPECT_EQ(pasteData.deviceId_.length(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "----- GetDataReportTest004  end-----");
}
}
} // namespace OHOS::MiscServices
