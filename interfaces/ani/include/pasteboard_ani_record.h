/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_ANI_RECORD_H
#define PASTEBOARD_ANI_RECORD_H

#include <ani.h>
#include <memory>
#include <string>
#include <vector>

#include "pasteboard_client.h"

namespace OHOS {
namespace MiscServices {

constexpr int32_t ANI_MAX_RECORD_NUM = 512;
constexpr int32_t ANI_MIMETYPE_MAX_LEN = 1024;
constexpr int32_t ANI_MAX_TEXT_LENGTH = 1048576;
constexpr int32_t ANI_MAX_HTML_LENGTH = 2097152;

class PasteboardAniRecordUtils {
public:
    static ani_object CreateRecordObject(ani_env *env);
    static bool FillRecordObject(ani_env *env, std::shared_ptr<PasteDataRecord> record, ani_object &obj);
    static bool FillRecordHtmlText(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);
    static bool FillRecordPlainText(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);
    static bool FillRecordUri(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);
    static bool FillRecordMimeType(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);
    static bool FillRecordWant(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);
    static bool FillRecordPixelMap(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);
    static bool FillRecordData(ani_env *env, ani_class cls, ani_object &obj,
        std::shared_ptr<PasteDataRecord> record);

    static std::shared_ptr<PasteDataRecord> ParseRecordFromAni(ani_env *env, ani_object recordObj);
    static std::string GetMimeTypeFromRecord(ani_env *env, ani_object recordObj);
    static std::string GetPlainTextFromRecord(ani_env *env, ani_object recordObj);
    static std::string GetUriFromRecord(ani_env *env, ani_object recordObj);
    static std::string GetHtmlTextFromRecord(ani_env *env, ani_object recordObj);
    static std::vector<uint8_t> GetDataFromRecord(ani_env *env, ani_object recordObj);

    static bool ValidateMimeType(ani_env *env, const std::string &mimeType);
    static bool ValidateRecordCount(ani_env *env, int32_t count);
    static bool ValidateTextLength(ani_env *env, const std::string &text);
    static bool ValidateHtmlLength(ani_env *env, const std::string &html);
};

class PasteboardAniDataAdapter {
public:
    static ani_object CreatePasteDataFromRecord(ani_env *env, ani_object recordObj);
    static ani_object CreatePasteDataFromMimeTypeValue(ani_env *env, ani_string type, ani_object value);
    static ani_object CreateRecordFromMimeTypeValue(ani_env *env, ani_string type, ani_object value);

    static ani_object GetPrimaryHtml(ani_env *env, ani_object pasteDataObj);
    static ani_object GetPrimaryText(ani_env *env, ani_object pasteDataObj);
    static ani_object GetPrimaryUri(ani_env *env, ani_object pasteDataObj);
    static ani_object GetPrimaryMimeType(ani_env *env, ani_object pasteDataObj);
    static ani_object GetPrimaryWant(ani_env *env, ani_object pasteDataObj);
    static ani_object GetPrimaryPixelMap(ani_env *env, ani_object pasteDataObj);

    static ani_array GetMimeTypes(ani_env *env, ani_object pasteDataObj);
    static ani_string GetTag(ani_env *env, ani_object pasteDataObj);
    static ani_boolean HasType(ani_env *env, ani_object pasteDataObj, ani_string type);
    static void RemoveRecord(ani_env *env, ani_object pasteDataObj, ani_int index);
    static void ReplaceRecord(ani_env *env, ani_object pasteDataObj, ani_int index, ani_object recordObj);
    static void PasteStart(ani_env *env, ani_object pasteDataObj);
    static void PasteComplete(ani_env *env, ani_object pasteDataObj);
};

class PasteboardAniSystemAdapter {
public:
    static ani_boolean HasData(ani_env *env);
    static ani_boolean HasDataSync(ani_env *env);
    static ani_boolean HasRemoteData(ani_env *env);
    static ani_boolean IsRemoteData(ani_env *env);
    static void ClearDataSync(ani_env *env);
    static void SetDataSync(ani_env *env, ani_object pasteDataObj);
    static ani_long GetChangeCount(ani_env *env);
    static ani_array GetMimeTypes(ani_env *env);
    static ani_array DetectPatterns(ani_env *env, ani_array patterns);
    static void SetAppShareOptions(ani_env *env, ani_int shareOption);
    static void RemoveAppShareOptions(ani_env *env);
    static ani_object GetDataWithProgress(ani_env *env, ani_object params);
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_ANI_RECORD_H
