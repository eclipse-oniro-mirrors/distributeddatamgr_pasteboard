
/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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


#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <iservice_registry.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <memory>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tuple>
#include <unistd.h>
#include <vector>

#include "datashare_helper.h"
#include "file_uri.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "paste_data.h"
#include "pasteboard_copy.h"
#include "pasteboard_error.h"
#include "pasteboard_fd_guard.h"
#include "pasteboard_hilog.h"
#include "system_ability_definition.h"
#include "trans_listener.h"

using namespace std::chrono;
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServices {
using namespace AppFileService::ModuleFileUri;
namespace fs = std::filesystem;
const std::string FILE_PREFIX_NAME = "file://";
const std::string NETWORK_PARA = "?networkid=";
const std::string PROCEDURE_COPY_NAME = "FileFSCopy";
const std::string MEDIALIBRARY_DATA_URI = "datashare:///media";
const std::string MEDIA = "media";
constexpr int DISMATCH = 0;
constexpr int PERCENTAGE = 100;
constexpr int MATCH = 1;
constexpr int BUF_SIZE = 1024;
constexpr int E_PERMISSION = 201;           //Just for compile
constexpr int ERRNO_NOERR = 0;           //Just for compile
constexpr size_t MAX_SIZE = 1024 * 1024 * 4;
constexpr std::chrono::milliseconds NOTIFY_PROGRESS_DELAY(100);
std::recursive_mutex mutex_;
std::map<CopyInfo, std::shared_ptr<CopyCallback>> cbMap_;
static ProgressListener progressListener_;

static std::string GetModeFromFlags(unsigned int flags)
{
    const std::string readMode = "r";
    const std::string writeMode = "w";
    const std::string appendMode = "a";
    const std::string truncMode = "t";
    std::string mode = readMode;
    mode += (((flags & O_RDWR) == O_RDWR) ? writeMode : "");
    mode = (((flags & O_WRONLY) == O_WRONLY) ? writeMode : mode);
    if (mode != readMode) {
        mode += ((flags & O_TRUNC) ? truncMode : "");
        mode += ((flags & O_APPEND) ? appendMode : "");
    }
    return mode;
}

static int32_t OpenSrcFile(const std::string &srcPath, std::shared_ptr<CopyInfo> copyInfo, int32_t &srcFd)
{
    Uri uri(copyInfo->srcUri);
    if (uri.GetAuthority() == MEDIA) {
        std::shared_ptr<DataShare::DataShareHelper> dataShareHelper = nullptr;
        sptr<FileIoToken> remote = new (std::nothrow) IRemoteStub<FileIoToken>();
        if (!remote) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to get remote object");
            return ENOMEM;
        }
        dataShareHelper = DataShare::DataShareHelper::Creator(remote->AsObject(), MEDIALIBRARY_DATA_URI);
        if (!dataShareHelper) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to connect to datashare");
            return E_PERMISSION;
        }
        srcFd = dataShareHelper->OpenFile(uri, GetModeFromFlags(O_RDONLY));
        if (srcFd < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Open media uri by data share fail. ret = %{public}d", srcFd);
            return EPERM;
        }
    } else {
        srcFd = open(srcPath.c_str(), O_RDONLY);
        if (srcFd < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Error opening file descriptor. errno = %{public}d", errno);
            return errno;
        }
    }
    return 0;
}

void fs_req_cleanup(uv_fs_t* req)
{
    uv_fs_req_cleanup(req);
    if (req) {
        delete req;
        req = nullptr;
    }
}

static int32_t SendFileCore(std::shared_ptr<FDGuard> srcFdg,
                            std::shared_ptr<FDGuard> destFdg,
                            std::shared_ptr<CopyInfo> copyInfo)
{
    std::unique_ptr<uv_fs_t, decltype(fs_req_cleanup)*> sendFileReq = {
        new (std::nothrow) uv_fs_t, fs_req_cleanup };
    if (sendFileReq == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory");
        return ENOMEM;
    }
    int64_t offset = 0;
    struct stat srcStat{};
    if (fstat(srcFdg->GetFD(), &srcStat) < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to get stat of file by fd=%{public}d, errno=%{public}d",
            srcFdg->GetFD(), errno);
        return errno;
    }
    int32_t ret = 0;
    int64_t size = static_cast<int64_t>(srcStat.st_size);
    while (size >= 0) {
        ret = uv_fs_sendfile(nullptr, sendFileReq.get(), destFdg->GetFD(), srcFdg->GetFD(),
            offset, MAX_SIZE, nullptr);
        if (ret < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to sendfile by errno=%{public}d", errno);
            return errno;
        }
        offset += static_cast<int64_t>(ret);
        size -= static_cast<int64_t>(ret);
        if (ret == 0) {
            break;
        }
    }
    if (size != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "The execution of the sendfile task was terminated,"
            "remaining file size %{public}" PRIu64, size);
        return EIO;
    }
    return ERRNO_NOERR;
}

