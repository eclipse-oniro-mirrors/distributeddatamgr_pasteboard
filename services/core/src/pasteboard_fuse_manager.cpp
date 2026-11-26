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

#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
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

void PasteboardFuseManager::FillFuseOps(struct fuse_lowlevel_ops &ops)
{
    ops.lookup = LookupHandler;
    ops.forget = ForgetHandler;
    ops.getattr = GetAttrHandler;
    ops.opendir = OpendirHandler;
    ops.readdir = ReaddirHandler;
    ops.releasedir = ReleasedirHandler;
    ops.open = OpenHandler;
    ops.read = ReadHandler;
    ops.release = ReleaseHandler;
    ops.init = InitHandler;
}

int32_t PasteboardFuseManager::CreateFuseSessionLocked(int32_t userId, const std::string &mountPath,
    int32_t fuseFd)
{
    auto &session = fuseSessions_[userId];
    session.wakeFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (session.wakeFd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "create wake fd failed, errno:%{public}d", errno);
        UMountFuse(userId, mountPath);
        fuseSessions_.erase(userId);
        return -1;
    }
    struct fuse_args args = { 0, nullptr, 0 };
    fuse_opt_add_arg(&args, "");
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, ("max_read=" + std::to_string(MAX_READ_SIZE)).c_str());
    struct fuse_lowlevel_ops ops = {};
    FillFuseOps(ops);
    session.context.manager = this;
    session.context.userId = userId;
    session.context.wakeFd = session.wakeFd;
    struct fuse_session *fuseSession = fuse_session_new(&args, &ops, sizeof(ops), &session.context);
    fuse_opt_free_args(&args);
    if (fuseSession == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_session_new failed");
        close(session.wakeFd);
        UMountFuse(userId, mountPath);
        fuseSessions_.erase(userId);
        return -1;
    }
    struct fuse_custom_io customIo = {};
    customIo.read = ReadFuseFd;
    customIo.writev = WriteFuseFd;
    int32_t ret = fuse_session_custom_io(fuseSession, &customIo, sizeof(customIo), fuseFd);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_session_custom_io failed, ret:%{public}d", ret);
        fuse_session_destroy(fuseSession);
        close(session.wakeFd);
        UMountFuse(userId, mountPath);
        fuseSessions_.erase(userId);
        return -1;
    }
    session.session = fuseSession;
    session.mountPath = mountPath;
    session.fuseFd = fuseFd;
    session.inodePaths[FUSE_ROOT_INO] = "/";
    session.pathInodes["/"] = FUSE_ROOT_INO;
    session.nextInode = FUSE_ROOT_INO + 1;
    session.running = true;
    return 0;
}

int32_t PasteboardFuseManager::OnGetData(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnGetData userId:%{public}d", userId);
    pid_t pid = IPCSkeleton::GetCallingPid();
    uid_t callingUid = static_cast<uid_t>(IPCSkeleton::GetCallingUid());
    bool hasDeadSession = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        authorizedPastePids_[userId].insert(pid);
        authorizedPasteUids_[userId].insert(callingUid);
        auto it = fuseSessions_.find(userId);
        if (it != fuseSessions_.end()) {
            if (it->second.running.load()) {
                PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
                    "FUSE session exists for userId:%{public}d, running:%{public}d", userId,
                    it->second.running.load());
                return 0;
            }
            hasDeadSession = true;
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "FUSE loop died previously for userId:%{public}d, cleaning up stale session", userId);
        }
    }
    if (hasDeadSession) {
        // The loop thread has already exited; StopSessionLoop joins it, destroys the dead
        // session and unmounts the stale fuse mount so a fresh one can be created below.
        StopSessionLoop(userId);
    }
    std::string mountPath = GetMountPath(userId);
    int32_t fuseFd = -1;
    int32_t ret = MountFuse(userId, mountPath, fuseFd);
    if (ret != 0) {
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
        ret = CreateFuseSessionLocked(userId, mountPath, fuseFd);
        if (ret != 0) {
            return ret;
        }
        StartSessionLoop(userId);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE mounted and session started for userId:%{public}d", userId);
    return 0;
}

void PasteboardFuseManager::StartSessionLoop(int32_t userId)
{
    auto &session = fuseSessions_[userId];
    session.loopThread = std::thread([this, userId]() { LoopMain(userId); });
}

