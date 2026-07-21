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
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace {
constexpr int32_t INVALID_VERSION = -1;
constexpr int32_t ADD_PERMISSION_CHECK_SDK_VERSION = 12;
const std::string SECURE_PASTE_PERMISSION = "ohos.permission.SECURE_PASTE";
const std::string READ_PASTEBOARD_PERMISSION = "ohos.permission.READ_PASTEBOARD";
}

class PasteboardServicePermissionTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServicePermissionTest SetUpTestCase");
    }

    static void TearDownTestCase()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServicePermissionTest TearDownTestCase");
    }

    void SetUp()
    {
        service_ = std::make_shared<PasteboardService>();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServicePermissionTest SetUp");
    }

    void TearDown()
    {
        service_ = nullptr;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardServicePermissionTest TearDown");
    }

protected:
    std::shared_ptr<PasteboardService> service_ = nullptr;
};

/**
 * @tc.name: IsDataValidTest004
 * @tc.desc: test IsDataValid with dragged data, should return INVALID_PARAM_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest004 start");
    PasteData pasteData;
    pasteData.SetDraggedData(true);
    uint32_t tokenId = 123456;
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest004 end");
}

/**
 * @tc.name: IsDataValidTest005
 * @tc.desc: test IsDataValid with invalid data, should return INVALID_PARAM_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest005 start");
    PasteData pasteData;
    pasteData.AddTextRecord("");
    uint32_t tokenId = 123456;
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest005 end");
}

/**
 * @tc.name: IsDataValidTest006
 * @tc.desc: test IsDataValid with InApp shareOption and different tokenId, should return PERMISSION_VERIFICATION_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest006 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetShareOption(ShareOption::InApp);
    pasteData.SetTokenId(111111);
    uint32_t tokenId = 222222;
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest006 end");
}

/**
 * @tc.name: IsDataValidTest007
 * @tc.desc: test IsDataValid with InApp shareOption and same tokenId, should return E_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest007 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetShareOption(ShareOption::InApp);
    uint32_t tokenId = 123456;
    pasteData.SetTokenId(tokenId);
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest007 end");
}

/**
 * @tc.name: IsDataValidTest008
 * @tc.desc: test IsDataValid with LocalDevice shareOption, should return E_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest008 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetShareOption(ShareOption::LocalDevice);
    uint32_t tokenId = 123456;
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest008 end");
}

/**
 * @tc.name: IsDataValidTest009
 * @tc.desc: test IsDataValid with CrossDevice shareOption, should return E_OK
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest009 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetShareOption(ShareOption::CrossDevice);
    uint32_t tokenId = 123456;
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest009 end");
}

/**
 * @tc.name: IsDataValidTest010
 * @tc.desc: test IsDataValid with invalid shareOption value, should return INVALID_DATA_ERROR
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataValidTest010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest010 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetShareOption(static_cast<ShareOption>(999));
    uint32_t tokenId = 123456;
    int32_t userId = 100;

    int32_t ret = service_->IsDataValid(pasteData, tokenId, userId);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataValidTest010 end");
}

/**
 * @tc.name: IsDataAgedTest002
 * @tc.desc: test IsDataAged with ERROR_USERID, should return true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataAgedTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest002 start");
    int32_t userId = ERROR_USERID;

    bool ret = service_->IsDataAged(userId);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest002 end");
}

/**
 * @tc.name: IsDataAgedTest003
 * @tc.desc: test IsDataAged with valid userId but no copyTime, should return true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataAgedTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest003 start");
    int32_t userId = 100;

    bool ret = service_->IsDataAged(userId);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest003 end");
}

/**
 * @tc.name: IsDataAgedTest004
 * @tc.desc: test IsDataAged with valid userId and copyTime exists, should return false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsDataAgedTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest004 start");
    int32_t userId = 100;
    uint64_t curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    service_->copyTime_.InsertOrAssign(userId, curTime);

    bool ret = service_->IsDataAged(userId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsDataAgedTest004 end");
}

/**
 * @tc.name: SetLocalPasteFlagTest001
 * @tc.desc: test SetLocalPasteFlag with local paste (isCrossPaste=false, tokenId matches)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, SetLocalPasteFlagTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlagTest001 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    uint32_t tokenId = 123456;
    pasteData.SetTokenId(tokenId);

    service_->SetLocalPasteFlag(false, tokenId, pasteData);
    EXPECT_TRUE(pasteData.IsLocalPaste());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlagTest001 end");
}

/**
 * @tc.name: SetLocalPasteFlagTest002
 * @tc.desc: test SetLocalPasteFlag with cross paste (isCrossPaste=true)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, SetLocalPasteFlagTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlagTest002 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    uint32_t tokenId = 123456;
    pasteData.SetTokenId(tokenId);

    service_->SetLocalPasteFlag(true, tokenId, pasteData);
    EXPECT_FALSE(pasteData.IsLocalPaste());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlagTest002 end");
}

/**
 * @tc.name: SetLocalPasteFlagTest003
 * @tc.desc: test SetLocalPasteFlag with tokenId mismatch
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, SetLocalPasteFlagTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlagTest003 start");
    PasteData pasteData;
    pasteData.AddTextRecord("test");
    pasteData.SetTokenId(111111);
    uint32_t tokenId = 222222;

    service_->SetLocalPasteFlag(false, tokenId, pasteData);
    EXPECT_FALSE(pasteData.IsLocalPaste());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetLocalPasteFlagTest003 end");
}

/**
 * @tc.name: IsBundleOwnUriPermissionTest001
 * @tc.desc: test IsBundleOwnUriPermission with matching bundleName and authority
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsBundleOwnUriPermissionTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBundleOwnUriPermissionTest002 start");
    std::string bundleName = "com.example.test";
    Uri uri("file://com.example.test/path");

    bool ret = service_->IsBundleOwnUriPermission(bundleName, uri);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBundleOwnUriPermissionTest002 end");
}

/**
 * @tc.name: IsBundleOwnUriPermissionTest003
 * @tc.desc: test IsBundleOwnUriPermission with different bundleName and authority
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsBundleOwnUriPermissionTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBundleOwnUriPermissionTest003 start");
    std::string bundleName = "com.example.test";
    Uri uri("file://com.example.other/path");

    bool ret = service_->IsBundleOwnUriPermission(bundleName, uri);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsBundleOwnUriPermissionTest003 end");
}

/**
 * @tc.name: IsCopyableTest002
 * @tc.desc: test IsCopyable with valid tokenId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsCopyableTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCopyableTest002 start");
    uint32_t tokenId = 123456;

    bool ret = service_->IsCopyable(tokenId);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsCopyableTest002 end");
}

/**
 * @tc.name: IsSystemAppByFullTokenIDTest002
 * @tc.desc: test IsSystemAppByFullTokenID with system app token ID
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsSystemAppByFullTokenIDTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest002 start");
    uint64_t systemAppTokenId = static_cast<uint64_t>(1) << 32;

    bool ret = service_->IsSystemAppByFullTokenID(systemAppTokenId);
    EXPECT_TRUE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest002 end");
}

/**
 * @tc.name: IsSystemAppByFullTokenIDTest003
 * @tc.desc: test IsSystemAppByFullTokenID with non-system app token ID
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, IsSystemAppByFullTokenIDTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest003 start");
    uint64_t normalTokenId = 123456;

    bool ret = service_->IsSystemAppByFullTokenID(normalTokenId);
    EXPECT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsSystemAppByFullTokenIDTest003 end");
}

/**
 * @tc.name: GetAppBundleNameTest001
 * @tc.desc: test GetAppBundleName with valid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, GetAppBundleNameTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest001 start");
    AppInfo appInfo;
    appInfo.userId = 100;
    appInfo.bundleName = "com.example.test";

    std::string ret = service_->GetAppBundleName(appInfo);
    EXPECT_EQ(ret, "com.example.test");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest001 end");
}

/**
 * @tc.name: GetAppBundleNameTest002
 * @tc.desc: test GetAppBundleName with ERROR_USERID
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServicePermissionTest, GetAppBundleNameTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest002 start");
    AppInfo appInfo;
    appInfo.userId = ERROR_USERID;
    appInfo.bundleName = "com.example.test";

    std::string ret = service_->GetAppBundleName(appInfo);
    EXPECT_EQ(ret, "error");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppBundleNameTest002 end");
}
}