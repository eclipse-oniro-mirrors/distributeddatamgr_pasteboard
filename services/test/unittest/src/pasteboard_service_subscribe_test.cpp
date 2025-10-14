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

class PasteboardServiceSubscribeTest : public testing::Test {
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

void PasteboardServiceSubscribeTest::SetUpTestCase(void) { }

void PasteboardServiceSubscribeTest::TearDownTestCase(void) { }

void PasteboardServiceSubscribeTest::SetUp(void) { }

void PasteboardServiceSubscribeTest::TearDown(void) { }

int32_t PasteboardServiceSubscribeTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: SubscribeEntityObserverTest001
 * @tc.desc: test Func SubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeEntityObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeEntityObserverTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeEntityObserverTest001 end");
}

/**
 * @tc.name: SubscribeEntityObserverTest002
 * @tc.desc: test Func SubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeEntityObserverTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeEntityObserverTest002 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeEntityObserverTest002 end");
}

/**
 * @tc.name: SubscribeEntityObserverTest003
 * @tc.desc: test Func SubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeEntityObserverTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeEntityObserverTest003 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    result = service->SubscribeEntityObserver(entityType, 1, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeEntityObserverTest003 end");
}

/**
 * @tc.name: UnsubscribeEntityObserverTest001
 * @tc.desc: test Func UnsubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, UnsubscribeEntityObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->UnsubscribeEntityObserver(entityType, expectedDataLength, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest001 end");
}

/**
 * @tc.name: UnsubscribeEntityObserverTest002
 * @tc.desc: test Func UnsubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, UnsubscribeEntityObserverTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest002 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    result = service->UnsubscribeEntityObserver(entityType, 1, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest002 end");
}

/**
 * @tc.name: UnsubscribeEntityObserverTest003
 * @tc.desc: test Func UnsubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, UnsubscribeEntityObserverTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest003 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    result = service->UnsubscribeEntityObserver(entityType, expectedDataLength, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest003 end");
}

/**
 * @tc.name: UnsubscribeEntityObserverTest004
 * @tc.desc: test Func UnsubscribeEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, UnsubscribeEntityObserverTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest004 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    EntityType entityType = EntityType::ADDRESS;
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    const sptr<IEntityRecognitionObserver> observer = sptr<MyTestEntityRecognitionObserver>::MakeSptr();

    int32_t result = service->SubscribeEntityObserver(entityType, expectedDataLength, observer);
    result = service->SubscribeEntityObserver(entityType, 1, observer);
    result = service->UnsubscribeEntityObserver(entityType, expectedDataLength, observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeEntityObserverTest004 end");
}

/**
 * @tc.name: SubscribeObserverTest001
 * @tc.desc: test Func SubscribeObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeObserverTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    const sptr<IPasteboardChangedObserver> observer = nullptr;
    auto ret = tempPasteboard->SubscribeObserver(PasteboardObserverType::OBSERVER_LOCAL, observer);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::ADD_OBSERVER_FAILED));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeObserverTest001 end");
}

/**
 * @tc.name: SubscribeObserverTest002
 * @tc.desc: test Func SubscribeObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeObserverTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeObserverTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    const sptr<IPasteboardChangedObserver> observer = nullptr;
    auto ret = tempPasteboard->SubscribeObserver(PasteboardObserverType::OBSERVER_REMOTE, observer);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::ADD_OBSERVER_FAILED));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeObserverTest002 end");
}

/**
 * @tc.name: SubscribeObserverTest003
 * @tc.desc: test Func SubscribeObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeObserverTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeObserverTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    const sptr<IPasteboardChangedObserver> observer = nullptr;
    auto ret = tempPasteboard->SubscribeObserver(PasteboardObserverType::OBSERVER_EVENT, observer);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::ADD_OBSERVER_FAILED));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeObserverTest003 end");
}

/**
 * @tc.name: UnsubscribeAllEntityObserverTest001
 * @tc.desc: test Func UnsubscribeAllEntityObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, UnsubscribeAllEntityObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeAllEntityObserverTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->UnsubscribeAllEntityObserver();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeAllEntityObserverTest001 end");
}

/**
 * @tc.name: SubscribeKeyboardEventTest001
 * @tc.desc: test Func SubscribeKeyboardEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeKeyboardEventTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeKeyboardEventTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    auto result = tempPasteboard->SubscribeKeyboardEvent();
    EXPECT_EQ(result, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeKeyboardEventTest001 end");
}

/**
 * @tc.name: SubscribeKeyboardEventTest002
 * @tc.desc: test Func SubscribeKeyboardEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, SubscribeKeyboardEventTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeKeyboardEventTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    auto result = tempPasteboard->SubscribeKeyboardEvent();
    result = tempPasteboard->SubscribeKeyboardEvent();
    EXPECT_EQ(result, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubscribeKeyboardEventTest002 end");
}

/**
 * @tc.name: PasteboardEventSubscriberTest001
 * @tc.desc: test Func PasteboardEventSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, PasteboardEventSubscriberTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardEventSubscriberTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->PasteboardEventSubscriber();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardEventSubscriberTest001 end");
}

/**
 * @tc.name: CommonEventSubscriberTest001
 * @tc.desc: test Func CommonEventSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, CommonEventSubscriberTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CommonEventSubscriberTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->CommonEventSubscriber();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CommonEventSubscriberTest001 end");
}

/**
 * @tc.name: CommonEventSubscriberTest002
 * @tc.desc: test Func CommonEventSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, CommonEventSubscriberTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CommonEventSubscriberTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->CommonEventSubscriber();
    tempPasteboard->CommonEventSubscriber();
    EXPECT_NE(tempPasteboard->commonEventSubscriber_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CommonEventSubscriberTest002 end");
}

/**
 * @tc.name: AccountStateSubscriberTest001
 * @tc.desc: test Func AccountStateSubscriber
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, AccountStateSubscriberTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AccountStateSubscriberTest001 start");
    sptr<PasteboardService> tempPasteboard = new PasteboardService();
    EXPECT_NE(tempPasteboard, nullptr);

    std::set<AccountSA::OsAccountState> states = {AccountSA::OsAccountState::STOPPING};
    AccountSA::OsAccountSubscribeInfo subscribeInfo(states, true);
    tempPasteboard->accountStateSubscriber_ = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo,
        tempPasteboard);
    tempPasteboard->AccountStateSubscriber();
    EXPECT_NE(tempPasteboard->accountStateSubscriber_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AccountStateSubscriberTest001 end");
}

/**
 * @tc.name: AddObserverTest001
 * @tc.desc: test Func AddObserver
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceSubscribeTest, AddObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddObserverTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteboardService::ObserverMap observerMap;
    int32_t user = 1234;

    tempPasteboard->AddObserver(user, nullptr, observerMap);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddObserverTest001 end");
}

/**
 * @tc.name: AddObserverTest002
 * @tc.desc: test Func AddObserver
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceSubscribeTest, AddObserverTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddObserverTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    sptr<IPasteboardChangedObserver> observer;
    PasteboardService::ObserverMap observerMap;
    int32_t user = 1234;

    tempPasteboard->AddObserver(user, observer, observerMap);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddObserverTest002 end");
}

/**
 * @tc.name: GetObserversSizeTest001
 * @tc.desc: test Func GetObserversSize
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, GetObserversSizeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetObserversSizeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    pid_t pid = 1;
    PasteboardService::ObserverMap observerMap;
    tempPasteboard->GetObserversSize(userId, pid, observerMap);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetObserversSizeTest001 end");
}

/**
 * @tc.name: GetObserversSizeTest002
 * @tc.desc: test Func GetObserversSize
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, GetObserversSizeTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetObserversSizeTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    pid_t pid = 1;
    PasteboardService::ObserverMap observerMap;
    auto result = tempPasteboard->GetObserversSize(userId, pid, observerMap);
    EXPECT_EQ(result, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetObserversSizeTest002 end");
}

/**
 * @tc.name: GetAllObserversSizeTest001
 * @tc.desc: test Func GetAllObserversSize
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, GetAllObserversSizeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllObserversSizeTest001 start");
    auto userId = 123;
    auto tokenId = 123;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetAllObserversSize(userId, tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllObserversSizeTest001 end");
}

/**
 * @tc.name: RemoveSingleObserverddObserverTest001
 * @tc.desc: test Func RemoveSingleObserverddObserver
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceSubscribeTest, RemoveSingleObserverddObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveSingleObserverddObserverTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteboardService::ObserverMap observerMap;
    int32_t user = 1234;

    tempPasteboard->RemoveSingleObserver(user, nullptr, observerMap);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveSingleObserverddObserverTest001 end");
}

/**
 * @tc.name: RemoveAllObserverTest001
 * @tc.desc: test Func RemoveAllObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, RemoveAllObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAllObserverTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    PasteboardService::ObserverMap observerMap;
    tempPasteboard->RemoveAllObserver(userId, observerMap);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAllObserverTest001 end");
}

/**
 * @tc.name: RemoveObserverByPidTest001
 * @tc.desc: test Func RemoveObserverByPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, RemoveObserverByPidTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveObserverByPidTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    pid_t pid = 1;
    PasteboardService::ObserverMap observerMap;
    tempPasteboard->RemoveObserverByPid(userId, pid, observerMap);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveObserverByPidTest001 end");
}

/**
 * @tc.name: RemoveObserverByPidTest002
 * @tc.desc: test Func RemoveObserverByPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, RemoveObserverByPidTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveObserverByPidTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    pid_t pid = 1;
    struct classcomp {};
    std::pair<int32_t, pid_t> callObserverKey = std::make_pair(userId, pid);
    PasteboardService::ObserverMap observerMap;
    observerMap.insert(std::make_pair(callObserverKey, nullptr));
    tempPasteboard->RemoveObserverByPid(userId, pid, observerMap);
    EXPECT_EQ(observerMap.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveObserverByPidTest002 end");
}

/**
 * @tc.name: RegisterClientDeathObserverTest001
 * @tc.desc: test Func RegisterClientDeathObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSubscribeTest, RegisterClientDeathObserverTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RegisterClientDeathObserverTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteboardService service;
    pid_t pid = 1;
    sptr<IRemoteObject> observer = sptr<RemoteObjectTest>::MakeSptr(u"test");
    auto result = tempPasteboard->RegisterClientDeathObserver(observer);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RegisterClientDeathObserverTest001 end");
}

} // namespace MiscServices
} // namespace OHOS