void PasteboardFuseManager::LoopMain(int32_t userId)
{
    struct fuse_session *session = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = fuseSessions_.find(userId);
        if (it == fuseSessions_.end()) {
            return;
        }
        session = it->second.session;
    }
    if (session == nullptr) {
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE session loop started, userId:%{public}d", userId);
    struct fuse_loop_config *loopConfig = fuse_loop_cfg_create();
    if (loopConfig == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fuse_loop_cfg_create failed, userId:%{public}d", userId);
        return;
    }
    fuse_loop_cfg_set_idle_threads(loopConfig, 1);
    fuse_loop_cfg_set_max_threads(loopConfig, MAX_THREADS);
    fuse_loop_cfg_set_clone_fd(loopConfig, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Starting FUSE loop, userId:%{public}d", userId);
    int32_t ret = fuse_session_loop_mt(session, loopConfig);
    fuse_loop_cfg_destroy(loopConfig);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE loop ended, userId:%{public}d, ret:%{public}d", userId, ret);
    {
        // Mark the session dead so the next OnGetData detects it, cleans up and remounts
        // instead of serving a stale fuse mount. The normal stop path erases the session
        // from the map, in which case this lookup is a no-op.
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = fuseSessions_.find(userId);
        if (it != fuseSessions_.end()) {
            it->second.running = false;
        }
    }
}

void PasteboardFuseManager::InitHandler(void *userdata, struct fuse_conn_info *conn)
{
    (void)userdata;
    if (conn == nullptr) {
        return;
    }
    // libfuse requires conn->max_read to match the max_read mount option passed to
    // fuse_session_new(), otherwise the INIT handshake fails with EPROTO and the
    // session loop exits immediately. Keep it in sync with MAX_READ_SIZE.
    conn->max_read = MAX_READ_SIZE;
}

void PasteboardFuseManager::StopSessionLoop(int32_t userId)
{
    struct fuse_session *session = nullptr;
    std::string mountPath;
    std::thread loopThread;
    int32_t wakeFd = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = fuseSessions_.find(userId);
        if (it == fuseSessions_.end()) {
            return;
        }
        auto &fuseSess = it->second;
        fuseSess.running = false;
        if (fuseSess.loopThread.joinable()) {
            loopThread = std::move(fuseSess.loopThread);
        }
        session = fuseSess.session;
        mountPath = fuseSess.mountPath;
        wakeFd = fuseSess.wakeFd;
        fuseSessions_.erase(it);
    }
    if (session != nullptr) {
        fuse_session_exit(session);
        if (wakeFd >= 0) {
            uint64_t value = 1;
            (void)write(wakeFd, &value, sizeof(value));
        }
    }
    if (loopThread.joinable()) {
        loopThread.join();
    }
    if (session != nullptr) {
        fuse_session_destroy(session);
    }
    if (wakeFd >= 0) {
        close(wakeFd);
    }
    if (!mountPath.empty()) {
        UMountFuse(userId, mountPath);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "FUSE session stopped, userId:%{public}d", userId);
}

ssize_t PasteboardFuseManager::ReadFuseFd(int fd, void *buf, size_t len, void *userdata)
{
    auto *sessionCtx = static_cast<SessionContext *>(userdata);
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        errno = EINVAL;
        return -1;
    }
    int32_t wakeFd = sessionCtx->wakeFd;
    struct pollfd pollFds[2] = { { fd, POLLIN, 0 }, { wakeFd, POLLIN, 0 } };
    int32_t pollRet = 0;
    do {
        pollRet = poll(pollFds, wakeFd >= 0 ? 2 : 1, -1);
    } while (pollRet < 0 && errno == EINTR);
    if (pollRet < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "poll fuse fd failed, errno:%{public}d", errno);
        return -1;
    }
    if (wakeFd >= 0 && (pollFds[1].revents & POLLIN) != 0) {
        errno = EINTR;
        return -1;
    }
    if (((pollFds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) && (pollFds[0].revents & POLLIN) == 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid fuse fd event, revents:%{public}d",
            pollFds[0].revents);
        errno = EIO;
        return -1;
    }
    ssize_t bytesRead = read(fd, buf, len);
    if (bytesRead == 0) {
        // A zero read makes the libfuse worker exit without terminating the coordinator loop.
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "empty fuse fd read");
        errno = EIO;
        return -1;
    }
    return bytesRead;
}

ssize_t PasteboardFuseManager::WriteFuseFd(int fd, struct iovec *iov, int count, void *userdata)
{
    (void)userdata;
    return writev(fd, iov, count);
}

void PasteboardFuseManager::CleanupLocked(int32_t userId)
{
    providers_.erase(userId);
    dataIds_.erase(userId);
    rootNodes_.erase(userId);
    for (auto it = fdMap_.begin(); it != fdMap_.end();) {
        if (it->second.userId == userId) {
            it = fdMap_.erase(it);
        } else {
            ++it;
        }
    }
    authorizedPastePids_.erase(userId);
    authorizedPasteUids_.erase(userId);
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
    auto it = providers_.find(userId);
    return it != providers_.end() && it->second != nullptr;
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
    authorizedPasteUids_.erase(userId);
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
    uint32_t dataId, const std::vector<FuseNode> &rootNodes)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetProvider userId:%{public}d, dataId:%{public}u", userId, dataId);
    std::lock_guard<std::mutex> lock(mutex_);
    providers_[userId] = provider;
    dataIds_[userId] = dataId;
    rootNodes_[userId] = rootNodes;
}

