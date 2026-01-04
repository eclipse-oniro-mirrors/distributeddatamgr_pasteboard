/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PASTE_BOARD_COPY_H
#define PASTE_BOARD_COPY_H

#include "pasteboard_client.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::AppExecFwk;
struct CopyInfo {
    std::string srcUri;
    std::string destUri;
    std::string srcPath;
    std::string destPath;
    bool operator==(const CopyInfo &infos) const
    {
        return (srcUri == infos.srcUri && destUri == infos.destUri);
    }
    bool operator<(const CopyInfo &infos) const
    {
        if (srcUri == infos.srcUri) {
            return destUri < infos.destUri;
        }
        return srcUri < infos.srcUri;
    }
};

class PasteBoardCopyFile {
public:
    static PasteBoardCopyFile &GetInstance();
    int32_t CopyPasteData(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams);
private:
    PasteBoardCopyFile() = default;
    ~PasteBoardCopyFile() = default;
    DISALLOW_COPY_AND_MOVE(PasteBoardCopyFile);
    static bool IsDirectory(const std::string &path);
    static bool IsFile(const std::string &path);
    static std::string GetRealPath(const std::string& path);
    static bool IsRemoteUri(const std::string &uri);
    static int32_t CheckCopyParam(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams);
    static int32_t InitCopyInfo(const std::string srcUri, std::shared_ptr<GetDataParams> dataParams,
        std::shared_ptr<CopyInfo> copyInfo);
    static void OnProgressNotify(std::shared_ptr<GetDataParams> params);
    static int32_t CopyFileData(PasteData &pasteData, std::shared_ptr<GetDataParams> dataParams);

    static void HandleProgress(int32_t index, const CopyInfo &info, uint32_t percentage,
        std::shared_ptr<GetDataParams> dataParams);
    static ProgressListener progressListener_;
    static std::atomic_bool canCancel_;
    static std::atomic_uint32_t recordSize_;
    static bool ShouldKeepRecord(int32_t &ret, const std::string &destUri, std::shared_ptr<PasteDataRecord> record);
};
} // namespace MiscServices
} // namespace OHOS

#endif // PASTE_BOARD_COPY_H