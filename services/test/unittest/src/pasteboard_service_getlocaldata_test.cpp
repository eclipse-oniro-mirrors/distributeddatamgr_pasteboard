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
#include <thread>
#include <unistd.h>

#include "ipc_skeleton.h"
#include "message_parcel_warp.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_observer_stub.h"
#include "pasteboard_service.h"
#include "pasteboard_time.h"
#include "paste_data_entry.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;
using namespace std::chrono;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace {
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
} // namespace

class PasteboardServiceGetLocalDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
        int64_t &tlvSize, MessageParcelWarp &messageData, MessageParcel &parcelPata);
};

void PasteboardServiceGetLocalDataTest::SetUpTestCase(void) { }

void PasteboardServiceGetLocalDataTest::TearDownTestCase(void) { }

void PasteboardServiceGetLocalDataTest::SetUp(void) { }

void PasteboardServiceGetLocalDataTest::TearDown(void) { }

int32_t PasteboardServiceGetLocalDataTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
    int64_t &tlvSize, MessageParcelWarp &messageData, MessageParcel &parcelPata)
{
    std::vector<uint8_t> pasteDataTlv(0);
    bool result = pasteData.Encode(pasteDataTlv);
    if (!result) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "paste data encode failed.");
        return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
    }
    tlvSize = static_cast<int64_t>(pasteDataTlv.size());
    if (tlvSize > MIN_ASHMEM_DATA_SIZE) {
        if (!messageData.WriteRawData(parcelPata, pasteDataTlv.data(), pasteDataTlv.size())) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to WriteRawData");
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
        fd = messageData.GetWriteDataFd();
        pasteDataTlv.clear();
    } else {
        fd = messageData.CreateTmpFd();
        if (fd < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to create tmp fd");
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
    }
    buffer = std::move(pasteDataTlv);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "set: fd:%{public}d, size:%{public}" PRId64, fd, tlvSize);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

namespace MiscServices {
/**
 * @tc.name: GetLocalDataTest001
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    PasteData pasteData;
    int32_t ret = tempPasteboard->GetLocalData(appInfo, pasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest001 end");
}

/**
 * @tc.name: GetLocalDataTest002
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest002 end");
}

/**
 * @tc.name: GetLocalDataTest003
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest003 end");
}

/**
 * @tc.name: GetLocalDataTest004
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest004 end");
}

/**
 * @tc.name: GetLocalDataTest005
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest005 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest005 end");
}

/**
 * @tc.name: GetLocalDataTest006
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest006 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest006 end");
}

/**
 * @tc.name: GetLocalDataTest007
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest007 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest007 end");
}

/**
 * @tc.name: GetLocalDataTest008
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest008 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest008 end");
}

/**
 * @tc.name: GetLocalDataTest009
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest009 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest009 end");
}

/**
 * @tc.name: GetLocalDataTest010
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest010 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest010 end");
}

/**
 * @tc.name: GetLocalDataTest011
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest011, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest011 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest011 end");
}

/**
 * @tc.name: GetLocalDataTest012
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest012, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest012 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetInvalid();

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest012 end");
}

/**
 * @tc.name: GetLocalDataTest013
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest013, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest013 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetInvalid();

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest013 end");
}

/**
 * @tc.name: GetLocalDataTest014
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest014, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest014 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetInvalid();

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest014 end");
}

/**
 * @tc.name: GetLocalDataTest015
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest015, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest015 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(true);
    pasteData.SetInvalid();

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest015 end");
}

/**
 * @tc.name: GetLocalDataTest016
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest016, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest016 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(true);
    pasteData.SetInvalid();

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest016 end");
}

/**
 * @tc.name: GetLocalDataTest017
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest017, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest017 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(true);
    pasteData.SetInvalid();

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest017 end");
}

/**
 * @tc.name: GetLocalDataTest018
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest018, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest018 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest018 end");
}

/**
 * @tc.name: GetLocalDataTest019
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest019, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest019 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest019 end");
}

/**
 * @tc.name: GetLocalDataTest020
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest020, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest020 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest020 end");
}

/**
 * @tc.name: GetLocalDataTest0021
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0021, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest021 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest021 end");
}

/**
 * @tc.name: GetLocalDataTest0022
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0022, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest022 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest022 end");
}

/**
 * @tc.name: GetLocalDataTest0023
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0023, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest023 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest023 end");
}

/**
 * @tc.name: GetLocalDataTest0024
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0024, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest024 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);
    pasteData.SetDelayRecord(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest024 end");
}

/**
 * @tc.name: GetLocalDataTest0025
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0025, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest025 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);
    pasteData.SetDelayRecord(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest025 end");
}

/**
 * @tc.name: GetLocalDataTest0026
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0026, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest026 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);
    pasteData.SetDelayRecord(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest026 end");
}

/**
 * @tc.name: GetLocalDataTest0027
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0027, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest027 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);
    pasteData.SetDelayRecord(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest027 end");
}

/**
 * @tc.name: GetLocalDataTest0028
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0028, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest028 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);
    pasteData.SetDelayRecord(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest028 end");
}

/**
 * @tc.name: GetLocalDataTest0029
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0029, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest029 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(false);
    pasteData.SetDelayRecord(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest029 end");
}

/**
 * @tc.name: GetLocalDataTest0030
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0030, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest030 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest030 end");
}

/**
 * @tc.name: GetLocalDataTest0031
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0031, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest031 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest031 end");
}

/**
 * @tc.name: GetLocalDataTest0032
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0032, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest032 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(false);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest032 end");
}

/**
 * @tc.name: GetLocalDataTest0033
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0033, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest033 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest033 end");
}

/**
 * @tc.name: GetLocalDataTest0034
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0034, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest034 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::LocalDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest034 end");
}

/**
 * @tc.name: GetLocalDataTest0035
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0035, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest035 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::CrossDevice;
    pasteData.SetShareOption(shareOption);
    pasteData.SetDraggedDataFlag(false);
    pasteData.SetDelayData(true);
    pasteData.SetDelayRecord(true);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest035 end");
}

/**
 * @tc.name: GetLocalDataTest0036
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest0036, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest036 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->copyTime_.Clear();
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest036 end");
}

/**
 * @tc.name: GetLocalDataTest037
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest037, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest037 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->clips_.Clear();
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest037 end");
}

/**
 * @tc.name: GetLocalDataTest038
 * @tc.desc: test Func GetLocalData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetLocalDataTest, GetLocalDataTest038, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest038 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    PasteData getPasteData;
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->copyTime_.Clear();
    tempPasteboard->clips_.Clear();
    ret = tempPasteboard->GetLocalData(appInfo, getPasteData);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalDataTest038 end");
}
} // namespace MiscServices
} // namespace OHOS