bool PasteboardFuseManager::CheckAuthorizationLocked(int32_t userId, uid_t callerUid)
{
    // Authorize by uid (process wide, stable across threads), following the same approach as
    // the UDMF fuse controller in datamgr_service: the kernel reports the TID of the
    // requesting thread in FUSE requests, which differs from the process pid registered by
    // OnGetData when the caller reads asynchronously from a worker thread. Comparing the
    // fuse ctx uid against the uids registered at OnGetData time accepts any thread of an
    // authorized process.
    auto it = authorizedPasteUids_.find(userId);
    if (it == authorizedPasteUids_.end() || it->second.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "CheckAuthorization no uids registered, userId:%{public}d, callerUid:%{public}d", userId, callerUid);
        return false;
    }
    if (it->second.count(callerUid) > 0) {
        return true;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
        "CheckAuthorization uid not allowed, userId:%{public}d, callerUid:%{public}d", userId, callerUid);
    return false;
}

const FuseNode *PasteboardFuseManager::FindNodeByPath(int32_t userId, uint32_t recordId,
    const std::string &filePath) const
{
    auto it = rootNodes_.find(userId);
    if (it == rootNodes_.end()) {
        return nullptr;
    }
    for (const auto &root : it->second) {
        if (root.recordId != recordId) {
            continue;
        }
        if (filePath == root.name) {
            return &root;
        }
        if (filePath.size() > root.name.size() + 1 &&
            filePath.compare(0, root.name.size(), root.name) == 0 &&
            filePath[root.name.size()] == '/') {
            return root.FindByPath(filePath.substr(root.name.size() + 1));
        }
        return nullptr;
    }
    return nullptr;
}

std::shared_ptr<FileInfo> PasteboardFuseManager::GetFileInfoLocked(int32_t userId, uint32_t dataId,
    uint32_t recordId, const std::string &fileName)
{
    const FuseNode *node = FindNodeByPath(userId, recordId, fileName);
    if (node != nullptr && node->IsFile()) {
        auto info = std::make_shared<FileInfo>();
        info->userId = userId;
        info->dataId = dataId;
        info->recordId = recordId;
        info->fileId = node->id;
        info->fileName = node->name;
        info->fileSize = node->size;
        info->modifyTime = node->modifyTime;
        info->isDir = false;
        auto providerIt = providers_.find(userId);
        if (providerIt != providers_.end()) {
            info->provider = providerIt->second;
        }
        return info;
    }
    return nullptr;
}

bool PasteboardFuseManager::ParseNumericSegment(const char *begin, size_t len, uint32_t &value)
{
    if (begin == nullptr || len == 0) {
        return false;
    }
    uint64_t result = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = begin[i];
        if (c < '0' || c > '9') {
            return false;
        }
        result = result * 10 + static_cast<uint64_t>(c - '0');
        if (result > UINT32_MAX) {
            return false;
        }
    }
    value = static_cast<uint32_t>(result);
    return true;
}

// Path layout inside the FUSE mount: /<dataId>/<recordId>/<fileName>.
int PasteboardFuseManager::ParsePath(const char *path, uint32_t &dataId, uint32_t &recordId,
    std::string &fileName)
{
    if (path == nullptr || path[0] != '/') {
        return PATH_INVALID;
    }
    const char *rest = path + 1;
    if (rest[0] == '\0') {
        return PATH_ROOT;
    }
    const char *slash = strchr(rest, '/');
    if (slash == nullptr) {
        if (!ParseNumericSegment(rest, strlen(rest), dataId)) {
            return PATH_INVALID;
        }
        return PATH_DATA_DIR;
    }
    if (!ParseNumericSegment(rest, slash - rest, dataId)) {
        return PATH_INVALID;
    }
    const char *recordSeg = slash + 1;
    if (recordSeg[0] == '\0') {
        return PATH_INVALID;
    }
    slash = strchr(recordSeg, '/');
    if (slash == nullptr) {
        if (!ParseNumericSegment(recordSeg, strlen(recordSeg), recordId)) {
            return PATH_INVALID;
        }
        return PATH_RECORD_DIR;
    }
    if (!ParseNumericSegment(recordSeg, slash - recordSeg, recordId)) {
        return PATH_INVALID;
    }
    fileName = std::string(slash + 1);
    if (fileName.empty()) {
        return PATH_INVALID;
    }
    return PATH_FILE;
}

void PasteboardFuseManager::FillFileStat(const FuseNode &node, struct stat *stbuf)
{
    if (node.IsDir()) {
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
    } else {
        stbuf->st_mode = S_IFREG | FUSE_FILE_MODE;
        stbuf->st_nlink = FUSE_FILE_NLINK;
        stbuf->st_size = node.size;
    }
    stbuf->st_mtime = static_cast<time_t>(node.modifyTime / MILLISEC_PER_SEC);
}

