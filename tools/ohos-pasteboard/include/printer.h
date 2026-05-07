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

#ifndef OHOS_PASTEBOARD_PRINTER_H
#define OHOS_PASTEBOARD_PRINTER_H

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

namespace OHOS {
namespace Pasteboard {
using json = nlohmann::json;

class OutputPrinter {
public:
    static std::string PrintSuccess(const json &data);
    static std::string PrintError(const std::string &errCode, const std::string &errMsg, const std::string &suggestion);
    static void PrintHelp(const std::string &helpText);
};
} // namespace Pasteboard
} // namespace OHOS
#endif // OHOS_PASTEBOARD_PRINTER_H
