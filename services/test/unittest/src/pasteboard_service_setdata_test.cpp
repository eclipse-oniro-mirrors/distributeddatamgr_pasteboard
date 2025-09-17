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

class PasteboardServiceSetDataTest : public testing::Test {
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

void PasteboardServiceSetDataTest::SetUpTestCase(void) { }

void PasteboardServiceSetDataTest::TearDownTestCase(void) { }

void PasteboardServiceSetDataTest::SetUp(void) { }

void PasteboardServiceSetDataTest::TearDown(void) { }

int32_t PasteboardServiceSetDataTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: SetPasteDataTest001
 * @tc.desc: test Func SetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest001 start");
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);
    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t result = tempPasteboard->SetPasteData(dup(fd), rawDataSize, buffer, delayGetter, entryGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest001 end");
}

/**
 * @tc.name: SetPasteDataTest002
 * @tc.desc: test Func SetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest002 start");
    int fd = -1;
    int64_t rawDataSize = DEFAULT_MAX_RAW_DATA_SIZE + 1;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);
    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t result = tempPasteboard->SetPasteData(dup(fd), rawDataSize, buffer, delayGetter, entryGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest002 end");
}

/**
 * @tc.name: SetPasteDataTest003
 * @tc.desc: test Func SetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest003 start");
    int fd = -1;
    int64_t rawDataSize = DEFAULT_MAX_RAW_DATA_SIZE;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);
    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t result = tempPasteboard->SetPasteData(dup(fd), rawDataSize, buffer, delayGetter, entryGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest003 end");
}

/**
 * @tc.name: SetPasteDataTest004
 * @tc.desc: test Func SetPasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest004 start");
    int fd = -1;
    int64_t rawDataSize = 1;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<PasteboardDelayGetterImpl> delayGetter = sptr<PasteboardDelayGetterImpl>::MakeSptr();
    EXPECT_NE(delayGetter, nullptr);
    sptr<PasteboardEntryGetterImpl> entryGetter = sptr<PasteboardEntryGetterImpl>::MakeSptr();
    EXPECT_NE(entryGetter, nullptr);

    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);
    int32_t result = tempPasteboard->SetPasteData(dup(fd), rawDataSize, buffer, delayGetter, entryGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest004 end");
}

/**
 * @tc.name: SetPasteDataTest005
 * @tc.desc: test Func SetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest005 start");
    auto tempPasteboard = std::make_shared<PasteboardService>();
    int32_t syncTime = 0;
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> recvTLV;
    auto ret = tempPasteboard->SetPasteData(dup(fd), rawDataSize, recvTLV, nullptr, nullptr);
    EXPECT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataTest005 end");
}

/**
 * @tc.name: SetPasteDataDelayDataTest001
 * @tc.desc: test Func SetPasteDataDelayData, return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDelayDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest001 start");
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataDelayData(dup(fd), rawDataSize, buffer, delayGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest001 end");
}

/**
 * @tc.name: SetPasteDataDelayDataTest002
 * @tc.desc: test Func SetPasteDataDelayData, return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDelayDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest002 start");
    int fd = -1;
    int64_t rawDataSize = INT64_MAX;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataDelayData(dup(fd), rawDataSize, buffer, delayGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest002 end");
}

/**
 * @tc.name: SetPasteDataDelayDataTest003
 * @tc.desc: test Func SetPasteDataDelayData, fd is error, map failed, return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDelayDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest003 start");
    int fd = -1;
    int64_t rawDataSize = USHRT_MAX;
    std::vector<uint8_t> buffer = { 'h', 'e', 'l', 'l', 'o' };
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataDelayData(dup(fd), rawDataSize, buffer, delayGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest003 end");
}

/**
 * @tc.name: SetPasteDataDelayDataTest004
 * @tc.desc: test Func SetPasteDataDelayData, fd is right, but buffer is empty, return NO_DATA_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDelayDataTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest004 start");
    int fd = 0XF;
    int64_t rawDataSize = USHRT_MAX;
    std::vector<uint8_t> buffer;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataDelayData(dup(fd), rawDataSize, buffer, delayGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest004 end");
}

/**
 * @tc.name: SetPasteDataDelayDataTest005
 * @tc.desc: test Func SetPasteDataDelayData, NOT goto map and buffer is empty, return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDelayDataTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest005 start");
    int fd = -1;
    int64_t rawDataSize = UINT8_MAX;
    std::vector<uint8_t> buffer;
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataDelayData(dup(fd), rawDataSize, buffer, delayGetter);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest005 end");
}

/**
 * @tc.name: SetPasteDataDelayDataTest006
 * @tc.desc: test Func SetPasteDataDelayData, NOT goto map and buffer is NOT empty, return ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDelayDataTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest006 start");
    int fd = -1;
    int64_t rawDataSize = UINT8_MAX;
    std::vector<uint8_t> buffer { 'h', 'e', 'l', 'l', 'o' };
    sptr<IPasteboardDelayGetter> delayGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataDelayData(dup(fd), rawDataSize, buffer, delayGetter);
    EXPECT_NE(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDelayDataTest006 end");
}

/**
 * @tc.name: SetPasteDataEntryDataTest001
 * @tc.desc: test Func SetPasteDataEntryData, NOT goto map and buffer is NOT empty.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataEntryDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataEntryDataTest001 start");
    int fd = -1;
    int64_t rawDataSize = UINT8_MAX;
    std::vector<uint8_t> buffer { 'h', 'e', 'l', 'l', 'o' };
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    int32_t result = tempPasteboard->SetPasteDataEntryData(dup(fd), rawDataSize, buffer, entryGetter);
    EXPECT_NE(result, ERR_OK);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataEntryDataTest001 end");
}

/**
 * @tc.name: SetPasteDataOnlyTest001
 * @tc.desc: test Func SetPasteDataOnly, it will be return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataOnlyTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataOnlyTest001 start");
    auto mpw = std::make_shared<MessageParcelWarp>();
    EXPECT_NE(mpw, nullptr);

    auto service = std::make_shared<PasteboardService>();
    EXPECT_NE(service, nullptr);

    int64_t rawDataSize = 0;
    std::vector<uint8_t> buffer;
    int fd = mpw->CreateTmpFd();
    int32_t result = service->SetPasteDataOnly(dup(fd), rawDataSize, buffer);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    mpw->writeRawDataFd_ = -1;
    close(fd);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataOnlyTest001 end");
}

/**
 * @tc.name: SetPasteDataInfoTest001
 * @tc.desc: test Func SetPasteDataInfo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataInfoTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataInfoTest001 start");
    std::string bundleName = "com.pastboard.test";
    int32_t appIndex = 1;
    PasteData pasteData;
    AppInfo appInfo;
    appInfo.bundleName = bundleName;
    appInfo.appIndex = appIndex;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->SetPasteDataInfo(pasteData, appInfo);
    std::pair<std::string, int32_t> originAuthority;
    originAuthority.first = bundleName;
    originAuthority.second = appIndex;
    EXPECT_EQ(pasteData.GetBundleName(), bundleName);
    EXPECT_EQ(pasteData.GetAppIndex(), appIndex);
    EXPECT_EQ(pasteData.GetOriginAuthority(), originAuthority);
    EXPECT_EQ(pasteData.GetDataId(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataInfoTest001 end");
}

/**
 * @tc.name: SetPasteDataDotTest001
 * @tc.desc: test Func SetPasteDataDot
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetPasteDataDotTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDotTest001 start");
    PasteData pasteData;
    int32_t userId = 0;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->SetPasteDataDot(pasteData, userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetPasteDataDotTest001 end");
}

/**
 * @tc.name: SetDistributedDataTest001
 * @tc.desc: test Func SetDistributedData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetDistributedDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetDistributedDataTest001 start");
    int32_t user = ACCOUNT_IDS_RANDOM;
    PasteData pasteData;
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->SetDistributedData(user, pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetDistributedDataTest001 end");
}

/**
 * @tc.name: SetCurrentDataTest001
 * @tc.desc: test Func SetCurrentData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceSetDataTest, SetCurrentDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCurrentDataTest001 start");
    PasteData pasteData;
    ClipPlugin::GlobalEvent event {};
    auto tempPasteboard = std::make_shared<PasteboardService>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->SetCurrentData(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetCurrentDataTest001 end");
}

} // namespace MiscServices
} // namespace OHOS