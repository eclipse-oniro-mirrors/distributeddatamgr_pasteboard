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

#include "ipc_skeleton_mock.h"

namespace OHOS {
IpcMock::IpcMock()
{
    mock_ = this;
}

IpcMock::~IpcMock()
{
    mock_ = nullptr;
}

IpcMock *IpcMock::GetMock()
{
    return mock_;
}

pid_t IPCSkeleton::GetCallingUid()
{
    auto mock = IpcMock::GetMock();
    if (mock) {
        return mock->GetCallingUid();
    }
    return 0;
}
} // namespace OHOS
