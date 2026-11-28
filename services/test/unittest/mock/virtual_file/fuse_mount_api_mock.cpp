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

#include "fuse_mount_api_mock.h"

FuseMountApiMock::FuseMountApiMock()
{
    FuseMountApiMock::mock_ = this;
}

FuseMountApiMock::~FuseMountApiMock()
{
    FuseMountApiMock::mock_ = nullptr;
}

FuseMountApiMock *FuseMountApiMock::GetMock()
{
    return FuseMountApiMock::mock_;
}

namespace OHOS {
namespace MiscServices {
int32_t MountUdmfClipFuse(int32_t userId, const std::string &path, int32_t &fuseFd)
{
    auto mock = FuseMountApiMock::GetMock();
    if (mock == nullptr) {
        fuseFd = -1;
        return -1;
    }
    return mock->MountUdmfClipFuse(userId, path, fuseFd);
}

int32_t UMountUdmfClipFuse(int32_t userId, const std::string &path)
{
    auto mock = FuseMountApiMock::GetMock();
    if (mock == nullptr) {
        return -1;
    }
    return mock->UMountUdmfClipFuse(userId, path);
}
} // namespace MiscServices
} // namespace OHOS
