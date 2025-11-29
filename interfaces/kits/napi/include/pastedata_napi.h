/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef N_NAPI_PASTEDATA_H
#define N_NAPI_PASTEDATA_H

#include "pasteboard_client.h"
#include "pixel_map_napi.h"

namespace OHOS {
namespace MiscServicesNapi {
class PasteDataNapi {
public:
    PasteDataNapi();
    ~PasteDataNapi();
    static napi_value PasteDataInit(napi_env env, napi_value exports);
    static napi_value New(napi_env env, napi_callback_info info);
    static napi_status NewInstance(napi_env env, napi_value &instance);
    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static napi_value GetSystemPasteboard(napi_env env, napi_callback_info info);
    static bool IsPasteData(napi_env env, napi_value in);
    static napi_value CreateInstance(napi_env env, const std::shared_ptr<MiscServices::PasteData> pasteData);
    std::shared_ptr<MiscServices::PasteData> value_ = nullptr;

private:
    static napi_value AddHtmlRecord(napi_env env, napi_callback_info info);
    static napi_value AddPixelMapRecord(napi_env env, napi_callback_info info);
    static napi_value AddTextRecord(napi_env env, napi_callback_info info);
    static napi_value AddUriRecord(napi_env env, napi_callback_info info);
    static void AddRecord(napi_env env, napi_value *argv, size_t argc, PasteDataNapi *obj);
    static void AddRecord(napi_env env, napi_value argv, PasteDataNapi *obj);
    static napi_value GetPrimaryHtml(napi_env env, napi_callback_info info);
    static napi_value GetPrimaryPixelMap(napi_env env, napi_callback_info info);
    static napi_value GetPrimaryText(napi_env env, napi_callback_info info);
    static napi_value GetPrimaryUri(napi_env env, napi_callback_info info);
    static napi_value HasMimeType(napi_env env, napi_callback_info info);
    static napi_value HasType(napi_env env, napi_callback_info info);
    static napi_value RemoveRecordAt(napi_env env, napi_callback_info info);
    static napi_value RemoveRecord(napi_env env, napi_callback_info info);
    static napi_value GetPrimaryMimeType(napi_env env, napi_callback_info info);
    static napi_value GetRecordCount(napi_env env, napi_callback_info info);
    static napi_value GetTag(napi_env env, napi_callback_info info);
    static napi_value GetMimeTypes(napi_env env, napi_callback_info info);
    static napi_value AddRecord(napi_env env, napi_callback_info info);
    static napi_value ReplaceRecordAt(napi_env env, napi_callback_info info);
    static napi_value ReplaceRecord(napi_env env, napi_callback_info info);
    static napi_value AddWantRecord(napi_env env, napi_callback_info info);
    static napi_value GetPrimaryWant(napi_env env, napi_callback_info info);
    static napi_value GetProperty(napi_env env, napi_callback_info info);
    static napi_value GetRecordAt(napi_env env, napi_callback_info info);
    static napi_value GetRecord(napi_env env, napi_callback_info info);
    static PasteDataNapi *RemoveAndGetRecordCommon(napi_env env, napi_callback_info info, uint32_t &index);
    static std::shared_ptr<MiscServices::PasteDataRecord> ParseRecord(napi_env env, napi_value &recordNapi);
    static bool SetStringProp(napi_env env, const std::string &propName, napi_value &propValueNapi,
        MiscServices::PasteDataRecord::Builder &builder);
    static napi_value SetProperty(napi_env env, napi_callback_info info);
    static void SetProperty(napi_env env, napi_value in, PasteDataNapi *obj);
    static bool IsProperty(napi_env env, napi_value in);
    static bool SetNapiProperty(napi_env env, const MiscServices::PasteDataProperty &property, napi_value &nProperty);
    napi_env env_;
    static napi_value PasteStart(napi_env env, napi_callback_info info);
    static napi_value PasteComplete(napi_env env, napi_callback_info info);
    static bool ParseProperty(napi_env env, const std::string& propName, napi_value propValueNapi,
        MiscServices::PasteDataRecord::Builder& builder);
};

extern "C" {
    API_EXPORT napi_value GetEtsPasteData(napi_env env, const std::shared_ptr<MiscServices::PasteData> pasteData);
}
} // namespace MiscServicesNapi
} // namespace OHOS
#endif