int PasteboardFuseManager::FillDirStatLocked(const HandlerContext &ctx, uint32_t dataId, uint32_t recordId,
    int pathType, struct stat *stbuf)
{
    auto self = ctx.self;
    if (pathType == PATH_ROOT) {
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
        return 0;
    }
    if (pathType == PATH_DATA_DIR) {
        auto dataIt = self->dataIds_.find(ctx.userId);
        if (dataIt == self->dataIds_.end() || dataIt->second != dataId) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
        return 0;
    }
    if (pathType == PATH_RECORD_DIR) {
        auto dataIt = self->dataIds_.find(ctx.userId);
        if (dataIt == self->dataIds_.end() || dataIt->second != dataId) {
            return -ENOENT;
        }
        auto nodesIt = self->rootNodes_.find(ctx.userId);
        if (nodesIt == self->rootNodes_.end()) {
            return -ENOENT;
        }
        bool recordFound = false;
        for (const auto &root : nodesIt->second) {
            if (root.recordId == recordId) {
                recordFound = true;
                break;
            }
        }
        if (!recordFound) {
            return -ENOENT;
        }
        stbuf->st_mode = S_IFDIR | FUSE_DIR_MODE;
        stbuf->st_nlink = FUSE_DIR_NLINK;
        return 0;
    }
    return -ENOENT;
}

int PasteboardFuseManager::StatLocked(const HandlerContext &ctx, const std::string &path, struct stat *stbuf)
{
    if (ctx.self == nullptr || stbuf == nullptr) {
        return -EACCES;
    }
    auto self = ctx.self;
    if (!self->CheckAuthorizationLocked(ctx.userId, ctx.uid)) {
        return -EACCES;
    }
    if (memset_s(stbuf, sizeof(struct stat), 0, sizeof(struct stat)) != EOK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "memset_s failed");
        return -EIO;
    }
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    int pathType = ParsePath(path.c_str(), dataId, recordId, fileName);
    if (pathType == PATH_FILE) {
        auto *node = self->FindNodeByPath(ctx.userId, recordId, fileName);
        if (node == nullptr) {
            return -ENOENT;
        }
        FillFileStat(*node, stbuf);
        return 0;
    }
    return FillDirStatLocked(ctx, dataId, recordId, pathType, stbuf);
}

int PasteboardFuseManager::StatByPath(const HandlerContext &ctx, const std::string &path, struct stat *stbuf,
    struct fuse_file_info *fi)
{
    (void)fi;
    if (ctx.self == nullptr || stbuf == nullptr) {
        return -EACCES;
    }
    auto self = ctx.self;
    std::shared_ptr<FileInfo> fileInfo;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        int ret = self->StatLocked(ctx, path, stbuf);
        if (ret != 0) {
            return ret;
        }
        // For file paths, grab the fileInfo so we can query dynamic size via OnStat.
        uint32_t dataId = 0;
        uint32_t recordId = 0;
        std::string fileName;
        if (ParsePath(path.c_str(), dataId, recordId, fileName) == PATH_FILE) {
            fileInfo = self->GetFileInfoLocked(ctx.userId, dataId, recordId, fileName);
        }
    }
    // Dynamic stat: call provider->OnStat outside the lock (IPC may block).
    // Pattern aligned with ReadByPath: semaphore + retry.
    if (fileInfo != nullptr && fileInfo->provider != nullptr) {
        self->readSemaphore_.acquire();
        FuseFileStat fuseStat{};
        int32_t ret = -1;
        for (int32_t attempt = 0; attempt <= MAX_READ_RETRIES; ++attempt) {
            if (attempt > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(READ_RETRY_DELAY_MS * attempt));
            }
            ret = fileInfo->provider->OnStat(fileInfo->recordId, fileInfo->fileId, fuseStat);
            if (ret == 0) {
                break;
            }
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE,
                "OnStat attempt %{public}d failed, ret:%{public}d", attempt + 1, ret);
        }
        self->readSemaphore_.release();
        if (ret == 0) {
            stbuf->st_size = fuseStat.size;
            stbuf->st_mtime = static_cast<time_t>(fuseStat.modifyTime / MILLISEC_PER_SEC);
        }
        // On failure, keep the static values from StatLocked (fallback).
    }
    return 0;
}

