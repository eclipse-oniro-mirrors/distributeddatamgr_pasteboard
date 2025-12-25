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
#include "entry_getter.h"
#include "pasteboard_entry_getter_client.h"
#include "pasteboard_error.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace MiscServices {
class PasteboardEntryGetterClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardEntryGetterClientTest::SetUpTestCase(void) { }

void PasteboardEntryGetterClientTest::TearDownTestCase(void) { }

void PasteboardEntryGetterClientTest::SetUp(void) { }

void PasteboardEntryGetterClientTest::TearDown(void) { }

class EntryGetterImpl : public UDMF::EntryGetter {
public:
    UDMF::ValueType GetValueByType(const std::string &utdid) override
    {
        return nullptr;
    }
};

/**
 * @tc.name: GetRecordValueByTypeTest001
 * @tc.desc: Test function GetRecordValueByType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterClientTest, GetRecordValueByTypeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest001 start");
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters;
    auto entryGetter = std::make_shared<EntryGetterImpl>();
    entryGetters.insert({0, entryGetter});
    auto pasteboardEntryGetterClient = std::make_shared<PasteboardEntryGetterClient>(entryGetters);
    PasteDataEntry pasteDataEntry;
    int32_t result = pasteboardEntryGetterClient->GetRecordValueByType(1, pasteDataEntry);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest001 end");
}

/**
 * @tc.name: GetRecordValueByTypeTest002
 * @tc.desc: Test function GetRecordValueByType
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntryGetterClientTest, GetRecordValueByTypeTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest002 start");
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters;
    std::shared_ptr<UDMF::EntryGetter> entryGetter = nullptr;
    entryGetters.insert({0, entryGetter});
    auto pasteboardEntryGetterClient = std::make_shared<PasteboardEntryGetterClient>(entryGetters);
    PasteDataEntry pasteDataEntry;
    int32_t result = pasteboardEntryGetterClient->GetRecordValueByType(0, pasteDataEntry);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetRecordValueByTypeTest002 end");
}
}
} // namespace OHOS::MiscServices