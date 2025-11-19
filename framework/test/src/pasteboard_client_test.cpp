/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "unistd.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
constexpr const uid_t EDM_UID = 3057;
constexpr int32_t PERCENTAGE = 70;
constexpr uint32_t MAX_RECOGNITION_LENGTH = 1000;
constexpr uint32_t TEST_RECOGNITION_LENGTH = 100;
using Patterns = std::set<Pattern>;
class PasteboardClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

class TestEntityRecognitionObserver : public EntityRecognitionObserver {
public:
    void OnRecognitionEvent(EntityType entityType, std::string &entity) override
    {
        entityType_ = entityType;
        entity_ = entity;
        uint32_t type = static_cast<uint32_t>(entityType);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "observer callback, entityType=%{public}u, entity=%{private}s", type, entity.c_str());
    }
    EntityType entityType_;
    std::string entity_ = "";
};

void PasteboardClientTest::SetUpTestCase(void)
{
    setuid(EDM_UID);
}

void PasteboardClientTest::TearDownTestCase(void)
{
    setuid(0);
}

void PasteboardClientTest::SetUp(void) { }

void PasteboardClientTest::TearDown(void) { }

/**
 * @tc.name: GetChangeCount001
 * @tc.desc: change count should not change after clear pasteboard.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetChangeCount001, TestSize.Level0)
{
    uint32_t changeCount = 0;
    int32_t ret = PasteboardClient::GetInstance()->GetChangeCount(changeCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PasteboardClient::GetInstance()->Clear();
    uint32_t newCount = 0;
    ret = PasteboardClient::GetInstance()->GetChangeCount(newCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_EQ(newCount, changeCount);
}

/**
 * @tc.name: GetChangeCount002
 * @tc.desc: change count should add 1 after successful SetPastedata once.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetChangeCount002, TestSize.Level0)
{
    uint32_t changeCount = 0;
    int32_t ret = PasteboardClient::GetInstance()->GetChangeCount(changeCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ret = PasteboardClient::GetInstance()->SetPasteData(*newData);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    uint32_t newCount = 0;
    ret = PasteboardClient::GetInstance()->GetChangeCount(newCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_EQ(newCount, changeCount + 1);
}

/**
 * @tc.name: GetChangeCount003
 * @tc.desc: change count should add 2 after successful SetPastedata twice.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetChangeCount003, TestSize.Level0)
{
    uint32_t changeCount = 0;
    int32_t ret = PasteboardClient::GetInstance()->GetChangeCount(changeCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ret = PasteboardClient::GetInstance()->SetPasteData(*newData);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    std::string htmlText = "<div class='disable'>helloWorld</div>";
    auto newData1 = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ret = PasteboardClient::GetInstance()->SetPasteData(*newData1);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    uint32_t newCount = 0;
    ret = PasteboardClient::GetInstance()->GetChangeCount(newCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_EQ(newCount, changeCount + 2);
}

/**
 * @tc.name: IsRemoteData001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, IsRemoteData001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    bool ret = PasteboardClient::GetInstance()->IsRemoteData();
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: IsRemoteData002
 * @tc.desc: pasteData is remote data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, IsRemoteData002, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    pasteData->SetRemote(true);
    PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    bool ret = PasteboardClient::GetInstance()->IsRemoteData();
    ASSERT_TRUE(ret);
}

/**
 * @tc.name: GetMimeTypes001
 * @tc.desc: get data type is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetMimeTypes001, TestSize.Level0)
{
    PasteboardClient::GetInstance()->Clear();
    std::vector<std::string> mimeTypes = PasteboardClient::GetInstance()->GetMimeTypes();
    ASSERT_EQ(0, mimeTypes.size());
}

/**
 * @tc.name: GetMimeTypes002
 * @tc.desc: get data type is MIMETYPE_TEXT_PLAIN.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetMimeTypes002, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(newData);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::vector<std::string> mimeTypes = PasteboardClient::GetInstance()->GetMimeTypes();
    ASSERT_EQ(1, mimeTypes.size());
    ASSERT_EQ(MIMETYPE_TEXT_PLAIN, mimeTypes[0]);
}

/**
 * @tc.name: GetMimeTypes003
 * @tc.desc: data type is MIMETYPE_TEXT_HTML.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetMimeTypes003, TestSize.Level0)
{
    std::string htmlText = "<div class='disable'>helloWorld</div>";
    auto newPasteData = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(newPasteData);
    PasteboardClient::GetInstance()->SetPasteData(*newPasteData);
    std::vector<std::string> mimeTypes = PasteboardClient::GetInstance()->GetMimeTypes();
    ASSERT_EQ(1, mimeTypes.size());
    ASSERT_EQ(MIMETYPE_TEXT_HTML, mimeTypes[0]);
}

/**
 * @tc.name: GetMimeTypes004
 * @tc.desc: data type is MIMETYPE_TEXT_URI.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetMimeTypes004, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto newPasteData = PasteboardClient::GetInstance()->CreateUriData(uri);
    ASSERT_TRUE(newPasteData);
    PasteboardClient::GetInstance()->SetPasteData(*newPasteData);
    std::vector<std::string> mimeTypes = PasteboardClient::GetInstance()->GetMimeTypes();
    ASSERT_EQ(1, mimeTypes.size());
    ASSERT_EQ(MIMETYPE_TEXT_URI, mimeTypes[0]);
}

/**
 * @tc.name: GetMimeTypes005
 * @tc.desc: get multi data types.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetMimeTypes005, TestSize.Level0)
{
    PasteData data;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::string uriStr = "/data/test/resource/pasteboardTest.txt";
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    auto record = builder.SetUri(uri).Build();
    data.AddRecord(*record);

    using namespace OHOS::AAFwk;
    std::string plainText = "helloWorld";
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string key = "id";
    int32_t id = 456;
    Want wantIn = want->SetParam(key, id);
    PasteDataRecord::Builder builder2(MIMETYPE_TEXT_WANT);
    std::shared_ptr<PasteDataRecord> pasteDataRecord = builder2.SetWant(std::make_shared<Want>(wantIn)).Build();
    data.AddRecord(pasteDataRecord);

    const uint32_t color[] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
    uint32_t len = sizeof(color) / sizeof(color[0]);
    Media::InitializationOptions opts;
    std::shared_ptr<OHOS::Media::PixelMap> pixelMap = Media::PixelMap::Create(color, len, 0, 2, opts);
    PasteDataRecord::Builder builder3(MIMETYPE_PIXELMAP);
    auto record3 = builder3.SetPixelMap(pixelMap).Build();
    data.AddRecord(*record3);
    PasteDataRecord::Builder builder4(MIMETYPE_TEXT_URI);
    std::string uriStr4 = "/data/test/resource/pasteboardTest.txt";
    auto uri4 = std::make_shared<OHOS::Uri>(uriStr4);
    auto record4 = builder.SetUri(uri4).Build();
    data.AddRecord(*record4);

    PasteboardClient::GetInstance()->SetPasteData(data);
    std::vector<std::string> mimeTypes = PasteboardClient::GetInstance()->GetMimeTypes();
    ASSERT_EQ(3, mimeTypes.size());
    std::set<std::string> mimeTypesSet(mimeTypes.begin(), mimeTypes.end());
    ASSERT_EQ(3, mimeTypesSet.size());
    for (const std::string &type : mimeTypesSet) {
        ASSERT_TRUE(MIMETYPE_TEXT_WANT == type ||
                    MIMETYPE_PIXELMAP == type ||
                    MIMETYPE_TEXT_URI == type);
    }
}

/**
 * @tc.name: HasDataType001
 * @tc.desc: data type is MIMETYPE_TEXT_PLAIN.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HasDataType001, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    auto ret = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_PLAIN);
    ASSERT_TRUE(ret);
    auto result = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_URI);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: HasDataType002
 * @tc.desc: data type is MIMETYPE_TEXT_HTML.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HasDataType002, TestSize.Level0)
{
    std::string htmlText = "<div class='disable'>helloWorld</div>";
    auto newPasteData = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    PasteboardClient::GetInstance()->SetPasteData(*newPasteData);
    auto ret = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_HTML);
    ASSERT_TRUE(ret);
    auto result = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_PLAIN);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: HasDataType003
 * @tc.desc: data type is MIMETYPE_TEXT_URI
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HasDataType003, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto newPasteData = PasteboardClient::GetInstance()->CreateUriData(uri);
    PasteboardClient::GetInstance()->SetPasteData(*newPasteData);
    auto ret = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_URI);
    ASSERT_TRUE(ret);
    auto result = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_PLAIN);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: HasDataType004
 * @tc.desc: data type is MIMETYPE_PIXELMAP
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HasDataType004, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto newPasteData = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMapIn);
    PasteboardClient::GetInstance()->SetPasteData(*newPasteData);
    auto ret = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_PIXELMAP);
    ASSERT_TRUE(ret);
    auto result = PasteboardClient::GetInstance()->HasDataType(MIMETYPE_TEXT_URI);
    ASSERT_FALSE(result);
}

/**
 * @tc.name: GetDataSource001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetDataSource001, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::string bundleName;
    PasteboardClient::GetInstance()->GetDataSource(bundleName);
    EXPECT_FALSE(bundleName.empty());
}

/**
 * @tc.name: SetGlobalShareOption
 * @tc.desc: Set global shareOption
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, SetGlobalShareOption, TestSize.Level0)
{
    std::map<uint32_t, ShareOption> settings = { { 100, ShareOption::InApp }, { 200, ShareOption::LocalDevice },
        { 300, ShareOption::CrossDevice } };
    PasteboardClient::GetInstance()->SetGlobalShareOption(settings);
    auto result = PasteboardClient::GetInstance()->GetGlobalShareOption({});
    EXPECT_TRUE(result.size() == 3);
    EXPECT_EQ(result[100], ShareOption::InApp);
    EXPECT_EQ(result[200], ShareOption::LocalDevice);
    EXPECT_EQ(result[300], ShareOption::CrossDevice);
    std::map<uint32_t, ShareOption> modify = { { 100, ShareOption::CrossDevice }, { 400, ShareOption::InApp } };
    PasteboardClient::GetInstance()->SetGlobalShareOption(modify);
    result = PasteboardClient::GetInstance()->GetGlobalShareOption({});
    EXPECT_TRUE(result.size() == 4);
    EXPECT_EQ(result[100], ShareOption::CrossDevice);
    EXPECT_EQ(result[400], ShareOption::InApp);
    PasteboardClient::GetInstance()->RemoveGlobalShareOption({ 100, 200, 300, 400 });
}

/**
 * @tc.name: GetGlobalShareOption
 * @tc.desc: Get global shareOption
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetGlobalShareOption, TestSize.Level0)
{
    std::map<uint32_t, ShareOption> settings = { { 100, ShareOption::InApp }, { 200, ShareOption::LocalDevice },
        { 300, ShareOption::CrossDevice } };
    PasteboardClient::GetInstance()->SetGlobalShareOption(settings);
    auto result = PasteboardClient::GetInstance()->GetGlobalShareOption({});
    EXPECT_TRUE(result.size() == 3);
    EXPECT_EQ(result[100], ShareOption::InApp);
    EXPECT_EQ(result[200], ShareOption::LocalDevice);
    EXPECT_EQ(result[300], ShareOption::CrossDevice);
    result = PasteboardClient::GetInstance()->GetGlobalShareOption({ 100, 400 });
    EXPECT_TRUE(result.size() == 1);
    EXPECT_EQ(result[100], ShareOption::InApp);
    EXPECT_TRUE(result.find(400) == result.end());
    PasteboardClient::GetInstance()->RemoveGlobalShareOption({ 100, 200, 300 });
}

/**
 * @tc.name: RemoveGlobalShareOption
 * @tc.desc: Remove global shareOption
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, RemoveGlobalShareOption, TestSize.Level0)
{
    std::map<uint32_t, ShareOption> settings = { { 100, ShareOption::InApp }, { 200, ShareOption::LocalDevice },
        { 300, ShareOption::CrossDevice } };
    PasteboardClient::GetInstance()->SetGlobalShareOption(settings);
    auto result = PasteboardClient::GetInstance()->GetGlobalShareOption({});
    EXPECT_TRUE(result.size() == 3);
    EXPECT_EQ(result[100], ShareOption::InApp);
    EXPECT_EQ(result[200], ShareOption::LocalDevice);
    EXPECT_EQ(result[300], ShareOption::CrossDevice);
    PasteboardClient::GetInstance()->RemoveGlobalShareOption({});
    result = PasteboardClient::GetInstance()->GetGlobalShareOption({});
    EXPECT_TRUE(result.size() == 3);
    PasteboardClient::GetInstance()->RemoveGlobalShareOption({ 100, 400 });
    result = PasteboardClient::GetInstance()->GetGlobalShareOption({});
    EXPECT_TRUE(result.size() == 2);
    EXPECT_TRUE(result.find(100) == result.end());
    PasteboardClient::GetInstance()->RemoveGlobalShareOption({ 200, 300 });
}

/**
 * @tc.name: DetectPatterns001
 * @tc.desc: Cover Permutation
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, DetectPatterns001, TestSize.Level0)
{
    std::string plainText("r法塔赫已经，速tdghf！】qd rqdswww.comsski,.sjopwe"
                          "ihhtpsdhttp我也带过去给他№のjioijhhu");
    std::string plainText0("https://giedqwrtheeeeeefub.cerm/meeeelkove/obaklo_tjokl"
                           "psetkjdttk/bkkjob/mwjweww.md）");
    std::string plainText1("2我就破888芙蓉王82h7");
    std::string plainText2("uhiyqydueuw@kahqw.oisko.sji");

    std::vector<std::string> plainTextVec{ plainText, plainText + plainText0, plainText + plainText1,
        plainText + plainText2, plainText + plainText0 + plainText1, plainText0 + plainText2 + plainText,
        plainText1 + plainText + plainText2, plainText0 + plainText1 + plainText + plainText2 };
    std::vector<Patterns> patternsVec { {}, { Pattern::URL }, { Pattern::NUMBER }, { Pattern::EMAIL_ADDRESS },
        { Pattern::URL, Pattern::NUMBER }, { Pattern::URL, Pattern::EMAIL_ADDRESS },
        { Pattern::NUMBER, Pattern::EMAIL_ADDRESS }, { Pattern::URL, Pattern::NUMBER, Pattern::EMAIL_ADDRESS } };
    std::vector<std::vector<int>> patternsRightIndexVec { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 1, 0, 0, 1, 1, 0, 1 },
        { 0, 0, 2, 0, 2, 0, 2, 2 }, { 0, 0, 0, 3, 0, 3, 3, 3 }, { 0, 1, 2, 0, 4, 1, 2, 4 }, { 0, 1, 0, 3, 1, 5, 3, 5 },
        { 0, 0, 2, 3, 2, 3, 6, 6 }, { 0, 1, 2, 3, 4, 5, 6, 7 } };
    for (int i = 0; i != 8; ++i) {
        for (int j = 0; j != 8; ++j) {
            auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainTextVec[i]);
            PasteboardClient::GetInstance()->SetPasteData(*newData);
            auto ret = PasteboardClient::GetInstance()->DetectPatterns(patternsVec[j]);
            int rightIndex = patternsRightIndexVec[i][j];
            ASSERT_EQ(ret, patternsVec[rightIndex]);
        }
    }
}

/**
 * @tc.name: DetectPatterns002
 * @tc.desc: check HTML
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, DetectPatterns002, TestSize.Level0)
{
    std::string htmlText1 = "<!DOCTYPE html><html><head><title>"
                            "超链案头研究。，封为啊啊</title></head><body><h2>发高热</h2>"
                            "<p>隔热隔热的氛围<a href=\"https://exq23amwerwqple.com\">"
                            "个人网站https://ex24t33tamp65hhle.com</a>。</p></body></html>";
    auto newData1 = PasteboardClient::GetInstance()->CreateHtmlData(htmlText1);
    PasteboardClient::GetInstance()->SetPasteData(*newData1);
    Patterns patternsToCheck1 { Pattern::URL, Pattern::EMAIL_ADDRESS };
    auto ret1 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck1);
    Patterns expected1{ Pattern::URL };
    ASSERT_EQ(ret1, expected1);

    std::string htmlText2 = "<!DOCTYPE html><html><head><title>"
                            "各个环节</title></head><body><h2>妈妈那边的</h2>"
                            "<p>啊啊分，凤凰方法，环境https://examjjuyewple.com问我的<a "
                            "href=\"https://ehhgxametgeple.com\">"
                            "阿婆吗weqkqo@exaetmple.com</a>。？？？？打法</p></body></html>";
    auto newData2 = PasteboardClient::GetInstance()->CreateHtmlData(htmlText2);
    PasteboardClient::GetInstance()->SetPasteData(*newData2);
    Patterns patternsToCheck2 { Pattern::URL, Pattern::EMAIL_ADDRESS, Pattern::NUMBER };
    auto ret2 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck2);
    Patterns expected2 { Pattern::URL, Pattern::EMAIL_ADDRESS };
    ASSERT_EQ(ret2, expected2);
}

/**
 * @tc.name: DetectPatterns003
 * @tc.desc: Outlier force cast uint32_t to unsupported Pattern
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, DetectPatterns003, TestSize.Level0)
{
    std::string plainText1 = "部分人的十点半：\n"
                             "「而飞过海」\n"
                             "方法：\n"
                             "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4"
                             "网gest加尔文iqru发的我ui哦计划任务i文化人:\n"
                             "~b0043fg3423tddj~";
    auto newData1 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText1);
    PasteboardClient::GetInstance()->SetPasteData(*newData1);
    Patterns patternsToCheck { Pattern::NUMBER, Pattern::URL, Pattern::EMAIL_ADDRESS, static_cast<Pattern>(1023) };
    auto ret1 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
    Patterns expected1{};
    ASSERT_EQ(ret1, expected1);
    std::string plainText2 = "【撒迪化，等我i却很难，无穷花的！】"
                             "额外i卡号！念佛为？，为单位打开陪我。而奋斗，我去二队去，威威：trfwrtg"
                             "(￥￥软骨素用人员为bdfdgse https://tgrthwerrwt.com/marrkerrerlorrve/ "
                             "usrdq12_22swe@16rtgre3.com）";
    auto newData2 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText2);
    PasteboardClient::GetInstance()->SetPasteData(*newData2);
    auto ret2 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
    Patterns expected2{};
    ASSERT_EQ(ret2, expected2);
    std::string plainText3 = "【撒迪化，等我i却很难，无穷花的！】"
                             "额外i卡号！念佛为？，为单位打开陪我。而奋斗，我去二队去，威威：trfwrtg";
    auto newData3 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText3);
    PasteboardClient::GetInstance()->SetPasteData(*newData3);
    auto ret3 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
    ASSERT_EQ(ret3, Patterns{});
}

/**
 * @tc.name: DetectPatterns004
 * @tc.desc: Outlier force cast uint32_t 0xffffffff to unsupported Pattern
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, DetectPatterns004, TestSize.Level0)
{
    std::string plainText1 = "部分人的十点半：\n"
                             "「而飞过海」\n"
                             "方法：\n"
                             "https://pr5yyye-drseyive.u54yk.cwerfe/s/42e1ewed77f3dab4"
                             "网gest加尔文iqru发的我ui哦计划任务i文化人:\n"
                             "~b0043fg3423tddj~";
    auto newData1 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText1);
    PasteboardClient::GetInstance()->SetPasteData(*newData1);
    std::set<Pattern> patternsToCheck { Pattern::NUMBER, Pattern::URL, Pattern::EMAIL_ADDRESS,
        static_cast<Pattern>(0xffffffff), static_cast<Pattern>(0xffffff1a) };
    auto ret1 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
    std::set<Pattern> expected1{};
    ASSERT_EQ(ret1, expected1);
    std::string plainText2 = "【撒迪化，等我i却很难，无穷花的！】"
                             "额外i卡号！念佛为？，为单位打开陪我。而奋斗，我去二队去，威威：trfwrtg"
                             "(￥￥软骨素用人员为bdfdgse https://tgrthwerrwt.com/marrkerrerlorrve/ "
                             "usrdq12_22swe@16rtgre3.com）";
    auto newData2 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText2);
    PasteboardClient::GetInstance()->SetPasteData(*newData2);
    auto ret2 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
    std::set<Pattern> expected2{};
    ASSERT_EQ(ret2, expected2);
    std::string plainText3 = "【撒迪化，等我i却很难，无穷花的！】"
                             "额外i卡号！念佛为？，为单位打开陪我。而奋斗，我去二队去，威威：trfwrtg";
    auto newData3 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText3);
    PasteboardClient::GetInstance()->SetPasteData(*newData3);
    auto ret3 = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
    ASSERT_EQ(ret3, std::set<Pattern>{});
}

/**
 * @tc.name: CreateMultiDelayRecord001
 * @tc.desc: call CreateMultiDelayRecord
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, CreateMultiDelayRecord001, TestSize.Level0)
{
    std::vector<std::string> mineTypes;
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateMultiDelayRecord(mineTypes, nullptr);
    EXPECT_NE(nullptr, pasteDataRecord);
}

/**
 * @tc.name: CreateMultiTypeData001
 * @tc.desc: call CreateMultiTypeData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, CreateMultiTypeData001, TestSize.Level0)
{
    std::map<std::string, std::shared_ptr<EntryValue>> typeValueMap;
    std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>> mapPtr =
        std::make_shared<std::map<std::string, std::shared_ptr<EntryValue>>>(typeValueMap);
    const std::string recordMimeType = "record_mime_type";
    auto pasteData = PasteboardClient::GetInstance()->CreateMultiTypeData(mapPtr, recordMimeType);
    EXPECT_NE(nullptr, pasteData);
}

/**
 * @tc.name: CreateMultiTypeDelayData001
 * @tc.desc: call CreateMultiTypeDelayData and some misc branches
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, CreateMultiTypeDelayData001, TestSize.Level0)
{
    std::vector<std::string> mimeTypes;
    auto pasteData = PasteboardClient::GetInstance()->CreateMultiTypeDelayData(mimeTypes, nullptr);
    EXPECT_NE(nullptr, pasteData);
    PasteboardClient::GetInstance()->PasteStart("paste_id");
    PasteboardClient::GetInstance()->PasteComplete("device_id", "paste_id");
    PasteboardClient::GetInstance()->Subscribe(PasteboardObserverType::OBSERVER_LOCAL, nullptr);
}

void ProgressNotify(std::shared_ptr<GetDataParams> params)
{
    if (params == nullptr) {
        printf("Error: params is nullptr\n");
        return;
    }

    if (params->info == nullptr) {
        printf("Error: params->info is nullptr\n");
        return;
    }

    printf("percentage=%d\n", params->info->percentage);
}

/**
 * @tc.name: GetDataWithProgress001
 * @tc.desc: Getting data without system default progress indicator.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetDataWithProgress001, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(newData != nullptr);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    PasteData pasteData;
    std::shared_ptr<GetDataParams> params = std::make_shared<GetDataParams>();
    params->fileConflictOption = FILE_OVERWRITE;
    params->progressIndicator = NONE_PROGRESS_INDICATOR;
    params->listener.ProgressNotify = ProgressNotify;
    int32_t ret = PasteboardClient::GetInstance()->GetDataWithProgress(pasteData, params);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetDataWithProgress002
 * @tc.desc: Getting data with system default progress indicator.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetDataWithProgress002, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(newData != nullptr);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    PasteData pasteData;
    std::shared_ptr<GetDataParams> params = std::make_shared<GetDataParams>();
    params->fileConflictOption = FILE_OVERWRITE;
    params->progressIndicator = DEFAULT_PROGRESS_INDICATOR;
    int32_t ret = PasteboardClient::GetInstance()->GetDataWithProgress(pasteData, params);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

void ProgressNotifyTest(std::shared_ptr<GetDataParams> params)
{
    if (params == nullptr) {
        printf("Error: params is nullptr\n");
        return;
    }

    if (params->info == nullptr) {
        printf("Error: params->info is nullptr\n");
        return;
    }

    printf("percentage=%d\n", params->info->percentage);
    if (params->info->percentage == PERCENTAGE) {
        ProgressSignalClient::GetInstance().Cancel();
    }
}

/**
 * @tc.name: GetDataWithProgress003
 * @tc.desc: When the progress reaches 80, the download is canceled.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetDataWithProgress003, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(newData != nullptr);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    PasteData pasteData;
    std::shared_ptr<GetDataParams> params = std::make_shared<GetDataParams>();
    params->fileConflictOption = FILE_OVERWRITE;
    params->progressIndicator = NONE_PROGRESS_INDICATOR;
    params->listener.ProgressNotify = ProgressNotifyTest;
    int32_t ret = PasteboardClient::GetInstance()->GetDataWithProgress(pasteData, params);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetDataWithProgress004
 * @tc.desc: GetDataWithProgress test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetDataWithProgress004, TestSize.Level0)
{
    PasteData pasteData;
    int32_t ret = PasteboardClient::GetInstance()->GetDataWithProgress(pasteData, nullptr);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(newData != nullptr);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::shared_ptr<GetDataParams> params = std::make_shared<GetDataParams>();
    params->fileConflictOption = FILE_OVERWRITE;
    params->progressIndicator = NONE_PROGRESS_INDICATOR;
    ret = PasteboardClient::GetInstance()->GetDataWithProgress(pasteData, params);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: GetDataWithProgress005
 * @tc.desc: GetDataWithProgress test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetDataWithProgress005, TestSize.Level0)
{
    PasteData pasteData;
    std::string plainText = "helloWorld";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(newData != nullptr);
    PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::shared_ptr<GetDataParams> params = std::make_shared<GetDataParams>();
    params->fileConflictOption = FILE_OVERWRITE;
    params->progressIndicator = DEFAULT_PROGRESS_INDICATOR;
    int32_t ret = PasteboardClient::GetInstance()->GetDataWithProgress(pasteData, params);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: HandleSignalValue001
 * @tc.desc: HandleSignalValue001 Test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HandleSignalValue001, TestSize.Level0)
{
    PasteboardClient pasteboardClient;
    std::string signalValue = "0";
    int32_t result = pasteboardClient.HandleSignalValue(signalValue);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: HandleSignalValue002
 * @tc.desc: HandleSignalValue002 Test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HandleSignalValue002, TestSize.Level0)
{
    PasteboardClient pasteboardClient;
    std::string signalValue = "invalid";
    int32_t result = pasteboardClient.HandleSignalValue(signalValue);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}

/**
 * @tc.name: HandleSignalValue003
 * @tc.desc: HandleSignalValue003 Test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HandleSignalValue003, TestSize.Level0)
{
    PasteboardClient pasteboardClient;
    int64_t value = INT32_MAX;
    std::string signalValue = std::to_string(value + 1);
    int32_t result = pasteboardClient.HandleSignalValue(signalValue);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}

/**
 * @tc.name: HandleSignalValue004
 * @tc.desc: HandleSignalValue004 Test.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, HandleSignalValue004, TestSize.Level0)
{
    PasteboardClient pasteboardClient;
    std::string signalValue(129, '1');
    int32_t result = pasteboardClient.HandleSignalValue(signalValue);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE));
}

/**
 * @tc.name: SubscribeEntityObserverTest001
 * @tc.desc: Subscribe EntityObserver when entityType is invalid value, should return ERR_INVALID_VALUE.
 * hen EntityType is MAX, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest001, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::MAX, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest002
 * @tc.desc: Subscribe EntityObserver when expectedDataLength exceeds limitation, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest002, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH + 1;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest003
 * @tc.desc: Subscribe EntityObserver when observer is nullptr, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest003, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = nullptr;
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest004
 * @tc.desc: Subscribe EntityObserver normally should return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest004, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest005
 * @tc.desc: Subscribe EntityObserver normally should return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest005, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    PasteData pasteData;
    std::string plainText = "陕西省西安市高新区丈八八路";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_NE(newData, nullptr);
    result = PasteboardClient::GetInstance()->SetPasteData(*newData);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest006
 * @tc.desc: Subscribe observer again will return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest006, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest007
 * @tc.desc: Subscribe another observer will replace old one and return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest007, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = new TestEntityRecognitionObserver();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    sptr<EntityRecognitionObserver> otherObserver = sptr<EntityRecognitionObserver>::MakeSptr();
    result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, otherObserver);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, otherObserver);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest008
 * @tc.desc: Subscribe observer again with different param will return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest008, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, TEST_RECOGNITION_LENGTH, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, TEST_RECOGNITION_LENGTH, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest009
 * @tc.desc: Unsubscribe EntityObserver and copy plainText, will not exec callback.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest009, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<TestEntityRecognitionObserver> observer = sptr<TestEntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    PasteData pasteData;
    std::string plainText = "陕西省西安市高新区丈八八路";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_NE(newData, nullptr);
    result = PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    ASSERT_TRUE(observer->entity_.empty());
}

/**
 * @tc.name: SubscribeEntityObserverTest010
 * @tc.desc: Subscribe EntityObserver and copy plainText with ShareOption::InApp, will not exec callback.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, SubscribeEntityObserverTest0010, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<TestEntityRecognitionObserver> observer = sptr<TestEntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    std::string plainText = "陕西省西安市高新区丈八八路";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_NE(newData, nullptr);
    newData->SetShareOption(ShareOption::InApp);
    result = PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    ASSERT_TRUE(observer->entity_.empty());
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest001
 * @tc.desc: Subscribe EntityObserver when EntityType is MAX, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, UnsubscribeEntityObserverTest001, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::MAX, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest002
 * @tc.desc: Subscribe EntityObserver when expectedDataLength exceeds limitation, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, UnsubscribeEntityObserverTest002, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH + 1;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest003
 * @tc.desc: Subscribe EntityObserver when observer is nullptr, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, UnsubscribeEntityObserverTest003, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = nullptr;
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest004
 * @tc.desc: Subscribe EntityObserver normally should return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, UnsubscribeEntityObserverTest004, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest005
 * @tc.desc: Unsubscribe observer again will return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, UnsubscribeEntityObserverTest005, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = new TestEntityRecognitionObserver();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: UpdateProgressTest001
 * @tc.desc: UpdateProgressTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, UpdateProgressTest001, TestSize.Level0)
{
    std::shared_ptr<GetDataParams> params = nullptr;
    PasteboardClient::GetInstance()->UpdateProgress(params, 50);
    EXPECT_TRUE(params == nullptr);
}

/**
 * @tc.name: ReleaseSaListenerTest001
 * @tc.desc: Release Sa Listener
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, ReleaseSaListenerTest001, TestSize.Level0)
{
    PasteboardClient::GetInstance()->ReleaseSaListener();
    EXPECT_TRUE(PasteboardClient::GetInstance()->isSubscribeSa_ == false);
}

/**
 * @tc.name: GetRecordValueByTypeTest001
 * @tc.desc: GetRecordValueByTypeTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientTest, GetRecordValueByTypeTest001, TestSize.Level0)
{
    PasteDataEntry entry;
    int32_t result = PasteboardClient::GetInstance()->GetRecordValueByType(1, 1, entry);
    ASSERT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ID));
}

/**
 * @tc.name: GetPasteIdTest001
 * @tc.desc: should get empty pasteId when get local paste data success
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, GetPasteIdTest001, TestSize.Level0)
{
    PasteData setData;
    setData.AddTextRecord("text");
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(setData);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    PasteData getData;
    ret = PasteboardClient::GetInstance()->GetPasteData(getData);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));

    std::string pasteId = getData.GetPasteId();
    EXPECT_STREQ(pasteId.c_str(), "");
}

/**
 * @tc.name: ClearByUserTest001
 * @tc.desc: ClearByUser empty test
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardClientTest, ClearByUserTest001, TestSize.Level0)
{
    PasteboardClient::GetInstance()-> ClearByUser(0);
    ASSERT_TRUE(true);
}
} // namespace OHOS::MiscServices