bool IsDirectory(const std::string &path)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Stat failed. errno=%{public}d", errno);
        return false;
    }
    return (buf.st_mode & S_IFMT) == S_IFDIR;
}

bool IsFile(const std::string &path)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Stat failed. errno = %{public}d", errno);
        return false;
    }
    return (buf.st_mode & S_IFMT) == S_IFREG;
}

bool IsMediaUri(const std::string &uriPath)
{
    Uri uri(uriPath);
    std::string bundleName = uri.GetAuthority();
    return bundleName == MEDIA;
}

int32_t CheckOrCreatePath(const std::string &destPath)
{
    std::error_code errCode;
    if (!std::filesystem::exists(destPath, errCode) && errCode.value() == ERRNO_NOERR) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "destPath not exist");
        auto file = open(destPath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        if (file < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Error opening file descriptor. errno = %{public}d", errno);
            return errno;
        }
        close(file);
    } else if (errCode.value() != 0) {
        return errCode.value();
    }
    return ERRNO_NOERR;
}

static int FilterFunc(const struct dirent *filename)
{
    if (std::string_view(filename->d_name) == "." || std::string_view(filename->d_name) == "..") {
        return DISMATCH;
    }
    return MATCH;
}

struct NameList {
    struct dirent **namelist = { nullptr };
    int direntNum = 0;
};

static void Deleter(struct NameList *arg)
{
    for (int i = 0; i < arg->direntNum; i++) {
        free((arg->namelist)[i]);
        (arg->namelist)[i] = nullptr;
    }
    free(arg->namelist);
    arg->namelist = nullptr;
    delete arg;
    arg = nullptr;
}

int32_t MakeDir(const std::string &path)
{
    std::filesystem::path destDir(path);
    std::error_code errCode;
    if (!std::filesystem::create_directory(destDir, errCode)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to create directory, errno code = %{public}d",
            errCode.value());
        return errCode.value();
    }
    return ERRNO_NOERR;
}

int32_t RecurCopyDir(const std::string &srcPath, const std::string &destPath, std::shared_ptr<CopyInfo> copyInfo)
{
    std::unique_ptr<struct NameList, decltype(Deleter) *> pNameList = { new (std::nothrow) struct NameList, Deleter };
    if (pNameList == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory.");
        return ENOMEM;
    }
    int num = scandir(srcPath.c_str(), &(pNameList->namelist), FilterFunc, alphasort);
    if (num <= 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid scandir num.");
        return ENOMEM;
    }
    pNameList->direntNum = num;

    for (int i = 0; i < num; i++) {
        std::string src = srcPath + '/' + std::string((pNameList->namelist[i])->d_name);
        std::string dest = destPath + '/' + std::string((pNameList->namelist[i])->d_name);
        if ((pNameList->namelist[i])->d_type == DT_LNK) {
            continue;
        }
        int ret = ERRNO_NOERR;
        if ((pNameList->namelist[i])->d_type == DT_DIR) {
            ret = CopySubDir(src, dest, copyInfo);
        } else {
            copyInfo->filePaths.insert(dest);
            ret = CopyFile(src, dest, copyInfo);
        }
        if (ret != ERRNO_NOERR) {
            return ret;
        }
    }
    return ERRNO_NOERR;
}

