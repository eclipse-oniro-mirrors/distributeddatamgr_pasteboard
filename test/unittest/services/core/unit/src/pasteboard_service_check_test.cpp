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

class PasteboardServiceCheckTest : public testing::Test {
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

void PasteboardServiceCheckTest::SetUpTestCase(void) { }

void PasteboardServiceCheckTest::TearDownTestCase(void) { }

void PasteboardServiceCheckTest::SetUp(void) { }

void PasteboardServiceCheckTest::TearDown(void) { }

int32_t PasteboardServiceCheckTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: IsDataAgedTest001
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataAgedTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    bool ret = tempPasteboard->IsDataAged();
    EXPECT_EQ(ret, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest001 end");
}

/**
 * @tc.name: IsDataAgedTest002
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataAgedTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto copyTime = curTime;
    tempPasteboard->copyTime_.InsertOrAssign(userId, copyTime);

    bool ret = tempPasteboard->IsDataAged();
    EXPECT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest002 end");
}

/**
 * @tc.name: IsDataAgedTest003
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataAgedTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto copyTime = curTime - ONE_HOUR_MILLISECONDS;
    tempPasteboard->copyTime_.InsertOrAssign(userId, copyTime);

    bool ret = tempPasteboard->IsDataAged();
    EXPECT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest003 end");
}

/**
 * @tc.name: IsDataAgedTest004
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataAgedTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    auto copyTime = curTime - 2;
    tempPasteboard->agedTime_ = 1;
    tempPasteboard->copyTime_.InsertOrAssign(userId, copyTime);

    bool ret = tempPasteboard->IsDataAged();
    EXPECT_EQ(ret, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest004 end");
}

/**
 * @tc.name: IsDataAgedTest005
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataAgedTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest005 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto copyTime = curTime + ONE_HOUR_MILLISECONDS;
    tempPasteboard->copyTime_.InsertOrAssign(userId, copyTime);

    bool ret = tempPasteboard->IsDataAged();
    EXPECT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest005 end");
}

/**
 * @tc.name: IsDataAgedTest006
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataAgedTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest006 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    auto copyTime = curTime - 2;
    tempPasteboard->copyTime_.InsertOrAssign(userId, copyTime);

    PasteData pasteData;
    tempPasteboard->agedTime_ = 1;
    tempPasteboard->clips_.InsertOrAssign(userId, std::make_shared<PasteData>(pasteData));

    bool ret = tempPasteboard->IsDataAged();
    EXPECT_EQ(ret, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest006 end");
}

/**
 * @tc.name: IsDataValidTest001
 * @tc.desc: test Func IsDataValid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataValidTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    uint32_t tokenId = 0x123456;
    int32_t ret = tempPasteboard->IsDataValid(pasteData, tokenId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest001 end");
}

/**
 * @tc.name: IsDataValidTest002
 * @tc.desc: test Func IsDataValid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataValidTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);
    ScreenEvent screenEvent = ScreenEvent::ScreenUnlocked;
    pasteData.SetScreenStatus(screenEvent);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    uint32_t tokenId = 0x123456;
    ret = tempPasteboard->IsDataValid(pasteData, tokenId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::CROSS_BORDER_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest002 end");
}

/**
 * @tc.name: IsDataValidTest003
 * @tc.desc: test Func IsDataValid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDataValidTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    appInfo.userId = tempPasteboard->GetCurrentAccountId();
    PasteData pasteData;
    std::string plainText = "hello";
    pasteData.AddTextRecord(plainText);

    ShareOption shareOption = ShareOption::InApp;
    pasteData.SetShareOption(shareOption);

    uint32_t tokenId = 0x123456;
    pasteData.SetTokenId(tokenId);

    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;

    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    ret = tempPasteboard->SetPasteData(dup(fd), tlvSize, pasteDataTlv, delayGetter, entryGetter);

    ret = tempPasteboard->IsDataValid(pasteData, tokenId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest003 end");
}

/**
 * @tc.name: IsFocusedAppTest001
 * @tc.desc: test Func IsFocusedApp
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsFocusedAppTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsFocusedAppTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = UINT32_ONE;
    tempPasteboard->IsFocusedApp(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsFocusedAppTest001 end");
}

/**
 * @tc.name: IsSystemAppByFullTokenIDTest001
 * @tc.desc: test Func IsSystemAppByFullTokenID
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsSystemAppByFullTokenIDTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = UINT32_ONE;
    tempPasteboard->IsSystemAppByFullTokenID(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest001 end");
}

/**
 * @tc.name: IsCopyableTest001
 * @tc.desc: test Func IsCopyable
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsCopyableTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCopyableTest001 start");
    auto tokenId = 123;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->IsCopyable(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCopyableTest001 end");
}

/**
 * @tc.name: IsBundleOwnUriPermission001
 * @tc.desc: test Func IsBundleOwnUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsBundleOwnUriPermission001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBundleOwnUriPermission001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string bundleName = "bundleName";
    std::shared_ptr<Uri> uri = std::make_shared<Uri>("text/html");
    tempPasteboard->IsBundleOwnUriPermission(bundleName, *uri);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBundleOwnUriPermission001 end");
}

/**
 * @tc.name: IsDisallowDistributedTest
 * @tc.desc: test Func IsDisallowDistributed, Check CallingUID contral collaboration.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsDisallowDistributedTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDisallowDistributedTest start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);
    EXPECT_EQ(tempPasteboard->IsDisallowDistributed(), false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDisallowDistributedTest end");
}

/**
 * @tc.name: IsConstraintEnabled001
 * @tc.desc: test Func IsConstraintEnabled
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsConstraintEnabled001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsConstraintEnabled001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 100;
    tempPasteboard->IsConstraintEnable(userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsConstraintEnabled001 end");
}

/**
 * @tc.name: IsBasicTypeTest001
 * @tc.desc: test Func IsBasicType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsBasicTypeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    bool ret = tempPasteboard->IsBasicType(MIMETYPE_TEXT_HTML);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest001 end");
}

/**
 * @tc.name: IsBasicTypeTest002
 * @tc.desc: test Func IsBasicType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsBasicTypeTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userld = 1;
    PasteData data;
    tempPasteboard->GetDelayPasteData(userld, data);

    bool ret = tempPasteboard->IsBasicType(MIMETYPE_TEXT_PLAIN);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest002 end");
}

/**
 * @tc.name: IsBasicTypeTest003
 * @tc.desc: test Func IsBasicType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsBasicTypeTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    bool ret = tempPasteboard->IsBasicType(MIMETYPE_TEXT_URI);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest003 end");
}

/**
 * @tc.name: IsBasicTypeTest004
 * @tc.desc: test Func IsBasicType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsBasicTypeTest004, TestSize.Level0)
{PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    
    bool ret = tempPasteboard->IsBasicType("application/octet-stream");
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest004 end");
}

/**
 * @tc.name: IsBasicTypeTest005
 * @tc.desc: test Func IsBasicType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsBasicTypeTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest005 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string mimeType = "text/html";
    tempPasteboard->IsBasicType(mimeType);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest005 end");
}

/**
 * @tc.name: IsNeedThawTest001
 * @tc.desc: test Func IsNeedThaw
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceCheckTest, IsNeedThawTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsNeedThawTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->IsNeedThaw();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsNeedThawTest001 end");
}


} // namespace MiscServices
} // namespace OHOS