int PasteboardFuseManager::OpenLocked(const HandlerContext &ctx, const std::string &path,
    struct fuse_file_info *fi)
{
    auto self = ctx.self;
    if (!self->CheckAuthorizationLocked(ctx.userId, ctx.uid)) {
        return -EACCES;
    }
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    int pathType = ParsePath(path.c_str(), dataId, recordId, fileName);
    if (pathType != PATH_FILE) {
        auto dataIt = self->dataIds_.find(ctx.userId);
        if (pathType == PATH_DATA_DIR && dataIt != self->dataIds_.end() && dataIt->second == dataId) {
            return -EISDIR;
        }
        return -ENOENT;
    }
    auto fileInfo = self->GetFileInfoLocked(ctx.userId, dataId, recordId, fileName);
    if (fileInfo == nullptr) {
        const FuseNode *node = self->FindNodeByPath(ctx.userId, recordId, fileName);
        if (node != nullptr && node->IsDir()) {
            return -EISDIR;
        }
        return -ENOENT;
    }
    int32_t fd = self->nextFd_++;
    self->fdMap_[fd] = { ctx.pid, ctx.userId, fileInfo->fileId, fileInfo->recordId, 0 };
    fi->fh = static_cast<uint64_t>(fd);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "OpenLocked path:%{public}s, fd:%{public}d", path.c_str(), fd);
    return 0;
}

int PasteboardFuseManager::OpenByPath(const HandlerContext &ctx, const std::string &path,
    struct fuse_file_info *fi)
{
    if (ctx.self == nullptr) {
        return -EACCES;
    }
    std::lock_guard<std::mutex> lock(ctx.self->mutex_);
    return ctx.self->OpenLocked(ctx, path, fi);
}

int PasteboardFuseManager::InvokeOnReadWithRetry(PasteboardFuseManager *self,
    const sptr<IPasteboardFuseGetter> &provider, const std::string &fileId, uint32_t recordId,
    const std::string &path, off_t offset, size_t size, char *buf)
{
    int32_t readLen = static_cast<int32_t>(std::min(size, static_cast<size_t>(MAX_READ_SIZE)));
    self->readSemaphore_.acquire();
    std::vector<uint8_t> data;
    int32_t ret = -1;
    for (int32_t attempt = 0; attempt <= MAX_READ_RETRIES; ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(READ_RETRY_DELAY_MS * attempt));
        }
        data.clear();
        ret = provider->OnRead(recordId, fileId, offset, readLen, data);
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
        "ReadByPath path:%{public}s, offset:%{public}ld, size:%{public}zu, read:%{public}zu",
        path.c_str(), static_cast<long>(offset), size, copyLen);
    return static_cast<int>(copyLen);
}

int PasteboardFuseManager::ReadByPath(const HandlerContext &ctx, const std::string &path, char *buf,
    size_t size, off_t offset, struct fuse_file_info *fi)
{
    auto self = ctx.self;
    int32_t fd = static_cast<int32_t>(fi->fh);
    FdEntry fdEntry;
    sptr<IPasteboardFuseGetter> provider;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        if (!self->CheckAuthorizationLocked(ctx.userId, ctx.uid)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "ReadByPath auth failed, userId:%{public}d, callerPid:%{public}d", ctx.userId, ctx.pid);
            return -EACCES;
        }
        auto it = self->fdMap_.find(fd);
        if (it == self->fdMap_.end()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ReadByPath fd not found, fd:%{public}d", fd);
            return -ENOENT;
        }
        fdEntry = it->second;
        auto providerIt = self->providers_.find(ctx.userId);
        if (providerIt != self->providers_.end()) {
            provider = providerIt->second;
        }
    }
    if (fdEntry.type != 0) {
        return -EISDIR;
    }
    if (provider == nullptr) {
        return -EIO;
    }
    if (size == 0) {
        return 0;
    }
    return InvokeOnReadWithRetry(self, provider, fdEntry.fileId, fdEntry.recordId, path, offset, size, buf);
}

int PasteboardFuseManager::ReleaseByPath(const HandlerContext &ctx, const std::string &path,
    struct fuse_file_info *fi)
{
    (void)path;
    if (ctx.self == nullptr) {
        return 0;
    }
    int32_t fd = static_cast<int32_t>(fi->fh);
    std::lock_guard<std::mutex> lock(ctx.self->mutex_);
    ctx.self->fdMap_.erase(fd);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "ReleaseByPath fd:%{public}d", fd);
    return 0;
}

int PasteboardFuseManager::ListRecordDirLocked(const HandlerContext &ctx, uint32_t dataId, uint32_t recordId,
    std::vector<std::string> &names)
{
    auto dataIt = dataIds_.find(ctx.userId);
    if (dataIt == dataIds_.end() || dataIt->second != dataId) {
        return -ENOENT;
    }
    auto it = rootNodes_.find(ctx.userId);
    if (it == rootNodes_.end()) {
        return -ENOENT;
    }
    bool recordFound = false;
    for (const auto &root : it->second) {
        if (root.recordId != recordId) {
            continue;
        }
        if (!recordFound) {
            recordFound = true;
            names.push_back(".");
            names.push_back("..");
        }
        names.push_back(root.name);
    }
    if (!recordFound) {
        return -ENOENT;
    }
    return 0;
}

