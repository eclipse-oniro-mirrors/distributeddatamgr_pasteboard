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

#include "pasteboard_fuse_manager.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)
#include <fuse.h>
#include <fuse_lowlevel.h>

#include "securec.h"

#include "fuse_mount.h"
#include "ipc_skeleton.h"
#include "paste_data_record.h"

namespace OHOS {
namespace MiscServices {

int32_t PasteboardFuseManager::MountFuse(int32_t userId, const std::string &path, int32_t &fuseFd)
{
    return MountUdmfClipFuse(userId, path, fuseFd);
}

int32_t PasteboardFuseManager::UMountFuse(int32_t userId, const std::string &path)
{
    return UMountUdmfClipFuse(userId, path);
}

PasteboardFuseManager::~PasteboardFuseManager()
{
    std::vector<int32_t> userIds;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &[userId, session] : fuseSessions_) {
            userIds.push_back(userId);
        }
    }
    for (auto userId : userIds) {
        StopSessionLoop(userId);
    }
}

std::string PasteboardFuseManager::GetMountPath(int32_t userId)
{
    return std::string(MOUNT_BASE) + "/" + std::to_string(userId) + MOUNT_SUFFIX;
}

int32_t PasteboardFuseManager::OnGetData(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetData userId:%{public}d", userId);
    pid_t pid = IPCSkeleton::GetCallingPid();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        authorizedPastePids_[userId].insert(pid);
        if (fuseSessions_.count(userId) > 0) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
                "FUSE session exists for userId:%{public}d, running:%{public}d", userId,
                fuseSessions_[userId].running.load());
            return 0;
        }
    }
    std::string mountPath = GetMountPath(userId);
    int32_t fuseFd = -1;
    int32_t ret = MountFuse(userId, mountPath, fuseFd);
    if (ret != 0 || fuseFd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "MountFuse failed, ret:%{public}d, fuseFd:%{public}d", ret, fuseFd);
        return ret;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fuseSessions_.count(userId) > 0) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
                "FUSE session already created by another thread, cleaning up duplicate mount");
            UMountFuse(userId, mountPath);
            return 0;
        }
        struct fuse_args args = { 0, nullptr, 0 };
        fuse_opt_add_arg(&args, "");
        fuse_opt_add_arg(&args, "-o");
        fuse_opt_add_arg(&args, ("max_read=" + std::to_string(MAX_READ_SIZE)).c_str());
        struct fuse_operations ops = {};
        ops.getattr = StatHandler;
        ops.open = OpenHandler;
        ops.read = ReadHandler;
        ops.release = ReleaseHandler;
        ops.readdir = [](const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
            struct fuse_file_info *fi, enum fuse_readdir_flags flags) -> int {
            return ReaddirHandler(path, buf, reinterpret_cast<void *>(filler), offset, fi,
                static_cast<int>(flags));
        };
        auto &session = fuseSessions_[userId];
        session.context.manager = this;
        session.context.userId = userId;
        struct fuse *fuseHandle = fuse_new(&args, &ops, sizeof(ops), &session.context);
        fuse_opt_free_args(&args);
        if (fuseHandle == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_new failed");
            UMountFuse(userId, mountPath);
            fuseSessions_.erase(userId);
            return -1;
        }
        session.handle = fuseHandle;
        session.mountPath = mountPath;
        session.fuseFd = fuseFd;
        StartSessionLoop(userId, fuseHandle);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE mounted and session started for userId:%{public}d", userId);
    return 0;
}

void PasteboardFuseManager::StartSessionLoop(int32_t userId, struct fuse *fuseHandle)
{
    auto &session = fuseSessions_[userId];
    session.running = true;
    int32_t fuseFd = session.fuseFd;
    session.loopThread = std::thread([this, userId, fuseHandle, fuseFd]() {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE session loop started, userId:%{public}d", userId);
        struct fuse_session *fuseSession = fuse_get_session(fuseHandle);
        if (fuseSession == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_get_session failed");
            return;
        }
        struct fuse_custom_io customIo {};
        customIo.read = [](int fd, void *buf, size_t sz, void *userdata) -> ssize_t {
            (void)userdata;
            return read(fd, buf, sz);
        };
        customIo.writev = [](int fd, struct iovec *iov, int count, void *userdata) -> ssize_t {
            (void)userdata;
            return writev(fd, iov, count);
        };
        if (fuse_session_custom_io(fuseSession, &customIo, sizeof(customIo), fuseFd) != 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_session_custom_io failed");
            return;
        }
        struct fuse_loop_config *loop_config = fuse_loop_cfg_create();
        if (loop_config == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_loop_cfg_create failed");
            return;
        }
        fuse_loop_cfg_set_max_threads(loop_config, MAX_THREADS);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Starting FUSE loop, userId:%{public}d", userId);
        fuse_session_loop_mt(fuseSession, loop_config);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE loop ended, userId:%{public}d", userId);
        fuse_loop_cfg_destroy(loop_config);
        fuse_unmount(fuseHandle);
        fuse_destroy(fuseHandle);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE session cleanup done, userId:%{public}d", userId);
    });
}

