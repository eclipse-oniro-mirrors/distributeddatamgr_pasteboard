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

#ifndef PASTEBOARD_FUSE_MANAGER_H
#define PASTEBOARD_FUSE_MANAGER_H

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <semaphore>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <thread>
#include <vector>

#include "ipasteboard_fuse_getter.h"
#include "paste_data_record.h"
#include "pasteboard_hilog.h"
#include "pasteboard_virtual_entry.h"

struct fuse_req;
struct fuse_session;
struct fuse_file_info;
struct fuse_conn_info;

namespace OHOS {
namespace MiscServices {

struct AppInfo;

struct FileInfo {
    int32_t userId = 0;
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileId;
    std::string fileName;
    int64_t fileSize = 0;
    int64_t modifyTime = 0;
    bool isDir = false;
    sptr<IPasteboardFuseGetter> provider;
};

class PasteboardFuseManager final {
public:
    PasteboardFuseManager() = default;
    ~PasteboardFuseManager();
    PasteboardFuseManager(const PasteboardFuseManager &) = delete;
    PasteboardFuseManager &operator=(const PasteboardFuseManager &) = delete;
    PasteboardFuseManager(PasteboardFuseManager &&) = delete;
    PasteboardFuseManager &operator=(PasteboardFuseManager &&) = delete;

    int32_t OnGetData(int32_t userId);
    int32_t OnAppExit(pid_t pid, int32_t userId);
    void OnAged(int32_t userId);
    void OnRemoveData(int32_t userId);
    void OnProviderDied(int32_t userId);
    bool HasProvider(int32_t userId);
    void SetProvider(int32_t userId, const sptr<IPasteboardFuseGetter> &provider,
        uint32_t dataId, const std::vector<FuseNode> &rootNodes);

private:
    int32_t MountFuse(int32_t userId, const std::string &path, int32_t &fuseFd);
    int32_t UMountFuse(int32_t userId, const std::string &path);
    void StartSessionLoop(int32_t userId);
    void StopSessionLoop(int32_t userId);
    void LoopMain(int32_t userId);
    void CleanupCopyAppLocked(int32_t userId);
    bool CleanupPasteAppLocked(pid_t pid, int32_t userId);
    void CleanupLocked(int32_t userId);
    bool CheckAuthorizationLocked(int32_t userId, uid_t callerUid);
    std::shared_ptr<FileInfo> GetFileInfoLocked(int32_t userId, uint32_t dataId,
        uint32_t recordId, const std::string &fileName);
    const FuseNode *FindNodeByPath(int32_t userId, uint32_t recordId,
        const std::string &filePath) const;
    static int ParsePath(const char *path, uint32_t &dataId, uint32_t &recordId, std::string &fileName);
    static bool ParseNumericSegment(const char *begin, size_t len, uint32_t &value);
    std::string GetMountPath(int32_t userId);

    // Per request context passed explicitly to the path based helpers.
    struct HandlerContext {
        PasteboardFuseManager *self = nullptr;
        int32_t userId = 0;
        pid_t pid = 0;
        uid_t uid = 0;
    };

    // Path based helpers, usable both from the lowlevel callbacks and tests.
    static int StatByPath(const HandlerContext &ctx, const std::string &path, struct stat *stbuf,
        struct fuse_file_info *fi);
    static int OpenByPath(const HandlerContext &ctx, const std::string &path, struct fuse_file_info *fi);
    static int ReadByPath(const HandlerContext &ctx, const std::string &path, char *buf, size_t size,
        off_t offset, struct fuse_file_info *fi);
    static int ReleaseByPath(const HandlerContext &ctx, const std::string &path, struct fuse_file_info *fi);
    static int ReaddirByPath(const HandlerContext &ctx, const std::string &path,
        std::vector<std::string> &names);

    // Cores that require mutex_ to be held by the caller (except ReadByPath, which locks itself).
    int StatLocked(const HandlerContext &ctx, const std::string &path, struct stat *stbuf);
    int OpenLocked(const HandlerContext &ctx, const std::string &path, struct fuse_file_info *fi);
    int ListDirLocked(const HandlerContext &ctx, const std::string &path, std::vector<std::string> &names);
    int ListRecordDirLocked(const HandlerContext &ctx, uint32_t dataId, uint32_t recordId,
        std::vector<std::string> &names);
    static void FillFileStat(const FuseNode &node, struct stat *stbuf);