int PasteboardFuseManager::ListDataDirLocked(const HandlerContext &ctx, uint32_t dataId,
    std::vector<std::string> &names)
{
    auto self = ctx.self;
    auto dataIt = self->dataIds_.find(ctx.userId);
    if (dataIt == self->dataIds_.end() || dataIt->second != dataId) {
        return -ENOENT;
    }
    names.push_back(".");
    names.push_back("..");
    auto nodesIt = self->rootNodes_.find(ctx.userId);
    if (nodesIt != self->rootNodes_.end()) {
        std::set<uint32_t> listedRecords;
        for (const auto &root : nodesIt->second) {
            if (listedRecords.insert(root.recordId).second) {
                names.push_back(std::to_string(root.recordId));
            }
        }
    }
    return 0;
}

int PasteboardFuseManager::ListTreeDirLocked(const HandlerContext &ctx, uint32_t recordId,
    const std::string &fileName, std::vector<std::string> &names)
{
    auto self = ctx.self;
    const FuseNode *node = self->FindNodeByPath(ctx.userId, recordId, fileName);
    if (node == nullptr) {
        return -ENOENT;
    }
    if (!node->IsDir()) {
        return -ENOTDIR;
    }
    names.push_back(".");
    names.push_back("..");
    for (const auto &child : node->children) {
        names.push_back(child.name);
    }
    return 0;
}

int PasteboardFuseManager::ListDirLocked(const HandlerContext &ctx, const std::string &path,
    std::vector<std::string> &names)
{
    if (ctx.self == nullptr) {
        return -EACCES;
    }
    auto self = ctx.self;
    if (!self->CheckAuthorizationLocked(ctx.userId, ctx.uid)) {
        return -EACCES;
    }
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    int pathType = ParsePath(path.c_str(), dataId, recordId, fileName);
    if (pathType == PATH_ROOT) {
        names.push_back(".");
        names.push_back("..");
        auto dataIt = self->dataIds_.find(ctx.userId);
        if (dataIt != self->dataIds_.end()) {
            names.push_back(std::to_string(dataIt->second));
        }
        return 0;
    }
    if (pathType == PATH_DATA_DIR) {
        return ListDataDirLocked(ctx, dataId, names);
    }
    if (pathType == PATH_RECORD_DIR) {
        return self->ListRecordDirLocked(ctx, dataId, recordId, names);
    }
    if (pathType == PATH_FILE) {
        return ListTreeDirLocked(ctx, recordId, fileName, names);
    }
    return -ENOENT;
}

int PasteboardFuseManager::ReaddirByPath(const HandlerContext &ctx, const std::string &path,
    std::vector<std::string> &names)
{
    if (ctx.self == nullptr) {
        return -EACCES;
    }
    std::lock_guard<std::mutex> lock(ctx.self->mutex_);
    return ctx.self->ListDirLocked(ctx, path, names);
}

// ===== libfuse lowlevel callbacks =====

int32_t PasteboardFuseManager::ResolveLookupEntryLocked(const HandlerContext &ctx, uint64_t parent,
    const char *name, std::string &childPath, struct fuse_entry_param &entry,
    std::shared_ptr<FileInfo> &fileInfo)
{
    auto self = ctx.self;
    auto it = self->fuseSessions_.find(ctx.userId);
    if (it == self->fuseSessions_.end()) {
        return ENOENT;
    }
    auto &session = it->second;
    auto parentIt = session.inodePaths.find(parent);
    if (parentIt == session.inodePaths.end()) {
        return ENOENT;
    }
    const std::string &parentPath = parentIt->second;
    childPath = parentPath == "/" ? "/" + std::string(name) : parentPath + "/" + name;
    int ret = self->StatLocked(ctx, childPath, &entry.attr);
    if (ret != 0) {
        return -ret;
    }
    auto inodeIt = session.pathInodes.find(childPath);
    if (inodeIt != session.pathInodes.end()) {
        entry.ino = inodeIt->second;
    } else {
        entry.ino = session.nextInode++;
        session.pathInodes[childPath] = entry.ino;
        session.inodePaths[entry.ino] = childPath;
    }
    entry.attr.st_ino = entry.ino;
    entry.attr_timeout = ATTR_TIMEOUT_SECONDS;
    entry.entry_timeout = ATTR_TIMEOUT_SECONDS;
    // Grab fileInfo for dynamic OnStat outside the lock.
    uint32_t dId = 0;
    uint32_t rId = 0;
    std::string fn;
    if (ParsePath(childPath.c_str(), dId, rId, fn) == PATH_FILE) {
        fileInfo = self->GetFileInfoLocked(ctx.userId, dId, rId, fn);
    }
    return 0;
}

