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

#include "pasteboard_copy.h"

#include "common_func.h"
#include "copy/file_copy_manager.h"
#include "file_uri.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
using namespace AppFileService::ModuleFileUri;
const std::string NETWORK_PARA = "?networkid=";
constexpr int PERCENTAGE = 100;
constexpr int ERRNO_NOERR = 0;
constexpr int E_EXIST = 17;
constexpr float FILE_PERCENTAGE = 0.8;
constexpr int BEGIN_PERCENTAGE = 20;
constexpr int DFS_CANCEL_SUCCESS = 204;

ProgressListener PasteBoardCopyFile::progressListener_;
std::atomic_bool PasteBoardCopyFile::canCancel_{ true };
std::atomic_uint32_t PasteBoardCopyFile::recordSize_{ 0 };

PasteBoardCopyFile &PasteBoardCopyFile::GetInstance()
{
    static PasteBoardCopyFile instance;
    return instance;
}

bool PasteBoardCopyFile::IsDirectory(const std::string &path)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Stat failed. errno=%{public}d", errno);
        return false;
    }
    return (buf.st_mode & S_IFMT) == S_IFDIR;
}

bool PasteBoardCopyFile::IsFile(const std::string &path)
{
    struct stat buf {};
    int ret = stat(path.c_str(), &buf);
    if (ret == -1) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Stat failed. errno = %{public}d", errno);
        return false;
    }
    return (buf.st_mode & S_IFMT) == S_IFREG;
}

void PasteBoardCopyFile::OnProgressNotify(std::shared_ptr<GetDataParams> params)
{
    if (params == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "params is null!");
        return;
    }

    if (params->info == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "params->info is null!");
        return;
    }

    if (params->info->percentage > PERCENTAGE) {
        params->info->percentage = PERCENTAGE;
    }
    params->info->percentage = static_cast<int32_t>(params->info->percentage * FILE_PERCENTAGE + BEGIN_PERCENTAGE);
    params->info->percentage = std::abs(params->info->percentage);
    params->info->percentage = std::max(params->info->percentage, 0);
    if (progressListener_.ProgressNotify != nullptr) {
        progressListener_.ProgressNotify(params);
    } else {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "ProgressNotify is nullptr.");
    }
}

std::string PasteBoardCopyFile::GetRealPath(const std::string& path)
{
    std::filesystem::path tempPath(path);
    std::filesystem::path realPath{};
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

bool PasteBoardCopyFile::IsRemoteUri(const std::string &uri)
{
    return uri.find(NETWORK_PARA) != uri.npos;
}

int32_t PasteBoardCopyFile::CheckCopyParam(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams)
{
    if (dataParams == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid dataParams");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    uint32_t recordSize = static_cast<uint32_t>(pasteData.GetRecordCount());
    if (recordSize == 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid records size");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    recordSize_.store(recordSize);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteBoardCopyFile::InitCopyInfo(const std::string srcUri, std::shared_ptr<GetDataParams> dataParams,
    std::shared_ptr<CopyInfo> copyInfo)
{
    copyInfo->srcUri = srcUri;
    copyInfo->destUri = dataParams->destUri;
    FileUri srcFileUri(copyInfo->srcUri);
    copyInfo->srcPath = srcFileUri.GetRealPath();
    FileUri destFileUri(copyInfo->destUri);
    copyInfo->destPath = destFileUri.GetPath();
    copyInfo->srcPath = GetRealPath(copyInfo->srcPath);
    copyInfo->destPath = GetRealPath(copyInfo->destPath);
    std::string realSrc = copyInfo->srcPath;
    if (IsRemoteUri(copyInfo->srcUri)) {
        uint32_t index = copyInfo->srcPath.rfind("?", 0);
        realSrc = copyInfo->srcPath.substr(0, index);
    }
    if (IsDirectory(copyInfo->destPath)) {
        std::filesystem::path filePath(realSrc);
        auto fileName = filePath.filename();
        if (copyInfo->destPath.back() != '/') {
            copyInfo->destPath += '/';
        }
        copyInfo->destPath += fileName.string();
        copyInfo->destUri = AppFileService::CommonFunc::GetUriFromPath(copyInfo->destPath);
    }
    std::error_code errCode;
    if (dataParams->fileConflictOption == FILE_SKIP &&
        std::filesystem::exists(copyInfo->destPath, errCode) && errCode.value() == ERRNO_NOERR) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "File existed, dstPath=%{private}s", copyInfo->destPath.c_str());
        return E_EXIST;
    }
    FileUri realFileUri(realSrc);
    std::string realPath = realFileUri.GetRealPath();
    realPath = GetRealPath(realPath);
    if (!IsFile(realPath)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Softbus not support dir to remote.");
        return ENOMEM;
    }
    return ERRNO_NOERR;
}

int32_t PasteBoardCopyFile::CopyFileData(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams)
{
    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    progressListener_ = dataParams->listener;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    int32_t recordCount = 0;
    int32_t recordProcessedIndex = 0;
    for (size_t index = 0; index < pasteData.GetRecordCount();) {
        recordProcessedIndex++;
        if (ProgressSignalClient::GetInstance().CheckCancelIfNeed()) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "canceled success!");
            pasteData.RemoveRecordAt(index);
            continue;
        }
        record = pasteData.GetRecordAt(index);
        if (record == nullptr) {
            return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
        }
        std::shared_ptr<OHOS::Uri> uri = record->GetUriV0();
        if (uri == nullptr) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Record has no uri");
            index++;
            continue;
        }
        std::shared_ptr<CopyInfo> copyInfo = std::make_shared<CopyInfo>();
        std::string srcUri = uri->ToString();
        if (InitCopyInfo(srcUri, dataParams, copyInfo) == E_EXIST) {
            recordCount++;
            pasteData.RemoveRecordAt(index);
            continue;
        }
        recordCount++;
        CopyInfo info = *copyInfo;
        using ProcessCallBack = std::function<void(uint64_t processSize, uint64_t totalSize)>;
        ProcessCallBack listener = [=](uint64_t processSize, uint64_t totalSize) {
            uint32_t percentage = 0;
            if (totalSize != 0) {
                percentage = static_cast<uint32_t>(static_cast<uint32_t>(PERCENTAGE) * processSize / totalSize);
            }
            HandleProgress(recordProcessedIndex, info, percentage, dataParams);
        };
        ret = Storage::DistributedFile::FileCopyManager::GetInstance().Copy(srcUri, copyInfo->destUri, listener);
        if (!ShouldKeepRecord(ret, copyInfo->destUri, record)) {
            pasteData.RemoveRecordAt(index);
        } else {
            index++;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DFS copy ret: %{public}d", ret);
    }
    return (ret == ERRNO_NOERR || ret == DFS_CANCEL_SUCCESS) ? static_cast<int32_t>(PasteboardError::E_OK) : ret;
}

