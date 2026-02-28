/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "pasteboard_common_utils.h"

#include <pthread.h>

namespace OHOS {
namespace MiscServices {
void PasteBoardCommon::SetThreadTaskName(std::thread &thread, const std::string &taskName)
{
#ifndef CROSS_PLATFORM
    pthread_setname_np(thread.native_handle(), taskName.c_str());
#endif
}

void PasteBoardCommon::SetTaskName(const std::string &taskName)
{
#ifndef CROSS_PLATFORM
    pthread_setname_np(pthread_self(), taskName.c_str());
#endif
}
} // namespace MiscServices
} // namespace OHOS