    // libfuse lowlevel callbacks invoked by the session loop.
    static void LookupHandler(struct fuse_req *req, uint64_t parent, const char *name);
    static void ForgetHandler(struct fuse_req *req, uint64_t ino, uint64_t nlookup);
    static void GetAttrHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    static void OpendirHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    static void ReaddirHandler(struct fuse_req *req, uint64_t ino, size_t size, off_t offset,
        struct fuse_file_info *fi);
    static void ReleasedirHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    static void OpenHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    static void ReadHandler(struct fuse_req *req, uint64_t ino, size_t size, off_t offset,
        struct fuse_file_info *fi);
    static void ReleaseHandler(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    static void InitHandler(void *userdata, struct fuse_conn_info *conn);

    // Custom io injected into the session so the storage_manager provided fd is used directly.
    static ssize_t ReadFuseFd(int fd, void *buf, size_t len, void *userdata);
    static ssize_t WriteFuseFd(int fd, struct iovec *iov, int count, void *userdata);

    // Legacy highlevel path-based handlers retained for unit-test compatibility.
    // Production uses the lowlevel handlers above; these wrappers obtain the context
    // via fuse_get_context (mocked in tests) and delegate to the ByPath helpers.
    static int StatHandler(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
    static int OpenHandler(const char *path, struct fuse_file_info *fi);
    static int ReadHandler(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi);
    static int ReleaseHandler(const char *path, struct fuse_file_info *fi);
    static int ReaddirHandler(const char *path, void *buf, void *filler, off_t offset,
        struct fuse_file_info *fi, int flags);
    static HandlerContext BuildHandlerContext();
    // Legacy authorization check via fuse_get_context (mocked in tests).
    bool CheckAuthorization(int32_t userId);

    struct SessionContext {
        PasteboardFuseManager *manager = nullptr;
        int32_t userId = 0;
        int wakeFd = -1;
    };

    struct FuseSession {
        struct fuse_session *session = nullptr;
        std::thread loopThread;
        std::atomic<bool> running{ false };
        std::string mountPath;
        int32_t fuseFd = -1;
        int32_t wakeFd = -1;
        SessionContext context;
        std::map<uint64_t, std::string> inodePaths;
        std::map<std::string, uint64_t> pathInodes;
        uint64_t nextInode = 0;
    };

    static constexpr int32_t MAX_THREADS = 5;
    static constexpr int32_t MAX_READ_RETRIES = 2;
    static constexpr int32_t READ_RETRY_DELAY_MS = 50;
    static constexpr int32_t MAX_READDIR_SIZE = 64 * 1024;
    static constexpr int32_t FUSE_DIR_MODE = 0755;
    static constexpr int32_t FUSE_FILE_MODE = 0444;
    static constexpr int32_t FUSE_DIR_NLINK = 2;
    static constexpr int32_t FUSE_FILE_NLINK = 1;
    static constexpr int32_t FUSE_INIT_FD = 1000;
    static constexpr uint64_t FUSE_ROOT_INO = 1;
    static constexpr int32_t MILLISEC_PER_SEC = 1000;
    static constexpr int32_t SEMAPHORE_MAX_COUNT = 5;
    static constexpr double ATTR_TIMEOUT_SECONDS = 1.0;
    // storage_manager only accepts clipboard FUSE mounts under /mnt/data/<userId>/clipboard_fuse/.
    static constexpr const char *const MOUNT_BASE = "/mnt/data";
    static constexpr const char *const MOUNT_SUFFIX = "/clipboard_fuse/files";
    static constexpr int32_t PATH_INVALID = -1;
    static constexpr int32_t PATH_ROOT = 0;
    static constexpr int32_t PATH_DATA_DIR = 1;
    static constexpr int32_t PATH_RECORD_DIR = 2;
    static constexpr int32_t PATH_FILE = 3;

    std::mutex mutex_;
    std::map<int32_t, FuseSession> fuseSessions_;
    std::map<int32_t, std::set<pid_t>> authorizedPastePids_;
    // The uid set registered when the paste target calls OnGetData. Authorization is checked
    // against fuse ctx uid (process wide, stable across threads) instead of pid, because the
    // kernel reports the TID of the requesting thread in FUSE requests, which differs from the
    // process pid when the caller reads asynchronously from a worker thread.
    std::map<int32_t, std::set<uid_t>> authorizedPasteUids_;
    std::map<int32_t, sptr<IPasteboardFuseGetter>> providers_;
    std::map<int32_t, uint32_t> dataIds_;
    std::map<int32_t, std::vector<FuseNode>> rootNodes_;
    struct FdEntry {
        pid_t openerPid = 0;
        int32_t userId = 0;
        std::string fileId;
        uint32_t recordId = 0;
        int32_t type = 0;
    };
    std::map<int32_t, FdEntry> fdMap_;
    std::counting_semaphore<SEMAPHORE_MAX_COUNT> readSemaphore_{ SEMAPHORE_MAX_COUNT };
    std::atomic<int32_t> nextFd_{ FUSE_INIT_FD };
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_FUSE_MANAGER_H
