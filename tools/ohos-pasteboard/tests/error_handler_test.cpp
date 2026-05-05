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
#include "error_handler.h"
#include "pasteboard_error.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Pasteboard;

class ErrorHandlerTest : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ErrorHandlerTest, HandleSetPasteDataError_ObtainServerError, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleSetPasteDataError(
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    
    EXPECT_TRUE(result.find("ERR_SERVICE_UNAVAILABLE") != std::string::npos);
    EXPECT_TRUE(result.find("Pasteboard service unavailable") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleSetPasteDataError_InvalidParamError, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleSetPasteDataError(
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    
    EXPECT_TRUE(result.find("ERR_INVALID_PARAM") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleSetPasteDataError_InvalidDataSize, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleSetPasteDataError(
        static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE));
    
    EXPECT_TRUE(result.find("ERR_DATA_SIZE") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleSetPasteDataError_NoDataError, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleSetPasteDataError(
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    
    EXPECT_TRUE(result.find("ERR_NO_DATA") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleSetPasteDataError_UnknownError, TestSize.Level1)
{
    std::string result = ErrorHandler::HandleSetPasteDataError(99999);
    
    EXPECT_TRUE(result.find("ERR_SET_DATA_FAILED") != std::string::npos);
    EXPECT_TRUE(result.find("99999") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_ObtainServerError, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    
    EXPECT_TRUE(result.find("ERR_SERVICE_UNAVAILABLE") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_PermissionDenied, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
    
    EXPECT_TRUE(result.find("ERR_PERMISSION_DENIED") != std::string::npos);
    EXPECT_TRUE(result.find("READ_PASTEBOARD") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_NoDataError, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
    
    EXPECT_TRUE(result.find("ERR_NO_DATA") != std::string::npos);
    EXPECT_TRUE(result.find("Pasteboard is empty") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_DataExpired, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR));
    
    EXPECT_TRUE(result.find("ERR_DATA_EXPIRED") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_TimeoutError, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::TIMEOUT_ERROR));
    
    EXPECT_TRUE(result.find("ERR_TIMEOUT") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_RemoteException, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::REMOTE_EXCEPTION));
    
    EXPECT_TRUE(result.find("ERR_REMOTE") != std::string::npos);
}

HWTEST_F(ErrorHandlerTest, HandleGetPasteDataError_CrossBorder, TestSize.Level1)
{
    using namespace OHOS::MiscServices;
    std::string result = ErrorHandler::HandleGetPasteDataError(
        static_cast<int32_t>(PasteboardError::CROSS_BORDER_ERROR));
    
    EXPECT_TRUE(result.find("ERR_CROSS_BORDER") != std::string::npos);
}