int32_t CopySubDir(const std::string &srcPath, const std::string &destPath, std::shared_ptr<CopyInfo> copyInfo)
{
    std::error_code errCode;
    if (!std::filesystem::exists(destPath, errCode) && errCode.value() == ERRNO_NOERR) {
        int res = MakeDir(destPath);
        if (res != ERRNO_NOERR) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to mkdir");
            return res;
        }
    } else if (errCode.value() != ERRNO_NOERR) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "fs exists fail, errcode is %{public}d", errCode.value());
        return errCode.value();
    }
    uint32_t watchEvents = IN_MODIFY;
    if (copyInfo->notifyFd >= 0) {
        int newWd = inotify_add_watch(copyInfo->notifyFd, destPath.c_str(), watchEvents);
        if (newWd < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "inotify_add_watch,unvalid newWd , newWd=%{public}d", newWd);
            return errno;
        }
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            auto iter = cbMap_.find(*copyInfo);
            std::shared_ptr<ReceiveInfo> receiveInfo = std::make_shared<ReceiveInfo>();
            if (receiveInfo == nullptr) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory.");
                return ENOMEM;
            }
            receiveInfo->path = destPath;
            if (iter == cbMap_.end() || iter->second == nullptr) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to find infos ,srcPath = %{public}s,"
                    " destPath = %{public}s", copyInfo->srcPath.c_str(), copyInfo->destPath.c_str());
                return UNKNOWN_ERROR;
            }
            iter->second->wds.push_back({ newWd, receiveInfo });
        }
    }
    return RecurCopyDir(srcPath, destPath, copyInfo);
}

int32_t CopyDirFunc(const std::string &src, const std::string &dest, std::shared_ptr<CopyInfo> copyInfo)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "CopyDirFunc in, src=%{public}s, dest=%{public}s",
        src.c_str(), dest.c_str());
    size_t found = dest.find(src);
    if (found != std::string::npos && found == 0) {
        return EINVAL;
    }
    fs::path srcPath = fs::u8path(src);
    std::string dirName;
    if (srcPath.has_parent_path()) {
        dirName = srcPath.parent_path().filename();
    }
    std::string destStr = dest + "/" + dirName;
    return CopySubDir(src, destStr, copyInfo);
}

int32_t CopyFile(const std::string &src, const std::string &dest, std::shared_ptr<CopyInfo> copyInfo)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "src = %{public}s, dest = %{public}s", src.c_str(), dest.c_str());
    int32_t srcFd = -1;
    int32_t ret = OpenSrcFile(src, copyInfo, srcFd);
    if (srcFd < 0) {
        return ret;
    }
    auto destFd = open(dest.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (destFd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Error opening file descriptor. errno = %{public}d", errno);
        close(srcFd);
        return errno;
    }
    std::shared_ptr<FDGuard> srcFdg = std::make_shared<FDGuard>(srcFd, true);
    std::shared_ptr<FDGuard> destFdg = std::make_shared<FDGuard>(destFd, true);
    if (srcFdg == nullptr || destFdg == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory.");
        close(srcFd);
        close(destFd);
        return ENOMEM;
    }
    return SendFileCore(move(srcFdg), move(destFdg), copyInfo);
}

int32_t ExecCopy(std::shared_ptr<CopyInfo> copyInfo)
{
    if (copyInfo->isFile && IsFile(copyInfo->destPath)) {
        return CopyFile(copyInfo->srcPath.c_str(), copyInfo->destPath.c_str(), copyInfo);
    }
    if (copyInfo->isFile && IsDirectory(copyInfo->destPath)) {
        if (copyInfo->srcPath.back() != '/') {
            copyInfo->srcPath += '/';
        }
        if (copyInfo->destPath.back() != '/') {
            copyInfo->destPath += '/';
        }
        return CopyDirFunc(copyInfo->srcPath.c_str(), copyInfo->destPath.c_str(), copyInfo);
    }
    return EINVAL;
}

uint64_t GetDirSize(std::shared_ptr<CopyInfo> infos, std::string path)
{
    std::unique_ptr<struct NameList, decltype(Deleter) *> pNameList = { new (std::nothrow) struct NameList, Deleter };
    if (pNameList == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory.");
        return ENOMEM;
    }
    int num = scandir(path.c_str(), &(pNameList->namelist), FilterFunc, alphasort);
    if (num <= 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid scandir num.");
        return ENOMEM;
    }
    pNameList->direntNum = num;

    long int size = 0;
    for (int i = 0; i < num; i++) {
        std::string dest = path + '/' + std::string((pNameList->namelist[i])->d_name);
        if ((pNameList->namelist[i])->d_type == DT_LNK) {
            continue;
        }
        if ((pNameList->namelist[i])->d_type == DT_DIR) {
            size += static_cast<int64_t>(GetDirSize(infos, dest));
        } else {
            struct stat st {};
            if (stat(dest.c_str(), &st) == -1) {
                return size;
            }
            size += st.st_size;
        }
    }
    return size;
}

std::tuple<int, uint64_t> GetFileSize(const std::string &path)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Stat failed.");
        return { errno, 0 };
    }
    return { ERRNO_NOERR, buf.st_size };
}

