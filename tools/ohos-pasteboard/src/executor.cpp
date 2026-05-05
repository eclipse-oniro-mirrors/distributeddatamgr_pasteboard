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

#include "executor.h"

#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "clear_data_command.h"
#include "command.h"
#include "has_data_command.h"
#include "has_data_type_command.h"
#include "has_remote_data_command.h"
#include "get_data_command.h"
#include "printer.h"
#include "set_data_command.h"

namespace OHOS {
namespace Pasteboard {
using json = nlohmann::json;

CommandRegistry &CommandRegistry::Instance()
{
    static CommandRegistry registry;
    return registry;
}

void CommandRegistry::Register(std::shared_ptr<Command> cmd)
{
    commands_[cmd->GetName()] = cmd;
}

std::shared_ptr<Command> CommandRegistry::GetCommand(const std::string &name)
{
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> CommandRegistry::GetAllCommandNames()
{
    std::vector<std::string> names;
    for (const auto &pair : commands_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

static void PrintHelpToStderr(const std::string &helpText)
{
    std::cerr << helpText << std::endl;
}

static std::string BuildGlobalHelp()
{
    constexpr uint32_t COMMAND_NAME_MAX_LEN = 20;

    auto& registry = CommandRegistry::Instance();
    auto cmdNames = registry.GetAllCommandNames();

    std::ostringstream oss;
    oss << "ohos-pasteboard - pasteboard CLI tool" << std::endl << std::endl;

    oss << "Usage:" << std::endl;
    oss << "  ohos-pasteboard --help" << std::endl;
    oss << "  ohos-pasteboard <command> --help" << std::endl;
    oss << "  ohos-pasteboard <command> [options]" << std::endl << std::endl;

    oss << "Parameters:" << std::endl;
    oss << "  --help\tDisplay this help message" << std::endl << std::endl;

    oss << "SubCommands:" << std::endl;
    for (const auto& name : cmdNames) {
        auto cmd = registry.GetCommand(name);
        if (cmd) {
            oss << "  " << std::left << std::setw(COMMAND_NAME_MAX_LEN) << name << cmd->GetDescription() << std::endl;
        }
    }

    oss << "\nExamples:" << std::endl;
    oss << "  # Check if system pasteboard has content" << std::endl; // 判断系统剪贴板中是否有内容
    oss << "  ohos-pasteboard has-data" << std::endl << std::endl;
    oss << "  # Write plain text 'Hello' to system pasteboard" << std::endl; // 将纯文本数据Hello写入系统剪贴板
    oss << "  ohos-pasteboard set-data --text \"Hello\"" << std::endl << std::endl;
    oss << "  # Read system pasteboard content" << std::endl; // 读取系统剪贴板内容
    oss << "  ohos-pasteboard get-data" << std::endl << std::endl;
    oss << "  # View subcommand help" << std::endl;
    oss << "  ohos-pasteboard has-data-type --help" << std::endl;
    return oss.str();
}

static std::string BuildCommandHelp(std::shared_ptr<Command> cmd)
{
    std::ostringstream oss;
    oss << "ohos-pasteboard " << cmd->GetName() << " - " << cmd->GetDescription() << "\n\n";
    oss << "Usage:\n";
    oss << "  " << cmd->GetUsage() << "\n\n";

    auto parameters = cmd->GetParameters();
    if (!parameters.empty()) {
        oss << "Parameters:\n";
        for (const auto& [name, desc, constraint] : parameters) {
            oss << "  " << name;
            if (!constraint.empty()) {
                oss << "  " << constraint;
            }
            if (!desc.empty()) {
                oss << "  " << desc;
            }
            oss << "\n";
        }
        oss << "\n";
    }

    auto examples = cmd->GetExamples();
    if (!examples.empty()) {
        oss << "Examples:\n";
        for (const auto& example : examples) {
            oss << "  " << example << "\n";
        }
    }
    return oss.str();
}

std::string ExecuteCommand(const std::vector<std::string> &args)
{
    RegisterAllCommands();

    if (args.empty()) {
        PrintHelpToStderr(BuildGlobalHelp());
        return "";
    }

    if (args[0] == "--help") {
        PrintHelpToStderr(BuildGlobalHelp());
        return "";
    }

    std::string cmdName = args[0];
    auto cmd = CommandRegistry::Instance().GetCommand(cmdName);
    if (!cmd) {
        return OutputPrinter::PrintError("ERR_CMD_INVALID",
            "Unknown command: " + cmdName,
            "Run 'ohos-pasteboard --help' to see available commands.");
    }

    std::vector<std::string> cmdArgs(args.begin() + 1, args.end());

    for (const auto &arg : cmdArgs) {
        if (arg == "--help") {
            PrintHelpToStderr(BuildCommandHelp(cmd));
            return "";
        }
    }

    return cmd->Execute(cmdArgs);
}

void RegisterAllCommands()
{
    auto& registry = CommandRegistry::Instance();
    if (!registry.GetCommand("set-data")) {
        registry.Register(std::make_shared<SetDataCommand>());
        registry.Register(std::make_shared<GetDataCommand>());
        registry.Register(std::make_shared<ClearDataCommand>());
        registry.Register(std::make_shared<HasDataCommand>());
        registry.Register(std::make_shared<HasDataTypeCommand>());
        registry.Register(std::make_shared<HasRemoteDataCommand>());
    }
}
} // namespace Pasteboard
} // namespace OHOS
