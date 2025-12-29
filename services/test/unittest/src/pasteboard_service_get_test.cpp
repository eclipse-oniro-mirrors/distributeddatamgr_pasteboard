/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
const int INT_ONE = 1;
const int32_t INT32_NEGATIVE_NUMBER = -1;
constexpr int32_t SET_VALUE_SUCCESS = 1;
const int INT_THREETHREETHREE = 333;
const uint32_t MAX_RECOGNITION_LENGTH = 1000;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
constexpr uint32_t EVENT_TIME_OUT = 2000;
const int32_t ACCOUNT_IDS_RANDOM = 1121;
const uint32_t UINT32_ONE = 1;
const std::string TEST_ENTITY_TEXT =
    "清晨，从杭州市中心出发，沿着湖滨路缓缓前行。湖滨路是杭州市中心通往西湖的主要街道之一，两旁绿树成荫，湖光山色尽收眼"
    "底。你可以选择步行或骑行，感受微风拂面的惬意。湖滨路的尽头是南山路，这里有一片开阔的广场，是欣赏西湖全景的绝佳位置"
    "。进入南山路后，继续前行，雷峰塔的轮廓会逐渐映入眼帘。雷峰塔是西湖的标志性建筑之一，矗立在南屏山下，与西湖相映成趣"
    "。你可以在这里稍作停留，欣赏塔的雄伟与湖水的柔美。南山路两旁有许多咖啡馆和餐厅，是补充能量的好去处。离开雷峰塔，沿"
    "着南山路继续前行，你会看到一条蜿蜒的堤岸——杨公堤。杨公堤是西湖十景之一，堤岸两旁种满了柳树和桃树，春夏之交，柳绿桃"
    "红，美不胜收。你可以选择沿着堤岸漫步，感受湖水的宁静与柳树的轻柔。杨公堤的尽头是湖心亭，这里是西湖的中心地带，也是"
    "观赏西湖全景的最佳位置之一。从湖心亭出发，沿着湖畔步行至北山街。北山街是西湖北部的一条主要街道，两旁有许多历史建筑"
    "和文化遗址。继续前行，你会看到保俶塔矗立在宝石流霞景区。保俶塔是西湖的另一座标志性建筑，与雷峰塔遥相呼应，形成“一"
    "南一北”的独特景观。离开保俶塔，沿着北山街继续前行，你会到达断桥。断桥是西湖十景之一，冬季可欣赏断桥残雪的美景。断"
    "桥的两旁种满了柳树，湖水清澈见底，是拍照留念的好地方。断桥的尽头是平湖秋月，这里是观赏西湖夜景的绝佳地点，夜晚灯光"
    "亮起时，湖面倒映着月光，美轮美奂。游览结束后，沿着湖畔返回杭州市中心。沿途可以再次欣赏西湖的湖光山色，感受大自然的"
    "和谐与宁静。如果你时间充裕，可以选择在湖畔的咖啡馆稍作休息，回味这一天的旅程。这条路线涵盖了西湖的主要经典景点，从"
    "湖滨路到南山路，再到杨公堤、北山街，最后回到杭州市中心，整个行程大约需要一天时间。沿着这条路线，你可以领略西湖的自"
    "然风光和文化底蕴，感受人间天堂的独特魅力。";
const std::string TEST_ENTITY_TEXT_CN_50 =
    "清晨,从杭州市中心出发，沿着湖滨路缓缓前行。湖滨路是杭州市中心通往西湖的主要街道之一，两旁绿树成荫。";
const std::string TEST_ENTITY_TEXT_CN_50 =
    "清晨,从杭州市中心出发，沿着湖滨路缓缓前行。湖滨路是杭州市中心通往西湖的主要街道之一，两旁绿树成荫。";
const std::string TEST_ENTITY_TEXT_CN_10 =
    "清晨,从杭州市中心出";
const std::string TEST_ENTITY_TEXT_CN_5 =
    "清晨,从杭";
const int64_t DEFAULT_MAX_RAW_DATA_SIZE = 128 * 1024 * 1024;
constexpr int32_t MIMETYPE_MAX_SIZE = 1024;
static constexpr uint64_t ONE_HOUR_MILLISECONDS = 60 * 60 * 1000;
} // namespace

