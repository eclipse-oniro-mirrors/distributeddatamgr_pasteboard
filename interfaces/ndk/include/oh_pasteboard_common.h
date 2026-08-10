/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#ifndef OH_PASTEBOARD_COMMON_H
#define OH_PASTEBOARD_COMMON_H

#include "oh_pasteboard.h"
#include "oh_pasteboard_err_code.h"
#include "pasteboard_error.h"
#include "pasteboard_observer.h"

namespace OHOS {
namespace MiscServices {
static constexpr uint32_t MAX_MIMETYPES_NUM = 10000;

const std::unordered_map<PasteboardError, PASTEBOARD_ErrCode> errCodeMap = {
    { PasteboardError::E_OK, PASTEBOARD_ErrCode::ERR_OK },
    { PasteboardError::PERMISSION_VERIFICATION_ERROR, ERR_PERMISSION_ERROR },
    { PasteboardError::INVALID_PARAM_ERROR, ERR_INVALID_PARAMETER },
    { PasteboardError::TASK_PROCESSING, ERR_BUSY },
    { PasteboardError::COPY_FILE_ERROR, ERR_PASTEBOARD_COPY_FILE_ERROR },
    { PasteboardError::PROGRESS_START_ERROR, ERR_PASTEBOARD_PROGRESS_START_ERROR },
    { PasteboardError::PROGRESS_ABNORMAL, ERR_PASTEBOARD_PROGRESS_ABNORMAL },
};

class PasteboardObserverCapiImpl;
} // namespace MiscServices
} // namespace OHOS

enum PasteboardNdkStructId : std::int64_t {
    SUBSCRIBER_STRUCT_ID = 1002950,
    PASTEBOARD_STRUCT_ID,
};

struct OH_Pasteboard {
    const int64_t cid = PASTEBOARD_STRUCT_ID;
    std::mutex mutex;
    std::map<const OH_PasteboardObserver *, OHOS::sptr<OHOS::MiscServices::PasteboardObserverCapiImpl>> observers_;
    std::vector<std::string> mimeTypes_;
    char **mimeTypesPtr = nullptr;
};

struct Pasteboard_ProgressInfo {
    int progress;
};

struct Pasteboard_GetDataParams {
    char *destUri;
    uint32_t destUriLen;
    Pasteboard_FileConflictOptions fileConflictOptions;
    Pasteboard_ProgressIndicator progressIndicator;
    OH_Pasteboard_ProgressListener progressListener;
    Pasteboard_ProgressInfo info;
};

/** @} */
#endif