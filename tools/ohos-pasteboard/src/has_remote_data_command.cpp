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

#include "has_remote_data_command.h"

#include <nlohmann/json.hpp>

#include "error_handler.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "printer.h"

namespace OHOS {
namespace Pasteboard {
using namespace OHOS::MiscServices;
using json = nlohmann::json;

std::string HasRemoteDataCommand::GetUsage() const
{
    return "ohos-pasteboard has-remote-data";
}

std::vector<std::string> HasRemoteDataCommand::GetExamples() const
{
    return {
        "    # Check if pasteboard data is from remote device", // 判断剪贴板中的数据是否是远端设备数据
        "    ohos-pasteboard has-remote-data",
    };
}

std::vector<std::tuple<std::string, std::string, std::string>> HasRemoteDataCommand::GetParameters() const
{
    return {
        {"--help", "Display this help message", ""}
    };
}

std::string HasRemoteDataCommand::Execute(const std::vector<std::string> &args)
{
    auto client = PasteboardClient::GetInstance();
    if (client == nullptr) {
        return OutputPrinter::PrintError("ERR_INTERNAL_ERROR",
            "Internal error: Failed to get PasteboardClient instance", // 内部错误：获取PasteboardClient实例失败
            "Check if pasteboard process failed to start"); // 请检查剪贴板进程是否拉起失败
    }

    bool hasRemoteData = client->HasRemoteData();

    json data;
    data["hasRemoteData"] = hasRemoteData;
    return OutputPrinter::PrintSuccess(data);
}
} // namespace Pasteboard
} // namespace OHOS
