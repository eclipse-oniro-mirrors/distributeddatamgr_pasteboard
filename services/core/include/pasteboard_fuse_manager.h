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
#include <set>
#include <semaphore>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "ipasteboard_fuse_getter.h"
#include "paste_data_record.h"
#include "pasteboard_hilog.h"

struct fuse;
struct fuse_file_info;

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
        uint32_t dataId, const std::vector<FileInfo> &fileInfos);

private:
    int32_t MountFuse(int32_t userId, const std::string &path, int32_t &fuseFd);
    int32_t UMountFuse(int32_t userId, const std::string &path);
    void StartSessionLoop(int32_t userId, struct fuse *fuseHandle);
    void StopSessionLoop(int32_t userId);
    void CleanupCopyAppLocked(int32_t userId);
    bool CleanupPasteAppLocked(pid_t pid, int32_t userId);
    void CleanupLocked(int32_t userId);
    bool CheckAuthorization(int32_t userId);
    bool CheckAuthorizationLocked(int32_t userId, pid_t callerPid);
    std::shared_ptr<FileInfo> GetFileInfoLocked(int32_t userId, uint32_t dataId,
        uint32_t recordId, const std::string &fileName);
    static int ParsePath(const char *path, uint32_t &dataId, uint32_t &recordId, std::string &fileName);
    std::string GetMountPath(int32_t userId);

    static int StatHandler(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
    static int OpenHandler(const char *path, struct fuse_file_info *fi);
    static int ReadHandler(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    static int ReleaseHandler(const char *path, struct fuse_file_info *fi);
    static int ReaddirHandler(const char *path, void *buf, void *filler, off_t offset,
        struct fuse_file_info *fi, int flags);
    static void FillFileStat(const FileInfo &info, struct stat *stbuf);
    int ReadDataIdDirLocked(int32_t userId, uint32_t dataId, void *filler, void *buf);

    struct SessionContext {
        PasteboardFuseManager *manager = nullptr;
        int32_t userId = 0;
    };

    struct FuseSession {
        struct fuse *handle = nullptr;
        std::thread loopThread;
        std::atomic<bool> running{false};
        std::string mountPath;
        int32_t fuseFd = -1;
        SessionContext context;
    };

    static constexpr int32_t MAX_READ_SIZE = 5 * 1024 * 1024;
    static constexpr int32_t MAX_THREADS = 5;
    static constexpr int32_t MAX_READ_RETRIES = 2;
    static constexpr int32_t READ_RETRY_DELAY_MS = 50;
    static constexpr int32_t FUSE_DIR_MODE = 0755;
    static constexpr int32_t FUSE_FILE_MODE = 0444;
    static constexpr int32_t FUSE_DIR_NLINK = 2;
    static constexpr int32_t FUSE_FILE_NLINK = 1;
    static constexpr int32_t FUSE_INIT_FD = 1000;
    static constexpr int32_t MILLISEC_PER_SEC = 1000;
    static constexpr int32_t SEMAPHORE_MAX_COUNT = 5;
    static constexpr const char *const MOUNT_BASE = "/mnt/data";
    static constexpr const char *const MOUNT_SUFFIX = "/clipboard_fuse/files";
    static constexpr int32_t PATH_ROOT = 0;
    static constexpr int32_t PATH_DATA_ID_DIR = 1;
    static constexpr int32_t PATH_RECORD_ID_DIR = 2;
    static constexpr int32_t PATH_FILE = 3;
    static constexpr int32_t PATH_INVALID = -1;

    std::mutex mutex_;
    std::map<int32_t, FuseSession> fuseSessions_;
    std::map<int32_t, std::set<pid_t>> authorizedPastePids_;
    std::map<int32_t, sptr<IPasteboardFuseGetter>> providers_;
    std::map<int32_t, std::vector<FileInfo>> fileInfos_;
    struct FdEntry {
        pid_t openerPid = 0;
        std::shared_ptr<FileInfo> fileInfo;
    };
    std::map<int32_t, FdEntry> fdMap_;
    std::counting_semaphore<SEMAPHORE_MAX_COUNT> readSemaphore_{SEMAPHORE_MAX_COUNT};
    std::atomic<int32_t> nextFd_{FUSE_INIT_FD};
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_FUSE_MANAGER_H