void PasteboardFuseManager::RefreshEntryStat(const std::shared_ptr<FileInfo> &fileInfo,
    struct fuse_entry_param &entry)
{
    if (fileInfo == nullptr || fileInfo->provider == nullptr) {
        return;
    }
    // Dynamic stat for files: call OnStat outside the lock (IPC + semaphore + retry).
    readSemaphore_.acquire();
    FuseFileStat fuseStat{};
    int32_t ret = -1;
    for (int32_t attempt = 0; attempt <= MAX_READ_RETRIES; ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(READ_RETRY_DELAY_MS * attempt));
        }
        ret = fileInfo->provider->OnStat(fileInfo->recordId, fileInfo->fileId, fuseStat);
        if (ret == 0) {
            break;
        }
    }
    readSemaphore_.release();
    if (ret == 0) {
        entry.attr.st_size = fuseStat.size;
        entry.attr.st_mtime = static_cast<time_t>(fuseStat.modifyTime / MILLISEC_PER_SEC);
    }
}

void PasteboardFuseManager::LookupHandler(struct fuse_req *req, uint64_t parent, const char *name)
{
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr || name == nullptr || name[0] == '\0') {
        fuse_reply_err(req, EINVAL);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    struct fuse_entry_param entry = {};
    std::string childPath;
    std::shared_ptr<FileInfo> fileInfo;
    int32_t err = 0;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        err = self->ResolveLookupEntryLocked(ctx, parent, name, childPath, entry, fileInfo);
    }
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    self->RefreshEntryStat(fileInfo, entry);
    fuse_reply_entry(req, &entry);
}

void PasteboardFuseManager::ForgetHandler(struct fuse_req *req, uint64_t ino, uint64_t nlookup)
{
    (void)ino;
    (void)nlookup;
    fuse_reply_none(req);
}

void PasteboardFuseManager::GetAttrHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi)
{
    (void)fi;
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    struct stat attr = {};
    int32_t err = 0;
    std::string path;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto it = self->fuseSessions_.find(ctx.userId);
        if (it == self->fuseSessions_.end()) {
            err = ENOENT;
        } else {
            auto inodeIt = it->second.inodePaths.find(ino);
            if (inodeIt == it->second.inodePaths.end()) {
                err = ENOENT;
            } else {
                path = inodeIt->second;
            }
        }
    }
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    // StatByPath handles locked stat + dynamic OnStat (for files) outside the lock.
    int ret = StatByPath(ctx, path, &attr, nullptr);
    if (ret != 0) {
        fuse_reply_err(req, -ret);
        return;
    }
    attr.st_ino = ino;
    fuse_reply_attr(req, &attr, ATTR_TIMEOUT_SECONDS);
}

void PasteboardFuseManager::OpendirHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi)
{
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    struct stat attr = {};
    int32_t err = 0;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto it = self->fuseSessions_.find(ctx.userId);
        if (it == self->fuseSessions_.end()) {
            err = ENOENT;
        } else {
            auto inodeIt = it->second.inodePaths.find(ino);
            if (inodeIt == it->second.inodePaths.end()) {
                err = ENOENT;
            } else {
                int ret = self->StatLocked(ctx, inodeIt->second, &attr);
                if (ret != 0) {
                    err = -ret;
                } else if ((attr.st_mode & S_IFDIR) == 0) {
                    err = ENOTDIR;
                }
            }
        }
    }
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    fuse_reply_open(req, fi);
}

void PasteboardFuseManager::ReaddirHandler(struct fuse_req *req, uint64_t ino, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    (void)fi;
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    std::vector<std::string> names;
    int32_t err = 0;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto it = self->fuseSessions_.find(ctx.userId);
        if (it == self->fuseSessions_.end()) {
            err = ENOENT;
        } else {
            auto inodeIt = it->second.inodePaths.find(ino);
            if (inodeIt == it->second.inodePaths.end()) {
                err = ENOENT;
            } else {
                int ret = self->ListDirLocked(ctx, inodeIt->second, names);
                if (ret != 0) {
                    err = -ret;
                }
            }
        }
    }
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    size_t bufferSize = std::min(size, static_cast<size_t>(MAX_READDIR_SIZE));
    std::vector<char> buffer(bufferSize);
    size_t used = 0;
    size_t start = offset < 0 ? 0 : static_cast<size_t>(offset);
    for (size_t index = start; index < names.size(); ++index) {
        struct stat entryStat = {};
        entryStat.st_ino = 0;
        entryStat.st_mode = (names[index] == "." || names[index] == "..") ? S_IFDIR : S_IFREG;
        size_t needed = fuse_add_direntry(req, nullptr, 0, names[index].c_str(), &entryStat,
            static_cast<off_t>(index + 1));
        if (needed > bufferSize - used) {
            break;
        }
        (void)fuse_add_direntry(req, buffer.data() + used, bufferSize - used, names[index].c_str(), &entryStat,
            static_cast<off_t>(index + 1));
        used += needed;
    }
    fuse_reply_buf(req, buffer.data(), used);
}

