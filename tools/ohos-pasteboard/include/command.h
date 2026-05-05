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

#ifndef OHOS_PASTEBOARD_COMMAND_H
#define OHOS_PASTEBOARD_COMMAND_H

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace OHOS {
namespace Pasteboard {
class Command {
public:
    virtual ~Command() {}
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual std::string GetUsage() const = 0;
    virtual std::vector<std::string> GetExamples() const = 0;
    virtual std::vector<std::tuple<std::string, std::string, std::string>> GetParameters() const = 0;
    virtual std::string Execute(const std::vector<std::string> &args) = 0;
};

class CommandRegistry {
public:
    static CommandRegistry &Instance();
    void Register(std::shared_ptr<Command> cmd);
    std::shared_ptr<Command> GetCommand(const std::string &name);
    std::vector<std::string> GetAllCommandNames();

private:
    CommandRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<Command>> commands_;
};
} // namespace Pasteboard
} // namespace OHOS
#endif // OHOS_PASTEBOARD_COMMAND_H