int32_t InitLocalListener(std::shared_ptr<CopyInfo> infos, std::shared_ptr<CopyCallback> callback)
{
    infos->notifyFd = inotify_init();
    if (infos->notifyFd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to init inotify, errno:%{public}d", errno);
        return errno;
    }
    infos->eventFd = eventfd(0, EFD_CLOEXEC);
    if (infos->eventFd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to init eventFd, errno:%{public}d", errno);
        return errno;
    }
    callback->notifyFd = infos->notifyFd;
    callback->eventFd = infos->eventFd;
    int newWd = inotify_add_watch(infos->notifyFd, infos->destPath.c_str(), IN_MODIFY);
    if (newWd < 0) {
        auto errCode = errno;
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to add watch, errno = %{public}d,"
            "notifyFd: %{public}d, destPath: %{public}s", errno, infos->notifyFd, infos->destPath.c_str());
        CloseNotifyFd(infos, callback);
        return errCode;
    }
    std::shared_ptr<ReceiveInfo> receiveInfo = std::make_shared<ReceiveInfo>();
    if (receiveInfo == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory.");
        inotify_rm_watch(infos->notifyFd, newWd);
        CloseNotifyFd(infos, callback);
        return ENOMEM;
    }
    receiveInfo->path = infos->destPath;
    callback->wds.push_back({ newWd, receiveInfo });
    if (!infos->isFile) {
        callback->totalSize = GetDirSize(infos, infos->srcPath);
        if (callback->totalSize == 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid totalSize.");
            return ENOMEM;
        }
        return ERRNO_NOERR;
    }
    auto [err, fileSize] = GetFileSize(infos->srcPath);
    if (err == ERRNO_NOERR) {
        callback->totalSize = fileSize;
        if (callback->totalSize == 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid totalSize.");
            return ENOMEM;
        }
    }
    infos->hasListener = true;
    return err;
}

std::shared_ptr<CopyCallback> GetRegisteredListener(std::shared_ptr<CopyInfo> infos)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto iter = cbMap_.find(*infos);
    if (iter == cbMap_.end()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "It is not registered.");
        return nullptr;
    }
    return iter->second;
}

std::shared_ptr<ReceiveInfo> GetReceivedInfo(int wd, std::shared_ptr<CopyCallback> callback)
{
    auto it = find_if(callback->wds.begin(), callback->wds.end(), [wd](const auto& item) {
        return item.first == wd;
    });
    if (it != callback->wds.end()) {
        return it->second;
    }
    return nullptr;
}

bool CheckFileValid(const std::string &filePath, std::shared_ptr<CopyInfo> infos)
{
    return infos->filePaths.count(filePath) != 0;
}

int UpdateProgressSize(const std::string &filePath, std::shared_ptr<ReceiveInfo> receivedInfo,
    std::shared_ptr<CopyCallback> callback)
{
    auto [err, fileSize] = GetFileSize(filePath);
    if (err != ERRNO_NOERR) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetFileSize failed, err: %{public}d.", err);
        return err;
    }
    auto size = fileSize;
    auto iter = receivedInfo->fileList.find(filePath);
    if (iter == receivedInfo->fileList.end()) {
        receivedInfo->fileList.insert({ filePath, size });
        callback->progressSize += size;
    } else { // file
        if (size > iter->second) {
            callback->progressSize += (size - iter->second);
            iter->second = size;
        }
    }
    if (callback->totalSize == 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid totalSize.");
        return ENOMEM;
    }
    callback->percentage = (int32_t)(PERCENTAGE * callback->progressSize / callback->totalSize);
    return ERRNO_NOERR;
}

std::tuple<bool, int, bool> HandleProgress(
    inotify_event *event, std::shared_ptr<CopyInfo> infos, std::shared_ptr<CopyCallback> callback)
{
    if (callback == nullptr) {
        return { true, EINVAL, false };
    }
    auto receivedInfo = GetReceivedInfo(event->wd, callback);
    if (receivedInfo == nullptr) {
        return { true, EINVAL, false };
    }
    std::string fileName = receivedInfo->path;
    if (!infos->isFile) { // files under subdir
        fileName += "/" + std::string(event->name);
        if (!CheckFileValid(fileName, infos)) {
            return { true, EINVAL, false };
        }
        auto err = UpdateProgressSize(fileName, receivedInfo, callback);
        if (err != ERRNO_NOERR) {
            return { false, err, false };
        }
    } else {
        auto [err, fileSize] = GetFileSize(fileName);
        if (err != ERRNO_NOERR) {
            return { false, err, false };
        }
        callback->progressSize = fileSize;
        if (callback->totalSize == 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid totalSize.");
            return { false, ENOMEM, false };
        }
        callback->percentage = (int32_t)(PERCENTAGE * callback->progressSize / callback->totalSize);
    }
    return { true, callback->errorCode, true };
}

