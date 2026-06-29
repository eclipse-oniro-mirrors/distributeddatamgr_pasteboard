/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#include "pasteboard_entity_recognizer.h"
#include "pasteboard_hilog.h"
#include "pasteboard_error.h"
#include "i_paste_data_processor.h"
#include "ffrt/ffrt_utils.h"
#include "common/pasteboard_common_utils.h"
#include <dlfcn.h>

namespace OHOS {
namespace MiscServices {

namespace {
constexpr const char* NLU_SO_PATH = "libpasteboard_nlu.z.so";
constexpr const char* SSL_SO_PATH = "libpasteboard_ssl.z.so";
constexpr const char* GET_PASTE_DATA_PROCESSOR = "GetPasteDataProcessor";
constexpr size_t MAX_RECOGNITION_LENGTH = 5000;
using GetProcessorFunc = IPasteDataProcessor&(*)();
}

PasteboardEntityRecognizer::PasteboardEntityRecognizer(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardEntityRecognizer constructed.");
}

PasteboardEntityRecognizer::~PasteboardEntityRecognizer()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardEntityRecognizer destructed.");
}

int32_t PasteboardEntityRecognizer::GetAllEntryPlainText(uint32_t dataId, uint32_t recordId,
    std::vector<std::shared_ptr<PasteDataEntry>>& entries, std::string& primaryText)
{
    for (auto& entry : entries) {
        if (primaryText.size() > MAX_RECOGNITION_LENGTH) {
            return static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION);
        }
        int32_t result = static_cast<int32_t>(PasteboardError::E_OK);
        if (entry->GetMimeType() == MIMETYPE_TEXT_PLAIN && !entry->HasContentByMimeType(MIMETYPE_TEXT_PLAIN)) {
            result = service_.GetRecordValueByType(dataId, recordId, *entry);
        }
        if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
            continue;
        }
        std::shared_ptr<std::string> plainTextPtr = entry->ConvertToPlainText();
        if (plainTextPtr != nullptr) {
            primaryText += *plainTextPtr;
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainText finished");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

std::string PasteboardEntityRecognizer::GetAllPrimaryText(const PasteData& pasteData)
{
    std::string primaryText = "";
    std::vector<std::shared_ptr<PasteDataRecord>> records = pasteData.AllRecords();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "size of records=%{public}zu", records.size());
    for (const auto& record : records) {
        if (primaryText.size() > MAX_RECOGNITION_LENGTH) {
            primaryText = "";
            break;
        }
        std::shared_ptr<std::string> plainTextPtr = record->GetPlainTextV0();
        if (plainTextPtr != nullptr) {
            primaryText += *plainTextPtr;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "primaryText in record");
            continue;
        }
        auto dataId = pasteData.GetDataId();
        auto recordId = record->GetRecordId();
        std::vector<std::shared_ptr<PasteDataEntry>> entries = record->GetEntries();
        int32_t result = GetAllEntryPlainText(dataId, recordId, entries, primaryText);
        if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "primaryText exceeded size, result=%{public}d", result);
            primaryText = "";
            break;
        }
    }
    return primaryText;
}

int32_t PasteboardEntityRecognizer::ExtractEntity(const std::string& entity, std::string& location)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!entity.empty(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "entity empty");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(nlohmann::json::accept(entity),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "entity invalid");
    nlohmann::json entityJson = nlohmann::json::parse(entity);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!entityJson.is_discarded(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "parse entity to json failed");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entityJson.contains("code") && entityJson["code"].is_number(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "entity find code failed");
    int code = entityJson["code"].get<int>();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        code == 0, static_cast<int32_t>(code), PASTEBOARD_MODULE_SERVICE, "failed to get entity");
    if (entityJson.contains("entity") && entityJson["entity"].contains("location") &&
        entityJson["entity"]["location"].is_array()) {
        nlohmann::json locationJson = entityJson["entity"]["location"].get<nlohmann::json>();
        location = locationJson.dump();
        PASTEBOARD_HILOGI(
            PASTEBOARD_MODULE_SERVICE, "location dump finished, location size=%{public}zu", location.size());
        return static_cast<int32_t>(PasteboardError::E_OK);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteData did not contain entity");
    return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
}

void PasteboardEntityRecognizer::OnRecognizePasteData(const std::string& primaryText)
{
    PasteBoardCommonUtils::SetTaskName("PasteDataRecognize");
    std::lock_guard lock(entityRecognizeMutex_);
    auto nulHandle = dlopen(NLU_SO_PATH, RTLD_NOW);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(nulHandle != nullptr, PASTEBOARD_MODULE_SERVICE, "Can not get AIEngine handle");
    std::unique_ptr<void, decltype(&dlclose)> nulGuard(nulHandle, dlclose);

    auto sslHandle = dlopen(SSL_SO_PATH, RTLD_NOW);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(sslHandle != nullptr, PASTEBOARD_MODULE_SERVICE, "Can not get SSL handle");
    std::unique_ptr<void, decltype(&dlclose)> sslGuard(sslHandle, dlclose);

    auto cleanSSL = reinterpret_cast<GetProcessorFunc>(dlsym(sslHandle, "OPENSSL_cleanup"));
    PASTEBOARD_CHECK_AND_RETURN_LOGE(cleanSSL != nullptr, PASTEBOARD_MODULE_SERVICE, "Can not get cleanSSL");

    OnRecognizePasteDataInner(primaryText, nulHandle);
    cleanSSL();
}

void PasteboardEntityRecognizer::OnRecognizePasteDataInner(const std::string& primaryText, void* nulHandle)
{
    GetProcessorFunc GetProcessor = reinterpret_cast<GetProcessorFunc>(dlsym(nulHandle, GET_PASTE_DATA_PROCESSOR));
    PASTEBOARD_CHECK_AND_RETURN_LOGE(GetProcessor != nullptr, PASTEBOARD_MODULE_SERVICE, "Can not get ProcessorFunc");

    IPasteDataProcessor& processor = GetProcessor();
    std::string entity = "";
    int32_t result = processor.Process(primaryText, entity);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(
        result == ERR_OK, PASTEBOARD_MODULE_SERVICE, "AI Process failed, result=%{public}d", result);
    
    std::string location = "";
    int32_t ret = ExtractEntity(entity, location);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "ExtractEntity failed, ret=%{public}d", ret);

    service_.NotifyEntityObservers(location, EntityType::ADDRESS, static_cast<uint32_t>(primaryText.size()));
}

void PasteboardEntityRecognizer::RecognizePasteData(PasteData& pasteData)
{
    if (pasteData.GetShareOption() == ShareOption::InApp) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "shareOption is InApp, recognition not allowed");
        return;
    }
    std::string primaryText = GetAllPrimaryText(pasteData);
    if (primaryText.empty()) {
        return;
    }
    FFRTTask task = [this, primaryText]() {
        std::thread thread([=]() {
            PASTEBOARD_CHECK_AND_RETURN_LOGE(PasteboardService::state_ == ServiceRunningState::STATE_RUNNING,
                PASTEBOARD_MODULE_SERVICE, "PasteboardService is not running.");
            OnRecognizePasteData(primaryText);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "RecognizePaste");
        thread.detach();
    };
    FFRTUtils::SubmitTask(task);
}

} // namespace MiscServices
} // namespace OHOS