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
    void OnPasteboardEvent(const PasteboardChangedEvent &event)
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

class PasteboardServiceGetDataTest : public testing::Test {
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

void PasteboardServiceGetDataTest::SetUpTestCase(void) { }

void PasteboardServiceGetDataTest::TearDownTestCase(void) { }

void PasteboardServiceGetDataTest::SetUp(void) { }

void PasteboardServiceGetDataTest::TearDown(void) { }

int32_t PasteboardServiceGetDataTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: GetPasteDataTest001
 * @tc.desc: test Func GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetDataTest, GetPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t syncTime = 0;
    int32_t realErrCode = 0;
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> recvTLV;
    std::string pasteId = "GetPasteData_001";
    auto ret = tempPasteboard->GetPasteData(fd, rawDataSize, recvTLV, pasteId, syncTime, realErrCode);
    if (fd >= 0) {
        close(fd);
    }
    ret = realErrCode;
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest001 end");
}

/**
 * @tc.name: GetPasteDataTest002
 * @tc.desc: test Func GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetDataTest, GetPasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t syncTime = 0;
    int32_t realErrCode = 0;
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> recvTLV;
    std::string pasteId = "GetPasteData_test_002";
    auto ret = tempPasteboard->GetPasteData(fd, rawDataSize, recvTLV, pasteId, syncTime, realErrCode);
    if (fd >= 0) {
        close(fd);
    }
    ret = realErrCode;
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataTest002 end");
}

/**
 * @tc.name: GetPasteDataDotTest001
 * @tc.desc: test Func GetPasteDataDot
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetPasteDataDotTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataDotTest001 start");
    PasteData pasteData;
    std::string bundleName;
    int32_t userId = 0;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetPasteDataDot(pasteData, bundleName, userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteDataDotTest001 end");
}

/**
 * @tc.name: GetRemoteDataTask001
 * @tc.desc: test Func GetRemoteDataTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteDataTask001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDataTask001 start");
    std::shared_ptr<PasteboardService::RemoteDataTaskManager> remoteDataTaskManager =
        std::make_shared<PasteboardService::RemoteDataTaskManager>();
    EXPECT_NE(remoteDataTaskManager, nullptr);

    TestEvent event;
    remoteDataTaskManager->GetRemoteDataTask(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDataTask001 end");
}

/**
 * @tc.name: GetRemoteDataTask002
 * @tc.desc: test Func GetRemoteDataTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteDataTask002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDataTask002 start");
    std::shared_ptr<PasteboardService::RemoteDataTaskManager> remoteDataTaskManager =
        std::make_shared<PasteboardService::RemoteDataTaskManager>();
    EXPECT_NE(remoteDataTaskManager, nullptr);

    TestEvent event;
    event.deviceId = "12345";
    event.seqId = 1;

    auto key = event.deviceId + std::to_string(event.seqId);
    auto it = remoteDataTaskManager->dataTasks_.find(key);
    it = remoteDataTaskManager->dataTasks_.emplace(key, std::make_shared<TaskContext>()).first;

    remoteDataTaskManager->GetRemoteDataTask(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteDataTask002 end");
}

/**
 * @tc.name: GetRemoteData001
 * @tc.desc: test Func GetRemoteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteData001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteData001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 0x123456;
    TestEvent event;
    PasteData data;
    int32_t syncTime = 1000;
    auto ret = tempPasteboard->GetRemoteData(userId, event, data, syncTime);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteData001 end");
}

/**
 * @tc.name: GetRemoteData002
 * @tc.desc: test Func GetRemoteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteData002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteData002 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 0x123456;
    TestEvent event;
    PasteData data;
    int32_t syncTime = 1000;

    PasteData pasteData;
    tempPasteboard->clips_.InsertOrAssign(userId, std::make_shared<PasteData>(pasteData));
    auto ret = tempPasteboard->GetRemoteData(userId, event, data, syncTime);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteData002 end");
}

/**
 * @tc.name: GetRemoteData003
 * @tc.desc: test Func GetRemoteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteData003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteData003 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 0x123456;
    TestEvent event;
    PasteData data;
    int32_t syncTime = 1000;
    event.seqId = 1;

    PasteData pasteData;
    tempPasteboard->clips_.InsertOrAssign(userId, std::make_shared<PasteData>(pasteData));
    auto ret = tempPasteboard->GetRemoteData(userId, event, data, syncTime);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteData003 end");
}

/**
 * @tc.name: GetRemotePasteData001
 * @tc.desc: test Func GetRemotePasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemotePasteData001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemotePasteData001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 0x123456;
    TestEvent event;
    PasteData data;
    int32_t syncTime = 1000;
    tempPasteboard->GetRemotePasteData(userId, event, data, syncTime);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemotePasteData001 end");
}

/**
 * @tc.name: GetDelayPasteRecord001
 * @tc.desc: test Func GetDelayPasteRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDelayPasteRecord001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDelayPasteRecord001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 0x123456;
    PasteData data;
    tempPasteboard->GetDelayPasteRecord(userId, data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDelayPasteRecord001 end");
}

/**
 * @tc.name: GetDelayPasteRecord002
 * @tc.desc: GetDelayPasteRecord002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDelayPasteRecord002, TestSize.Level1)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t userId = 0x123456;
    auto record = std::make_shared<PasteDataRecord>();
    ASSERT_NE(record, nullptr);
    record->SetDelayRecordFlag(true);
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    ASSERT_NE(entry, nullptr);
    std::string utdid = "utdid";
    entry->SetUtdId(utdid);
    std::string plainText = "text/plain";
    entry->SetValue(plainText);
    record->AddEntry(utdid, entry);
    PasteData data;
    data.AddRecord(record);
    sptr<IPasteboardEntryGetter> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    ASSERT_NE(entryGetter, nullptr);
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient = new
        OHOS::MiscServices::PasteboardService::EntryGetterDeathRecipient(userId, *tempPasteboard);
    ASSERT_NE(deathRecipient, nullptr);
    tempPasteboard->entryGetters_.InsertOrAssign(userId, std::make_pair(entryGetter, deathRecipient));
    tempPasteboard->GetDelayPasteRecord(userId + 1, data);
    tempPasteboard->GetDelayPasteRecord(userId, data);
    EXPECT_EQ(tempPasteboard->entryGetters_.Size(), 1);
}

/**
 * @tc.name: GetDelayPasteRecord003
 * @tc.desc: GetDelayPasteRecord003
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDelayPasteRecord003, TestSize.Level1)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t userId = 0x123456;
    auto record = std::make_shared<PasteDataRecord>();
    ASSERT_NE(record, nullptr);
    record->SetDelayRecordFlag(false);
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    ASSERT_NE(entry, nullptr);
    std::string utdid = "utdid";
    entry->SetUtdId(utdid);
    std::string plainText = "text/plain";
    entry->SetValue(plainText);
    record->AddEntry(utdid, entry);
    PasteData data;
    data.AddRecord(record);
    sptr<IPasteboardEntryGetter> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    ASSERT_NE(entryGetter, nullptr);
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient = new
        OHOS::MiscServices::PasteboardService::EntryGetterDeathRecipient(userId, *tempPasteboard);
    ASSERT_NE(deathRecipient, nullptr);
    tempPasteboard->entryGetters_.InsertOrAssign(userId, std::make_pair(entryGetter, deathRecipient));
    tempPasteboard->GetDelayPasteRecord(userId, data);
    EXPECT_EQ(tempPasteboard->entryGetters_.Size(), 1);
}

/**
 * @tc.name: GetDelayPasteRecord004
 * @tc.desc: GetDelayPasteRecord004
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDelayPasteRecord004, TestSize.Level1)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t userId = 0x123456;
    auto record = std::make_shared<PasteDataRecord>();
    ASSERT_NE(record, nullptr);
    record->SetDelayRecordFlag(true);
    PasteData data;
    data.AddRecord(record);
    sptr<IPasteboardEntryGetter> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    ASSERT_NE(entryGetter, nullptr);
    sptr<PasteboardService::EntryGetterDeathRecipient> deathRecipient = new
        OHOS::MiscServices::PasteboardService::EntryGetterDeathRecipient(userId, *tempPasteboard);
    ASSERT_NE(deathRecipient, nullptr);
    tempPasteboard->entryGetters_.InsertOrAssign(userId, std::make_pair(entryGetter, deathRecipient));
    tempPasteboard->GetDelayPasteRecord(userId, data);
    EXPECT_EQ(tempPasteboard->entryGetters_.Size(), 1);
}

/**
 * @tc.name: GetDelayPasteData001
 * @tc.desc: test Func GetDelayPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDelayPasteData001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDelayPasteData001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t userId = 0x123456;
    PasteData data;
    tempPasteboard->GetDelayPasteData(userId, data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDelayPasteData001 end");
}

/**
 * @tc.name: GetDistributedDataTest001
 * @tc.desc: test Func GetDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDistributedDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDistributedDataTest001 start");
    ClipPlugin::GlobalEvent event {};
    int32_t user = ACCOUNT_IDS_RANDOM;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->GetDistributedData(event, user);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDistributedDataTest001 end");
}

/**
 * @tc.name: GetDistributedDelayDataTest001
 * @tc.desc: test Func GetDistributedDelayData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDistributedDelayDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDistributedDelayDataTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    TestEvent event;
    std::vector<uint8_t> rawData;
    tempPasteboard->GetDistributedDelayData(event, 0, rawData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDistributedDelayDataTest001 end");
}

/**
 * @tc.name: GetDistributedDelayEntryTest001
 * @tc.desc: test Func GetDistributedDelayEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetDistributedDelayEntryTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDistributedDelayEntryTest001 start");
    ClipPlugin::GlobalEvent evt {};
    uint32_t recordId = UINT32_ONE;
    std::string utdId;
    std::vector<uint8_t> rawData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->GetDistributedDelayEntry(evt, recordId, utdId, rawData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetDistributedDelayEntryTest001 end");
}

/**
 * @tc.name: GetLocalEntryValueTest001
 * @tc.desc: test Func GetLocalEntryValue
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetLocalEntryValueTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalEntryValueTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    PasteData pasteData;
    PasteDataRecord record;
    PasteDataEntry entry;
    tempPasteboard->GetLocalEntryValue(userId, pasteData, record, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalEntryValueTest001 end");
}

/**
 * @tc.name: GetFullDelayPasteDataTest001
 * @tc.desc: test Func GetFullDelayPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetFullDelayPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetFullDelayPasteDataTest001 start");
    int32_t userId = ACCOUNT_IDS_RANDOM;
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetFullDelayPasteData(userId, pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetFullDelayPasteDataTest001 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest002
 * @tc.desc: test Func GetRecordValueByType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRecordValueByTypeTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t dataId = 1;
    uint32_t recordId = 1;
    int64_t rawDataSize = 1024;
    std::vector<uint8_t> buffer;
    int fd = 0;

    int32_t result = tempPasteboard->GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest002 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest001
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest001 start");
    uint32_t dataId = 1;
    uint32_t recordId = 1;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    std::string primaryText = "hello";
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest001 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest002
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest002 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    tempEntries.emplace_back(std::make_shared<PasteDataEntry>());
    std::string primaryText = TEST_ENTITY_TEXT;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest002 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest003
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest003 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    EXPECT_NE(entry, nullptr);

    entry->SetMimeType(MIMETYPE_TEXT_URI);
    tempEntries.emplace_back(entry);
    std::string primaryText = "";
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest003 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest004
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest004 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    EXPECT_NE(entry, nullptr);

    entry->SetMimeType(MIMETYPE_TEXT_PLAIN);
    tempEntries.emplace_back(entry);
    std::string primaryText = "";
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest004 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest005
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest005 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    EXPECT_NE(entry, nullptr);

    entry->SetMimeType(MIMETYPE_TEXT_PLAIN);
    entry->SetValue("test");
    tempEntries.emplace_back(entry);
    std::string primaryText = "";
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest005 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest006
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest006 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    entry->SetMimeType(MIMETYPE_TEXT_URI);
    entry->SetValue(1);
    tempEntries.emplace_back(entry);
    std::string primaryText = "";
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest006 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest007
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest007 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    tempEntries.emplace_back(std::make_shared<PasteDataEntry>());
    std::string primaryText = TEST_ENTITY_TEXT_CN_50;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest007 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest008
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest008 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    tempEntries.emplace_back(std::make_shared<PasteDataEntry>());
    std::string primaryText = TEST_ENTITY_TEXT_CN_10;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest008 end");
}

/**
 * @tc.name: GetAllEntryPlainTextTest009
 * @tc.desc: test Func GetAllEntryPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllEntryPlainTextTest009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest009 start");
    uint32_t dataId = 1;
    uint32_t recordId = 0;
    std::vector<std::shared_ptr<PasteDataEntry>> tempEntries;
    tempEntries.emplace_back(std::make_shared<PasteDataEntry>());
    std::string primaryText = TEST_ENTITY_TEXT_CN_5;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->SetDataId(dataId);
    pasteData->AddTextRecord("test");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    tempPasteboard->clips_.InsertOrAssign(appInfo.userId, pasteData);
    auto ret = tempPasteboard->GetAllEntryPlainText(dataId, recordId, tempEntries, primaryText);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION));

    tempPasteboard->clips_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainTextTest009 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest001
 * @tc.desc: test Func GetAllPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllPrimaryTextTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest001 start");
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    auto ret = tempPasteboard->GetAllPrimaryText(pasteData);

    EXPECT_EQ(ret, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest001 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest002
 * @tc.desc: test Func GetAllPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllPrimaryTextTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest002 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    pasteData.AddTextRecord("testRecord");
    pasteData.AddTextRecord(TEST_ENTITY_TEXT);
    pasteData.AddTextRecord("testRecord");
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto ret = tempPasteboard->GetAllPrimaryText(pasteData);
    EXPECT_EQ(ret, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest002 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest003
 * @tc.desc: test Func GetAllPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllPrimaryTextTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest003 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    pasteData.AddTextRecord("testRecord");
    pasteData.AddTextRecord(TEST_ENTITY_TEXT_CN_50);
    pasteData.AddTextRecord("testRecord");
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto ret = tempPasteboard->GetAllPrimaryText(pasteData);
    EXPECT_EQ(ret, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest003 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest004
 * @tc.desc: test Func GetAllPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllPrimaryTextTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest004 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    pasteData.AddTextRecord("testRecord");
    pasteData.AddTextRecord(TEST_ENTITY_TEXT_CN_10);
    pasteData.AddTextRecord("testRecord");
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto ret = tempPasteboard->GetAllPrimaryText(pasteData);
    EXPECT_EQ(ret, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest004 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest005
 * @tc.desc: test Func GetAllPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetAllPrimaryTextTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest005 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    pasteData.AddTextRecord("testRecord");
    pasteData.AddTextRecord(TEST_ENTITY_TEXT_CN_5);
    pasteData.AddTextRecord("testRecord");
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto ret = tempPasteboard->GetAllPrimaryText(pasteData);
    EXPECT_EQ(ret, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest005 end");
}

/**
 * @tc.name: GetRemoteEntryValueTest001
 * @tc.desc: test Func GetRemoteEntryValue
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteEntryValueTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteEntryValueTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    AppInfo appInfo;
    PasteData pasteData;
    PasteDataRecord record;
    PasteDataEntry entry;
    tempPasteboard->GetRemoteEntryValue(appInfo, pasteData, record, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRemoteEntryValueTest001 end");
}

/**
 * @tc.name: GetRemoteEntryValueTest002
 * @tc.desc: GetRemoteEntryValueTest002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRemoteEntryValueTest002, TestSize.Level1)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string remoteDeviceId = "remoteDeviceId";
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    PasteData data;
    PasteDataRecord record;
    PasteDataEntry entry;

    int32_t ret = tempPasteboard->GetRemoteEntryValue(appInfo, data, record, entry);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL));
}

/**
 * @tc.name: GetRecordValueByTypeTest001
 * @tc.desc: test Func GetRecordValueByType
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceGetDataTest, GetRecordValueByTypeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t dataId = 1;
    uint32_t recordId = 1;
    int64_t rawDataSize = -1;
    std::vector<uint8_t> buffer;
    int fd = 0;

    int32_t result = tempPasteboard->GetRecordValueByType(dataId, recordId, rawDataSize, buffer, fd);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest001 end");
}

} // namespace MiscServices
} // namespace OHOS