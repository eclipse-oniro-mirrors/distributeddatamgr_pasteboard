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

#include "has_data_type_command.h"

#include <nlohmann/json.hpp>

#include "parser.h"
#include "pasteboard_client.h"
#include "printer.h"

namespace OHOS {
namespace Pasteboard {
using namespace OHOS::MiscServices;
using json = nlohmann::json;

std::string HasDataTypeCommand::GetUsage() const
{
    return "ohos-pasteboard has-data-type --type <string> [--timeout <int>]";
}

std::vector<std::string> HasDataTypeCommand::GetExamples() const
{
    return {
        // 检查剪贴板内容中是否有纯文本类型的数据
        "    # Check if pasteboard contains plain text data",
        "    ohos-pasteboard has-data-type --type text/plain",
        "",
        // 检查剪贴板内容中是否有HTML类型的数据
        "    # Check if pasteboard contains HTML data",
        "    ohos-pasteboard has-data-type --type text/html",
        "",
        // 检查剪贴板内容中是否有URI类型的数据
        "    # Check if pasteboard contains URI data",
        "    ohos-pasteboard has-data-type --type text/uri",
        "",
        // 检查剪贴板内容中是否有纯文本类型的数据，超时时间为3000毫秒
        "    # Check if pasteboard contains plain text data with timeout of 3000ms",
        "    ohos-pasteboard has-data-type --type text/html --timeout 3000",
    };
}

std::vector<std::tuple<std::string, std::string, std::string>> HasDataTypeCommand::GetParameters() const
{
    return {
        {"--type <string>", "Type to check",
            "(required, \"text/plain\" for text type, \"text/html\" for HTML type, \"text/uri\" for URI type)"},
        {"--timeout <int>", "Timeout in milliseconds", "(optional, range: 1-5000)"},
        {"--help", "Display this help message", ""}
    };
}

std::string HasDataTypeCommand::Execute(const std::vector<std::string> &args)
{
    std::string type = ParamParser::FindParam(args, "--type");
    if (type.empty()) {
        // 参数缺失：必须提供数据类型参数。请输入有效的数据类型。例如查询剪贴板中是否有纯文本数据类型：'--type text/plain'
        return OutputPrinter::PrintError("ERR_ARG_MISSING",
            "Parameter missing: Must provide data type parameter",
            "Enter a valid data type. Example: '--type text/plain' to check for plain text data");
    }

    std::string timeoutStr = ParamParser::FindParam(args, "--timeout");
    uint32_t timeout = 0;
    bool hasTimeout = !timeoutStr.empty();
    if (hasTimeout) {
        timeout = static_cast<uint32_t>(std::stoul(timeoutStr));
        if (timeout < 1 || timeout > 5000) {
            // 时时间XXX无效：超时时间必须在1-5000范围内。超时时间必须在1-5000范围内（单位：毫秒）。例如超时时间100ms：'--timeout 100'
            return OutputPrinter::PrintError("ERR_ARG_OUT_OF_RANGE",
            "Timeout value " + timeoutStr + " invalid: Timeout must be in range 1-5000",
            "Timeout must be in range 1-5000 (milliseconds). Example: '--timeout 100'");
        }
    }

    auto client = PasteboardClient::GetInstance();
    if (client == nullptr) {
        return OutputPrinter::PrintError("ERR_INTERNAL_ERROR",
            "Internal error: Failed to get PasteboardClient instance", // 内部错误：获取PasteboardClient实例失败
            "Check if pasteboard process failed to start"); // 请检查剪贴板进程是否拉起失败
    }

    bool hasType = hasTimeout ? client->HasDataType(type, timeout) : client->HasDataType(type);

    json data;
    data["hasType"] = hasType;
    data["type"] = type;
    if (hasTimeout) {
        data["timeout"] = static_cast<int>(timeout);
    }
    return OutputPrinter::PrintSuccess(data);
}
} // namespace Pasteboard
} // namespace OHOS
