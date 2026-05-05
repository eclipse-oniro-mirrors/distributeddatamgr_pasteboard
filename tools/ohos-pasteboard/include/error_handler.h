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

#ifndef OHOS_PASTEBOARD_ERROR_HANDLER_H
#define OHOS_PASTEBOARD_ERROR_HANDLER_H

#include <string>
#include "pasteboard_error.h"
#include "printer.h"

namespace OHOS {
namespace Pasteboard {
class ErrorHandler {
public:
    static std::string HandleSetPasteDataError(int32_t errorCode);
    static std::string HandleGetPasteDataError(int32_t errorCode);

private:
    static std::string BuildErrorResponse(const std::string &code, const std::string &message,
        const std::string &suggestion);
};
} // namespace Pasteboard
} // namespace OHOS
#endif // OHOS_PASTEBOARD_ERROR_HANDLER_H
