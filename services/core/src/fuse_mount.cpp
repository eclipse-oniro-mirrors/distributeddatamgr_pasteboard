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

#include "fuse_mount.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "pasteboard_hilog.h"
#include "storage_manager_proxy.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace MiscServices {

namespace {
sptr<StorageManager::IStorageManager> GetStorageManagerProxy()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Get system ability manager failed");
        return nullptr;
    }
    auto remote = samgr->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    if (remote == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Get STORAGE_MANAGER_MANAGER_ID failed");
        return nullptr;
    }
    return iface_cast<StorageManager::IStorageManager>(remote);
}

int32_t PrepareMountPoint(const std::string &mountPoint)
{
    constexpr mode_t MOUNT_POINT_MODE = S_IRWXU | S_IXGRP | S_IXOTH;
    if (mkdir(mountPoint.c_str(), MOUNT_POINT_MODE) == 0) {
        return 0;
    }
    int32_t error = errno;
    if (error != EEXIST) {
        return -error;
    }
    struct stat info {};
    if (lstat(mountPoint.c_str(), &info) != 0) {
        return -errno;
    }
    return S_ISDIR(info.st_mode) ? 0 : -ENOTDIR;
}
}

int32_t MountUdmfClipFuse(int32_t userId, const std::string &path, int32_t &fuseFd)
{
    fuseFd = -1;
    int32_t ret = PrepareMountPoint(path);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "Prepare mount point failed, path:%{public}s, ret:%{public}d", path.c_str(), ret);
        return ret;
    }
    auto proxy = GetStorageManagerProxy();
    if (proxy == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Get storage manager proxy failed");
        return -1;
    }
    ret = proxy->MountUdmfClipFuse(userId, path, fuseFd);
    if (ret != 0 || fuseFd < 0) {
        if (fuseFd >= 0) {
            close(fuseFd);
            fuseFd = -1;
        }
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "MountUdmfClipFuse failed, userId:%{public}d, path:%{public}s, fuseFd:%{public}d, ret:%{public}d",
            userId, path.c_str(), fuseFd, ret);
        return ret == 0 ? -1 : ret;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "MountUdmfClipFuse success, userId:%{public}d, path:%{public}s, fuseFd:%{public}d",
        userId, path.c_str(), fuseFd);
    return ret;
}

int32_t UMountUdmfClipFuse(int32_t userId, const std::string &path)
{
    auto proxy = GetStorageManagerProxy();
    if (proxy == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Get storage manager proxy failed");
        return -1;
    }
    int32_t ret = proxy->UMountUdmfClipFuse(userId, path);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "UMountUdmfClipFuse failed, userId:%{public}d, path:%{public}s, ret:%{public}d",
            userId, path.c_str(), ret);
    } else {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "UMountUdmfClipFuse success, userId:%{public}d, path:%{public}s", userId, path.c_str());
    }
    return ret;
}

} // namespace MiscServices
} // namespace OHOS
