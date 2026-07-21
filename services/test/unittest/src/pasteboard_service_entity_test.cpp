/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "pasteboard_service.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
const std::string TEST_ENTITY_TEXT =
    "清晨，从杭州市中心出发，沿着湖滨路缓缓前行。湖滨路是杭州市中心通往西湖的主要街道之一，两旁绿树成荫，湖光山色尽收眼"
    "底。你可以选择步行或骑行，感受微风拂面的惬意。湖滨路的尽头是南山路，这里有一片开阔的广场，是欣赏西湖全景的绝佳位置"
    "。进入南山路后，继续前行，雷峰塔的轮廓会逐渐映入眼帘。雷峰塔是西湖的标志性建筑之一，矗立在南屏山下，与西湖相映成趣"
    "。你可以在这里稍作停留，欣赏塔的雄伟与湖水的柔美。南山路两旁有许多咖啡馆和餐厅，是补充能量的好去处。离开雷峰塔，沿"
    "着南山路继续前行，你会看到一条蜿蜒的堤岸——杨公堤。杨公堤是西湖十景之一，堤岸两旁种满了柳树和桃树，春夏之交，柳绿桃"
    "红，美不胜收。你可以选择沿着堤岸漫步，感受湖水的宁静与柳树的轻柔。杨公堤的尽头是湖心亭，这里是西湖的中心地带，也是"
    "观赏西湖全景的最佳位置之一。从湖心亭出发，沿着湖畔步行至北山街。北山街是西湖北部的一条主要街道，两旁有许多历史建筑"
    "和文化遗址。继续前行，你会看到保俶塔矗立在宝石流霞景区。保俶塔是西湖的另一座标志性建筑，与雷峰塔遥相呼应，形成"一"
    "南一北"的独特景观。离开保俶塔，沿着北山街继续前行，你会到达断桥。断桥是西湖十景之一，冬季可欣赏断桥残雪的美景。断"
    "桥的两旁种满了柳树，湖水清澈见底，是拍照留念的好地方。断桥的尽头是平湖秋月，这里是观赏西湖夜景的绝佳地点，夜晚灯光"
    "亮起时，湖面倒映着月光，美轮美奂。游览结束后，沿着湖畔返回杭州市中心。沿途可以再次欣赏西湖的湖光山色，感受大自然的"
    "和谐与宁静。如果你时间充裕，可以选择在湖畔的咖啡馆稍作休息，回味这一天的旅程。这条路线涵盖了西湖的主要经典景点，从"
    "湖滨路到南山路，再到杨公堤、北山街，最后回到杭州市中心，整个行程大约需要一天时间。沿着这条路线，你可以领略西湖的自"
    "然风光和文化底蕴，感受人间天堂的独特魅力。";
const std::string TEST_ENTITY_TEXT_CN_50 =
    "清晨,从杭州市中心出发，沿着湖滨路缓缓前行。湖滨路是杭州市中心通往西湖的主要街道之一，两旁绿树成荫。";
const std::string TEST_ENTITY_TEXT_CN_10 =
    "清晨,从杭州市中心出";
const std::string TEST_ENTITY_TEXT_CN_5 =
    "清晨,从杭";
constexpr uint32_t MAX_RECOGNITION_LENGTH = 1000;
}

class PasteboardServiceEntityTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceEntityTest SetUpTestCase");
    }

    static void TearDownTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceEntityTest TearDownTestCase");
    }

    void SetUp()
    {
        service_ = std::make_shared<PasteboardService>();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceEntityTest SetUp");
    }

    void TearDown()
    {
        service_ = nullptr;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServiceEntityTest TearDown");
    }

protected:
    std::shared_ptr<PasteboardService> service_ = nullptr;
};