void PasteboardFuseManager::StopSessionLoop(int32_t userId)
{
    std::thread loopThread;
    struct fuse *handle = nullptr;
    std::string mountPath;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = fuseSessions_.find(userId);
        if (it == fuseSessions_.end()) return;
        auto &session = it->second;
        if (!session.running) {
            fuseSessions_.erase(it);
            return;
        }
        session.running = false;
        handle = session.handle;
        mountPath = session.mountPath;
        if (session.loopThread.joinable()) {
            loopThread = std::move(session.loopThread);
        }
    }
    if (handle != nullptr) {
        fuse_exit(handle);
    }
    if (loopThread.joinable()) {
        loopThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        fuseSessions_.erase(userId);
    }
    if (!mountPath.empty()) {
        UMountFuse(userId, mountPath);
    }
}

void PasteboardFuseManager::CleanupLocked(int32_t userId)
{
    providers_.erase(userId);
    fileInfos_.erase(userId);
    for (auto it = fdMap_.begin(); it != fdMap_.end();) {
        if (it->second.fileInfo && it->second.fileInfo->userId == userId) {
            it = fdMap_.erase(it);
        } else {
            ++it;
        }
    }
}

void PasteboardFuseManager::OnProviderDied(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnProviderDied userId:%{public}d", userId);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CleanupLocked(userId);
    }
    StopSessionLoop(userId);
}

bool PasteboardFuseManager::HasProvider(int32_t userId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return providers_.find(userId) != providers_.end() && providers_[userId] != nullptr;
}

int32_t PasteboardFuseManager::OnAppExit(pid_t pid, int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAppExit pid:%{public}d, userId:%{public}d", pid, userId);
    bool needStop = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto providerIt = providers_.find(userId);
        bool isCopyApp = providerIt != providers_.end() && providerIt->second != nullptr;
        if (isCopyApp) {
            CleanupCopyAppLocked(userId);
            needStop = true;
        } else {
            needStop = CleanupPasteAppLocked(pid, userId);
        }
    }
    if (needStop) {
        StopSessionLoop(userId);
    }
    return 0;
}

void PasteboardFuseManager::CleanupCopyAppLocked(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CleanupCopyAppLocked userId:%{public}d", userId);
    authorizedPastePids_.erase(userId);
    CleanupLocked(userId);
}

bool PasteboardFuseManager::CleanupPasteAppLocked(pid_t pid, int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CleanupPasteAppLocked pid:%{public}d, userId:%{public}d",
        pid, userId);
    auto &pidSet = authorizedPastePids_[userId];
    pidSet.erase(pid);
    for (auto it = fdMap_.begin(); it != fdMap_.end();) {
        if (it->second.openerPid == pid) {
            it = fdMap_.erase(it);
        } else {
            ++it;
        }
    }
    if (pidSet.empty()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "No more authorized paste apps, unmounting FUSE");
        return true;
    }
    return false;
}

void PasteboardFuseManager::OnAged(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnAged userId:%{public}d", userId);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CleanupLocked(userId);
    }
    StopSessionLoop(userId);
}

void PasteboardFuseManager::OnRemoveData(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnRemoveData userId:%{public}d", userId);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        CleanupLocked(userId);
    }
    StopSessionLoop(userId);
}

void PasteboardFuseManager::SetProvider(int32_t userId, const sptr<IPasteboardFuseGetter> &provider,
    uint32_t dataId, const std::vector<FileInfo> &fileInfos)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetProvider userId:%{public}d, dataId:%{public}u", userId, dataId);
    std::lock_guard<std::mutex> lock(mutex_);
    providers_[userId] = provider;
    std::vector<FileInfo> infos;
    infos.reserve(fileInfos.size());
    for (auto info : fileInfos) {
        info.userId = userId;
        info.dataId = dataId;
        info.provider = provider;
        infos.push_back(info);
    }
    fileInfos_[userId] = std::move(infos);
}

