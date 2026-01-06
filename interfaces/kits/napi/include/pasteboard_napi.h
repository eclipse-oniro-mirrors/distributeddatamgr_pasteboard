/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTEDDATAMGR_PASTEBOARD_PASTEBOARD_NAPI_H
#define DISTRIBUTEDDATAMGR_PASTEBOARD_PASTEBOARD_NAPI_H

#include "pastedata_napi.h"
namespace OHOS {
namespace MiscServicesNapi {

#define PASTEBOARD_GET_AND_THROW_LAST_ERROR(env)                                                        \
    do {                                                                                                \
        const napi_extended_error_info* errorInfo = nullptr;                                            \
        napi_get_last_error_info((env), &errorInfo);                                                    \
        bool isPending = false;                                                                         \
        napi_is_exception_pending((env), &isPending);                                                   \
        if (!isPending && errorInfo != nullptr) {                                                       \
            const char* errMessage =                                                                    \
                errorInfo->error_message != nullptr ? errorInfo->error_message : "empty error message"; \
                napi_value errorObj = nullptr;                                                          \
                napi_value errorCode = nullptr;                                                         \
                napi_value errorMessage = nullptr;                                                      \
                napi_create_string_utf8(env, errMessage, NAPI_AUTO_LENGTH, &errorMessage);              \
                napi_create_error(env, errorCode, errorMessage, &errorObj);                             \
                napi_throw(env, errorObj);                                                              \
        }                                                                                               \
    } while (0)

#define PASTEBOARD_ASSERT_CALL(env, theCall, object)    \
    do {                                     \
        if ((theCall) != napi_ok) {          \
            delete (object);                 \
            PASTEBOARD_GET_AND_THROW_LAST_ERROR((env)); \
            return nullptr;                  \
        }                                    \
    } while (0)

class PasteboardNapi {
public:
    static napi_value PasteBoardInit(napi_env env, napi_value exports);

private:
    using FUNC = napi_value (*)(napi_env, napi_value);
    static std::unordered_map<std::string, FUNC> createRecordMap_;
    static std::unordered_map<std::string, FUNC> createDataMap_;

    static napi_value CreateHtmlRecord(napi_env env, napi_value in);
    static napi_value CreatePlainTextRecord(napi_env env, napi_value in);
    static napi_value CreateUriRecord(napi_env env, napi_value in);
    static napi_value CreatePixelMapRecord(napi_env env, napi_value in);
    static napi_value CreateWantRecord(napi_env env, napi_value in);

    static PasteDataNapi *CreateDataCommon(napi_env env, napi_value in, std::string &str, napi_value &instance);
    static napi_value CreateHtmlData(napi_env env, napi_value in);
    static napi_value CreatePlainTextData(napi_env env, napi_value in);
    static napi_value CreateUriData(napi_env env, napi_value in);
    static napi_value CreatePixelMapData(napi_env env, napi_value in);
    static napi_value CreateWantData(napi_env env, napi_value in);
    static napi_value CreateMultiTypeData(napi_env env,
        std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<MiscServices::EntryValue>>>> typeValueMap);
    static napi_value CreateMultiTypeDelayData(napi_env env, std::vector<std::string> mimeTypes,
        std::shared_ptr<UDMF::EntryGetter> entryGetter);

    static napi_value JScreateHtmlTextRecord(napi_env env, napi_callback_info info);
    static napi_value JScreateWantRecord(napi_env env, napi_callback_info info);
    static napi_value JScreateShareOption(napi_env env, napi_callback_info info);
    static napi_value JScreatePattern(napi_env env, napi_callback_info info);
    static napi_value JScreateFileConflictOptions(napi_env env, napi_callback_info info);
    static napi_value JScreateProgressIndicator(napi_env env, napi_callback_info info);
    static napi_value JScreatePlainTextRecord(napi_env env, napi_callback_info info);
    static napi_value JScreatePixelMapRecord(napi_env env, napi_callback_info info);
    static napi_value JScreateUriRecord(napi_env env, napi_callback_info info);
    static napi_value JSCreateRecord(napi_env env, napi_callback_info info);

    static napi_value JScreateHtmlData(napi_env env, napi_callback_info info);
    static napi_value JScreateWantData(napi_env env, napi_callback_info info);
    static napi_value JScreatePlainTextData(napi_env env, napi_callback_info info);
    static napi_value JScreatePixelMapData(napi_env env, napi_callback_info info);
    static napi_value JScreateUriData(napi_env env, napi_callback_info info);
    static napi_value JSCreateKvData(
        napi_env env, const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer);
    static napi_value JSCreateData(napi_env env, napi_callback_info info);
    static napi_value JSgetSystemPasteboard(napi_env env, napi_callback_info info);
};
} // namespace MiscServicesNapi
} // namespace OHOS

#endif // DISTRIBUTEDDATAMGR_PASTEBOARD_PASTEBOARD_NAPI_H