void PasteboardFuseManager::ReleasedirHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi)
{
    (void)ino;
    (void)fi;
    fuse_reply_err(req, 0);
}

void PasteboardFuseManager::OpenHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi)
{
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    int32_t err = 0;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto it = self->fuseSessions_.find(ctx.userId);
        if (it == self->fuseSessions_.end()) {
            err = ENOENT;
        } else {
            auto inodeIt = it->second.inodePaths.find(ino);
            if (inodeIt == it->second.inodePaths.end()) {
                err = ENOENT;
            } else {
                int ret = self->OpenLocked(ctx, inodeIt->second, fi);
                if (ret != 0) {
                    err = -ret;
                }
            }
        }
    }
    if (err != 0) {
        fuse_reply_err(req, err);
        return;
    }
    fuse_reply_open(req, fi);
}

void PasteboardFuseManager::ReadHandler(struct fuse_req *req, uint64_t ino, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    std::string path;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto it = self->fuseSessions_.find(ctx.userId);
        if (it != self->fuseSessions_.end()) {
            auto inodeIt = it->second.inodePaths.find(ino);
            if (inodeIt != it->second.inodePaths.end()) {
                path = inodeIt->second;
            }
        }
    }
    std::vector<char> buffer(size);
    int ret = ReadByPath(ctx, path, buffer.data(), size, offset, fi);
    if (ret < 0) {
        fuse_reply_err(req, -ret);
        return;
    }
    fuse_reply_buf(req, buffer.data(), static_cast<size_t>(ret));
}

void PasteboardFuseManager::ReleaseHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi)
{
    (void)ino;
    auto *sessionCtx = static_cast<SessionContext *>(fuse_req_userdata(req));
    if (sessionCtx == nullptr || sessionCtx->manager == nullptr) {
        fuse_reply_err(req, 0);
        return;
    }
    auto self = sessionCtx->manager;
    const struct fuse_ctx *reqCtx = fuse_req_ctx(req);
    HandlerContext ctx{ self, sessionCtx->userId, reqCtx == nullptr ? -1 : reqCtx->pid,
        reqCtx == nullptr ? -1 : reqCtx->uid };
    (void)ReleaseByPath(ctx, "", fi);
    fuse_reply_err(req, 0);
}

// ===== Legacy highlevel wrappers for unit-test compatibility =====

PasteboardFuseManager::HandlerContext PasteboardFuseManager::BuildHandlerContext()
{
    struct fuse_context *fuseCtx = fuse_get_context();
    if (fuseCtx == nullptr || fuseCtx->private_data == nullptr) {
        return {};
    }
    auto *sessionCtx = static_cast<SessionContext *>(fuseCtx->private_data);
    return { sessionCtx->manager, sessionCtx->userId, fuseCtx->pid, fuseCtx->uid };
}

bool PasteboardFuseManager::CheckAuthorization(int32_t userId)
{
    struct fuse_context *fuseCtx = fuse_get_context();
    if (fuseCtx == nullptr) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    return CheckAuthorizationLocked(userId, fuseCtx->uid);
}

int PasteboardFuseManager::StatHandler(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    return StatByPath(BuildHandlerContext(), path ? std::string(path) : std::string(), stbuf, nullptr);
}

int PasteboardFuseManager::OpenHandler(const char *path, struct fuse_file_info *fi)
{
    return OpenByPath(BuildHandlerContext(), path ? std::string(path) : std::string(), fi);
}

int PasteboardFuseManager::ReadHandler(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
    return ReadByPath(BuildHandlerContext(), path ? std::string(path) : std::string(), buf, size, offset, fi);
}

int PasteboardFuseManager::ReleaseHandler(const char *path, struct fuse_file_info *fi)
{
    return ReleaseByPath(BuildHandlerContext(), path ? std::string(path) : std::string(), fi);
}

int PasteboardFuseManager::ReaddirHandler(const char *path, void *buf, void *filler, off_t offset,
    struct fuse_file_info *fi, int flags)
{
    (void)offset;
    (void)fi;
    (void)flags;
    auto ctx = BuildHandlerContext();
    std::vector<std::string> names;
    int ret = ReaddirByPath(ctx, path ? std::string(path) : std::string(), names);
    if (ret != 0) {
        return ret;
    }
    auto fillFunc = reinterpret_cast<fuse_fill_dir_t>(filler);
    if (fillFunc != nullptr) {
        for (const auto &name : names) {
            struct stat entryStat = {};
            entryStat.st_ino = 0;
            entryStat.st_mode = (name == "." || name == "..") ? S_IFDIR : S_IFREG;
            fillFunc(buf, name.c_str(), &entryStat, 0, FUSE_FILL_DIR_DEFAULTS);
        }
    }
    return 0;
}

} // namespace MiscServices
} // namespace OHOS
