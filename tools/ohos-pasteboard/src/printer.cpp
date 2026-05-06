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

#include "printer.h"

namespace OHOS {
namespace Pasteboard {
std::string OutputPrinter::PrintSuccess(const json &data)
{
    json result;
    result["type"] = "result";
    result["status"] = "success";
    result["data"] = data;
    return result.dump();
}

std::string OutputPrinter::PrintError(const std::string &errCode, const std::string &errMsg,
    const std::string &suggestion)
{
    json result;
    result["type"] = "result";
    result["status"] = "failed";
    result["errCode"] = errCode;
    result["errMsg"] = errMsg;
    result["suggestion"] = suggestion;
    return result.dump();
}

void OutputPrinter::PrintHelp(const std::string &helpText)
{
    std::cout << helpText << std::endl;
}
} // namespace Pasteboard
} // namespace OHOS
