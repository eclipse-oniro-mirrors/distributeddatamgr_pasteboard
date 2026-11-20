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

#ifndef FUSE_MOUNT_SYS_MOCK_H
#define FUSE_MOUNT_SYS_MOCK_H

#include <gmock/gmock.h>
#include <sys/stat.h>

// Wraps the libc filesystem calls (mkdir/lstat) that fuse_mount.cpp's
// PrepareMountPoint (anonymous-namespace helper) depends on, so unit tests can
// drive every branch of PrepareMountPoint through the public MountUdmfClipFuse
// entry point without touching the real filesystem.
// The matching .cpp provides link-time `extern "C"` overrides of the libc
// mkdir/lstat symbols that delegate to the registered gmock.
class IFuseMountSysApi {
public:
    virtual ~IFuseMountSysApi() = default;
    virtual int mkdir(const char *path, mode_t mode) = 0;
    virtual int lstat(const char *path, struct stat *buf) = 0;
};

class FuseMountSysMock : public IFuseMountSysApi {
public:
    FuseMountSysMock();
    ~FuseMountSysMock() override;
    MOCK_METHOD(int, mkdir, (const char *path, mode_t mode), (override));
    MOCK_METHOD(int, lstat, (const char *path, struct stat *buf), (override));
    static FuseMountSysMock *GetMock();

private:
    static inline FuseMountSysMock *mock_ = nullptr;
};
#endif // FUSE_MOUNT_SYS_MOCK_H