/**
 * @tc.name: ExtractEntityTest006
 * @tc.desc: test ExtractEntity with empty entity string
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest006 start");
    std::string entity = "";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest006 end");
}

/**
 * @tc.name: ExtractEntityTest007
 * @tc.desc: test ExtractEntity with invalid JSON string
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest007 start");
    std::string entity = "invalid json string";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest007 end");
}

/**
 * @tc.name: ExtractEntityTest008
 * @tc.desc: test ExtractEntity with JSON missing code field
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest008 start");
    std::string entity = R"({"message": "test"})";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest008 end");
}

/**
 * @tc.name: ExtractEntityTest009
 * @tc.desc: test ExtractEntity with code field as string instead of number
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest009 start");
    std::string entity = R"({"code": "0"})";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest009 end");
}

/**
 * @tc.name: ExtractEntityTest010
 * @tc.desc: test ExtractEntity with non-zero code value
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest010 start");
    std::string entity = R"({"code": 1, "message": "error"})";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest010 end");
}

/**
 * @tc.name: ExtractEntityTest011
 * @tc.desc: test ExtractEntity with code 0 but missing location field
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest011, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest011 start");
    std::string entity = R"({"code": 0, "message": "success"})";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest011 end");
}

/**
 * @tc.name: ExtractEntityTest012
 * @tc.desc: test ExtractEntity with code 0 and location field but not array
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest012, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest012 start");
    std::string entity = R"({"code": 0, "entity": {"location": "room1"}})";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest012 end");
}

/**
 * @tc.name: ExtractEntityTest013
 * @tc.desc: test ExtractEntity with code 0 and valid location array
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, ExtractEntityTest013, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest013 start");
    std::string entity = R"({"code": 0, "entity": {"location": ["room1", "room2"]}})";
    std::string location = "";
    
    int32_t ret = service_->ExtractEntity(entity, location);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    EXPECT_FALSE(location.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ExtractEntityTest013 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest001
 * @tc.desc: test GetAllPrimaryText with empty pasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, GetAllPrimaryTextTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest001 start");
    PasteData pasteData;
    
    std::string ret = service_->GetAllPrimaryText(pasteData);
    EXPECT_TRUE(ret.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest001 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest002
 * @tc.desc: test GetAllPrimaryText with single text record
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, GetAllPrimaryTextTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest002 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test text");
    
    std::string ret = service_->GetAllPrimaryText(pasteData);
    EXPECT_EQ(ret, "test text");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest002 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest003
 * @tc.desc: test GetAllPrimaryText with multiple text records
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, GetAllPrimaryTextTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest003 start");
    PasteData pasteData;
    pasteData.AddTextRecord("text1");
    pasteData.AddTextRecord("text2");
    pasteData.AddTextRecord("text3");
    
    std::string ret = service_->GetAllPrimaryText(pasteData);
    EXPECT_EQ(ret, "text1text2text3");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest003 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest004
 * @tc.desc: test GetAllPrimaryText with text exceeding MAX_RECOGNITION_LENGTH
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, GetAllPrimaryTextTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest004 start");
    PasteData pasteData;
    std::string longText(MAX_RECOGNITION_LENGTH + 100, 'a');
    pasteData.AddTextRecord(longText);
    pasteData.AddTextRecord("more text");
    
    std::string ret = service_->GetAllPrimaryText(pasteData);
    EXPECT_TRUE(ret.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest004 end");
}

/**
 * @tc.name: GetAllPrimaryTextTest005
 * @tc.desc: test GetAllPrimaryText with HTML record
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, GetAllPrimaryTextTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest005 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<div>html content</div>");
    
    std::string ret = service_->GetAllPrimaryText(pasteData);
    EXPECT_TRUE(ret.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllPrimaryTextTest005 end");
}

/**
 * @tc.name: RecognizePasteDataTest002
 * @tc.desc: test RecognizePasteData with InApp shareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, RecognizePasteDataTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest002 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetShareOption(ShareOption::InApp);
    
    service_->RecognizePasteData(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest002 end");
}

/**
 * @tc.name: RecognizePasteDataTest003
 * @tc.desc: test RecognizePasteData with empty text
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, RecognizePasteDataTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest003 start");
    PasteData pasteData;
    pasteData.AddTextRecord("");
    
    service_->RecognizePasteData(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest003 end");
}

/**
 * @tc.name: RecognizePasteDataTest004
 * @tc.desc: test RecognizePasteData with valid shareOption and text
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, RecognizePasteDataTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest004 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test recognition");
    pasteData.SetShareOption(ShareOption::CrossDevice);
    
    service_->RecognizePasteData(pasteData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "RecognizePasteDataTest004 end");
}

/**
 * @tc.name: NotifyEntityObserversTest001
 * @tc.desc: test NotifyEntityObservers with empty observerMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceEntityTest, NotifyEntityObserversTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "NotifyEntityObserversTest001 start");
    std::string entity = "test entity";
    EntityType entityType = EntityType::ADDRESS;
    uint32_t dataLength = 100;
    
    service_->NotifyEntityObservers(entity, entityType, dataLength);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "NotifyEntityObserversTest001 end");
}
}