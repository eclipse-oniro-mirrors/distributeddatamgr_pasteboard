/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <functional>

#include <gtest/gtest.h>

#include "parcel.h"
#include "paste_data_info.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

namespace {
constexpr int64_t TEST_RAW_DATA_SIZE = 0x100000000; // 4GB, 超过INT32_MAX, 用于验证不发生截断
constexpr int32_t TEST_TEXT_DATA_SIZE = 200;
constexpr int32_t TEST_HTML_DATA_SIZE = 300;

// 构造一个各字段均已填充的PasteDataInfo
PasteDataInfo BuildPasteDataInfo()
{
    PasteDataInfo info;
    info.rawDataSize = TEST_RAW_DATA_SIZE;
    info.textDataSize = TEST_TEXT_DATA_SIZE;
    info.htmlDataSize = TEST_HTML_DATA_SIZE;
    info.isDelayedData = true;
    info.isDelayedRecord = true;
    info.mimeTypes = { "text/plain", "text/html" };
    return info;
}

// Marshalling失败用例参数: 序列化前在parcel中预留freeSize字节, 使目标字段写入时空间不足而失败
struct MarshallingFailParam {
    std::string caseName; // 目标失败字段, 同时作为用例名
    size_t freeSize;      // parcel剩余可写字节数
};

// Unmarshalling失败用例参数: prepare向parcel写入目标字段之前的所有字段, 使目标字段读取时数据不足而失败
struct UnmarshallingFailParam {
    std::string caseName;                  // 目标失败字段, 同时作为用例名
    std::function<void(Parcel &)> prepare; // 预写parcel数据
};
} // namespace

class PasteDataInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) { }
    static void TearDownTestCase(void) { }
    void SetUp() { }
    void TearDown() { }
};

/**
 * @tc.name: DefaultConstructTest001
 * @tc.desc: 默认构造函数应将所有字段初始化为0/false/空
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, DefaultConstructTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DefaultConstructTest001 start");
    PasteDataInfo info;
    EXPECT_EQ(info.rawDataSize, 0);
    EXPECT_EQ(info.textDataSize, 0);
    EXPECT_EQ(info.htmlDataSize, 0);
    EXPECT_EQ(info.isDelayedData, false);
    EXPECT_EQ(info.isDelayedRecord, false);
    EXPECT_TRUE(info.mimeTypes.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DefaultConstructTest001 end");
}

/**
 * @tc.name: CopyConstructTest001
 * @tc.desc: 拷贝构造函数应复制所有字段
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, CopyConstructTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CopyConstructTest001 start");
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo copyInfo(info);
    EXPECT_EQ(copyInfo.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(copyInfo.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(copyInfo.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(copyInfo.isDelayedData, true);
    EXPECT_EQ(copyInfo.isDelayedRecord, true);
    EXPECT_EQ(copyInfo.mimeTypes, info.mimeTypes);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CopyConstructTest001 end");
}

/**
 * @tc.name: OperatorAssignTest001
 * @tc.desc: 赋值运算符作用于不同对象(this != &dataInfo)时应复制所有字段
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, OperatorAssignTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignTest001 start");
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo assignInfo;
    assignInfo = info;
    EXPECT_EQ(assignInfo.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(assignInfo.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(assignInfo.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(assignInfo.isDelayedData, true);
    EXPECT_EQ(assignInfo.isDelayedRecord, true);
    EXPECT_EQ(assignInfo.mimeTypes, info.mimeTypes);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignTest001 end");
}

/**
 * @tc.name: OperatorAssignSelfTest001
 * @tc.desc: 自赋值(this == &dataInfo)时应命中提前返回, 字段保持不变
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, OperatorAssignSelfTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignSelfTest001 start");
    const std::vector<std::string> expectMimeTypes = { "text/plain", "text/html" };
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo &selfRef = info;
    info = selfRef;
    EXPECT_EQ(info.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(info.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(info.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(info.isDelayedData, true);
    EXPECT_EQ(info.isDelayedRecord, true);
    EXPECT_EQ(info.mimeTypes, expectMimeTypes);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignSelfTest001 end");
}

/**
 * @tc.name: MarshallingTest001
 * @tc.desc: 含mimeTypes时, 序列化后再反序列化应保持所有字段
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest001 start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));

    PasteDataInfo *newInfo = PasteDataInfo::Unmarshalling(parcel);
    ASSERT_NE(newInfo, nullptr);
    EXPECT_EQ(newInfo->rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(newInfo->textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(newInfo->htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(newInfo->isDelayedData, true);
    EXPECT_EQ(newInfo->isDelayedRecord, true);
    EXPECT_EQ(newInfo->mimeTypes, info.mimeTypes);
    delete newInfo;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest001 end");
}

/**
 * @tc.name: MarshallingTest002
 * @tc.desc: mimeTypes为空时序列化/反序列化应正常(跳过mimeType写入循环)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest002 start");
    PasteDataInfo info;
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));

    PasteDataInfo *newInfo = PasteDataInfo::Unmarshalling(parcel);
    ASSERT_NE(newInfo, nullptr);
    EXPECT_EQ(newInfo->rawDataSize, 0);
    EXPECT_EQ(newInfo->isDelayedData, false);
    EXPECT_EQ(newInfo->isDelayedRecord, false);
    EXPECT_TRUE(newInfo->mimeTypes.empty());
    delete newInfo;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest002 end");
}

class PasteDataInfoMarshallingFailTest : public testing::TestWithParam<MarshallingFailParam> {};

/**
 * @tc.name: MarshallingFail
 * @tc.desc: 参数化覆盖Marshalling中每个字段写入失败时返回false的分支
 *           (rawDataSize/textDataSize/htmlDataSize/isDelayedData/isDelayedRecord/mimeTypesSize/mimeType字符串)
 * @tc.type: FUNC
 */
