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

#include "folder.h"
#include "pasteboard_client.h"
#include "pasteboard_copy.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "unified_data.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using UnifiedData = UDMF::UnifiedData;

class PasteboardCopyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    UnifiedData InitFileData();

protected:
    Details details_;
    std::string uri_;
};

void PasteboardCopyTest::SetUpTestCase(void) { }

void PasteboardCopyTest::TearDownTestCase(void) { }

void PasteboardCopyTest::SetUp(void) { }

void PasteboardCopyTest::TearDown(void) { }

UnifiedData PasteboardCopyTest::InitFileData()
{
    UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE);
    uri_ = "file://uri";
    std::shared_ptr<UDMF::File> fileRecord = std::make_shared<UDMF::File>(uri_);
    fileRecord->SetDetails(details_);
    data.AddRecord(fileRecord);
    return data;
}

/**
 * @tc.name: IsDirectoryTest
 * @tc.desc: Test the path is a directory
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, IsDirectoryTest, TestSize.Level0)
{
    std::string directoryPath = "/invalid/path";
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();

    bool result = pasteBoardCopyFile.IsDirectory(directoryPath);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsFileTest
 * @tc.desc: Test the path is a file
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, IsFileTest, TestSize.Level0)
{
    std::string filePath = "/invalid/path";
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();

    bool result = pasteBoardCopyFile.IsFile(filePath);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: OnProgressNotifyTest
 * @tc.desc: Test progress notify
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, OnProgressNotifyTest, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    pasteBoardCopyFile.OnProgressNotify(nullptr);

    std::shared_ptr<GetDataParams> params = std::make_shared<GetDataParams>();
    params->info = nullptr;
    pasteBoardCopyFile.OnProgressNotify(params);

    params->info = new ProgressInfo();
    params->info->percentage = 101;
    pasteBoardCopyFile.OnProgressNotify(params);
    EXPECT_EQ(params->info->percentage, 100);
    delete params->info;
    params->info = nullptr;
}

/**
 * @tc.name: GetRealPathTest
 * @tc.desc: Test get real path
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, GetRealPathTest, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();

    std::string path1 = "/home/user/./test";
    std::string expected1 = "/home/user/test";
    std::string realPath1 = pasteBoardCopyFile.GetRealPath(path1);
    EXPECT_EQ(realPath1, expected1);

    std::string path2 = "/home/user/../test";
    std::string expected2 = "/home/test";
    std::string realPath2 = pasteBoardCopyFile.GetRealPath(path2);
    EXPECT_EQ(realPath2, expected2);

    std::string path3 = "/home/user/test";
    std::string expected3 = "/home/user/test";
    std::string realPath3 = pasteBoardCopyFile.GetRealPath(path3);
    EXPECT_EQ(realPath3, expected3);

    std::string path4 = "";
    std::string expected4 = "";
    std::string realPath4 = pasteBoardCopyFile.GetRealPath(path4);
    EXPECT_EQ(realPath4, expected4);
}

/**
 * @tc.name: IsRemoteUriTest
 * @tc.desc: Test the uri is remote
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, IsRemoteUriTest, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    std::string uri = "http://www.example.com";
    bool isRemoteUri = pasteBoardCopyFile.IsRemoteUri(uri);
    EXPECT_FALSE(isRemoteUri);
}

/**
 * @tc.name: CheckCopyParamTest
 * @tc.desc: Test check copy param
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, CheckCopyParamTest, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    PasteData pasteData;
    std::shared_ptr<GetDataParams> params = nullptr;
    int32_t result = pasteBoardCopyFile.CheckCopyParam(pasteData, params);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));

    params = std::make_shared<GetDataParams>();
    result = pasteBoardCopyFile.CheckCopyParam(pasteData, params);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));

    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(data != nullptr);

    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));

    bool has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);

    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));

    result = pasteBoardCopyFile.CheckCopyParam(pasteData, params);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: HandleProgressTest
 * @tc.desc: Test handle progress
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, HandleProgressTest, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    std::shared_ptr<CopyInfo> copyInfo = std::make_shared<CopyInfo>();
    CopyInfo info = *copyInfo;
    std::shared_ptr<GetDataParams> params = nullptr;
    pasteBoardCopyFile.HandleProgress(1, info, 10, params);

    params = std::make_shared<GetDataParams>();
    params->info = nullptr;
    pasteBoardCopyFile.HandleProgress(1, info, 10, params);

    params->info = new ProgressInfo();
    pasteBoardCopyFile.HandleProgress(0, info, 10, params);

    pasteBoardCopyFile.HandleProgress(1, info, 10, params);

    auto data = InitFileData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);

    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());

    delete params->info;
    params->info = nullptr;
}

/**
 * @tc.name: InitCopyInfoTest001
 * @tc.desc: Test init copy info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, InitCopyInfoTest001, TestSize.Level0)
{
    std::string srcUri = "file:///source/path";
    std::string destUri = "file:///destination/path";
    std::shared_ptr<GetDataParams> dataParams = std::make_shared<GetDataParams>();
    dataParams->destUri = destUri;
    std::shared_ptr<CopyInfo> copyInfo = std::make_shared<CopyInfo>();

    int32_t result = PasteBoardCopyFile::GetInstance().InitCopyInfo(srcUri, dataParams, copyInfo);
    EXPECT_EQ(result, ENOMEM);
}

/**
 * @tc.name: InitCopyInfoTest002
 * @tc.desc: Test init copy info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, InitCopyInfoTest002, TestSize.Level0)
{
    std::string srcUri = R"(file:///source/path/%25%E2%80%94.png)";
    std::string destUri = "file:///data";
    std::shared_ptr<GetDataParams> dataParams = std::make_shared<GetDataParams>();
    dataParams->destUri = destUri;
    std::shared_ptr<CopyInfo> copyInfo = std::make_shared<CopyInfo>();

    int32_t result = PasteBoardCopyFile::GetInstance().InitCopyInfo(srcUri, dataParams, copyInfo);
    EXPECT_EQ(result, ENOMEM);

    EXPECT_STREQ(copyInfo->srcUri.c_str(), R"(file:///source/path/%25%E2%80%94.png)");
    EXPECT_STREQ(copyInfo->destUri.c_str(), R"(file:///data/%25%E2%80%94.png)");
    EXPECT_STREQ(copyInfo->srcPath.c_str(), R"(/source/path/%—.png)");
    EXPECT_STREQ(copyInfo->destPath.c_str(), R"(/data/%—.png)");
}

/**
 * @tc.name: CopyFileDataTest001
 * @tc.desc: Test copy file data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, CopyFileDataTest001, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    PasteData pasteData;
    std::string destUri = "file:///destination/path";
    std::shared_ptr<GetDataParams> dataParams = std::make_shared<GetDataParams>();
    dataParams->destUri = destUri;

    int32_t result = pasteBoardCopyFile.CopyFileData(pasteData, dataParams);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: CopyFileDataTest002
 * @tc.desc: Test copy file data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, CopyFileDataTest002, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    PasteData pasteData;
    OHOS::Uri uri("/");
    pasteData.AddUriRecord(uri);
    std::string destUri = "file:///destination/path";
    std::shared_ptr<GetDataParams> dataParams = std::make_shared<GetDataParams>();
    dataParams->destUri = destUri;
    EXPECT_NE(dataParams, nullptr);

    int32_t result = pasteBoardCopyFile.CopyFileData(pasteData, dataParams);
}

/**
 * @tc.name: CopyFileDataTest003
 * @tc.desc: Test copy file data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, CopyFileDataTest003, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    PasteData pasteData;
    pasteData.AddTextRecord("hello");
    std::shared_ptr<GetDataParams> dataParams = std::make_shared<GetDataParams>();

    int32_t result = pasteBoardCopyFile.CopyFileData(pasteData, dataParams);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
}

/**
 * @tc.name: CopyPasteDataTest001
 * @tc.desc: Test copy paste data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, CopyPasteDataTest001, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    PasteData pasteData;
    pasteData.AddTextRecord("hello");
    std::shared_ptr<GetDataParams> dataParams = nullptr;

    int32_t result = pasteBoardCopyFile.CopyPasteData(pasteData, dataParams);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
}

/**
 * @tc.name: CopyPasteDataTest002
 * @tc.desc: Test copy paste data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, CopyPasteDataTest002, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    PasteData pasteData;
    pasteData.AddTextRecord("hello");
    std::shared_ptr<GetDataParams> dataParams = std::make_shared<GetDataParams>();
    dataParams->fileConflictOption = FILE_OVERWRITE;
    dataParams->progressIndicator = DEFAULT_PROGRESS_INDICATOR;
    dataParams->info = new ProgressInfo();
    EXPECT_NE(dataParams, nullptr);

    int32_t result = pasteBoardCopyFile.CopyPasteData(pasteData, dataParams);
    delete dataParams->info;
    dataParams->info = nullptr;
}

/**
 * @tc.name: ShouldKeepRecordTest001
 * @tc.desc: Test should keep record
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardCopyTest, ShouldKeepRecordTest001, TestSize.Level0)
{
    PasteBoardCopyFile &pasteBoardCopyFile = PasteBoardCopyFile::GetInstance();
    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    std::string destUri = "file:///destination/path";
    std::shared_ptr<PasteDataRecord> record = nullptr;
    int32_t result = pasteBoardCopyFile.ShouldKeepRecord(ret, destUri, record);
    EXPECT_FALSE(result);
}
} // namespace OHOS::MiscServices