bool PasteboardFuseManager::CheckAuthorization(int32_t userId)
{
    struct fuse_context *ctx = fuse_get_context();
    if (ctx == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_get_context returned null");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return CheckAuthorizationLocked(userId, ctx->pid);
}

bool PasteboardFuseManager::CheckAuthorizationLocked(int32_t userId, pid_t callerPid)
{
    auto it = authorizedPastePids_.find(userId);
    if (it == authorizedPastePids_.end()) {
        return false;
    }
    return it->second.count(callerPid) > 0;
}

std::shared_ptr<FileInfo> PasteboardFuseManager::GetFileInfoLocked(int32_t userId, uint32_t dataId,
    uint32_t recordId, const std::string &fileName)
{
    auto it = fileInfos_.find(userId);
    if (it == fileInfos_.end()) {
        return nullptr;
    }
    for (auto &info : it->second) {
        if (info.dataId == dataId && info.recordId == recordId && info.fileName == fileName) {
            return std::make_shared<FileInfo>(info);
        }
    }
    return nullptr;
}

int PasteboardFuseManager::ParsePath(const char *path, uint32_t &dataId, uint32_t &recordId, std::string &fileName)
{
    if (path == nullptr || path[0] != '/') {
        return PATH_INVALID;
    }
    const char *rest = path + 1;
    if (rest[0] == '\0') {
        return PATH_ROOT;
    }
    auto parseUint = [](const char *begin, const char *end, uint32_t &out) -> bool {
        if (begin >= end || *begin == '\0') {
            return false;
        }
        errno = 0;
        char *endPtr = nullptr;
        unsigned long val = strtoul(begin, &endPtr, 10);
        if (errno != 0 || endPtr != end) {
            return false;
        }
        out = static_cast<uint32_t>(val);
        return true;
    };
    const char *firstSlash = strchr(rest, '/');
    if (firstSlash == nullptr) {
        if (!parseUint(rest, rest + strlen(rest), dataId)) {
            return PATH_INVALID;
        }
        return PATH_DATA_ID_DIR;
    }
    if (!parseUint(rest, firstSlash, dataId)) {
        return PATH_INVALID;
    }
    const char *afterFirst = firstSlash + 1;
    if (afterFirst[0] == '\0') {
        return PATH_DATA_ID_DIR;
    }
    const char *secondSlash = strchr(afterFirst, '/');
    if (secondSlash == nullptr) {
        if (!parseUint(afterFirst, afterFirst + strlen(afterFirst), recordId)) {
            return PATH_INVALID;
        }
        return PATH_DATA_ID_DIR;
    }
    if (!parseUint(afterFirst, secondSlash, recordId)) {
        return PATH_INVALID;
    }
    const char *afterSecond = secondSlash + 1;
    if (afterSecond[0] == '\0') {
        return PATH_RECORD_ID_DIR;
    }
    fileName = std::string(afterSecond);
    return PATH_FILE;
}

int PasteboardFuseManager::StatHandler(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    struct fuse_context *ctx = fuse_get_context();
    if (ctx == nullptr || ctx->private_data == nullptr) return -EACCES;
    auto *sessionCtx = static_cast<SessionContext *>(ctx->private_data);
    auto *self = sessionCtx->manager;
    if (self == nullptr) return -EACCES;
    int32_t userId = sessionCtx->userId;
    std::lock_guard<std::mutex> lock(self->mutex_);
    if (!self->CheckAuthorizationLocked(userId, ctx->pid)) {
        return -EACCES;
    }
    if (memset_s(stbuf, sizeof(struct stat), 0, sizeof(struct stat)) != EOK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "memset_s failed");
        return -EIO;
    }
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    int pathType = ParsePath(path, dataId, recordId, fileName);
    if (pathType == PATH_ROOT) {
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
        return 0;
    }
    if (pathType == PATH_DATA_ID_DIR) {
        auto it = self->fileInfos_.find(userId);
        if (it == self->fileInfos_.end()) {
            return -ENOENT;
        }
        bool found = false;
        for (const auto &info : it->second) {
            if (info.dataId == dataId) {
                found = true;
                break;
            }
        }
        if (!found) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
        return 0;
    }
    if (pathType == PATH_RECORD_ID_DIR) {
        auto it = self->fileInfos_.find(userId);
        if (it == self->fileInfos_.end()) {
            return -ENOENT;
        }
        bool found = false;
        for (const auto &info : it->second) {
            if (info.dataId == dataId && info.recordId == recordId) {
                found = true;
                break;
            }
        }
        if (!found) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
        return 0;
    }
    if (pathType == PATH_FILE) {
        auto fileInfo = self->GetFileInfoLocked(userId, dataId, recordId, fileName);
        if (fileInfo == nullptr) {
            return -ENOENT;
        }
        FillFileStat(*fileInfo, stbuf);
        return 0;
    }
    return -ENOENT;
}

