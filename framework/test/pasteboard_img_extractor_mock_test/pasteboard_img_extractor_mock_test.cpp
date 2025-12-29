/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "pasteboard_common_mock.h"
#include "pasteboard_img_extractor.h"
#include "ipc_skeleton_mock.h"
#include "sandbox_helper_mock.h"

using namespace testing;
using namespace testing::ext;

namespace {
using namespace OHOS::MiscServices;

class PasteboardImgExtractorMockTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
    }

    static void TearDownTestCase(void)
    {
    }

    void SetUp(void)
    {
    }

    void TearDown(void)
    {
    }
};

/**
 * @tc.name: ExtractImgSrcTest001
 * @tc.desc: ExtractImgSrc should return empty vector when html is invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest001, TestSize.Level0)
{
    std::string htmlContent = "<invalid html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_TRUE(uris.empty());
}

/**
 * @tc.name: ExtractImgSrcTest002
 * @tc.desc: ExtractImgSrc should extract valid img src from html
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest002, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///test.png\"><img src=\"file:///test.jpg\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid path
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/test.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(2U, uris.size());
    ASSERT_EQ("file:///test.png", uris[0]);
    ASSERT_EQ("file:///test.jpg", uris[1]);
}

/**
 * @tc.name: ExtractImgSrcTest009
 * @tc.desc: ExtractImgSrc should match valid image extensions (case sensitive)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest009, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///test.png\"><img src=\"file:///test.jpg\">"
        "<img src=\"file:///test.gif\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/test.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(3U, uris.size());
}

/**
 * @tc.name: ExtractImgSrcTest010
 * @tc.desc: ExtractImgSrc should filter out invalid image extensions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest010, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///test.png\"><img src=\"file:///test.txt\">"
        "<img src=\"file:///test.pdf\"><img src=\"file:///test.doc\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/test.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///test.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest003
 * @tc.desc: ExtractImgSrc should filter out non-existent files
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest003, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///Existing.png\"><img src=\"file:///Noexisting.png\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            if (uri.find("Existing") != std::string::npos) {
                physicalPath = "/mnt/Existing.png";
            } else {
                physicalPath = "/mnt/Noexisting.png";
            }
            return 0;
        });

    // Mock stat to return success for existing file and failure for non-existing file
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            std::string pathStr(path);
            if (pathStr.find("Existing") != std::string::npos) {
                buf->st_mode = S_IFREG; // Regular file
                buf->st_size = 1024;
                return 0;
            } else {
                errno = ENOENT;
                return -1;
            }
        });

    // Use ExtractImgSrc to test
    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);

    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///Existing.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest004
 * @tc.desc: ExtractImgSrc should filter out directories
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest004, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///file.png\"><img src=\"file:///directory\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            if (uri.find("file") != std::string::npos) {
                physicalPath = "/mnt/file.png";
            } else {
                physicalPath = "/mnt/directory";
            }
            return 0;
        });

    // Mock stat to return success for both but indicate directory for the second
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            std::string pathStr(path);
            if (pathStr.find("file") != std::string::npos) {
                buf->st_mode = S_IFREG; // Regular file
            } else {
                buf->st_mode = S_IFDIR; // Directory
            }
            buf->st_size = 1024;
            return 0;
        });

    // Use ExtractImgSrc to test
    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);

    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///file.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest005
 * @tc.desc: ExtractImgSrc should handle invalid paths
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest005, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///Valid.png\">"
        "<img src=\"file:///Invalid.png\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true for valid path and false for invalid path
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly([](const std::string &path) {
            return path.find("Valid") != std::string::npos;
        });

    // Only valid path should reach GetPhysicalPath
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/Valid.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    // Use ExtractImgSrc to test
    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);

    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///Valid.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest006
 * @tc.desc: ExtractImgSrc should filter out non-image files
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest006, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///test.png\"><img src=\"file:///test.txt\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/test.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///test.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest007
 * @tc.desc: ExtractImgSrc should handle case insensitive extensions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest007, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///test.PNG\"><img src=\"file:///test.JPEG\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/test.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(2U, uris.size());
}

/**
 * @tc.name: ExtractImgSrcTest008
 * @tc.desc: ExtractImgSrc should handle complex image extensions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest008, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///test.heic\">"
        "<img src=\"file:///test.webp\"><img src=\"file:///test.avif\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/test.heic";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(3U, uris.size());
}

/**
 * @tc.name: ExtractImgSrcTest011
 * @tc.desc: ExtractImgSrc should filter out non-file URIs
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest011, TestSize.Level0)
{
    std::string htmlContent = "<html><body>"
        "<img src=\"http://example.com/image.png\">"
        "<img src=\"https://example.com/image.jpg\">"
        "<img src=\"file:///local/image.png\">"
        "<img src=\"relative/path/image.gif\">"
        "</body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/image.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///local/image.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest012
 * @tc.desc: ExtractImgSrc should handle IMG_LOCAL_URI and DOC_LOCAL_URI correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest012, TestSize.Level0)
{
    std::string htmlContent = "<html><body>"
        "<img src=\"file:///local/image.png\">"
        "<img src=\"file:///docs/document/image.jpg\">"
        "</body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to verify URI transformation
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .Times(2)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            // Verify that the URIs are transformed correctly
            if (uri == "file://testBundle/local/image.png" || uri == "file://docs/document/image.jpg") {
                physicalPath = "/mnt/image.png";
                return 0;
            }
            return -1;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(2U, uris.size());
}

/**
 * @tc.name: ExtractImgSrcTest013
 * @tc.desc: ExtractImgSrc should handle GetPhysicalPath error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest013, TestSize.Level0)
{
    std::string htmlContent = "<html><body>"
        "<img src=\"file:///local/image.png\">"
        "<img src=\"file:///local/image2.png\">"
        "</body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return error for one image
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .Times(2)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            if (uri == "file://testBundle/local/image2.png") {
                // Return error for the second image
                return -1;
            }
            physicalPath = "/mnt/image.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .Times(1) // Only one image should reach stat
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///local/image.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest014
 * @tc.desc: ExtractImgSrc should handle EACCES error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest014, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///restricted/image.png\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to set errno=EACCES
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .Times(1)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/image.png";
            return 0;
        });

    // Mock stat to return failed, with set errno=EACCES
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .Times(1)
        .WillRepeatedly([](auto, auto) {
            errno = EACCES;
            return -1;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///restricted/image.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest015
 * @tc.desc: ExtractImgSrc should handle MatchImgExtension boundary conditions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest015, TestSize.Level0)
{
    // Test edge cases for MatchImgExtension
    std::string htmlContent = "<html><body>"
        "<img src=\"file:///no_extension\">"
        "<img src=\"file:///.png\">"
        "<img src=\"file:///only.dot\">"
        "<img src=\"file:///multiple.dots.png\">"
        "<img src=\"file:///path.with.dots/image.png\">"
        "</body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/image.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    // Only the files with valid extensions should be included
    // multiple.dots.png and path.with.dots/image.png should be included
    ASSERT_EQ(2U, uris.size());
}

/**
 * @tc.name: ExtractImgSrcTest016
 * @tc.desc: ExtractImgSrc should handle HWF_SERVICE_UID with specific path
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest016, TestSize.Level0)
{
    std::string htmlContent = "<html><body><img src=\"file:///docs/storage/Users/currentUser/image.png\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath should not be called for HWF_SERVICE_UID
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .Times(0);

    // Mock GetCallingUid to return HWF_SERVICE_UID
    NiceMock<OHOS::IpcMock> ipcMock;
    EXPECT_CALL(ipcMock, GetCallingUid)
        .Times(1)
        .WillOnce(Return(7700)); // HWF_SERVICE_UID

    // Mock stat to return success for the transformed path
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .Times(1)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            std::string pathStr(path);
            // For HWF_SERVICE_UID, the path should be directly used without transformation
            // since it's already in the correct format
            if (pathStr.find("image.png") != std::string::npos) {
                buf->st_mode = S_IFREG; // Regular file
                buf->st_size = 1024;
                return 0;
            }
            return -1;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    // The file should be included because callingUid == HWF_SERVICE_UID
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///docs/storage/Users/currentUser/image.png", uris[0]);
}

/**
 * @tc.name: ExtractImgSrcTest017
 * @tc.desc: ExtractImgSrc should handle htmlReadDoc with special characters
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest017, TestSize.Level0)
{
    // Test with HTML containing special characters
    std::string htmlContent = "<html><body><img src=\"file:///test.png\" alt=\"Test Image\">"
        "<img src=\"file:///test.jpg\" title=\"Another Image\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/image.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(2U, uris.size());
}

/**
 * @tc.name: ExtractImgSrcTest018
 * @tc.desc: ExtractImgSrc should handle empty img src
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardImgExtractorMockTest, ExtractImgSrcTest018, TestSize.Level0)
{
    // Test with empty img src
    std::string htmlContent = "<html><body><img src=\"\"><img src=\"file:///test.png\"></body></html>";
    std::string bundleIndex = "testBundle";
    int32_t userId = 100;

    // Mock IsValidPath to return true
    NiceMock<OHOS::AppFileService::SandboxHelperMock> sandboxMock;
    EXPECT_CALL(sandboxMock, IsValidPath)
        .WillRepeatedly(Return(true));

    // Mock GetPhysicalPath to return valid paths
    EXPECT_CALL(sandboxMock, GetPhysicalPath)
        .WillRepeatedly([](const std::string &uri, const std::string &userId, std::string &physicalPath) {
            physicalPath = "/mnt/image.png";
            return 0;
        });

    // Mock stat to return success
    NiceMock<PasteBoardCommonMock> PasteBoardCommonMock;
    EXPECT_CALL(PasteBoardCommonMock, Stat)
        .WillRepeatedly([](const char *path, struct stat *buf) {
            buf->st_mode = S_IFREG; // Regular file
            buf->st_size = 1024;
            return 0;
        });

    auto uris = PasteboardImgExtractor::GetInstance().ExtractImgSrc(htmlContent, bundleIndex, userId);
    ASSERT_EQ(1U, uris.size());
    ASSERT_EQ("file:///test.png", uris[0]);
}
} // anonymous namespace
