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

// 序列化时parcel预留的剩余可写字节数, 使目标字段写入时空间不足而失败;
// rawDataSize为int64(8字节), 其后textDataSize/htmlDataSize/isDelayedData/isDelayedRecord/mimeTypesSize各占4字节
constexpr size_t FREE_FOR_RAW_DATA_SIZE = 0;
constexpr size_t FREE_FOR_TEXT_DATA_SIZE = 8;
constexpr size_t FREE_FOR_HTML_DATA_SIZE = 12;
constexpr size_t FREE_FOR_IS_DELAYED_DATA = 16;
constexpr size_t FREE_FOR_IS_DELAYED_RECORD = 20;
constexpr size_t FREE_FOR_MIME_TYPES_SIZE = 24;
constexpr size_t FREE_FOR_MIME_TYPE_STRING = 28;

// 反序列化前需成功写入的定长字段个数, 使紧随其后的目标字段读取时数据不足而失败
constexpr uint32_t WRITTEN_NONE = 0;              // 触发rawDataSize读取失败
constexpr uint32_t WRITTEN_RAW = 1;               // 触发textDataSize读取失败
constexpr uint32_t WRITTEN_TEXT = 2;              // 触发htmlDataSize读取失败
constexpr uint32_t WRITTEN_HTML = 3;              // 触发isDelayedData读取失败
constexpr uint32_t WRITTEN_IS_DELAYED_DATA = 4;   // 触发isDelayedRecord读取失败
constexpr uint32_t WRITTEN_IS_DELAYED_RECORD = 5; // 触发mimeTypesSize读取失败
constexpr uint32_t WRITTEN_MIME_TYPES_SIZE = 6;   // 全部定长字段写完, 再声明mimeType数量后触发字符串读取失败
} // namespace

class PasteDataInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) { }
    static void TearDownTestCase(void) { }
    void SetUp() { }
    void TearDown() { }

    // 构造一个各字段均已填充的PasteDataInfo
    static PasteDataInfo BuildPasteDataInfo();
    // 序列化: 预留freeSize字节使目标字段写入失败, 断言Marshalling返回false
    static void ExpectMarshallingFail(size_t freeSize);
    // 反序列化: 向parcel成功写入前fixedFieldCount个定长字段(rawDataSize为int64, 其余按4字节写入)
    static void WriteFixedFields(Parcel &parcel, uint32_t fixedFieldCount);
    // 反序列化: 断言Unmarshalling返回nullptr
    static void ExpectUnmarshallingNull(Parcel &parcel);
};

PasteDataInfo PasteDataInfoTest::BuildPasteDataInfo()
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

void PasteDataInfoTest::ExpectMarshallingFail(size_t freeSize)
{
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    std::vector<uint8_t> filler(parcel.GetMaxCapacity() - freeSize, 0);
    EXPECT_TRUE(parcel.WriteBuffer(filler.data(), filler.size()));
    EXPECT_FALSE(info.Marshalling(parcel));
}

void PasteDataInfoTest::WriteFixedFields(Parcel &parcel, uint32_t fixedFieldCount)
{
    if (fixedFieldCount >= WRITTEN_RAW) {
        EXPECT_TRUE(parcel.WriteInt64(0)); // rawDataSize
    }
    for (uint32_t i = WRITTEN_RAW; i < fixedFieldCount; ++i) {
        EXPECT_TRUE(parcel.WriteInt32(0)); // textDataSize/htmlDataSize/isDelayedData/isDelayedRecord/mimeTypesSize
    }
}

void PasteDataInfoTest::ExpectUnmarshallingNull(Parcel &parcel)
{
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
}

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

/**
 * @tc.name: MarshallingRawDataSizeFailTest
 * @tc.desc: 写入rawDataSize空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingRawDataSizeFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_RAW_DATA_SIZE);
}

/**
 * @tc.name: MarshallingTextDataSizeFailTest
 * @tc.desc: 写入textDataSize空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTextDataSizeFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_TEXT_DATA_SIZE);
}

/**
 * @tc.name: MarshallingHtmlDataSizeFailTest
 * @tc.desc: 写入htmlDataSize空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingHtmlDataSizeFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_HTML_DATA_SIZE);
}

/**
 * @tc.name: MarshallingIsDelayedDataFailTest
 * @tc.desc: 写入isDelayedData空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingIsDelayedDataFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_IS_DELAYED_DATA);
}

/**
 * @tc.name: MarshallingIsDelayedRecordFailTest
 * @tc.desc: 写入isDelayedRecord空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingIsDelayedRecordFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_IS_DELAYED_RECORD);
}

/**
 * @tc.name: MarshallingMimeTypesSizeFailTest
 * @tc.desc: 写入mimeTypesSize空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingMimeTypesSizeFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_MIME_TYPES_SIZE);
}

/**
 * @tc.name: MarshallingMimeTypeStringFailTest
 * @tc.desc: 循环中写入mimeType字符串空间不足时Marshalling应返回false
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingMimeTypeStringFailTest, TestSize.Level0)
{
    ExpectMarshallingFail(FREE_FOR_MIME_TYPE_STRING);
}

/**
 * @tc.name: UnmarshallingRawDataSizeFailTest
 * @tc.desc: 读取rawDataSize失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingRawDataSizeFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_NONE);
    ExpectUnmarshallingNull(parcel);
}

/**
 * @tc.name: UnmarshallingTextDataSizeFailTest
 * @tc.desc: 读取textDataSize失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingTextDataSizeFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_RAW);
    ExpectUnmarshallingNull(parcel);
}

/**
 * @tc.name: UnmarshallingHtmlDataSizeFailTest
 * @tc.desc: 读取htmlDataSize失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingHtmlDataSizeFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_TEXT);
    ExpectUnmarshallingNull(parcel);
}

/**
 * @tc.name: UnmarshallingIsDelayedDataFailTest
 * @tc.desc: 读取isDelayedData失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingIsDelayedDataFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_HTML);
    ExpectUnmarshallingNull(parcel);
}

/**
 * @tc.name: UnmarshallingIsDelayedRecordFailTest
 * @tc.desc: 读取isDelayedRecord失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingIsDelayedRecordFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_IS_DELAYED_DATA);
    ExpectUnmarshallingNull(parcel);
}

/**
 * @tc.name: UnmarshallingMimeTypesSizeFailTest
 * @tc.desc: 读取mimeTypesSize失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingMimeTypesSizeFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_IS_DELAYED_RECORD);
    ExpectUnmarshallingNull(parcel);
}

/**
 * @tc.name: UnmarshallingMimeTypeStringFailTest
 * @tc.desc: 循环中读取mimeType字符串失败时Unmarshalling应返回nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingMimeTypeStringFailTest, TestSize.Level0)
{
    Parcel parcel;
    WriteFixedFields(parcel, WRITTEN_MIME_TYPES_SIZE);
    EXPECT_TRUE(parcel.WriteUint32(1)); // 声明1个mimeType但不写入其内容, 使ReadString失败
    ExpectUnmarshallingNull(parcel);
}
} // namespace OHOS::MiscServices
