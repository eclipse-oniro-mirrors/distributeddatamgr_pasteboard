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

#ifndef FUSE_MOUNT_H
#define FUSE_MOUNT_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace MiscServices {
// Mount a FUSE filesystem for virtual-file pasteboard access via the UDMF clip fuse interface.
// On success returns 0 and writes the FUSE fd to fuseFd; returns the storage
// manager error code (or -1 if the proxy is unavailable) on failure.
int32_t MountUdmfClipFuse(int32_t userId, const std::string &path, int32_t &fuseFd);
// Unmount the FUSE filesystem previously mounted via MountUdmfClipFuse.
// Returns 0 on success, the storage manager error code (or -1) on failure.
int32_t UMountUdmfClipFuse(int32_t userId, const std::string &path);
} // namespace MiscServices
} // namespace OHOS
#endif // FUSE_MOUNT_H
