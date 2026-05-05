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

#include "error_handler.h"

#include <map>

namespace OHOS {
namespace Pasteboard {
using namespace OHOS::MiscServices;

static const std::map<int32_t, std::tuple<std::string, std::string, std::string>> SET_DATA_ERR_MAP = {
    {
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        {
            "ERR_SERVICE_UNAVAILABLE",
            "Pasteboard service unavailable",
            "Check if pasteboard service is running properly."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR),
        {
            "ERR_INVALID_PARAM",
            "Invalid parameter",
            "Reduce the data size and try again."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE),
        {
            "ERR_DATA_SIZE",
            "Data size exceeds limit",
            "Reduce the data size and try again."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR),
        {
            "ERR_DATA_INVALID",
            "Invalid paste data format",
            "Check if the data format is supported."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        {
            "ERR_NO_DATA",
            "No data to set",
            "Ensure at least one data type (text/html/uri) is provided."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
        {
            "ERR_SERIALIZATION",
            "Data serialization failed",
            "Reduce the data size and try again."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR),
        {
            "ERR_DESERIALIZATION",
            "Data deserialization failed",
            "Reduce the data size and try again."
        }
    },
};

static const std::map<int32_t, std::tuple<std::string, std::string, std::string>> GET_DATA_ERR_MAP = {
    {
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        {
            "ERR_SERVICE_UNAVAILABLE",
            "Pasteboard service unavailable",
            "Check if pasteboard service is running properly."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR),
        {
            "ERR_PERMISSION_DENIED",
            "Permission denied",
            "Request permission ohos.permission.READ_PASTEBOARD"
        }
    },
    {
        static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        {
            "ERR_NO_DATA",
            "Pasteboard is empty",
            "No paste data available. Use 'set-data' to add data."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::NO_USER_DATA_ERROR),
        {
            "ERR_NO_USER_DATA",
            "No paste data for current user",
            "No paste data available. Use 'set-data' to add data."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR),
        {
            "ERR_DATA_EXPIRED",
            "Paste data has expired",
            "The data is too old and has been cleared automatically."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
        {
            "ERR_SERIALIZATION",
            "Data serialization error",
            "Failed to serialize paste data."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR),
        {
            "ERR_DESERIALIZATION",
            "Data deserialization error",
            "Failed to deserialize paste data."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::TIMEOUT_ERROR),
        {
            "ERR_TIMEOUT",
            "Operation timeout",
            "The operation took too long. Try again later."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::REMOTE_EXCEPTION),
        {
            "ERR_REMOTE",
            "Remote device exception",
            "Failed to get data from remote device."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR),
        {
            "ERR_REMOTE_TASK",
            "Remote task failed",
            "Failed to create remote data task."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::GET_REMOTE_DATA_ERROR),
        {
            "ERR_GET_REMOTE",
            "Failed to get remote data",
            "Remote device is unavailable or network error."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::CROSS_BORDER_ERROR),
        {
            "ERR_CROSS_BORDER",
            "Screen status mismatch",
            "Data was set in different screen status."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::TASK_PROCESSING),
        {
            "ERR_TASK_PROCESSING",
            "Task is processing",
            "Another paste operation is in progress. Wait and try again."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR),
        {
            "ERR_INVALID_USER",
            "Invalid user ID",
            "Current user account is invalid."
        }
    },
    {
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR),
        {
            "ERR_INVALID_PARAM",
            "Invalid pasteId parameter",
            "Internal parameter error."
        }
    },
};

std::string ErrorHandler::BuildErrorResponse(const std::string &code, const std::string &message,
    const std::string &suggestion)
{
    return OutputPrinter::PrintError(code, message, suggestion);
}

std::string ErrorHandler::HandleSetPasteDataError(int32_t errorCode)
{
    auto it = SET_DATA_ERR_MAP.find(errorCode);
    if (it != SET_DATA_ERR_MAP.end()) {
        auto [code, message, suggestion] = it->second;
        return BuildErrorResponse(code, message, suggestion);
    }

    return BuildErrorResponse("ERR_SET_DATA_FAILED",
        "Failed to set paste data, error code: " + std::to_string(errorCode),
        "Check if pasteboard service is available.");
}

std::string ErrorHandler::HandleGetPasteDataError(int32_t errorCode)
{
    auto it = GET_DATA_ERR_MAP.find(errorCode);
    if (it != GET_DATA_ERR_MAP.end()) {
        auto [code, message, suggestion] = it->second;
        return BuildErrorResponse(code, message, suggestion);
    }

    return BuildErrorResponse("ERR_GET_DATA_FAILED",
        "Failed to get paste data, error code: " + std::to_string(errorCode),
        "Check if pasteboard has data.");
}
} // namespace Pasteboard
} // namespace OHOS
