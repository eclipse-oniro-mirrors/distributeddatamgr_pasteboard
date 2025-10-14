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

class PasteboardServiceRemoteTest : public testing::Test {
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

void PasteboardServiceRemoteTest::SetUpTestCase(void) { }

void PasteboardServiceRemoteTest::TearDownTestCase(void) { }

void PasteboardServiceRemoteTest::SetUp(void) { }

void PasteboardServiceRemoteTest::TearDown(void) { }

int32_t PasteboardServiceRemoteTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: ClearRemoteDataTask001
 * @tc.desc: test Func ClearRemoteDataTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, ClearRemoteDataTask001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearRemoteDataTask001 start");
    std::shared_ptr<PasteboardService::RemoteDataTaskManager> remoteDataTaskManager =
        std::make_shared<PasteboardService::RemoteDataTaskManager>();
    EXPECT_NE(remoteDataTaskManager, nullptr);

    TestEvent event;
    remoteDataTaskManager->ClearRemoteDataTask(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearRemoteDataTask001 end");
}

/**
 * @tc.name: WaitRemoteData001
 * @tc.desc: test Func WaitRemoteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, WaitRemoteData001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WaitRemoteData001 start");
    std::shared_ptr<PasteboardService::RemoteDataTaskManager> remoteDataTaskManager =
        std::make_shared<PasteboardService::RemoteDataTaskManager>();
    EXPECT_NE(remoteDataTaskManager, nullptr);

    TestEvent event;
    remoteDataTaskManager->WaitRemoteData(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WaitRemoteData001 end");
}

/**
 * @tc.name: WaitRemoteData002
 * @tc.desc: test Func WaitRemoteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, WaitRemoteData002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WaitRemoteData002 start");
    std::shared_ptr<PasteboardService::RemoteDataTaskManager> remoteDataTaskManager =
        std::make_shared<PasteboardService::RemoteDataTaskManager>();
    EXPECT_NE(remoteDataTaskManager, nullptr);

    TestEvent event;
    event.deviceId = "12345";
    event.seqId = 1;

    auto key = event.deviceId + std::to_string(event.seqId);
    auto it = remoteDataTaskManager->dataTasks_.find(key);
    it = remoteDataTaskManager->dataTasks_.emplace(key, std::make_shared<TaskContext>()).first;

    remoteDataTaskManager->WaitRemoteData(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WaitRemoteData002 end");
}

/**
 * @tc.name: ProcessRemoteDelayHtmlTest001
 * @tc.desc: test Func ProcessRemoteDelayHtml
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, ProcessRemoteDelayHtmlTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessRemoteDelayHtmlTest001 start");
    std::string remoteDeviceId;
    AppInfo appInfo;
    const std::vector<uint8_t> rawData;
    PasteData data;
    PasteDataRecord record;
    PasteDataEntry entry;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ProcessRemoteDelayHtml(remoteDeviceId, appInfo, rawData, data, record, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessRemoteDelayHtmlTest001 end");
}

/**
 * @tc.name: IsRemoteDataTest001
 * @tc.desc: test Func IsRemoteData, funcResult is false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, IsRemoteDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsRemoteDataTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentUserId_ = ERROR_USERID;
    bool funcResult;
    int32_t result = service->IsRemoteData(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsRemoteDataTest001 end");
}

/**
 * @tc.name: IsRemoteDataTest002
 * @tc.desc: test Func IsRemoteData, currentUserId_ is INT32_MAX.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, IsRemoteDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsRemoteDataTest002 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->currentUserId_ = INT32_MAX;
    bool funcResult;
    int32_t result = service->IsRemoteData(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsRemoteDataTest002 end");
}

/**
 * @tc.name: IsRemoteDataTest003
 * @tc.desc: test Func IsRemoteData, currentUserId_ is 0XF
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, IsRemoteDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsRemoteDataTest003 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    int32_t userId = service->currentUserId_ = 0XF;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    EXPECT_NE(pasteData, nullptr);

    pasteData->AddTextRecord("hello");
    service->clips_.InsertOrAssign(userId, pasteData);
    bool funcResult;
    int32_t result = service->IsRemoteData(funcResult);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsRemoteDataTest003 end");
}

/**
 * @tc.name: ProcessRemoteDelayHtmlInnerTest001
 * @tc.desc: ProcessRemoteDelayHtmlInnerTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, ProcessRemoteDelayHtmlInnerTest001, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string remoteDeviceId = "remoteDeviceId";
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    PasteData tmpData;
    tmpData.SetFileSize(1);
    PasteData data;
    PasteDataEntry entry;
    
    int32_t ret = tempPasteboard->ProcessRemoteDelayHtmlInner(remoteDeviceId, appInfo, tmpData, data, entry);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::REBUILD_HTML_FAILED));
}

/**
 * @tc.name: ProcessRemoteDelayHtmlInnerTest002
 * @tc.desc: ProcessRemoteDelayHtmlInnerTest002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, ProcessRemoteDelayHtmlInnerTest002, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string remoteDeviceId = "remoteDeviceId";
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    PasteData tmpData;
    tmpData.SetFileSize(0);
    PasteData data;
    PasteDataEntry entry;
    
    int32_t ret = tempPasteboard->ProcessRemoteDelayHtmlInner(remoteDeviceId, appInfo, tmpData, data, entry);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::REBUILD_HTML_FAILED));
}

/**
 * @tc.name: ProcessRemoteDelayHtmlTest002
 * @tc.desc: ProcessRemoteDelayHtmlTest002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, ProcessRemoteDelayHtmlTest002, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);
    std::string remoteDeviceId = "remoteDeviceId";
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    std::vector<uint8_t> rawData(0);
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
    data.Encode(rawData);
    
    int32_t ret = tempPasteboard->ProcessRemoteDelayHtml(remoteDeviceId, appInfo, rawData, data, *record, *entry);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED));
}

/**
 * @tc.name: ProcessRemoteDelayUriTest001
 * @tc.desc: ProcessRemoteDelayUriTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceRemoteTest, ProcessRemoteDelayUriTest001, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string deviceId = "deviceId";
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    std::vector<uint8_t> rawData(0);
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
    data.Encode(rawData);
    
    int32_t ret = tempPasteboard->ProcessRemoteDelayUri(deviceId, appInfo, data, *record, *entry);
    EXPECT_NE(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

} // namespace MiscServices
} // namespace OHOS