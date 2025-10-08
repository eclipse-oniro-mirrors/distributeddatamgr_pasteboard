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

class PasteboardServiceTest : public testing::Test {
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

void PasteboardServiceTest::SetUpTestCase(void) { }

void PasteboardServiceTest::TearDownTestCase(void) { }

void PasteboardServiceTest::SetUp(void) { }

void PasteboardServiceTest::TearDown(void) { }

int32_t PasteboardServiceTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
HWTEST_F(PasteboardServiceTest, OnInputEventTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnInputEventTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnInputEventTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnInputEventTest004, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsCtrlVProcessTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsCtrlVProcessTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsCtrlVProcessTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsCtrlVProcessTest004, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsCtrlVProcessTest005, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsCtrlVProcessTest006, TestSize.Level0)
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
 * @tc.name: CallbackEnterTest001
 * @tc.desc: test Func CallbackEnter
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, CallbackEnterTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackEnterTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t code = static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA);

    int32_t res = tempPasteboard->CallbackEnter(code);

    EXPECT_EQ(res, ERR_NONE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackEnterTest001 end");
}

/**
 * @tc.name: CallbackEnterTest002
 * @tc.desc: test Func CallbackEnter
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, CallbackEnterTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackEnterTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t code = static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA) + 1;

    int32_t res = tempPasteboard->CallbackEnter(code);

    EXPECT_EQ(res, ERR_NONE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackEnterTest002 end");
}

/**
 * @tc.name: CallbackExitTest001
 * @tc.desc: test Func CallbackExit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, CallbackExitTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackExitTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t code = static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA);
    int32_t result = 123;

    int32_t res = tempPasteboard->CallbackExit(code, result);

    EXPECT_EQ(res, ERR_NONE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackExitTest001 end");
}

/**
 * @tc.name: CallbackExitTest002
 * @tc.desc: test Func CallbackExit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, CallbackExitTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackExitTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t code = static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA) + 1;
    int32_t result = 456;

    int32_t res = tempPasteboard->CallbackExit(code, result);

    EXPECT_EQ(res, ERR_NONE);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CallbackExitTest002 end");
}

/**
 * @tc.name: ClearTest001
 * @tc.desc: test Func Clear
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ClearTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentUserId_ = ACCOUNT_IDS_RANDOM;
    tempPasteboard->clips_.InsertOrAssign(ACCOUNT_IDS_RANDOM, std::make_shared<PasteData>());
    int32_t result = tempPasteboard->Clear();
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearTest001 end");
}

/**
 * @tc.name: ClearTest002
 * @tc.desc: test Func Clear
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ClearTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearTest002 start");
    auto tempPasteboard = std::make_shared<InputEventCallback>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearTest002 end");
}

/**
 * @tc.name: GetMimeTypesTest001
 * @tc.desc: test Func GetMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetMimeTypesTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetMimeTypesTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetMimeTypesTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetMimeTypesTest004, TestSize.Level0)
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
 * @tc.name: HasDataTypeTest001
 * @tc.desc: test Func HasDataType, currentScreenStatus is default.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, HasDataTypeTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasDataTypeTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasDataTypeTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasDataTypeTest004, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasDataTypeTest005, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasDataTypeTest006, TestSize.Level0)
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
 * @tc.name: DetectPatternsTest001
 * @tc.desc: test Func DetectPatterns, return PasteboardError::NO_DATA_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DetectPatternsTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    std::vector<Pattern> patternsToCheck;
    std::vector<Pattern> funcResult;
    EXPECT_EQ(tempPasteboard->DetectPatterns(patternsToCheck, funcResult),
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest001 end");
}

/**
 * @tc.name: DetectPatternsTest002
 * @tc.desc: test Func DetectPatterns, return PasteboardError::NO_DATA_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DetectPatternsTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    int32_t userId = INT32_MAX;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    ASSERT_NE(pasteData, nullptr);

    pasteData->AddTextRecord("hello");
    tempPasteboard->clips_.InsertOrAssign(userId, pasteData);
    std::vector<Pattern> patternsToCheck;
    std::vector<Pattern> funcResult;
    EXPECT_EQ(tempPasteboard->DetectPatterns(patternsToCheck, funcResult),
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest002 end");
}

/**
 * @tc.name: DetectPatternsTest003
 * @tc.desc: test Func DetectPatterns, test MIMETYPE_TEXT_PLAIN
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DetectPatternsTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    int32_t userId = tempPasteboard->currentUserId_ = 1;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    ASSERT_NE(pasteData, nullptr);

    pasteData->AddTextRecord("hello");
    tempPasteboard->clips_.InsertOrAssign(userId, pasteData);
    std::vector<Pattern> patternsToCheck;
    std::vector<Pattern> funcResult;
    EXPECT_EQ(tempPasteboard->DetectPatterns(patternsToCheck, funcResult),
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest003 end");
}

/**
 * @tc.name: DetectPatternsTest004
 * @tc.desc: test Func DetectPatterns, test MIMETYPE_TEXT_HTML
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DetectPatternsTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    int32_t userId = tempPasteboard->currentUserId_ = 1;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    ASSERT_NE(pasteData, nullptr);

    pasteData->AddHtmlRecord("<div class='disable'>helloWorld</div>");
    tempPasteboard->clips_.InsertOrAssign(userId, pasteData);
    std::vector<Pattern> patternsToCheck;
    std::vector<Pattern> funcResult;
    EXPECT_EQ(tempPasteboard->DetectPatterns(patternsToCheck, funcResult),
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest004 end");
}

/**
 * @tc.name: DetectPatternsTest005
 * @tc.desc: test Func DetectPatterns, test MIMETYPE_TEXT_URI
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DetectPatternsTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest005 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);

    int32_t userId = tempPasteboard->currentUserId_ = 1;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    ASSERT_NE(pasteData, nullptr);

    OHOS::Uri uri("/");
    pasteData->AddUriRecord(uri);
    tempPasteboard->clips_.InsertOrAssign(userId, pasteData);
    std::vector<Pattern> patternsToCheck;
    std::vector<Pattern> funcResult;
    EXPECT_EQ(tempPasteboard->DetectPatterns(patternsToCheck, funcResult),
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DetectPatternsTest005 end");
}

/**
 * @tc.name: GetDataSourceTest001
 * @tc.desc: test Func GetDataSource
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetDataSourceTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetDataSourceTest002, TestSize.Level0)
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
 * @tc.name: RemoveGlobalShareOptionTest001
 * @tc.desc: test Func RemoveGlobalShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RemoveGlobalShareOptionTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveGlobalShareOptionTest001 start");
    std::vector<uint32_t> tokenIds = { 1001, 1002, 1003 };
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->RemoveGlobalShareOption(tokenIds);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveGlobalShareOptionTest001 end");
}

/**
 * @tc.name: SetAppShareOptionsTest001
 * @tc.desc: test Func SetAppShareOptions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, SetAppShareOptionsTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t shareOptions = 0;
    tempPasteboard->SetAppShareOptions(shareOptions);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest001 end");
}

/**
 * @tc.name: SetAppShareOptionsTest002
 * @tc.desc: test Func SetAppShareOptions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, SetAppShareOptionsTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    int32_t shareOptions = 3;
    auto ret = tempPasteboard->SetAppShareOptions(shareOptions);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest002 end");
}

/**
 * @tc.name: SetAppShareOptionsTest003
 * @tc.desc: test Func SetAppShareOptions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, SetAppShareOptionsTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t shareOptions = 2;
    auto ret = tempPasteboard->SetAppShareOptions(shareOptions);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest003 end");
}

/**
 * @tc.name: SetAppShareOptionsTest004
 * @tc.desc: test Func SetAppShareOptions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, SetAppShareOptionsTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t shareOptions = 1;
    auto ret = tempPasteboard->SetAppShareOptions(shareOptions);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetAppShareOptionsTest004 end");
}

/**
 * @tc.name: RemoveAppShareOptionsTest001
 * @tc.desc: test Func RemoveAppShareOptions
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RemoveAppShareOptionsTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAppShareOptionsTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto ret = tempPasteboard->RemoveAppShareOptions();
    EXPECT_EQ(0, ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemoveAppShareOptionsTest001 end");
}

/**
 * @tc.name: OnStopTest001
 * @tc.desc: test Func OnStop
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnStopTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStopTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnStop();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStopTest001 end");
}

/**
 * @tc.name: PasteCompleteTest001
 * @tc.desc: test Func PasteComplete
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteCompleteTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteCompleteTest001 start");
    std::string deviceId = "testId";
    std::string pasteId;
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    int32_t result = service->PasteComplete(deviceId, pasteId);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteCompleteTest001 end");
}

/**
 * @tc.name: PasteCompleteTest002
 * @tc.desc: test Func PasteComplete
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteCompleteTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteCompleteTest002 start");
    std::string deviceId;
    std::string pasteId;
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    int32_t result = service->PasteComplete(deviceId, pasteId);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteCompleteTest002 end");
}

/**
 * @tc.name: PasteStartTest001
 * @tc.desc: test Func PasteStart
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteStartTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteStartTest001 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->ffrtTimer_ = nullptr;
    std::string pasteId;
    int32_t result = service->PasteStart(pasteId);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteStartTest001 end");
}

/**
 * @tc.name: PasteStartTest002
 * @tc.desc: test Func PasteStart
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteStartTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteStartTest002 start");
    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    service->ffrtTimer_ = std::make_shared<FFRTTimer>();
    std::string pasteId;
    int32_t result = service->PasteStart(pasteId);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteStartTest002 end");
}

/**
 * @tc.name: GetRemoteDeviceNameTest001
 * @tc.desc: test Func GetRemoteDeviceName
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetRemoteDeviceNameTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetRemoteDeviceNameTest002, TestSize.Level1)
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
 * @tc.name: DumpTest001
 * @tc.desc: test Func Dump
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DumpTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpTest001 start");
    auto fd = 1;
    std::vector<std::u16string> args;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->Dump(fd, args);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpTest001 end");
}

/**
 * @tc.name: GetChangeCountTest001
 * @tc.desc: test Func GetChangeCount
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetChangeCountTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, ChangeStoreStatusTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ChangeStoreStatusTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t userId = 1;
    tempPasteboard->ChangeStoreStatus(userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ChangeStoreStatusTest001 end");
}

/**
 * @tc.name: CloseDistributedStoreTest001
 * @tc.desc: test Func CloseDistributedStore
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CloseDistributedStoreTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseDistributedStoreTest001 start");
    int32_t user = ACCOUNT_IDS_RANDOM;
    bool isNeedClear = false;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->CloseDistributedStore(user, isNeedClear);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseDistributedStoreTest001 end");
}

/**
 * @tc.name: GetCurrentAccountIdTest001
 * @tc.desc: test Func GetCurrentAccountId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetCurrentAccountIdTest001, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, GetCurrentAccountIdTest002, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, GetCurrentAccountIdTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->currentUserId_ = ERROR_USERID;
    int32_t result = tempPasteboard->GetCurrentAccountId();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountIdTest003 end");
}

/**
 * @tc.name: SetPasteboardHistoryTest001
 * @tc.desc: test Func SetPasteboardHistory
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetPasteboardHistoryTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteboardHistoryTest001 start");
    HistoryInfo info;
    info.time = "2023-01-01";
    info.bundleName = "com.example.app";
    info.state = "copied";
    info.remote = "false";
    info.userId = 0;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->SetPasteboardHistory(info);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteboardHistoryTest001 end");
}

/**
 * @tc.name: IsFocusedAppTest001
 * @tc.desc: test Func IsFocusedApp
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsFocusedAppTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsFocusedAppTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = UINT32_ONE;
    tempPasteboard->IsFocusedApp(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsFocusedAppTest001 end");
}

/**
 * @tc.name: SetInputMethodPidTest001
 * @tc.desc: test Func SetInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetInputMethodPidTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    pid_t callPid = 1;
    tempPasteboard->SetInputMethodPid(userId, callPid);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest001 end");
}

/**
 * @tc.name: SetInputMethodPidTest002
 * @tc.desc: test Func SetInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetInputMethodPidTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = 1;
    pid_t callPid = 1;
    tempPasteboard->SetInputMethodPid(userId, callPid);
    auto it = tempPasteboard->imeMap_.Find(userId);
    auto hasPid = it.first;
    EXPECT_NE(hasPid, true);

    tempPasteboard->imeMap_.InsertOrAssign(userId, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest002 end");
}

/**
 * @tc.name: SetInputMethodPidTest003
 * @tc.desc: test Func SetInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetInputMethodPidTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = 1;
    pid_t callPid = 0;
    tempPasteboard->SetInputMethodPid(userId, callPid);
    auto it = tempPasteboard->imeMap_.Find(userId);
    auto hasPid = it.first;
    EXPECT_NE(hasPid, true);

    tempPasteboard->imeMap_.InsertOrAssign(userId, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);

    tempPasteboard->ClearInputMethodPidByPid(userId + 1, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest003 end");
}

/**
 * @tc.name: SetInputMethodPidTest004
 * @tc.desc: test Func SetInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetInputMethodPidTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = 1;
    pid_t callPid = 0;
    tempPasteboard->SetInputMethodPid(userId, callPid);
    auto it = tempPasteboard->imeMap_.Find(userId);
    auto hasPid = it.first;
    EXPECT_NE(hasPid, true);

    tempPasteboard->imeMap_.InsertOrAssign(userId, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);

    tempPasteboard->ClearInputMethodPidByPid(userId, callPid + 1);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest004 end");
}

/**
 * @tc.name: SetInputMethodPidTest005
 * @tc.desc: test Func SetInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetInputMethodPidTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest005 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = 1;
    pid_t callPid = 0;
    tempPasteboard->SetInputMethodPid(userId, callPid);
    auto it = tempPasteboard->imeMap_.Find(userId);
    auto hasPid = it.first;
    EXPECT_NE(hasPid, true);

    tempPasteboard->imeMap_.InsertOrAssign(userId, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);

    tempPasteboard->ClearInputMethodPidByPid(userId + 1, callPid + 1);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest005 end");
}

/**
 * @tc.name: SetInputMethodPidTest006
 * @tc.desc: test Func SetInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetInputMethodPidTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest006 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = 1;
    pid_t callPid = 0;
    tempPasteboard->SetInputMethodPid(userId, callPid);
    auto it = tempPasteboard->imeMap_.Find(userId);
    auto hasPid = it.first;
    EXPECT_NE(hasPid, true);

    tempPasteboard->imeMap_.InsertOrAssign(userId, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_EQ(hasPid, true);

    tempPasteboard->ClearInputMethodPidByPid(userId, callPid);
    it = tempPasteboard->imeMap_.Find(userId);
    hasPid = it.first;
    EXPECT_NE(hasPid, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetInputMethodPidTest006 end");
}

/**
 * @tc.name: ClearInputMethodPidByPidTest001
 * @tc.desc: test Func ClearInputMethodPidByPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ClearInputMethodPidByPidTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInputMethodPidByPidTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    pid_t callPid = 1;
    tempPasteboard->ClearInputMethodPidByPid(userId, callPid);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInputMethodPidByPidTest001 end");
}

/**
 * @tc.name: ClearInputMethodPidTest001
 * @tc.desc: test Func ClearInputMethodPid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ClearInputMethodPidTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInputMethodPidTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->ClearInputMethodPid();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInputMethodPidTest001 end");
}

/**
 * @tc.name: IsSystemAppByFullTokenIDTest001
 * @tc.desc: test Func IsSystemAppByFullTokenID
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsSystemAppByFullTokenIDTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = UINT32_ONE;
    tempPasteboard->IsSystemAppByFullTokenID(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest001 end");
}

/**
 * @tc.name: GetFocusedAppInfoTest001
 * @tc.desc: test Func GetFocusedAppInfo
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetFocusedAppInfoTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetFocusedAppInfoTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetFocusedAppInfo();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetFocusedAppInfoTest001 end");
}

/**
 * @tc.name: OnRemoteDiedTest001
 * @tc.desc: test Func OnRemoteDied
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnRemoteDiedTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnRemoteDiedTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnRemoteDiedTest003, TestSize.Level0)
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
 * @tc.name: ClearRemoteDataTask001
 * @tc.desc: test Func ClearRemoteDataTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ClearRemoteDataTask001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, WaitRemoteData001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, WaitRemoteData002, TestSize.Level0)
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
 * @tc.name: AddSysAbilityListenerTest001
 * @tc.desc: test Func AddSysAbilityListener
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, AddSysAbilityListenerTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddSysAbilityListenerTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->AddSysAbilityListener();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddSysAbilityListenerTest001 end");
}

/**
 * @tc.name: InitScreenStatusTest001
 * @tc.desc: test Func InitScreenStatus
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, InitScreenStatusTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "InitScreenStatusTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->InitScreenStatus();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "InitScreenStatusTest001 end");
}

/**
 * @tc.name: GetCurrentScreenStatusTest001
 * @tc.desc: test Func GetCurrentScreenStatus
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetCurrentScreenStatusTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentScreenStatusTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetCurrentScreenStatus();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentScreenStatusTest001 end");
}

/**
 * @tc.name: DumpHistoryTest001
 * @tc.desc: test Func DumpHistory
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DumpHistoryTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpHistoryTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->DumpHistory();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpHistoryTest001 end");
}

/**
 * @tc.name: DumpDataTest001
 * @tc.desc: test Func DumpData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DumpDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->DumpData();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DumpDataTest001 end");
}

/**
 * @tc.name: ThawInputMethodTest001
 * @tc.desc: test Func ThawInputMethod
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ThawInputMethodTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ThawInputMethodTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    pid_t callPid = 1;
    tempPasteboard->ThawInputMethod(callPid);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ThawInputMethodTest001 end");
}

/**
 * @tc.name: IsNeedThawTest001
 * @tc.desc: test Func IsNeedThaw
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsNeedThawTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsNeedThawTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->IsNeedThaw();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsNeedThawTest001 end");
}

/**
 * @tc.name: ExtractEntityTest001
 * @tc.desc: test Func ExtractEntity
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ExtractEntityTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string entity = "{\"code\": \"0\"}";
    std::string location = "location";
    tempPasteboard->ExtractEntity(entity, location);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest001 end");
}

/**
 * @tc.name: ExtractEntityTest002
 * @tc.desc: test Func ExtractEntity
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ExtractEntityTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest002 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string entity = R"({"code": 0})";
    std::string location = "";
    tempPasteboard->ExtractEntity(entity, location);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest002 end");
}

/**
 * @tc.name: ExtractEntityTest003
 * @tc.desc: test Func ExtractEntity
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ExtractEntityTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest003 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string entity = R"({"code": 0, "entity": {"name": "example"}})";
    std::string location = "";
    tempPasteboard->ExtractEntity(entity, location);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest003 end");
}

/**
 * @tc.name: ExtractEntityTest004
 * @tc.desc: test Func ExtractEntity
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ExtractEntityTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest004 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string entity = R"({"code": 0,"entity": {"location": "room1"}})";
    std::string location = "";
    tempPasteboard->ExtractEntity(entity, location);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest004 end");
}

/**
 * @tc.name: ExtractEntityTest005
 * @tc.desc: test Func ExtractEntity
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ExtractEntityTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest005 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string entity = R"({"code": 0,"entity": {"location": ["room1", "room2"]}})";
    std::string location = "";
    tempPasteboard->ExtractEntity(entity, location);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest005 end");
}

/**
 * @tc.name: InitServiceHandlerTest001
 * @tc.desc: test Func InitServiceHandler
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, InitServiceHandlerTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "InitServiceHandlerTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->InitServiceHandler();
    tempPasteboard->InitServiceHandler();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "InitServiceHandlerTest001 end");
}

/**
 * @tc.name: IsCopyableTest001
 * @tc.desc: test Func IsCopyable
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsCopyableTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCopyableTest001 start");
    auto tokenId = 123;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->IsCopyable(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCopyableTest001 end");
}

/**
 * @tc.name: SaveData001
 * @tc.desc: test Func SaveData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SaveData001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SaveData001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);

    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    int64_t dataSize = 0;
    tempPasteboard->SaveData(pasteData, dataSize, delayGetter, entryGetter);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SaveData001 end");
}

/**
 * @tc.name: HandleDelayDataAndRecord001
 * @tc.desc: test Func HandleDelayDataAndRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, HandleDelayDataAndRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleDelayDataAndRecord001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);

    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    AppInfo appInfo;
    tempPasteboard->HandleDelayDataAndRecord(pasteData, delayGetter, entryGetter, appInfo);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleDelayDataAndRecord001 end");
}

/**
 * @tc.name: RemovePasteDataTest001
 * @tc.desc: test Func RemovePasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RemovePasteDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemovePasteDataTest001 start");
    AppInfo appInfo;
    appInfo.tokenId = 123;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->RemovePasteData(appInfo);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemovePasteDataTest001 end");
}

/**
 * @tc.name: RemovePasteDataTest002
 * @tc.desc: test Func RemovePasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RemovePasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemovePasteDataTest002 start");
    AppInfo appInfo;
    appInfo.tokenId = 123;
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);

    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->RemovePasteData(appInfo);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemovePasteDataTest002 end");
}

/**
 * @tc.name: GetValidDistributeEventTest001
 * @tc.desc: test Func GetValidDistributeEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetValidDistributeEventTest001, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, GetValidDistributeEventTest002, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, GetValidDistributeEventTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t user = 1234;

    tempPasteboard->GetValidDistributeEvent(user);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetValidDistributeEventTest003 end");
}

/**
 * @tc.name: GetSdkVersionTest001
 * @tc.desc: test Func GetSdkVersion
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetSdkVersionTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetSdkVersionTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetCommonStateTest001, TestSize.Level0)
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
 * @tc.name: SetRadarEventTest001
 * @tc.desc: test Func SetRadarEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetRadarEventTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, SetUeEventTest001, TestSize.Level0)
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
 * @tc.name: PostProcessDelayHtmlEntryTest001
 * @tc.desc: test Func PostProcessDelayHtmlEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PostProcessDelayHtmlEntryTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    AppInfo targetAppInfo;
    PasteDataEntry entry;
    tempPasteboard->PostProcessDelayHtmlEntry(pasteData, targetAppInfo, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest001 end");
}

/**
 * @tc.name: PostProcessDelayHtmlEntryTest002
 * @tc.desc: test Func PostProcessDelayHtmlEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PostProcessDelayHtmlEntryTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest002 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    AppInfo targetAppInfo;
    PasteDataEntry entry;
    entry.SetValue("test");
    tempPasteboard->PostProcessDelayHtmlEntry(pasteData, targetAppInfo, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest002 end");
}

/**
 * @tc.name: PostProcessDelayHtmlEntryTest003
 * @tc.desc: test Func PostProcessDelayHtmlEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PostProcessDelayHtmlEntryTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest003 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    AppInfo targetAppInfo;
    PasteDataEntry entry;
    auto object = std::make_shared<Object>();
    EXPECT_NE(object, nullptr);

    object->value_[UDMF::HTML_CONTENT] = "<div class='disable'>helloWorld</div>";
    entry.SetValue(object);
    tempPasteboard->PostProcessDelayHtmlEntry(pasteData, targetAppInfo, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest003 end");
}

/**
 * @tc.name: PostProcessDelayHtmlEntryTest004
 * @tc.desc: test Func PostProcessDelayHtmlEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PostProcessDelayHtmlEntryTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest004 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    PasteData pasteData;
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    AppInfo targetAppInfo;
    PasteDataEntry entry;
    entry.SetValue(1);
    tempPasteboard->PostProcessDelayHtmlEntry(pasteData, targetAppInfo, entry);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PostProcessDelayHtmlEntryTest004 end");
}

/**
 * @tc.name: GrantUriPermissionTest001
 * @tc.desc: test Func GrantUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GrantUriPermissionTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest001 start");
    PasteboardService service;
    std::vector<Uri> emptyUris;
    std::string targetBundleName = "com.example.app";
    int32_t appIndex = 1;
    int32_t result = service.GrantUriPermission(emptyUris, targetBundleName, false, appIndex);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GrantUriPermissionTest001 end");
}

/**
 * @tc.name: GenerateDistributedUriTest001
 * @tc.desc: test Func GenerateDistributedUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GenerateDistributedUriTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest001 start");
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->GenerateDistributedUri(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest001 end");
}

/**
 * @tc.name: GenerateDistributedUriTest002
 * @tc.desc: test Func GenerateDistributedUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GenerateDistributedUriTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest002 start");
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    pasteData.AddTextRecord("testRecord");
    pasteData.AddTextRecord(TEST_ENTITY_TEXT);
    pasteData.AddTextRecord("testRecord");
    OHOS::Uri uri("/");
    pasteData.AddUriRecord(uri);
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");

    tempPasteboard->GenerateDistributedUri(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest002 end");
}

/**
 * @tc.name: GenerateDistributedUriTest003
 * @tc.desc: test Func GenerateDistributedUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GenerateDistributedUriTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest003 start");
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");
    pasteData.AddTextRecord("testRecord");
    pasteData.AddTextRecord(TEST_ENTITY_TEXT);
    pasteData.AddTextRecord("testRecord");
    pasteData.AddHtmlRecord("<div class='disable'>helloWorld</div>");

    tempPasteboard->GenerateDistributedUri(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GenerateDistributedUriTest003 end");
}

/**
 * @tc.name: IsBundleOwnUriPermission001
 * @tc.desc: test Func IsBundleOwnUriPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsBundleOwnUriPermission001, TestSize.Level0)
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
 * @tc.name: GetAppLabelTest001
 * @tc.desc: test Func GetAppLabel
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetAppLabelTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetAppBundleManagerTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleManagerTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard->GetAppBundleManager(), nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleManagerTest001 end");
}

/**
 * @tc.name: EstablishP2PLink001
 * @tc.desc: test Func EstablishP2PLink
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, EstablishP2PLink001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLink001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string networkId = "networkId";
    std::string pasteId = "pasteId";
    tempPasteboard->EstablishP2PLink(networkId, pasteId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLink001 end");
}

/**
 * @tc.name: EstablishP2PLinkTaskTest001
 * @tc.desc: test Func EstablishP2PLinkTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, EstablishP2PLinkTaskTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLinkTaskTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string pasteId;
    ClipPlugin::GlobalEvent event{};
    tempPasteboard->EstablishP2PLinkTask(pasteId, event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLinkTaskTest001 end");
}

/**
 * @tc.name: OnEstablishP2PLinkTaskTest001
 * @tc.desc: test Func OnEstablishP2PLinkTask
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnEstablishP2PLinkTaskTest001, TestSize.Level0)
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
 * @tc.name: CloseP2PLink001
 * @tc.desc: test Func CloseP2PLink
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CloseP2PLink001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseP2PLink001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    std::string networkId = "networkId";
    tempPasteboard->CloseP2PLink(networkId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseP2PLink001 end");
}

/**
 * @tc.name: ProcessDistributedDelayUriTest001
 * @tc.desc: test Func ProcessDistributedDelayUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ProcessDistributedDelayUriTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessDistributedDelayUriTest001 start");
    int32_t userId = ACCOUNT_IDS_RANDOM;
    PasteData data;
    PasteDataEntry entry;
    std::vector<uint8_t> rawData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ProcessDistributedDelayUri(userId, data, entry, rawData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessDistributedDelayUriTest001 end");
}

/**
 * @tc.name: ProcessDistributedDelayHtmlTest001
 * @tc.desc: test Func ProcessDistributedDelayHtml
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ProcessDistributedDelayHtmlTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessDistributedDelayHtmlTest001 start");
    PasteData data;
    PasteDataEntry entry;
    std::vector<uint8_t> rawData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ProcessDistributedDelayHtml(data, entry, rawData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessDistributedDelayHtmlTest001 end");
}

/**
 * @tc.name: ProcessDistributedDelayEntryTest001
 * @tc.desc: test Func ProcessDistributedDelayEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ProcessDistributedDelayEntryTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessDistributedDelayEntryTest001 start");
    PasteDataEntry pasteData;
    std::vector<uint8_t> rawData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ProcessDistributedDelayEntry(pasteData, rawData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ProcessDistributedDelayEntryTest001 end");
}

/**
 * @tc.name: ProcessRemoteDelayHtmlTest001
 * @tc.desc: test Func ProcessRemoteDelayHtml
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ProcessRemoteDelayHtmlTest001, TestSize.Level0)
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
 * @tc.name: IsDisallowDistributedTest
 * @tc.desc: test Func IsDisallowDistributed, Check CallingUID contral collaboration.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsDisallowDistributedTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDisallowDistributedTest start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);
    EXPECT_EQ(tempPasteboard->IsDisallowDistributed(), false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDisallowDistributedTest end");
}

/**
 * @tc.name: CleanDistributedDataTest001
 * @tc.desc: test Func CleanDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CleanDistributedDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CleanDistributedDataTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t user = ACCOUNT_IDS_RANDOM;
    tempPasteboard->CleanDistributedData(user);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CleanDistributedDataTest001 end");
}

/**
 * @tc.name: OnConfigChangeTest001
 * @tc.desc: test Func OnConfigChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnConfigChangeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnConfigChangeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->OnConfigChange(true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnConfigChangeTest001 end");
}

/**
 * @tc.name: IncreaseChangeCountTest001
 * @tc.desc: test Func IncreaseChangeCount, should reset to 0 after reach maximum limit.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IncreaseChangeCountTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IncreaseChangeCountTest002, TestSize.Level0)
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
 * @tc.name: GetTimeTest001
 * @tc.desc: test Func GetTime
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetTimeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetTimeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->GetTime();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetTimeTest001 end");
}

/**
 * @tc.name: IsDataAgedTest001
 * @tc.desc: test Func IsDataAged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsDataAgedTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsDataAgedTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsDataAgedTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsDataAgedTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest004 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(PasteboardTime::GetBootTimeMs());
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
HWTEST_F(PasteboardServiceTest, IsDataAgedTest005, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsDataAgedTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest006 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto userId = tempPasteboard->GetCurrentAccountId();
    auto curTime = static_cast<uint64_t>(PasteboardTime::GetBootTimeMs());
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
 * @tc.name: VerifyPermissionTest001
 * @tc.desc: test Func VerifyPermission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, VerifyPermissionTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "VerifyPermissionTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = 0x123456;
    tempPasteboard->VerifyPermission(tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "VerifyPermissionTest001 end");
}

/**
 * @tc.name: IsDataValidTest001
 * @tc.desc: test Func IsDataValid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsDataValidTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsDataValidTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsDataValidTest003, TestSize.Level0)
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
 * @tc.name: GetAppInfoTest001
 * @tc.desc: test Func GetAppInfo
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetAppInfoTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetAppBundleNameTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetAppBundleNameTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetAppBundleNameTest003, TestSize.Level0)
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
 * @tc.name: SetLocalPasteFlag001
 * @tc.desc: test Func SetLocalPasteFlag
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetLocalPasteFlag001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlag001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    bool isCrossPaste = false;
    uint32_t tokenId = 0x123456;
    PasteData pasteData;
    tempPasteboard->SetLocalPasteFlag(isCrossPaste, tokenId, pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlag001 end");
}

/**
 * @tc.name: RecognizePasteDataTest001
 * @tc.desc: test Func RecognizePasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RecognizePasteDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest001 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData pasteData;
    tempPasteboard->RecognizePasteData(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest001 end");
}

/**
 * @tc.name: ShowHintToastTest001
 * @tc.desc: test Func ShowHintToast
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ShowHintToastTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ShowHintToastTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = UINT32_ONE;
    uint32_t pid = 0;
    tempPasteboard->ShowHintToast(tokenId, pid);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ShowHintToastTest001 end");
}

/**
 * @tc.name: OnAddSystemAbilityTest001
 * @tc.desc: test Func OnAddSystemAbility
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnAddSystemAbilityTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnAddSystemAbilityTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnAddSystemAbilityTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnRemoveSystemAbilityTest001, TestSize.Level0)
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
 * @tc.name: CancelCriticalTimerTest001
 * @tc.desc: test Func CancelCriticalTimer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CancelCriticalTimerTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CancelCriticalTimerTest001 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddMemoryManager();
    tempPasteboard->CancelCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CancelCriticalTimerTest001 end");
}

/**
 * @tc.name: CancelCriticalTimerTest002
 * @tc.desc: test Func CancelCriticalTimer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CancelCriticalTimerTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CancelCriticalTimerTest002 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ffrtTimer_ = nullptr;
    tempPasteboard->CancelCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CancelCriticalTimerTest002 end");
}

/**
 * @tc.name: SetCriticalTimerTest001
 * @tc.desc: test Func SetCriticalTimer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetCriticalTimerTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest001 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddMemoryManager();
    tempPasteboard->SetCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest001 end");
}

/**
 * @tc.name: SetCriticalTimerTest002
 * @tc.desc: test Func SetCriticalTimer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetCriticalTimerTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest002 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->ffrtTimer_ = nullptr;
    tempPasteboard->SetCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest002 end");
}

/**
 * @tc.name: SetCriticalTimerTest003
 * @tc.desc: test Func SetCriticalTimer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetCriticalTimerTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest003 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddMemoryManager();
    tempPasteboard->isCritical_ = true;
    tempPasteboard->SetCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest003 end");
}

/**
 * @tc.name: SetCriticalTimerTest004
 * @tc.desc: test Func SetCriticalTimer
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetCriticalTimerTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest004 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddMemoryManager();
    tempPasteboard->isCritical_ = false;
    tempPasteboard->SetCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCriticalTimerTest004 end");
}

/**
 * @tc.name: OnAddDeviceManagerTest001
 * @tc.desc: test Func OnAddDeviceManager
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnAddDeviceManagerTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnAddMemoryManagerTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddMemoryManagerTest001 start");
    constexpr int32_t userId = 111;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->OnAddMemoryManager();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAddMemoryManagerTest001 end");
}

/**
 * @tc.name: ReportUeCopyEventTest001
 * @tc.desc: test Func ReportUeCopyEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ReportUeCopyEventTest001, TestSize.Level0)
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
 * @tc.name: HasPasteDataTest001
 * @tc.desc: test Func HasPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, HasPasteDataTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasPasteDataTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasPasteDataTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasPasteDataTest004, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, HasPasteDataTest005, TestSize.Level0)
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
 * @tc.name: IsRemoteDataTest001
 * @tc.desc: test Func IsRemoteData, funcResult is false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsRemoteDataTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsRemoteDataTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsRemoteDataTest003, TestSize.Level0)
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
 * @tc.name: DealDataTest001
 * @tc.desc: test Func DealData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, DealDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest001 start");
    auto service = std::make_shared<PasteboardService>();
    int fd = -1;
    int64_t size;
    std::vector<uint8_t> rawData;
    PasteData data;
    int32_t result = service->DealData(fd, size, rawData, data);
    EXPECT_EQ(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DealDataTest001 end");
}

/**
 * @tc.name: WriteRawDataTest001
 * @tc.desc: test Func WriteRawData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, WriteRawDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest001 start");
    auto service = std::make_shared<PasteboardService>();
    char rawData[] = "testData";
    int32_t fd = INT32_NEGATIVE_NUMBER;
    bool result = service->WriteRawData(rawData, sizeof(rawData), fd);
    EXPECT_EQ(result, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest001 end");
}

/**
 * @tc.name: WriteRawDataTest002
 * @tc.desc: test Func WriteRawData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, WriteRawDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int serFd = 0;
    int64_t size = 10;

    bool result = tempPasteboard->WriteRawData(nullptr, size, serFd);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest002 end");
}

/**
 * @tc.name: WriteRawDataTest003
 * @tc.desc: test Func WriteRawData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, WriteRawDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int serFd = 0;
    char data[10];

    bool result = tempPasteboard->WriteRawData(data, 0, serFd);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WriteRawDataTest003 end");
}

/**
 * @tc.name: WritePasteDataTest001
 * @tc.desc: test Func WritePasteDataTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, WritePasteDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WritePasteDataTest001 start");
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    PasteData output;
    bool hasData = false;

    PasteData input;
    input.AddTextRecord("test");
    bool result = input.Encode(buffer);
    ASSERT_TRUE(result);

    rawDataSize = static_cast<int64_t>(buffer.size());
    MessageParcelWarp messageData;
    ASSERT_TRUE(rawDataSize <= MIN_ASHMEM_DATA_SIZE);

    fd = messageData.CreateTmpFd();
    ASSERT_TRUE(fd >= 0);

    auto tempPasteboard = std::make_shared<PasteboardService>();
    auto ret = tempPasteboard->WritePasteData(fd, rawDataSize, buffer, output, hasData);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WritePasteDataTest001 end");
}

/**
 * @tc.name: WritePasteDataTest002
 * @tc.desc: test Func WritePasteDataTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, WritePasteDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WritePasteDataTest002 start");
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    PasteData output;
    bool hasData = false;

    PasteData input;
    std::string text = "test";
    for (int i = 0; i < INT_THREETHREETHREE; ++i) {
        text += TEST_ENTITY_TEXT;
    }
    input.AddTextRecord(text);
    bool result = input.Encode(buffer);
    ASSERT_TRUE(result);

    rawDataSize = static_cast<int64_t>(buffer.size());
    MessageParcelWarp messageData;
    MessageParcel parcelData;
    ASSERT_TRUE(rawDataSize > MIN_ASHMEM_DATA_SIZE);

    result = messageData.WriteRawData(parcelData, buffer.data(), buffer.size());
    ASSERT_TRUE(result);

    fd = messageData.GetWriteDataFd();
    buffer.clear();
    auto tempPasteboard = std::make_shared<PasteboardService>();
    auto ret = tempPasteboard->WritePasteData(fd, rawDataSize, buffer, output, hasData);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::E_OK), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WritePasteDataTest002 end");
}

/**
 * @tc.name: WritePasteDataTest003
 * @tc.desc: test Func WritePasteDataTest
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceTest, WritePasteDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WritePasteDataTest003 start");
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    PasteData output;
    bool hasData = false;

    PasteData input;
    std::string text = "test";
    for (int i = 0; i < INT_THREETHREETHREE; ++i) {
        text += TEST_ENTITY_TEXT;
    }
    input.AddTextRecord(text);
    bool result = input.Encode(buffer);
    ASSERT_TRUE(result);

    rawDataSize = static_cast<int64_t>(buffer.size() + 1);
    MessageParcelWarp messageData;
    MessageParcel parcelData;
    ASSERT_TRUE(rawDataSize > MIN_ASHMEM_DATA_SIZE);

    result = messageData.WriteRawData(parcelData, buffer.data(), buffer.size());
    ASSERT_TRUE(result);

    fd = messageData.GetWriteDataFd();
    buffer.clear();
    auto tempPasteboard = std::make_shared<PasteboardService>();
    auto ret = tempPasteboard->WritePasteData(fd, rawDataSize, buffer, output, hasData);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "WritePasteDataTest003 end");
}

/**
 * @tc.name: GetLocalMimeTypesTest001
 * @tc.desc: test Func GetLocalMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetLocalMimeTypesTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetLocalMimeTypesTest002, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, GetLocalMimeTypesTest003, TestSize.Level1)
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
 * @tc.name: HasLocalDataTypeTest001
 * @tc.desc: test Func HasLocalDataType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, HasLocalDataTypeTest001, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, HasLocalDataTypeTest002, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, HasLocalDataTypeTest003, TestSize.Level1)
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
HWTEST_F(PasteboardServiceTest, HasLocalDataTypeTest004, TestSize.Level1)
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

/**
 * @tc.name: AddPermissionRecordTest001
 * @tc.desc: test Func AddPermissionRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, AddPermissionRecordTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddPermissionRecordTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    uint32_t tokenId = 0x123456;
    tempPasteboard->AddPermissionRecord(tokenId, true, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddPermissionRecordTest001 end");
}

/**
 * @tc.name: IsAllowSendDataTest001
 * @tc.desc: test Func IsAllowSendData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsAllowSendDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsAllowSendDataTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->IsAllowSendData();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsAllowSendDataTest001 end");
}

/**
 * @tc.name: UpdateShareOptionTest001
 * @tc.desc: test Func UpdateShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, UpdateShareOptionTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UpdateShareOptionTest001 start");
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->UpdateShareOption(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UpdateShareOptionTest001 end");
}

/**
 * @tc.name: CheckMdmShareOptionTest001
 * @tc.desc: test Func CheckMdmShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CheckMdmShareOptionTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CheckMdmShareOptionTest001 start");
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->CheckMdmShareOption(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CheckMdmShareOptionTest001 end");
}

/**
 * @tc.name: IsBasicTypeTest001
 * @tc.desc: test Func IsBasicType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, IsBasicTypeTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsBasicTypeTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsBasicTypeTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsBasicTypeTest004, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, IsBasicTypeTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest005 start");
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    std::string mimeType = "text/html";
    tempPasteboard->IsBasicType(mimeType);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBasicTypeTest005 end");
}

/**
 * @tc.name: RemotePasteboardChangeTest001
 * @tc.desc: test Func RemotePasteboardChange
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RemotePasteboardChangeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemotePasteboardChangeTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto result = tempPasteboard->RemotePasteboardChange();
    EXPECT_NE(result, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RemotePasteboardChangeTest001 end");
}

/**
 * @tc.name: AppExitTest001
 * @tc.desc: test Func AppExit
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, AppExitTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AppExitTest001 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    pid_t pid = 3;
    tempPasteboard->AppExit(pid);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AppExitTest001 end");
}

/**
 * @tc.name: AppExitTest002
 * @tc.desc: test Func AppExit
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, AppExitTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AppExitTest002 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t pid = 1234;
    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = pid;
    p2pInfo.isSuccess = false;

    std::string networkId = "networkId1";
    std::string key = "key1";
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(key, p2pInfo);
    tempPasteboard->p2pMap_.Insert(networkId, p2pMap);
    int32_t ret = tempPasteboard->AppExit(pid);
    EXPECT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AppExitTest002 end");
}

/**
 * @tc.name: AppExitTest003
 * @tc.desc: test Func AppExit
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, AppExitTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AppExitTest003 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t pid = 1234;
    PasteboardService::PasteboardP2pInfo p2pInfo;
    p2pInfo.callPid = pid;
    p2pInfo.isSuccess = false;

    std::string networkId = "networkId1";
    std::string key = "key1";
    ConcurrentMap<std::string, PasteboardService::PasteboardP2pInfo> p2pMap;
    p2pMap.Insert(key, p2pInfo);
    tempPasteboard->p2pMap_.Insert(networkId, p2pMap);
    p2pMap.Clear();

    p2pInfo.callPid = pid + 1;
    p2pMap.Insert(key, p2pInfo);
    tempPasteboard->p2pMap_.Insert(networkId, p2pMap);
    int32_t ret = tempPasteboard->AppExit(pid);
    EXPECT_EQ(ret, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AppExitTest003 end");
}

/**
 * @tc.name: OnReceiveEventTest001
 * @tc.desc: test Func OnReceiveEvent
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, OnReceiveEventTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnReceiveEventTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnReceiveEventTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnReceiveEventTest004, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnReceiveEventTest005, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnStateChangedTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnStateChangedTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnStateChangedTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, OnStateChangedTest004, TestSize.Level0)
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
 * @tc.name: RegisterClientDeathObserverTest001
 * @tc.desc: test Func RegisterClientDeathObserver
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, RegisterClientDeathObserverTest001, TestSize.Level0)
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

/**
 * @tc.name: GetDelayPasteRecord002
 * @tc.desc: GetDelayPasteRecord002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetDelayPasteRecord002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetDelayPasteRecord003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, GetDelayPasteRecord004, TestSize.Level0)
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
 * @tc.name: CloseP2PLink002
 * @tc.desc: CloseP2PLink002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, CloseP2PLink002, TestSize.Level0)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    ASSERT_NE(tempPasteboard, nullptr);
    std::string networkId = "networkId";
    tempPasteboard->CloseP2PLink(networkId);
    EXPECT_NE(networkId, "");
#endif
}

/**
 * @tc.name: SaveData002
 * @tc.desc: SaveData002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SaveData002, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData pasteData;
    sptr<PasteboardDelayGetterImpl> delayGetter = nullptr;
    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    ASSERT_NE(entryGetter, nullptr);
    int64_t dataSize = 0;
    int32_t ret = tempPasteboard->SaveData(pasteData, dataSize, delayGetter, entryGetter);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: SaveData003
 * @tc.desc: SaveData003
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SaveData003, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData pasteData;
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    ASSERT_NE(delayGetter, nullptr);
    sptr<PasteboardEntryGetterImpl> entryGetter = nullptr;
    int64_t dataSize = 0;
    int32_t ret = tempPasteboard->SaveData(pasteData, dataSize, delayGetter, entryGetter);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: SaveData004
 * @tc.desc: SaveData004
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SaveData004, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    PasteData pasteData;
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    ASSERT_NE(delayGetter, nullptr);
    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    ASSERT_NE(entryGetter, nullptr);
    int64_t dataSize = INT64_MAX;
    int32_t ret = tempPasteboard->SaveData(pasteData, dataSize, delayGetter, entryGetter);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetCommonStateTest002
 * @tc.desc: GetCommonStateTest002
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetCommonStateTest002, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int64_t dataSize = INT64_MAX;
    CommonInfo info = tempPasteboard->GetCommonState(dataSize);
    EXPECT_EQ(info.dataSize, dataSize);
}

/**
 * @tc.name: ProcessRemoteDelayHtmlInnerTest001
 * @tc.desc: ProcessRemoteDelayHtmlInnerTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ProcessRemoteDelayHtmlInnerTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, ProcessRemoteDelayHtmlInnerTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceTest, ProcessRemoteDelayHtmlTest002, TestSize.Level0)
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
 * @tc.name: GetRemoteEntryValueTest001
 * @tc.desc: GetRemoteEntryValueTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, GetRemoteEntryValueTest001, TestSize.Level0)
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
 * @tc.name: ProcessRemoteDelayUriTest001
 * @tc.desc: ProcessRemoteDelayUriTest001
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ProcessRemoteDelayUriTest001, TestSize.Level0)
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

/**
 * @tc.name: ClearAgedDataTest001
 * @tc.desc: Test ClearAgedData function to clear expired data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, ClearAgedDataTest001, TestSize.Level0)
{
    std::shared_ptr<PasteboardService> tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = tempPasteboard->GetAppInfo(tokenId);
    int32_t userId = appInfo.userId;
    tempPasteboard->ffrtTimer_ = nullptr;
    tempPasteboard->SetDataExpirationTimer(userId);
    tempPasteboard->ffrtTimer_ = std::make_shared<FFRTTimer>();
    tempPasteboard->SetDataExpirationTimer(userId);
    std::shared_ptr<PasteData> testData = std::make_shared<PasteData>();
    tempPasteboard->clips_.InsertOrAssign(userId, testData);
    auto result = tempPasteboard->clips_.Find(userId);
    EXPECT_TRUE(result.first);
    EXPECT_NE(result.second, nullptr);
    tempPasteboard->ClearAgedData(userId);
    result = tempPasteboard->clips_.Find(userId);
    EXPECT_FALSE(result.first);
}

/**
 * @tc.name: SetCurrentDistributedDataTest001
 * @tc.desc: test Func SetCurrentDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, SetCurrentDistributedDataTest001, TestSize.Level0)
{
    PasteData pasteData;
    ClipPlugin::GlobalEvent event{};
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    tempPasteboard->SetCurrentDistributedData(pasteData, event);
}


} // namespace MiscServices
} // namespace OHOS