class MyTestEntityRecognitionObserver : public IEntityRecognitionObserver {
    void OnRecognitionEvent(EntityType entityType, std::string &entity)
    {
        return;
    }
    sptr<IRemoteObject> AsObject()
    {
        return nullptr;
    }
};

class MyTestPasteboardChangedObserver : public PasteboardObserverStub {
    void OnPasteboardChanged()
    {
        return;
    }
    void OnPasteboardEvent(std::string bundleName, int32_t status)
    {
        return;
    }
};

class PasteboardEntryGetterImpl : public IPasteboardEntryGetter {
public:
    PasteboardEntryGetterImpl() {};
    ~PasteboardEntryGetterImpl() {};
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &value)
    {
        return 0;
    };
    sptr<IRemoteObject> AsObject()
    {
        return nullptr;
    };
};

class PasteboardDelayGetterImpl : public IPasteboardDelayGetter {
public:
    PasteboardDelayGetterImpl() {};
    ~PasteboardDelayGetterImpl() {};
    void GetPasteData(const std::string &type, PasteData &data) {};
    void GetUnifiedData(const std::string &type, UDMF::UnifiedData &data) {};
    sptr<IRemoteObject> AsObject()
    {
        return nullptr;
    };
};

class RemoteObjectTest : public IRemoteObject {
public:
    explicit RemoteObjectTest(std::u16string descriptor) : IRemoteObject(descriptor) { }
    ~RemoteObjectTest() { }

    int32_t GetObjectRefCount()
    {
        return 0;
    }
    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return 0;
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

class PasteboardServiceGetTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
        int64_t &tlvSize, MessageParcelWarp &messageData, MessageParcel &parcelPata);
    using TestEvent = ClipPlugin::GlobalEvent;
    using TaskContext = PasteboardService::RemoteDataTaskManager::TaskContext;
};

void PasteboardServiceGetTest::SetUpTestCase(void) { }

void PasteboardServiceGetTest::TearDownTestCase(void) { }

void PasteboardServiceGetTest::SetUp(void) { }

void PasteboardServiceGetTest::TearDown(void) { }

int32_t PasteboardServiceGetTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: GetMimeTypesTest001
 * @tc.desc: test Func GetMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetMimeTypesTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentScreenStatus = ScreenEvent::ScreenUnlocked;
    std::vector<std::string> funcResult;
    int32_t result = service->GetMimeTypes(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest001 end");
}

