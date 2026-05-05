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

#include "get_data_command.h"

#include <nlohmann/json.hpp>

#include "error_handler.h"
#include "paste_data.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "printer.h"

namespace OHOS {
namespace Pasteboard {
using namespace OHOS::MiscServices;
using json = nlohmann::json;

std::string GetDataCommand::GetUsage() const
{
    return "ohos-pasteboard get-data";
}

std::vector<std::string> GetDataCommand::GetExamples() const
{
    return {
        "# Read system pasteboard content", // 读取系统剪贴板内容
        "ohos-pasteboard get-data",
    };
}

std::vector<std::tuple<std::string, std::string, std::string>> GetDataCommand::GetParameters() const
{
    return {
        {"--help", "Display this help message", ""}
    };
}

std::string GetDataCommand::Execute(const std::vector<std::string> &args)
{
    auto client = PasteboardClient::GetInstance();
    if (client == nullptr) {
        return OutputPrinter::PrintError("ERR_INTERNAL_ERROR",
            "Internal error: Failed to get PasteboardClient instance", // 内部错误：获取PasteboardClient实例失败
            "Check if pasteboard process failed to start"); // 请检查剪贴板进程是否拉起失败
    }

    PasteData pasteData;
    int32_t ret = client->GetPasteData(pasteData);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        return ErrorHandler::HandleGetPasteDataError(ret);
    }

    json recordsArray = json::array();
    auto records = pasteData.AllRecords();
    for (const auto &record : records) {
        auto text = record->GetPlainText();
        auto html = record->GetHtmlText();
        auto uri = record->GetUri();

        json recordJson;
        recordJson["mimeType"] = record->GetMimeType();
        if (text != nullptr) {
            recordJson["plainText"] = *text;
        }
        if (html != nullptr) {
            recordJson["htmlText"] = *html;
        }
        if (uri != nullptr) {
            recordJson["uri"] = uri->ToString();
        }
        recordsArray.push_back(recordJson);
    }

    json mimeTypesArray = json::array();
    auto mimeTypes = pasteData.GetMimeTypes();
    for (const auto& mimeType : mimeTypes) {
        mimeTypesArray.push_back(mimeType);
    }

    json data;
    data["mimeTypes"] = mimeTypesArray;
    data["recordCount"] = pasteData.GetRecordCount();
    data["records"] = recordsArray;
    return OutputPrinter::PrintSuccess(data);
}
} // namespace Pasteboard
} // namespace OHOS