void ReadNotifyEvent(std::shared_ptr<CopyInfo> infos)
{
    char buf[BUF_SIZE] = { 0 };
    struct inotify_event *event = nullptr;
    int len = 0;
    int64_t index = 0;
    auto callback = GetRegisteredListener(infos);
    while (((len = read(infos->notifyFd, &buf, sizeof(buf))) <= 0)) {
        if (errno != EINTR) {
            break;
        }
    }
    while (infos->run && index < len) {
        event = reinterpret_cast<inotify_event *>(buf + index);
        auto [needContinue, errCode, needSend] = HandleProgress(event, infos, callback);
        if (!needContinue) {
            infos->exceptionCode = errCode;
            return;
        }
        if (needContinue && !needSend) {
            index += static_cast<int64_t>(sizeof(struct inotify_event) + event->len);
            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= infos->notifyTime) {
                std::shared_ptr<ProgressInfo> proInfo = std::make_shared<ProgressInfo>();
                proInfo->percentage = callback->percentage;
                progressListener_.ProgressNotify(proInfo);
                infos->notifyTime = currentTime + NOTIFY_PROGRESS_DELAY;
            }
            continue;
        }
        if (callback->progressSize == callback->totalSize) {
            infos->run = false;
            return;
        }
        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime >= infos->notifyTime) {
            std::shared_ptr<ProgressInfo> proInfo = std::make_shared<ProgressInfo>();
            proInfo->percentage = callback->percentage;
            progressListener_.ProgressNotify(proInfo);
            infos->notifyTime = currentTime + NOTIFY_PROGRESS_DELAY;
        }
        index += static_cast<int64_t>(sizeof(struct inotify_event) + event->len);
    }
}

void GetNotifyEvent(std::shared_ptr<CopyInfo> infos)
{
    auto callback = GetRegisteredListener(infos);
    if (callback == nullptr) {
        infos->exceptionCode = EINVAL;
        return;
    }
    prctl(PR_SET_NAME, "NotifyThread");
    nfds_t nfds = 2;
    struct pollfd fds[2];
    fds[0].events = 0;
    fds[1].events = POLLIN;
    fds[0].fd = infos->eventFd;
    fds[1].fd = infos->notifyFd;
    while (infos->run && infos->exceptionCode == ERRNO_NOERR && infos->eventFd != -1 && infos->notifyFd != -1) {
        auto ret = poll(fds, nfds, -1);
        if (ret > 0) {
            if (static_cast<unsigned short>(fds[0].revents) & POLLNVAL) {
                infos->run = false;
                return;
            }
            if (static_cast<unsigned short>(fds[1].revents) & POLLIN) {
                ReadNotifyEvent(infos);
            }
        } else if (ret < 0 && errno == EINTR) {
            continue;
        } else {
            infos->exceptionCode = errno;
            return;
        }
    }
}

void SetNotify(std::shared_ptr<CopyInfo> infos, std::shared_ptr<CopyCallback> callback)
{
    if (infos->hasListener && callback != nullptr) {
        callback->notifyHandler = std::thread([infos] {
            GetNotifyEvent(infos);
            });
    }
}

int32_t ExecLocal(std::shared_ptr<CopyInfo> copyInfo, std::shared_ptr<CopyCallback> callback)
{
    if (copyInfo->isFile) {
        if (copyInfo->srcPath == copyInfo->destPath) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "The src and dest is same");
            return EINVAL;
        }
        int ret = CheckOrCreatePath(copyInfo->destPath);
        if (ret != ERRNO_NOERR) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Check or create fail, errorCode is %{public}d", ret);
            return ret;
        }
    }
    if (copyInfo->hasListener) {
        return ExecCopy(copyInfo);
    }
    int32_t ret = InitLocalListener(copyInfo, callback);
    if (ret != ERRNO_NOERR) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to init local listener, errorCode is %{public}d", ret);
        return ret;
    }
    SetNotify(copyInfo, callback);
    return ExecCopy(copyInfo);
}

std::string GetRealPath(const std::string& path)
{
    fs::path tempPath(path);
    fs::path realPath{};
    for (const auto& component : tempPath) {
        if (component == ".") {
            continue;
        } else if (component == "..") {
            realPath = realPath.parent_path();
        } else {
            realPath /= component;
        }
    }
    return realPath.string();
}

