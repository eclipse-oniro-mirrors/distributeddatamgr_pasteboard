/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_PASTEBOARD_JS_ERR_H
#define DISTRIBUTEDDATAMGR_PASTEBOARD_JS_ERR_H

namespace OHOS::MiscServices {
enum class JSErrorCode : int32_t {
    SUCCESS = 0,
    NO_PERMISSION = 201,
    NO_SYSTEM_PERMISSION = 202,
    INVALID_PARAMETERS = 401,
    DEVICE_NOT_SUPPORT = 801,
    OUT_OF_RANGE = 12900001,
    RECORD_EXCEEDS_LIMIT,
    OTHER_COPY_OR_PASTE_IN_PROCESSING,
    COPY_FORBIDDEN,
    REQUEST_TIME_OUT,
    SETTINGS_ALREADY_EXIST,
    ERR_COPY_FILE_ERROR,
    ERR_PROGRESS_START_ERROR,
    ERR_PROGRESS_ABNORMAL,
    ERR_GET_DATA_FAILED
};
} // namespace OHOS::MiscServices

#endif // DISTRIBUTEDDATAMGR_PASTEBOARD_JS_ERR_H