bool PasteBoardCopyFile::ShouldKeepRecord(
    int32_t &ret, const std::string &destUri, std::shared_ptr<PasteDataRecord> record)
{
    if (record == nullptr) {
        return false;
    }
    if ((ret == static_cast<int32_t>(PasteboardError::E_OK) || ret == ERRNO_NOERR) &&
        !ProgressSignalClient::GetInstance().CheckCancelIfNeed()) {
        auto sharedUri = std::make_shared<OHOS::Uri>(destUri);
        record->SetUri(sharedUri);
        record->SetConvertUri("");
        return true;
    } else if (record->GetFrom() > 0 && record->GetRecordId() != record->GetFrom() &&
        !ProgressSignalClient::GetInstance().CheckCancelIfNeed()) {
        ret = ERRNO_NOERR;
        return true;
    } else {
        return false;
    }
}

void PasteBoardCopyFile::HandleProgress(int32_t index, const CopyInfo &info, uint32_t percentage,
    std::shared_ptr<GetDataParams> dataParams)
{
    if (dataParams == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "dataParams is nullptr.");
        return;
    }

    if (dataParams->info == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "dataParams->info is nullptr.");
        return;
    }

    if (index < 1) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "invalid parameter");
        return;
    }
    int32_t recordSize = static_cast<int32_t>(recordSize_.load());
    if (recordSize <= 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "no record");
        return;
    }

    if (ProgressSignalClient::GetInstance().CheckCancelIfNeed() && canCancel_.load()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Cancel copy.");
        std::thread([info]() {
            canCancel_.store(false);
            auto ret = Storage::DistributedFile::FileCopyManager::GetInstance().Cancel(info.srcUri, info.destUri);
            if (ret != ERRNO_NOERR) {
                canCancel_.store(true);
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Cancel failed. errno=%{public}d", ret);
            }
        }).detach();
        return;
    }

    int32_t totalProgress = ((index - 1) * PERCENTAGE + static_cast<int32_t>(percentage)) / recordSize;
    dataParams->info->percentage = totalProgress;

    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "process record index:%{public}d/%{public}d, progress=%{public}d",
        index, recordSize, totalProgress);
    OnProgressNotify(dataParams);
}

int32_t PasteBoardCopyFile::CopyPasteData(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams)
{
    canCancel_.store(true);
    int32_t ret = CheckCopyParam(pasteData, dataParams);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid copy params");
        return ret;
    }
    ret = CopyFileData(pasteData, dataParams);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "copy file failed, ret=%{public}d", ret);
        ret = static_cast<int32_t>(PasteboardError::COPY_FILE_ERROR);
    }
    dataParams->info->percentage = PERCENTAGE;
    OnProgressNotify(dataParams);
    recordSize_.store(0);
    return ret;
}
} // namespace MiscServices
} // namespace OHOS