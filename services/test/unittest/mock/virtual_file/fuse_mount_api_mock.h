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

#ifndef FUSE_MOUNT_API_MOCK_H
#define FUSE_MOUNT_API_MOCK_H

#include <gmock/gmock.h>
#include <string>

class IFuseMountApi {
public:
    virtual ~IFuseMountApi() = default;
    virtual int32_t MountUdmfClipFuse(int32_t userId, const std::string &path, int32_t &fuseFd) = 0;
    virtual int32_t UMountUdmfClipFuse(int32_t userId, const std::string &path) = 0;
};

class FuseMountApiMock : public IFuseMountApi {
public:
    FuseMountApiMock();
    ~FuseMountApiMock() override;
    MOCK_METHOD(int32_t, MountUdmfClipFuse, (int32_t userId, const std::string &path, int32_t &fuseFd), (override));
    MOCK_METHOD(int32_t, UMountUdmfClipFuse, (int32_t userId, const std::string &path), (override));
    static FuseMountApiMock *GetMock();

private:
    static inline FuseMountApiMock *mock_ = nullptr;
};
#endif // FUSE_MOUNT_API_MOCK_H
