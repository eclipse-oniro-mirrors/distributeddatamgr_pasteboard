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

#include "pasteboard_linked_list.h"

namespace OHOS {
namespace MiscServices {
using namespace testing::ext;

class PasteboardLinkedListTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardLinkedListTest::SetUpTestCase(void)
{
}

void PasteboardLinkedListTest::TearDownTestCase(void)
{
}

void PasteboardLinkedListTest::SetUp(void)
{
}

void PasteboardLinkedListTest::TearDown(void)
{
}

/**
 * @tc.name: TestInsert001
 * @tc.desc: Test InsertFront
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLinkedListTest, TestInsert001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestInsert001 start");
    LinkedList<int32_t> lst;
    lst.InsertFront(0);
    lst.InsertFront(1);
    lst.InsertFront(2);

    std::vector<int32_t> vec;
    lst.ForEach([&vec](int32_t value) {
        vec.push_back(value);
    });

    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 2);
    EXPECT_EQ(vec[1], 1);
    EXPECT_EQ(vec[2], 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestInsert001 end");
}

/**
 * @tc.name: TestInsert002
 * @tc.desc: Test InsertTail
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLinkedListTest, TestInsert002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestInsert002 start");
    LinkedList<int32_t> lst;
    lst.InsertTail(0);
    lst.InsertTail(1);
    lst.InsertTail(2);

    std::vector<int32_t> vec;
    lst.ForEach([&vec](int32_t value) {
        vec.push_back(value);
    });

    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 0);
    EXPECT_EQ(vec[1], 1);
    EXPECT_EQ(vec[2], 2);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestInsert002 end");
}

/**
 * @tc.name: TestInsert003
 * @tc.desc: Test InsertFront & InsertTail
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLinkedListTest, TestInsert003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestInsert003 start");
    LinkedList<int32_t> lst;
    lst.InsertTail(0);
    lst.InsertFront(1);
    lst.InsertTail(2);

    std::vector<int32_t> vec;
    lst.ForEach([&vec](int32_t value) {
        vec.push_back(value);
    });

    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 0);
    EXPECT_EQ(vec[2], 2);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestInsert003 end");
}

/**
 * @tc.name: TestFindExist001
 * @tc.desc: Test FindExist
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLinkedListTest, TestFindExist001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestFindExist001 start");
    LinkedList<int32_t> lst;
    lst.InsertTail(0);
    EXPECT_TRUE(lst.FindExist(0));
    EXPECT_FALSE(lst.FindExist(1));
    EXPECT_FALSE(lst.FindExist(2));
    EXPECT_FALSE(lst.FindExist([](int32_t value) {
        return value > 0;
    }));

    lst.InsertFront(1);
    EXPECT_TRUE(lst.FindExist(0));
    EXPECT_TRUE(lst.FindExist(1));
    EXPECT_FALSE(lst.FindExist(2));
    EXPECT_TRUE(lst.FindExist([](int32_t value) {
        return value > 0;
    }));
    EXPECT_FALSE(lst.FindExist([](int32_t value) {
        return value > 1;
    }));

    lst.InsertTail(2);
    EXPECT_TRUE(lst.FindExist(0));
    EXPECT_TRUE(lst.FindExist(1));
    EXPECT_TRUE(lst.FindExist(2));
    EXPECT_TRUE(lst.FindExist([](int32_t value) {
        return value > 1;
    }));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestFindExist001 end");
}

/**
 * @tc.name: TestRemoveIf001
 * @tc.desc: Test RemoveIf
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardLinkedListTest, TestRemoveIf001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestRemoveIf001 start");
    LinkedList<int32_t> lst;
    lst.InsertTail(0);
    lst.InsertFront(1);
    lst.InsertTail(2);
    lst.InsertFront(2);
    EXPECT_TRUE(lst.FindExist(0));
    EXPECT_TRUE(lst.FindExist(1));
    EXPECT_TRUE(lst.FindExist(2));

    lst.RemoveIf([](int32_t value) {
        return value == 0;
    });
    EXPECT_FALSE(lst.FindExist(0));
    EXPECT_TRUE(lst.FindExist(1));
    EXPECT_TRUE(lst.FindExist(2));

    lst.InsertTail(0);
    lst.RemoveIf([](int32_t value) {
        return value > 0;
    });
    EXPECT_TRUE(lst.FindExist(0));
    EXPECT_FALSE(lst.FindExist(1));
    EXPECT_FALSE(lst.FindExist(2));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestRemoveIf001 end");
}

} // namespace MiscServices
} // namespace OHOS
