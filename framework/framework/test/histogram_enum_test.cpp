/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "common/histogram_enum.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

class HistogramEnumTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void HistogramEnumTest::SetUpTestCase(void) { }

void HistogramEnumTest::TearDownTestCase(void) { }

void HistogramEnumTest::SetUp(void) { }

void HistogramEnumTest::TearDown(void) { }

/**
 * @tc.name: GetHistogramMimeTypeEnumTest001
 * @tc.desc: Test GetHistogramMimeTypeEnum with text/html mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest001, TestSize.Level0)
{
    std::string mimeType = "text/html";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_HTML));
}

/**
 * @tc.name: GetHistogramMimeTypeEnumTest002
 * @tc.desc: Test GetHistogramMimeTypeEnum with text/plain mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest002, TestSize.Level0)
{
    std::string mimeType = "text/plain";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_PLAIN));
}

/**
 * @tc.name: GetHistogramMimeTypeEnumTest003
 * @tc.desc: Test GetHistogramMimeTypeEnum with text/uri mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest003, TestSize.Level0)
{
    std::string mimeType = "text/uri";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_URI));
}

/**
 * @tc.name: GetHistogramMimeTypeEnumTest004
 * @tc.desc: Test GetHistogramMimeTypeEnum with pixelMap mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest004, TestSize.Level0)
{
    std::string mimeType = "pixelMap";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_PIXELMAP));
}

/**
 * @tc.name: GetHistogramMimeTypeEnumTest005
 * @tc.desc: Test GetHistogramMimeTypeEnum with text/want mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest005, TestSize.Level0)
{
    std::string mimeType = "text/want";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_WANT));
}

/**
 * @tc.name: GetHistogramMimeTypeEnumTest006
 * @tc.desc: Test GetHistogramMimeTypeEnum with unknown mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest006, TestSize.Level0)
{
    std::string mimeType = "unknown/type";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_UNKNOWN));
}

/**
 * @tc.name: GetHistogramMimeTypeEnumTest007
 * @tc.desc: Test GetHistogramMimeTypeEnum with unknown mime type.
 * @tc.type: FUNC
 */
HWTEST_F(HistogramEnumTest, GetHistogramMimeTypeEnumTest007, TestSize.Level0)
{
    std::string mimeType = "";
    int32_t result = GetHistogramMimeTypeEnum(mimeType);
    EXPECT_EQ(result, static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_UNKNOWN));
}

} // namespace OHOS::MiscServices