void PasteboardFuseManager::FillFileStat(const FileInfo &info, struct stat *stbuf)
{
    if (info.isDir) {
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
    } else {
        stbuf->st_mode = S_IFREG | FUSE_FILE_MODE;
        stbuf->st_nlink = FUSE_FILE_NLINK;
        stbuf->st_size = info.fileSize;
    }
    stbuf->st_mtime = static_cast<time_t>(info.modifyTime / MILLISEC_PER_SEC);
}

int PasteboardFuseManager::OpenHandler(const char *path, struct fuse_file_info *fi)
{
    struct fuse_context *ctx = fuse_get_context();
    if (ctx == nullptr || ctx->private_data == nullptr) return -EACCES;
    auto *sessionCtx = static_cast<SessionContext *>(ctx->private_data);
    auto *self = sessionCtx->manager;
    if (self == nullptr) return -EACCES;
    int32_t userId = sessionCtx->userId;
    std::lock_guard<std::mutex> lock(self->mutex_);
    if (!self->CheckAuthorizationLocked(userId, ctx->pid)) {
        return -EACCES;
    }
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    int pathType = ParsePath(path, dataId, recordId, fileName);
    if (pathType != PATH_FILE) {
        if (pathType == PATH_DATA_ID_DIR || pathType == PATH_RECORD_ID_DIR) {
            return -EISDIR;
        }
        return -ENOENT;
    }
    auto fileInfo = self->GetFileInfoLocked(userId, dataId, recordId, fileName);
    if (fileInfo == nullptr) {
        return -ENOENT;
    }
    int32_t fd = self->nextFd_++;
    self->fdMap_[fd] = {ctx->pid, fileInfo};
    fi->fh = static_cast<uint64_t>(fd);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "OpenHandler path:%{public}s, fd:%{public}d", path, fd);
    return 0;
}

int PasteboardFuseManager::ReadHandler(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    struct fuse_context *ctx = fuse_get_context();
    if (ctx == nullptr || ctx->private_data == nullptr) return -EACCES;
    auto *sessionCtx = static_cast<SessionContext *>(ctx->private_data);
    auto *self = sessionCtx->manager;
    if (self == nullptr) return -EACCES;
    int32_t userId = sessionCtx->userId;
    int32_t fd = static_cast<int32_t>(fi->fh);
    std::shared_ptr<FileInfo> fileInfo;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        if (!self->CheckAuthorizationLocked(userId, ctx->pid)) {
            return -EACCES;
        }
        auto it = self->fdMap_.find(fd);
        if (it == self->fdMap_.end()) {
            return -ENOENT;
        }
        fileInfo = it->second.fileInfo;
    }
    if (fileInfo == nullptr || fileInfo->provider == nullptr) {
        return -EIO;
    }
    int32_t readLen = std::min(static_cast<int32_t>(size), MAX_READ_SIZE);
    if (readLen <= 0) {
        return 0;
    }
    self->readSemaphore_.acquire();
    std::vector<uint8_t> data;
    int32_t ret = -1;
    for (int32_t attempt = 0; attempt <= MAX_READ_RETRIES; ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(READ_RETRY_DELAY_MS * attempt));
        }
        data.clear();
        ret = fileInfo->provider->OnRead(fileInfo->recordId, fileInfo->fileId, offset, readLen, data);
        if (ret == 0) {
            break;
        }
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE,
            "OnRead attempt %{public}d failed, ret:%{public}d, offset:%{public}ld", attempt + 1, ret,
            static_cast<long>(offset));
    }
    self->readSemaphore_.release();
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "OnRead failed after %{public}d attempts, ret:%{public}d", MAX_READ_RETRIES + 1, ret);
        return -EIO;
    }
    size_t copyLen = std::min(data.size(), size);
    if (copyLen > 0) {
        if (memcpy_s(buf, size, data.data(), copyLen) != EOK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "memcpy_s failed");
            return -EIO;
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
        "ReadHandler path:%{public}s, offset:%{public}ld, size:%{public}zu, read:%{public}zu",
        path, static_cast<long>(offset), size, copyLen);
    return static_cast<int>(copyLen);
}

