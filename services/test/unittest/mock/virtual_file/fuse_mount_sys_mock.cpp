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

#include "fuse_mount_sys_mock.h"

#include <cerrno>
#include <sys/stat.h>

FuseMountSysMock::FuseMountSysMock()
{
    mock_ = this;
}

FuseMountSysMock::~FuseMountSysMock()
{
    mock_ = nullptr;
}

FuseMountSysMock *FuseMountSysMock::GetMock()
{
    return mock_;
}

// Link-time overrides of the libc symbols consumed by PrepareMountPoint in
// fuse_mount.cpp. They carry C linkage so the symbol name matches the libc
// declaration from <sys/stat.h>, which lets the locally compiled definition
// take precedence over the shared libc for this test binary only. When no mock
// is registered they return a safe failure (ENOENT) so PrepareMountPoint can
// never accidentally fall through to the proxy with a stale fd.
extern "C" {
int mkdir(const char *path, mode_t mode)
{
    auto *mock = FuseMountSysMock::GetMock();
    if (mock == nullptr) {
        errno = ENOENT;
        return -1;
    }
    return mock->mkdir(path, mode);
}

int lstat(const char *path, struct stat *buf)
{
    auto *mock = FuseMountSysMock::GetMock();
    if (mock == nullptr) {
        errno = ENOENT;
        return -1;
    }
    return mock->lstat(path, buf);
}
}
