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
    return "ohos-pasteboard has-data-type --type <string>";
}

std::vector<std::string> HasDataTypeCommand::GetExamples() const
{
    return {
        "# Check if pasteboard contains plain text data",
        "ohos-pasteboard has-data-type --type text/plain",
        "",
        "# Check if pasteboard contains HTML data",
        "ohos-pasteboard has-data-type --type text/html",
        "",
        "# Check if pasteboard contains URI data",
        "ohos-pasteboard has-data-type --type text/uri",
    };
}

std::vector<std::tuple<std::string, std::string, std::string>> HasDataTypeCommand::GetParameters() const
{
    return {
        {"--type <string>", "Type to check", "(required, \"text/plain\" for text type)"},
        {"--help", "Display this help message", ""}
    };
}

std::string HasDataTypeCommand::Execute(const std::vector<std::string> &args)
{
    std::string type = ParamParser::FindParam(args, "--type");
    if (type.empty()) {
        return OutputPrinter::PrintError("ERR_ARG_MISSING",
            "Parameter missing: Must provide data type parameter",
            "Enter a valid data type. Example: '--type text/plain' to check for plain text data");
    }

    auto client = PasteboardClient::GetInstance();
    if (client == nullptr) {
        return OutputPrinter::PrintError("ERR_INTERNAL_ERROR",
            "Internal error: Failed to get PasteboardClient instance",
            "Check if pasteboard process failed to start");
    }

    bool hasType = client->HasDataType(type);

    json data;
    data["hasType"] = hasType;
    data["type"] = type;
    return OutputPrinter::PrintSuccess(data);
}
} // namespace Pasteboard
} // namespace OHOS