HWTEST_P(PasteDataInfoMarshallingFailTest, MarshallingFail, TestSize.Level0)
{
    MarshallingFailParam param = GetParam();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingFail_%{public}s start", param.caseName.c_str());
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    // 填充parcel使其仅剩freeSize字节可写, 令目标字段写入时空间不足
    std::vector<uint8_t> filler(parcel.GetMaxCapacity() - param.freeSize, 0);
    EXPECT_TRUE(parcel.WriteBuffer(filler.data(), filler.size()));
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingFail_%{public}s end", param.caseName.c_str());
}

// rawDataSize为int64(8字节), 其后textDataSize/htmlDataSize/isDelayedData/isDelayedRecord/mimeTypesSize各占4字节
INSTANTIATE_TEST_SUITE_P(PasteDataInfo, PasteDataInfoMarshallingFailTest,
    testing::Values(MarshallingFailParam { "RawDataSize", 0 }, MarshallingFailParam { "TextDataSize", 8 },
        MarshallingFailParam { "HtmlDataSize", 12 }, MarshallingFailParam { "IsDelayedData", 16 },
        MarshallingFailParam { "IsDelayedRecord", 20 }, MarshallingFailParam { "MimeTypesSize", 24 },
        MarshallingFailParam { "MimeTypeString", 28 }),
    [](const testing::TestParamInfo<MarshallingFailParam> &info) { return info.param.caseName; });

class PasteDataInfoUnmarshallingFailTest : public testing::TestWithParam<UnmarshallingFailParam> {};

/**
 * @tc.name: UnmarshallingFail
 * @tc.desc: 参数化覆盖Unmarshalling中每个字段读取失败时返回nullptr的分支
 *           (rawDataSize/textDataSize/htmlDataSize/isDelayedData/isDelayedRecord/mimeTypesSize/mimeType字符串)
 * @tc.type: FUNC
 */
HWTEST_P(PasteDataInfoUnmarshallingFailTest, UnmarshallingFail, TestSize.Level0)
{
    UnmarshallingFailParam param = GetParam();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingFail_%{public}s start", param.caseName.c_str());
    Parcel parcel;
    param.prepare(parcel);
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingFail_%{public}s end", param.caseName.c_str());
}

// 每条prepare只写入目标字段之前的字段, 使目标字段读取时数据不足; MimeTypeString额外声明1个mimeType但不写入其内容
INSTANTIATE_TEST_SUITE_P(PasteDataInfo, PasteDataInfoUnmarshallingFailTest,
    testing::Values(
        UnmarshallingFailParam { "RawDataSize", [](Parcel &) { } },
        UnmarshallingFailParam { "TextDataSize", [](Parcel &parcel) { EXPECT_TRUE(parcel.WriteInt64(0)); } },
        UnmarshallingFailParam { "HtmlDataSize",
            [](Parcel &parcel) {
                EXPECT_TRUE(parcel.WriteInt64(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
            } },
        UnmarshallingFailParam { "IsDelayedData",
            [](Parcel &parcel) {
                EXPECT_TRUE(parcel.WriteInt64(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
            } },
        UnmarshallingFailParam { "IsDelayedRecord",
            [](Parcel &parcel) {
                EXPECT_TRUE(parcel.WriteInt64(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteBool(false));
            } },
        UnmarshallingFailParam { "MimeTypesSize",
            [](Parcel &parcel) {
                EXPECT_TRUE(parcel.WriteInt64(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteBool(false));
                EXPECT_TRUE(parcel.WriteBool(false));
            } },
        UnmarshallingFailParam { "MimeTypeString",
            [](Parcel &parcel) {
                EXPECT_TRUE(parcel.WriteInt64(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteInt32(0));
                EXPECT_TRUE(parcel.WriteBool(false));
                EXPECT_TRUE(parcel.WriteBool(false));
                EXPECT_TRUE(parcel.WriteUint32(1));
            } }),
    [](const testing::TestParamInfo<UnmarshallingFailParam> &info) { return info.param.caseName; });
} // namespace OHOS::MiscServices