static void InitCopyInfo(PasteData &pasteData, const std::string destUri, std::shared_ptr<CopyInfo> copyInfo)
{
    std::shared_ptr<OHOS::Uri> srcUri = pasteData.GetPrimaryUri();
    if (srcUri == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetPrimaryUri is nullptr!");
        return;
    }
    copyInfo->srcUri = srcUri->ToString();
    copyInfo->destUri = destUri;
    FileUri srcFileUri(copyInfo->srcUri);
    copyInfo->srcPath = srcFileUri.GetRealPath();
    FileUri destFileUri(copyInfo->destUri);
    copyInfo->destPath = destFileUri.GetPath();
    copyInfo->srcPath = GetRealPath(copyInfo->srcPath);
    copyInfo->destPath = GetRealPath(copyInfo->destPath);
    copyInfo->isFile = IsMediaUri(copyInfo->srcUri) || IsFile(copyInfo->srcPath);
    copyInfo->notifyTime = std::chrono::steady_clock::now() + NOTIFY_PROGRESS_DELAY;
}

std::shared_ptr<CopyCallback> RegisterListener(const std::shared_ptr<CopyInfo> &infos)
{
    std::shared_ptr<CopyCallback> callback = std::make_shared<CopyCallback>();
    if (callback == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to request heap memory.");
        return nullptr;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto iter = cbMap_.find(*infos);
    if (iter != cbMap_.end()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "RegisterListener, already registered.");
        return nullptr;
    }
    cbMap_.insert({ *infos, callback });
    return callback;
}

void UnregisterListener(std::shared_ptr<CopyInfo> infos)
{
    if (infos == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "infos is nullptr");
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto iter = cbMap_.find(*infos);
    if (iter == cbMap_.end()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "It is not be registered.");
        return;
    }
    cbMap_.erase(*infos);
}

void CloseNotifyFd(std::shared_ptr<CopyInfo> infos, std::shared_ptr<CopyCallback> callback)
{
    callback->CloseFd();
    infos->eventFd = -1;
    infos->notifyFd = -1;
}

void WaitNotifyFinished(std::shared_ptr<CopyCallback> callback)
{
    if (callback != nullptr) {
        if (callback->notifyHandler.joinable()) {
            callback->notifyHandler.join();
        }
    }
}

void CopyComplete(std::shared_ptr<CopyInfo> infos, std::shared_ptr<CopyCallback> callback)
{
    if (callback != nullptr && infos->hasListener) {
        callback->progressSize = callback->totalSize;
        callback->percentage = PERCENTAGE;
        std::shared_ptr<ProgressInfo> proInfo = std::make_shared<ProgressInfo>();
        proInfo->percentage = callback->percentage;
        progressListener_.ProgressNotify(proInfo);
    }
}

bool IsRemoteUri(const std::string &uri)
{
    return uri.find(NETWORK_PARA) != uri.npos;
}

int32_t CopyPasteData(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams)
{
    if (dataParams == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid dataParams");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    if (pasteData.GetPrimaryUri() == nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PasteData has no uri");
        return static_cast<int32_t>(PasteboardError::E_OK);
    }
    std::string destUri = dataParams->destUri;
    std::shared_ptr<CopyInfo> copyInfo = std::make_shared<CopyInfo>();
    if (copyInfo == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid copyInfo");
        return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
    }
    InitCopyInfo(pasteData, destUri, copyInfo);
    auto callback = RegisterListener(copyInfo);
    if (callback == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "CopyCallback registe failed");
        return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
    }
    if (pasteData.IsRemote() || IsRemoteUri(copyInfo->srcUri)) {
        int32_t ret = TransListener::CopyFileFromSoftBus(copyInfo->srcUri,
            copyInfo->destUri, copyInfo, callback, dataParams);
        UnregisterListener(copyInfo);
        return ret;
    }
    progressListener_ = dataParams->listener;
    int32_t result = ExecLocal(copyInfo, callback);
    CloseNotifyFd(copyInfo, callback);
    copyInfo->run = false;
    WaitNotifyFinished(callback);
    if (result != ERRNO_NOERR) {
        copyInfo->exceptionCode = result;
        return result;
    }
    CopyComplete(copyInfo, callback);
    UnregisterListener(copyInfo);
    return result;
}
} // namespace MiscServices
} // namespace OHOS