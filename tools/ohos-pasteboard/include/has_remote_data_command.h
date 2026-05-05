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

#ifndef OHOS_PASTEBOARD_HAS_REMOTE_DATA_COMMAND_H
#define OHOS_PASTEBOARD_HAS_REMOTE_DATA_COMMAND_H

#include "command.h"

namespace OHOS {
namespace Pasteboard {
class HasRemoteDataCommand : public Command {
public:
    std::string GetName() const override { return "has-remote-data"; }
    std::string GetDescription() const override { return "Check if pasteboard has remote data"; }
    std::string GetUsage() const override;
    std::vector<std::string> GetExamples() const override;
    std::vector<std::tuple<std::string, std::string, std::string>> GetParameters() const override;
    std::string Execute(const std::vector<std::string> &args) override;
};
} // namespace Pasteboard
} // namespace OHOS
#endif // OHOS_PASTEBOARD_HAS_REMOTE_DATA_COMMAND_H
