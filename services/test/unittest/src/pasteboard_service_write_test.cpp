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
const int32_t INT32_NEGATIVE_NUMBER = -1;
const int INT_THREETHREETHREE = 333;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
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

class PasteboardServiceWriteTest : public testing::Test {
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

void PasteboardServiceWriteTest::SetUpTestCase(void) { }

void PasteboardServiceWriteTest::TearDownTestCase(void) { }

void PasteboardServiceWriteTest::SetUp(void) { }

void PasteboardServiceWriteTest::TearDown(void) { }

int32_t PasteboardServiceWriteTest::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
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
 * @tc.name: WriteRawDataTest001
 * @tc.desc: test Func WriteRawData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceWriteTest, WriteRawDataTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceWriteTest, WriteRawDataTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceWriteTest, WriteRawDataTest003, TestSize.Level0)
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
HWTEST_F(PasteboardServiceWriteTest, WritePasteDataTest001, TestSize.Level0)
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
HWTEST_F(PasteboardServiceWriteTest, WritePasteDataTest002, TestSize.Level0)
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
HWTEST_F(PasteboardServiceWriteTest, WritePasteDataTest003, TestSize.Level0)
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

} // namespace MiscServices
} // namespace OHOS