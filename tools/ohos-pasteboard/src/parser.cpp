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

#include "parser.h"

namespace OHOS {
namespace Pasteboard {

std::string ParamParser::FindParam(const std::vector<std::string> &args, const std::string &name)
{
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == name && i + 1 < args.size()) {
            return args[i + 1];
        }
    }
    return "";
}

bool ParamParser::HasParam(const std::vector<std::string> &args, const std::string &name)
{
    for (const auto &arg : args) {
        if (arg == name) {
            return true;
        }
    }
    return false;
}

SpecialParser::SetDataResult SpecialParser::ParseSetData(const std::vector<std::string> &args)
{
    SetDataResult result;
    result.success = false;

    bool hasText = ParamParser::HasParam(args, "--text");
    bool hasHtml = ParamParser::HasParam(args, "--html");
    bool hasUri = ParamParser::HasParam(args, "--uri");
    if (!hasText && !hasHtml && !hasUri) {
        result.errMsg = "Missing required parameter: At least one of --text, --html, --uri";
        return result;
    }

    std::vector<std::pair<size_t, std::string>> positions;
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--text" && i + 1 < args.size() && !args[i + 1].empty()) {
            positions.push_back({i, "text"});
        } else if (args[i] == "--html" && i + 1 < args.size() && !args[i + 1].empty()) {
            positions.push_back({i, "html"});
        } else if (args[i] == "--uri" && i + 1 < args.size() && !args[i + 1].empty()) {
            positions.push_back({i, "uri"});
        }
    }

    std::sort(positions.begin(), positions.end());
    for (const auto& [pos, type] : positions) {
        std::string value = args[pos + 1];
        result.orderedParams.push_back({type, value});
    }

    if (result.orderedParams.empty()) {
        result.errMsg = "All provided parameters have empty values";
        return result;
    }

    result.success = true;
    return result;
}

SpecialParser::HasDataTypeResult SpecialParser::ParseHasDataType(const std::vector<std::string> &args)
{
    HasDataTypeResult result;
    result.success = false;

    if (args.empty()) {
        result.errMsg = "Missing required parameter: --type";
        return result;
    }

    result.type = ParamParser::FindParam(args, "--type");
    if (result.type.empty()) {
        result.errMsg = "Missing required parameter: --type";
        return result;
    }

    result.success = true;
    return result;
}
} // namespace Pasteboard
} // namespace OHOS
