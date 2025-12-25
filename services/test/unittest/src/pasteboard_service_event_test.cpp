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

class PasteboardServiceEventTest : public testing::Test {
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

void PasteboardServiceEventTest::SetUpTestCase(void) { }

void PasteboardServiceEventTest::TearDownTestCase(void) { }

void PasteboardServiceEventTest::SetUp(void) { }

void PasteboardServiceEventTest::TearDown(void) { }

int32_t PasteboardServiceEventTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: OnInputEventTest001
 * @tc.desc: test Func OnInputEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnInputEventTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest001 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    MMI::KeyEvent::KeyItem keyItem1;
    keyItem1.SetKeyCode(MMI::KeyEvent::KEYCODE_CTRL_LEFT);
    std::vector<MMI::KeyEvent::KeyItem> keys = {keyItem1};
    auto keyEvent = std::make_shared<MMI::KeyEvent>(0);
    EXPECT_NE(keyEvent, nullptr);

    keyEvent->SetKeyItem(keys);
    keyEvent->SetAction(MMI::KeyEvent::KEY_ACTION_DOWN);
    tempPasteboard->OnInputEvent(keyEvent);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest001 end");
}

/**
 * @tc.name: OnInputEventTest002
 * @tc.desc: test Func OnInputEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnInputEventTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest002 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    MMI::KeyEvent::KeyItem keyItem1;
    keyItem1.SetKeyCode(MMI::KeyEvent::KEYCODE_CTRL_LEFT);
    std::vector<MMI::KeyEvent::KeyItem> keys = {keyItem1, keyItem1};
    auto keyEvent = std::make_shared<MMI::KeyEvent>(0);
    EXPECT_NE(keyEvent, nullptr);

    keyEvent->SetKeyItem(keys);
    keyEvent->SetAction(MMI::KeyEvent::KEY_ACTION_DOWN);
    tempPasteboard->OnInputEvent(keyEvent);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest002 end");
}

/**
 * @tc.name: OnInputEventTest003
 * @tc.desc: test Func OnInputEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnInputEventTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest003 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    MMI::KeyEvent::KeyItem keyItem1;
    keyItem1.SetKeyCode(MMI::KeyEvent::KEYCODE_CTRL_LEFT);
    MMI::KeyEvent::KeyItem keyItem2;
    keyItem2.SetKeyCode(MMI::KeyEvent::KEYCODE_V);
    std::vector<MMI::KeyEvent::KeyItem> keys = {keyItem1, keyItem2};
    auto keyEvent = std::make_shared<MMI::KeyEvent>(0);
    EXPECT_NE(keyEvent, nullptr);

    keyEvent->SetKeyItem(keys);
    keyEvent->SetAction(MMI::KeyEvent::KEY_ACTION_DOWN);
    keyEvent->SetTargetWindowId(123);
    tempPasteboard->OnInputEvent(keyEvent);
    tempPasteboard->OnInputEvent(keyEvent);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest003 end");
}

/**
 * @tc.name: OnInputEventTest004
 * @tc.desc: test Func OnInputEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnInputEventTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest004 start");
    auto pasteboardServer = std::make_shared<PasteboardService>();
    EXPECT_NE(pasteboardServer, nullptr);

    auto inputCB = std::make_shared<InputEventCallback>(InputEventCallback::INPUTTYPE_PRESYNC, pasteboardServer.get());
    EXPECT_NE(inputCB, nullptr);

    MMI::KeyEvent::KeyItem keyItem1;
    keyItem1.SetKeyCode(MMI::KeyEvent::KEYCODE_CTRL_LEFT);
    std::vector<MMI::KeyEvent::KeyItem> keys = {keyItem1};
    auto keyEvent = std::make_shared<MMI::KeyEvent>(0);
    EXPECT_NE(keyEvent, nullptr);

    keyEvent->SetKeyItem(keys);
    keyEvent->SetAction(MMI::KeyEvent::KEY_ACTION_DOWN);
    inputCB->OnInputEvent(keyEvent);
    auto pointerEvent = MMI::PointerEvent::Create();
    EXPECT_NE(pointerEvent, nullptr);

    inputCB->OnInputEvent(pointerEvent);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnInputEventTest004 end");
}

/**
 * @tc.name: IsCtrlVProcessTest001
 * @tc.desc: test Func IsCtrlVProcess
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IsCtrlVProcessTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest001 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t callingPid = 1;
    bool isFocused = 1;
    auto result = tempPasteboard->IsCtrlVProcess(callingPid, isFocused);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest001 end");
}

/**
 * @tc.name: IsCtrlVProcessTest002
 * @tc.desc: test Func IsCtrlVProcess
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IsCtrlVProcessTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest002 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t callingPid = 1;
    bool isFocused = true;
    tempPasteboard->windowPid_ = callingPid;
    tempPasteboard->actionTime_ = static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    std::shared_ptr<BlockObject<int32_t>> block = std::make_shared<BlockObject<int32_t>>(2000, SET_VALUE_SUCCESS);
    block->SetValue(SET_VALUE_SUCCESS);
    tempPasteboard->blockMap_.insert(std::make_pair(callingPid, block));
    auto result = tempPasteboard->IsCtrlVProcess(callingPid, isFocused);
    EXPECT_EQ(result, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest002 end");
}

/**
 * @tc.name: IsCtrlVProcessTest003
 * @tc.desc: test Func IsCtrlVProcess
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IsCtrlVProcessTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest003 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t callingPid = 1;
    bool isFocused = true;
    std::shared_ptr<BlockObject<int32_t>> block = nullptr;
    tempPasteboard->blockMap_.insert(std::make_pair(callingPid, block));
    tempPasteboard->windowPid_ = 2;
    tempPasteboard->actionTime_ = static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto result = tempPasteboard->IsCtrlVProcess(callingPid, isFocused);
    EXPECT_EQ(result, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest003 end");
}

/**
 * @tc.name: IsCtrlVProcessTest004
 * @tc.desc: test Func IsCtrlVProcess
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IsCtrlVProcessTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest004 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t callingPid = 1;
    bool isFocused = false;
    std::shared_ptr<BlockObject<int32_t>> block = nullptr;
    tempPasteboard->blockMap_.insert(std::make_pair(callingPid, block));
    tempPasteboard->windowPid_ = 2;
    auto result = tempPasteboard->IsCtrlVProcess(callingPid, isFocused);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest004 end");
}

/**
 * @tc.name: IsCtrlVProcessTest005
 * @tc.desc: test Func IsCtrlVProcess
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IsCtrlVProcessTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest005 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t callingPid = 1;
    bool isFocused = true;
    std::shared_ptr<BlockObject<int32_t>> block = nullptr;
    tempPasteboard->blockMap_.insert(std::make_pair(callingPid, block));
    tempPasteboard->actionTime_ = static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()) + 2000;
    auto result = tempPasteboard->IsCtrlVProcess(callingPid, isFocused);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest005 end");
}

/**
 * @tc.name: IsCtrlVProcessTest006
 * @tc.desc: test Func IsCtrlVProcess
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IsCtrlVProcessTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest006 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t callingPid = 1;
    bool isFocused = true;
    std::shared_ptr<BlockObject<int32_t>> block = nullptr;
    tempPasteboard->blockMap_.insert(std::make_pair(callingPid, block));
    tempPasteboard->actionTime_ = static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto result = true;
    if (tempPasteboard->actionTime_ > EVENT_TIME_OUT) {
        tempPasteboard->actionTime_ -= EVENT_TIME_OUT;
    } else {
        tempPasteboard->actionTime_ += EVENT_TIME_OUT;
    }
    result = tempPasteboard->IsCtrlVProcess(callingPid, isFocused);
    EXPECT_EQ(result, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCtrlVProcessTest006 end");
}

/**
 * @tc.name: GetValidDistributeEventTest001
 * @tc.desc: test Func GetValidDistributeEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, GetValidDistributeEventTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t user = 100;
    tempPasteboard->GetValidDistributeEvent(user);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest001 end");
}

/**
 * @tc.name: GetValidDistributeEventTest002
 * @tc.desc: test Func GetValidDistributeEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, GetValidDistributeEventTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    TestEvent evt;
    int32_t user = 100;
    auto plugin = nullptr;
    auto result = tempPasteboard->GetValidDistributeEvent(user);
    EXPECT_EQ(result.first, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest002 end");
}

/**
 * @tc.name: GetValidDistributeEventTest003
 * @tc.desc: test Func GetValidDistributeEvent
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceEventTest, GetValidDistributeEventTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t user = 1234;

    tempPasteboard->GetValidDistributeEvent(user);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest003 end");
}

/**
 * @tc.name: SetRadarEventTest001
 * @tc.desc: test Func SetRadarEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, SetRadarEventTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetRadarEventTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    AppInfo appInfo;
    bool isPeerOnline = false;
    RadarReportInfo radarReportInfo;
    std::string peerNetId = "";
    tempPasteboard->SetRadarEvent(appInfo, pasteData, isPeerOnline, radarReportInfo, peerNetId);
    EXPECT_EQ(radarReportInfo.stageRes, 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetRadarEventTest001 end");
}

/**
 * @tc.name: SetUeEventTest001
 * @tc.desc: test Func SetUeEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, SetUeEventTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetUeEventTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    AppInfo appInfo;
    bool isPeerOnline = false;
    UeReportInfo ueReportInfo;
    std::string peerNetId = "";
    tempPasteboard->SetUeEvent(appInfo, pasteData, isPeerOnline, ueReportInfo, peerNetId);
    EXPECT_EQ(ueReportInfo.pasteInfo.onlineDevNum, DMAdapter::GetInstance().GetNetworkIds().size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetUeEventTest001 end");
}

/**
 * @tc.name: ReportUeCopyEventTest001
 * @tc.desc: test Func ReportUeCopyEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, ReportUeCopyEventTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ReportUeCopyEventTest001 start");
    constexpr int32_t result = 111;
    PasteData pasteData;
    int64_t dataSize = 0;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ReportUeCopyEvent(pasteData, dataSize, result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ReportUeCopyEventTest001 end");
}

/**
 * @tc.name: OnReceiveEventTest001
 * @tc.desc: test Func OnReceiveEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnReceiveEventTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service;
    auto tempPasteboard = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    const EventFwk::CommonEventData data;
    tempPasteboard->OnReceiveEvent(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest001 end");
}

/**
 * @tc.name: OnReceiveEventTest002
 * @tc.desc: test Func OnReceiveEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnReceiveEventTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest002 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto tempPasteboard = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    EventFwk::CommonEventData data;
    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING);
    data.SetWant(want);
    tempPasteboard->OnReceiveEvent(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest002 end");
}

/**
 * @tc.name: OnReceiveEventTest003
 * @tc.desc: test Func OnReceiveEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnReceiveEventTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest003 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto tempPasteboard = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    EventFwk::CommonEventData data;
    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING);
    data.SetWant(want);
    tempPasteboard->OnReceiveEvent(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest003 end");
}

/**
 * @tc.name: OnReceiveEventTest004
 * @tc.desc: test Func OnReceiveEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnReceiveEventTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest004 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto tempPasteboard = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    EventFwk::CommonEventData data;
    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    data.SetWant(want);
    tempPasteboard->OnReceiveEvent(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest004 end");
}

/**
 * @tc.name: OnReceiveEventTest005
 * @tc.desc: test Func OnReceiveEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnReceiveEventTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest005 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto tempPasteboard = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    EventFwk::CommonEventData data;
    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    data.SetWant(want);
    tempPasteboard->OnReceiveEvent(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventTest005 end");
}

/**
 * @tc.name: OnStateChangedTest001
 * @tc.desc: test Func OnStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnStateChangedTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest001 start");
    AccountSA::OsAccountSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service;
    auto tempPasteboard = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    const AccountSA::OsAccountStateData data;
    tempPasteboard->OnStateChanged(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest001 end");
}

/**
 * @tc.name: OnStateChangedTest002
 * @tc.desc: test Func OnStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnStateChangedTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest002 start");
    AccountSA::OsAccountSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto tempPasteboard = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    AccountSA::OsAccountStateData data;
    data.state = AccountSA::OsAccountState::STOPPING;
    data.fromId = 1;
    data.toId = 2;
    data.callback = nullptr;
    tempPasteboard->OnStateChanged(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest002 end");
}

/**
 * @tc.name: OnStateChangedTest003
 * @tc.desc: test Func OnStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnStateChangedTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest003 start");
    AccountSA::OsAccountSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto tempPasteboard = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    AccountSA::OsAccountStateData data;
    data.state = AccountSA::OsAccountState::INVALID_TYPE;
    data.fromId = 1;
    data.toId = 2;
    data.callback = nullptr;
    tempPasteboard->OnStateChanged(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest003 end");
}

/**
 * @tc.name: OnStateChangedTest004
 * @tc.desc: test Func OnStateChanged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnStateChangedTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest004 start");
    AccountSA::OsAccountSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;

    auto tempPasteboard = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, service);
    EXPECT_NE(tempPasteboard, nullptr);

    AccountSA::OsAccountStateData data;
    data.state = AccountSA::OsAccountState::STOPPING;
    data.fromId = 1;
    data.toId = 2;
    data.callback = nullptr;
    tempPasteboard->OnStateChanged(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStateChangedTest004 end");
}

/**
 * @tc.name: OnAddDeviceManagerTest001
 * @tc.desc: test Func OnAddDeviceManager
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnAddDeviceManagerTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddDeviceManagerTest001 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddDeviceManager();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddDeviceManagerTest001 end");
}

/**
 * @tc.name: OnAddMemoryManagerTest001
 * @tc.desc: test Func OnAddMemoryManager
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnAddMemoryManagerTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddMemoryManagerTest001 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddMemoryManager();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddMemoryManagerTest001 end");
}

/**
 * @tc.name: OnAddSystemAbilityTest001
 * @tc.desc: test Func OnAddSystemAbility
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnAddSystemAbilityTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddSystemAbilityTest001 start");
    int32_t systemAbilityId = 1;
    const std::string deviceId = "test_string";
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddSystemAbility(systemAbilityId, deviceId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddSystemAbilityTest001 end");
}

/**
 * @tc.name: OnAddSystemAbilityTest002
 * @tc.desc: test Func OnAddSystemAbility
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnAddSystemAbilityTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddSystemAbilityTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    const std::string deviceId = "test_string";
    tempPasteboard->OnAddSystemAbility(static_cast<int32_t>(DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID), deviceId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddSystemAbilityTest002 end");
}

/**
 * @tc.name: OnAddSystemAbilityTest003
 * @tc.desc: test Func OnAddSystemAbility
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnAddSystemAbilityTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddSystemAbilityTest003 start");
    auto pbs = std::make_shared<PasteboardService>();
    EXPECT_NE(pbs, nullptr);

    pbs->OnAddSystemAbility(DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID, "");
    pbs->OnAddSystemAbility(MEMORY_MANAGER_SA_ID, "");
    pbs->OnAddSystemAbility(DISTRIBUTED_DEVICE_PROFILE_SA_ID, "");
    pbs->OnAddSystemAbility(PASTEBOARD_SERVICE_ID, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddSystemAbilityTest003 end");
}

/**
 * @tc.name: OnRemoveSystemAbilityTest001
 * @tc.desc: test Func OnRemoveSystemAbility
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnRemoveSystemAbilityTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoveSystemAbilityTest001 start");
    auto pbs = std::make_shared<PasteboardService>();

    EXPECT_NE(pbs, nullptr);
    pbs->OnRemoveSystemAbility(DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID, "");
    pbs->OnRemoveSystemAbility(MEMORY_MANAGER_SA_ID, "");
    pbs->OnRemoveSystemAbility(DISTRIBUTED_DEVICE_PROFILE_SA_ID, "");
    pbs->OnRemoveSystemAbility(PASTEBOARD_SERVICE_ID, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoveSystemAbilityTest001 end");
}

/**
 * @tc.name: OnConfigChangeTest001
 * @tc.desc: test Func OnConfigChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnConfigChangeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnConfigChangeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnConfigChange(true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnConfigChangeTest001 end");
}

/**
 * @tc.name: OnRemoteDiedTest001
 * @tc.desc: test Func OnRemoteDied
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnRemoteDiedTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteDiedTest001 start");
    wptr<IRemoteObject> deadRemote;
    constexpr int32_t userId = 111;
    PasteboardService service;
    auto tempPasteboard = std::make_shared<PasteboardService::DelayGetterDeathRecipient>(userId, service);
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnRemoteDied(deadRemote);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteDiedTest001 end");
}

/**
 * @tc.name: OnRemoteDiedTest002
 * @tc.desc: test Func OnRemoteDied
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnRemoteDiedTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteDiedTest002 start");
    constexpr int32_t userId = 111;
    PasteboardService service;
    wptr<IRemoteObject> deadRemote;
    auto tempPasteboard = std::make_shared<PasteboardService::EntryGetterDeathRecipient>(userId, service);
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnRemoteDied(deadRemote);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteDiedTest002 end");
}

/**
 * @tc.name: OnRemoteDiedTest003
 * @tc.desc: test Func OnRemoteDied
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnRemoteDiedTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteDiedTest003 start");
    PasteboardService service;
    pid_t pid = 2;
    auto tempPasteboard = std::make_shared<PasteboardService::PasteboardDeathRecipient>(service, pid);
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnRemoteDied(nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoteDiedTest003 end");
}

/**
 * @tc.name: OnStopTest001
 * @tc.desc: test Func OnStop
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnStopTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStopTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnStop();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStopTest001 end");
}

/**
 * @tc.name: OnEstablishP2PLinkTaskTest001
 * @tc.desc: test Func OnEstablishP2PLinkTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, OnEstablishP2PLinkTaskTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnEstablishP2PLinkTaskTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string networkId = "networkId";
    std::shared_ptr<BlockObject<int32_t>> block = std::make_shared<BlockObject<int32_t>>(2000, SET_VALUE_SUCCESS);
    tempPasteboard->OnEstablishP2PLinkTask(networkId, block);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnEstablishP2PLinkTaskTest001 end");
}

/**
 * @tc.name: GetChangeCountTest001
 * @tc.desc: test Func GetChangeCount
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, GetChangeCountTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetChangeCountTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    uint32_t changeCount = 0;
    int32_t result = service->GetChangeCount(changeCount);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetChangeCountTest001 end");
}

/**
 * @tc.name: ChangeStoreStatusTest001
 * @tc.desc: test Func ChangeStoreStatus
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, ChangeStoreStatusTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ChangeStoreStatusTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    tempPasteboard->ChangeStoreStatus(userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ChangeStoreStatusTest001 end");
}

/**
 * @tc.name: IncreaseChangeCountTest001
 * @tc.desc: test Func IncreaseChangeCount, should reset to 0 after reach maximum limit.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IncreaseChangeCountTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    int32_t userId = 10;
    tempPasteboard->clipChangeCount_.Compute(userId, [](auto, uint32_t &changeCount) {
        changeCount = UINT32_MAX;
        return true;
    });
    uint32_t testCount = 0;
    auto it = tempPasteboard->clipChangeCount_.Find(userId);
    if (it.first) {
        testCount = it.second;
    }
    EXPECT_EQ(testCount, UINT32_MAX);
    tempPasteboard->IncreaseChangeCount(userId);
    it = tempPasteboard->clipChangeCount_.Find(userId);
    if (it.first) {
        testCount = it.second;
    }
    EXPECT_EQ(testCount, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest001 end");
}

/**
 * @tc.name: IncreaseChangeCountTest002
 * @tc.desc: test Func IncreaseChangeCount, should reset to 0 after switch to a new user.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, IncreaseChangeCountTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    uint32_t testCount = 0;
    tempPasteboard->GetChangeCount(testCount);
    EXPECT_EQ(testCount, 0);

    tempPasteboard->currentUserId_ = 10;
    auto userId = tempPasteboard->GetCurrentAccountId();
    tempPasteboard->IncreaseChangeCount(userId);
    tempPasteboard->GetChangeCount(testCount);
    EXPECT_EQ(testCount, 1);

    tempPasteboard->currentUserId_ = 100;
    tempPasteboard->GetChangeCount(testCount);
    EXPECT_EQ(testCount, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCountTest002 end");
}

/**
 * @tc.name: RemotePasteboardChangeTest001
 * @tc.desc: test Func RemotePasteboardChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEventTest, RemotePasteboardChangeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemotePasteboardChangeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto result = tempPasteboard->RemotePasteboardChange();
    EXPECT_NE(result, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemotePasteboardChangeTest001 end");
}

} // namespace MiscServices
} // namespace OHOS