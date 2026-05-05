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

#include "set_data_command.h"

#include <nlohmann/json.hpp>

#include "error_handler.h"
#include "parser.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "paste_data.h"
#include "paste_data_entry.h"
#include "paste_data_record.h"
#include "printer.h"
#include "uri.h"

namespace OHOS {
namespace Pasteboard {
using namespace OHOS::MiscServices;

std::string SetDataCommand::GetUsage() const
{
    return "ohos-pasteboard set-data [--html <string>] [--uri <string>] [--text <string>]";
}

std::vector<std::string> SetDataCommand::GetExamples() const
{
    return {
        // 将纯文本类型数据Hello World写入系统剪贴板
        "# Write plain text 'Hello World' to system pasteboard",
        "ohos-pasteboard set-data --text \"Hello World\"",
        "",
        // 将HTML类型数据<html><p>Hello World</p></html>写入系统剪贴板
        "# Write HTML '<html><p>Hello World</p></html>' to system pasteboard",
        "ohos-pasteboard set-data --html \"<html><p>Hello</p></html>\"",
        "",
        // 将URI类型数据Hello World写入系统剪贴板
        "# Write URI data to system pasteboard",
        "ohos-pasteboard set-data --uri \"file:///path/to/file\""
    };
}

std::vector<std::tuple<std::string, std::string, std::string>> SetDataCommand::GetParameters() const
{
    return {
        {"--text <string>", "Plain text content to set in pasteboard", "(optional)"},
        {"--html <string>", "HTML content to set in pasteboard", "(optional)"},
        {"--uri <string>", "URI content to set in pasteboard", "(optional)"},
        {"--help", "Display this help message", ""},
    };
}

std::string SetDataCommand::Execute(const std::vector<std::string> &args)
{
    auto result = SpecialParser::ParseSetData(args);
    if (!result.success) {
        // 请检查参数有效性，需要提供至少一种数据类型和内容，支持的数据类型包括：纯文本 --text、富文本 --html、URI --uri。
        // 例如设置纯文本数据内容：'--text content'
        return OutputPrinter::PrintError("ERR_ARG_INVALID", result.errMsg,
            "Check parameter validity. Provide at least one data type and content. "
            "Supported types: plain text --text, rich text --html, URI --uri. Example: '--text content'");
    }

    auto client = PasteboardClient::GetInstance();
    if (client == nullptr) {
        return OutputPrinter::PrintError("ERR_INTERNAL_ERROR",
            "Internal error: Failed to get PasteboardClient instance", // 内部错误：获取PasteboardClient实例失败
            "Check if pasteboard process failed to start"); // 请检查剪贴板进程是否拉起失败
    }

    PasteData pasteData;
    std::shared_ptr<PasteDataRecord> record = nullptr;
    
    if (result.orderedParams[0].first == "html") {
        record = PasteDataRecord::NewHtmlRecord(result.orderedParams[0].second);
    } else if (result.orderedParams[0].first == "uri") {
        OHOS::Uri uri(result.orderedParams[0].second);
        record = PasteDataRecord::NewUriRecord(uri);
    } else { // result.orderedParams[0].first == "text"
        record = PasteDataRecord::NewPlainTextRecord(result.orderedParams[0].second);
    }

    for (size_t i = 1; i < result.orderedParams.size(); i++) {
        auto entry = std::make_shared<PasteDataEntry>();
        
        if (result.orderedParams[i].first == "text") {
            entry->SetValue(result.orderedParams[i].second);
            record->AddEntryByMimeType(MIMETYPE_TEXT_PLAIN, entry);
        } else if (result.orderedParams[i].first == "html") {
            entry->SetValue(result.orderedParams[i].second);
            record->AddEntryByMimeType(MIMETYPE_TEXT_HTML, entry);
        } else if (result.orderedParams[i].first == "uri") {
            entry->SetValue(result.orderedParams[i].second);
            record->AddEntryByMimeType(MIMETYPE_TEXT_URI, entry);
        }
    }
    
    pasteData.AddRecord(record);

    int32_t ret = client->SetPasteData(pasteData);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        return ErrorHandler::HandleSetPasteDataError(ret);
    }

    json data;
    data["primaryMimeType"] = record->GetMimeType();
    data["recordCount"] = pasteData.GetRecordCount();
    return OutputPrinter::PrintSuccess(data);
}
} // namespace Pasteboard
} // namespace OHOS