int PasteboardFuseManager::ReleaseHandler(const char *path, struct fuse_file_info *fi)
{
    struct fuse_context *ctx = fuse_get_context();
    if (ctx == nullptr || ctx->private_data == nullptr) return 0;
    auto *sessionCtx = static_cast<SessionContext *>(ctx->private_data);
    auto *self = sessionCtx->manager;
    if (self == nullptr) return 0;
    int32_t fd = static_cast<int32_t>(fi->fh);
    std::lock_guard<std::mutex> lock(self->mutex_);
    self->fdMap_.erase(fd);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "ReleaseHandler path:%{public}s, fd:%{public}d", path, fd);
    return 0;
}

int PasteboardFuseManager::ReaddirHandler(const char *path, void *buf, void *filler, off_t offset,
    struct fuse_file_info *fi, int flags)
{
    (void)offset;
    (void)fi;
    (void)flags;
    struct fuse_context *ctx = fuse_get_context();
    if (ctx == nullptr || ctx->private_data == nullptr) return -EACCES;
    auto *sessionCtx = static_cast<SessionContext *>(ctx->private_data);
    auto *self = sessionCtx->manager;
    if (self == nullptr) return -EACCES;
    int32_t userId = sessionCtx->userId;
    std::lock_guard<std::mutex> lock(self->mutex_);
    if (!self->CheckAuthorizationLocked(userId, ctx->pid)) {
        return -EACCES;
    }
    auto fillFunc = reinterpret_cast<fuse_fill_dir_t>(filler);
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    int pathType = ParsePath(path, dataId, recordId, fileName);
    if (pathType == PATH_ROOT) {
        fillFunc(buf, ".", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
        fillFunc(buf, "..", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
        auto it = self->fileInfos_.find(userId);
        if (it != self->fileInfos_.end()) {
            std::set<uint32_t> listedDataIds;
            for (const auto &info : it->second) {
                if (listedDataIds.insert(info.dataId).second) {
                    fillFunc(buf, std::to_string(info.dataId).c_str(), nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
                }
            }
        }
        return 0;
    }
    if (pathType == PATH_DATA_ID_DIR) {
        return self->ReadDataIdDirLocked(userId, dataId, filler, buf);
    }
    if (pathType == PATH_RECORD_ID_DIR) {
        auto fillFunc2 = reinterpret_cast<fuse_fill_dir_t>(filler);
        auto it = self->fileInfos_.find(userId);
        if (it == self->fileInfos_.end()) {
            return -ENOENT;
        }
        bool found = false;
        for (const auto &info : it->second) {
            if (info.dataId == dataId && info.recordId == recordId) {
                found = true;
                break;
            }
        }
        if (!found) {
            return -ENOENT;
        }
        fillFunc2(buf, ".", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
        fillFunc2(buf, "..", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
        for (const auto &info : it->second) {
            if (info.dataId == dataId && info.recordId == recordId) {
                fillFunc2(buf, info.fileName.c_str(), nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
            }
        }
        return 0;
    }
    return -ENOENT;
}

int PasteboardFuseManager::ReadDataIdDirLocked(int32_t userId, uint32_t dataId, void *filler, void *buf)
{
    auto fillFunc = reinterpret_cast<fuse_fill_dir_t>(filler);
    auto it = fileInfos_.find(userId);
    if (it == fileInfos_.end()) {
        return -ENOENT;
    }
    bool found = false;
    for (const auto &info : it->second) {
        if (info.dataId == dataId) {
            found = true;
            break;
        }
    }
    if (!found) {
        return -ENOENT;
    }
    fillFunc(buf, ".", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    fillFunc(buf, "..", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    std::set<uint32_t> listedRecordIds;
    for (const auto &info : it->second) {
        if (info.dataId == dataId && listedRecordIds.insert(info.recordId).second) {
            fillFunc(buf, std::to_string(info.recordId).c_str(), nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
        }
    }
    return 0;
}
} // namespace MiscServices
} // namespace OHOS