/**
 * @tc.name: GetMimeTypesTest002
 * @tc.desc: test Func GetMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetMimeTypesTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest002 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentUserId_ = ERROR_USERID;
    std::vector<std::string> funcResult;
    int32_t result = service->GetMimeTypes(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest002 end");
}

/**
 * @tc.name: GetMimeTypesTest003
 * @tc.desc: test Func GetMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetMimeTypesTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest003 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentScreenStatus = ScreenEvent::Default;
    std::vector<std::string> funcResult;
    int32_t result = service->GetMimeTypes(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest003 end");
}

/**
 * @tc.name: GetMimeTypesTest004
 * @tc.desc: test Func GetMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetMimeTypesTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest004 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentScreenStatus = ScreenEvent::ScreenLocked;
    std::vector<std::string> funcResult;
    int32_t result = service->GetMimeTypes(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetMimeTypesTest004 end");
}

/**
 * @tc.name: GetDataSourceTest001
 * @tc.desc: test Func GetDataSource
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetDataSourceTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSourceTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string bundlename = "test";
    tempPasteboard->GetDataSource(bundlename);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSourceTest001 end");
}

/**
 * @tc.name: GetDataSourceTest002
 * @tc.desc: test Func GetDataSource
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetDataSourceTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSourceTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string bundleName = "test_bundle_name";
    int32_t userId = 100;
    tempPasteboard->currentUserId_ = userId;
    PasteDataProperty props_;
    auto data = std::make_shared<PasteData>();
    data->props_.isRemote = false;
    data->props_.tokenId = 1;

    tempPasteboard->clips_.Insert(userId, data);
    auto result = tempPasteboard->GetDataSource(bundleName);
    EXPECT_EQ(result, ERR_OK);
    tempPasteboard->clips_.Erase(userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDataSourceTest002 end");
}

/**
 * @tc.name: GetRemoteDeviceNameTest001
 * @tc.desc: test Func GetRemoteDeviceName
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetRemoteDeviceNameTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDeviceNameTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentUserId_ = ERROR_USERID;
    std::string deviceName;
    bool isRemote = false;
    int32_t result = service->GetRemoteDeviceName(deviceName, isRemote);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDeviceNameTest001 end");
}

/**
 * @tc.name: GetRemoteDeviceNameTest002
 * @tc.desc: test Func GetRemoteDeviceName
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetRemoteDeviceNameTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDeviceNameTest002 start");
    PasteboardService service;
    std::string deviceName;
    bool isRemote;

    EXPECT_EQ(service.GetRemoteDeviceName(deviceName, isRemote), ERR_OK);
    EXPECT_EQ(deviceName, "local");
    EXPECT_FALSE(isRemote);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDeviceNameTest002 end");
}

/**
 * @tc.name: GetCurrentAccountIdTest001
 * @tc.desc: test Func GetCurrentAccountId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetCurrentAccountIdTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetCurrentAccountId();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest001 end");
}

/**
 * @tc.name: GetCurrentAccountIdTest002
 * @tc.desc: test Func GetCurrentAccountId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetCurrentAccountIdTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentUserId_ = 1;
    int32_t result = tempPasteboard->GetCurrentAccountId();
    EXPECT_EQ(result, 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest002 end");
}

/**
 * @tc.name: GetCurrentAccountIdTest003
 * @tc.desc: test Func GetCurrentAccountId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetCurrentAccountIdTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentUserId_ = ERROR_USERID;
    int32_t result = tempPasteboard->GetCurrentAccountId();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest003 end");
}

/**
 * @tc.name: GetFocusedAppInfoTest001
 * @tc.desc: test Func GetFocusedAppInfo
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetFocusedAppInfoTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetFocusedAppInfoTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetFocusedAppInfo();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetFocusedAppInfoTest001 end");
}

/**
 * @tc.name: GetCurrentScreenStatusTest001
 * @tc.desc: test Func GetCurrentScreenStatus
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetCurrentScreenStatusTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentScreenStatusTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetCurrentScreenStatus();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentScreenStatusTest001 end");
}

/**
 * @tc.name: GetSdkVersionTest001
 * @tc.desc: test Func GetSdkVersion
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetSdkVersionTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetSdkVersionTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = 0x123456;
    tempPasteboard->GetSdkVersion(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetSdkVersionTest001 end");
}

/**
 * @tc.name: GetSdkVersionTest002
 * @tc.desc: test Func GetSdkVersion
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetTest, GetSdkVersionTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetSdkVersionTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = 123;

    int32_t result = tempPasteboard->GetSdkVersion(tokenId);
    EXPECT_EQ(result, -1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetSdkVersionTest002 end");
}

/**
 * @tc.name: GetCommonStateTest001
 * @tc.desc: test Func GetCommonState
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetCommonStateTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCommonStateTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int64_t dataSize = 0;
    CommonInfo info = tempPasteboard->GetCommonState(dataSize);
    EXPECT_EQ(info.deviceType, DMAdapter::GetInstance().GetLocalDeviceType());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCommonStateTest001 end");
}

/**
 * @tc.name: GetAppLabelTest001
 * @tc.desc: test Func GetAppLabel
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetAppLabelTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppLabelTest001 start");
    uint32_t tokenId = UINT32_ONE;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->GetAppLabel(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppLabelTest001 end");
}

/**
 * @tc.name: GetAppBundleManagerTest001
 * @tc.desc: test Func GetAppBundleManager
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetAppBundleManagerTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleManagerTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard->GetAppBundleManager(), nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleManagerTest001 end");
}

/**
 * @tc.name: GetTimeTest001
 * @tc.desc: test Func GetTime
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetTimeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetTimeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetTime();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetTimeTest001 end");
}

/**
 * @tc.name: GetAppInfoTest001
 * @tc.desc: test Func GetAppInfo
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetAppInfoTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppInfoTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = 0x123456;
    tempPasteboard->GetAppInfo(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppInfoTest001 end");
}

/**
 * @tc.name: GetAppBundleNameTest001
 * @tc.desc: test Func GetAppBundleName
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetAppBundleNameTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    tempPasteboard->GetAppBundleName(appInfo);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest001 end");
}

/**
 * @tc.name: GetAppBundleNameTest002
 * @tc.desc: test Func GetAppBundleName
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetAppBundleNameTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = 0;
    appInfo.bundleName = "test";
    auto ret = tempPasteboard->GetAppBundleName(appInfo);
    EXPECT_EQ(ret, appInfo.bundleName);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest002 end");
}

/**
 * @tc.name: GetAppBundleNameTest003
 * @tc.desc: test Func GetAppBundleName
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetAppBundleNameTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = ERROR_USERID;
    appInfo.bundleName = "test";
    auto ret = tempPasteboard->GetAppBundleName(appInfo);
    EXPECT_EQ(ret, "error");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest003 end");
}

/**
 * @tc.name: GetLocalMimeTypesTest001
 * @tc.desc: test Func GetLocalMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetLocalMimeTypesTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalMimeTypesTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetLocalMimeTypes();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalMimeTypesTest001 end");
}

/**
 * @tc.name: GetLocalMimeTypesTest002
 * @tc.desc: test Func GetLocalMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetLocalMimeTypesTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalMimeTypesTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto result = tempPasteboard->GetLocalMimeTypes();
    std::vector<std::string> emptyVector;
    EXPECT_EQ(result, emptyVector);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalMimeTypesTest002 end");
}

/**
 * @tc.name: GetLocalMimeTypesTest003
 * @tc.desc: test Func GetLocalMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetLocalMimeTypesTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalMimeTypesTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    tempPasteboard->clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, std::make_shared<PasteData>());

    auto result = tempPasteboard->GetLocalMimeTypes();
    std::vector<std::string> emptyVector;
    EXPECT_EQ(result, emptyVector);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalMimeTypesTest003 end");
}

/**
 * @tc.name: GetCommonStateTest002
 * @tc.desc: GetCommonStateTest002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, GetCommonStateTest002, TestSize.Level1)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int64_t dataSize = INT64_MAX;
    CommonInfo info = tempPasteboard->GetCommonState(dataSize);
    EXPECT_EQ(info.dataSize, dataSize);
}

/**
 * @tc.name: HasDataTypeTest001
 * @tc.desc: test Func HasDataType, currentScreenStatus is default.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasDataTypeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    bool funcResult;
    int32_t res = tempPasteboard->HasDataType(MIMETYPE_TEXT_PLAIN, funcResult);
    EXPECT_EQ(res, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest001 end");
}

/**
 * @tc.name: HasDataTypeTest002
 * @tc.desc: test Func HasDataType, currentScreenStatus is ScreenEvent::ScreenUnlocked.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasDataTypeTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentScreenStatus = ScreenEvent::ScreenUnlocked;
    tempPasteboard->currentUserId_ = 1;
    tempPasteboard->clipPlugin_ = nullptr;
    bool funcResult;
    int32_t res = tempPasteboard->HasDataType(MIMETYPE_TEXT_PLAIN, funcResult);
    EXPECT_EQ(res, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest002 end");
}

/**
 * @tc.name: HasDataTypeTest003
 * @tc.desc: test Func HasDataType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetTest, HasDataTypeTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    std::string mimeType = "";
    bool hasType = false;
    auto ret = tempPasteboard->HasDataType(mimeType, hasType);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest003 end");
}

/**
 * @tc.name: HasDataTypeTest004
 * @tc.desc: test Func HasDataType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetTest, HasDataTypeTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    std::string mimeType(MIMETYPE_MAX_SIZE + 1, 'x');
    bool hasType = false;
    auto ret = tempPasteboard->HasDataType(mimeType, hasType);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest004 end");
}

/**
 * @tc.name: HasDataTypeTest005
 * @tc.desc: test Func HasDataType, currentScreenStatus is ScreenEvent::Default.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasDataTypeTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest005 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentScreenStatus = ScreenEvent::Default;
    tempPasteboard->currentUserId_ = 1;
    tempPasteboard->clipPlugin_ = nullptr;
    bool funcResult;
    int32_t res = tempPasteboard->HasDataType(MIMETYPE_TEXT_PLAIN, funcResult);
    EXPECT_EQ(res, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest005 end");
}

/**
 * @tc.name: HasDataTypeTest006
 * @tc.desc: test Func HasDataType, currentScreenStatus is ScreenEvent::ScreenLocked.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasDataTypeTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest006 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentScreenStatus = ScreenEvent::ScreenLocked;
    tempPasteboard->currentUserId_ = 1;
    tempPasteboard->clipPlugin_ = nullptr;
    bool funcResult;
    int32_t res = tempPasteboard->HasDataType(MIMETYPE_TEXT_PLAIN, funcResult);
    EXPECT_EQ(res, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasDataTypeTest006 end");
}

/**
 * @tc.name: HasPasteDataTest001
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentScreenStatus = ScreenEvent::ScreenUnlocked;
    bool flag = false;
    int32_t result = service->HasPasteData(flag);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_EQ(flag, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest001 end");
}

/**
 * @tc.name: HasPasteDataTest002
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasPasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest002 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->clips_.Clear();
    std::shared_ptr<PasteData> dataPtr(nullptr);
    int32_t userId = INT_ONE;
    service->clips_.InsertOrAssign(userId, dataPtr);
    bool flag = false;
    int32_t result = service->HasPasteData(flag);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest002 end");
}

/**
 * @tc.name: HasPasteDataTest003
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasPasteDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest003 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->clips_.Clear();
    std::shared_ptr<PasteData> dataPtr = std::make_shared<PasteData>();
    int32_t userId = INT_ONE;
    service->clips_.InsertOrAssign(userId, dataPtr);
    bool flag = false;
    int32_t result = service->HasPasteData(flag);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest003 end");
}

/**
 * @tc.name: HasPasteDataTest004
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasPasteDataTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest004 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentScreenStatus = ScreenEvent::Default;
    bool flag = false;
    int32_t result = service->HasPasteData(flag);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_EQ(flag, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest004 end");
}

/**
 * @tc.name: HasPasteDataTest005
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasPasteDataTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest005 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentScreenStatus = ScreenEvent::ScreenLocked;
    bool flag = false;
    int32_t result = service->HasPasteData(flag);
    EXPECT_EQ(result, ERR_OK);
    EXPECT_EQ(flag, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasPasteDataTest005 end");
}

/**
 * @tc.name: HasLocalDataTypeTest001
 * @tc.desc: test Func HasLocalDataType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasLocalDataTypeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    std::string mimeType = "text/html";
    tempPasteboard->HasLocalDataType(mimeType);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest001 end");
}

/**
 * @tc.name: HasLocalDataTypeTest002
 * @tc.desc: test Func HasLocalDataType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasLocalDataTypeTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    std::string mimeType = "text/html";
    bool result = tempPasteboard->HasLocalDataType(mimeType);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest002 end");
}

/**
 * @tc.name: HasLocalDataTypeTest003
 * @tc.desc: test Func HasLocalDataType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasLocalDataTypeTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = -1;
    std::string mimeType = "text/html";
    bool result = tempPasteboard->HasLocalDataType(mimeType);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest003 end");
}

/**
 * @tc.name: HasLocalDataTypeTest004
 * @tc.desc: test Func HasLocalDataType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetTest, HasLocalDataTypeTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->AddTextRecord("hello");
    tempPasteboard->clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, pasteData);

    std::string mimeType = "text/plain";
    bool result = tempPasteboard->HasLocalDataType(mimeType);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HasLocalDataTypeTest004 end");
}

} // namespace MiscServices
} // namespace OHOS