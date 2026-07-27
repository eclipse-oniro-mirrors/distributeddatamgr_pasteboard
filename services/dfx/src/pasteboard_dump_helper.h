/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef MISCSERVICES_PASTEBOARD_DUMP_HELPER_H
#define MISCSERVICES_PASTEBOARD_DUMP_HELPER_H

#include <map>
#include <memory>

#include "command.h"

namespace OHOS {
namespace MiscServices {
class PasteboardDumpHelper {
public:
    static PasteboardDumpHelper &GetInstance();
    void RegisterCommand(std::shared_ptr<Command> &cmd);
    bool Dump(int fd, const std::vector<std::string> &args) const;

private:
    std::map<std::string, std::shared_ptr<Command>> cmdHandler;
};
} // namespace MiscServices
} // namespace OHOS

#endif // MISCSERVICES_PASTEBOARD_DUMP_HELPER_H
