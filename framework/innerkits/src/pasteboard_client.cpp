/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include <chrono>
#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <iservice_registry.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>

#include "convert_utils.h"
#include "file_uri.h"
#include "hitrace_meter.h"
#include "hiview_adapter.h"
#include "in_process_call_wrapper.h"
#include "ipasteboard_client_death_observer.h"
#include "pasteboard_client.h"
#include "pasteboard_deduplicate_memory.h"
#include "pasteboard_delay_getter_client.h"
#include "pasteboard_entry_getter_client.h"
#include "pasteboard_error.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_load_callback.h"
#include "pasteboard_observer.h"
#include "pasteboard_progress.h"
#include "pasteboard_utils.h"
#include "pasteboard_web_controller.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "udmf_client.h"
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServices {
constexpr const int32_t HITRACE_GETPASTEDATA = 0;
std::string SIGNAL_KEY_DEFAULT = "0";
std::string PROGRESS_START_PERCENT = "0";
std::string progressKey_;
constexpr int32_t LOADSA_TIMEOUT_MS = 4000;
constexpr int32_t PASTEBOARD_PROGRESS_UPDATE_PERCENT = 5;
constexpr int32_t PASTEBOARD_PROGRESS_TWENTY_PERCENT = 20;
constexpr int32_t PASTEBOARD_PROGRESS_END_PERCENT = 100;
constexpr int32_t PASTEBOARD_PROGRESS_SLEEP_TIME = 100; // ms
constexpr int64_t REPORT_DUPLICATE_TIMEOUT = 2 * 60 * 1000; // 2 minutes
static constexpr int32_t HAP_PULL_UP_TIME = 500; // ms
sptr<IPasteboardService> PasteboardClient::pasteboardServiceProxy_;
PasteboardClient::StaticDestoryMonitor PasteboardClient::staticDestoryMonitor_;
std::mutex PasteboardClient::instanceLock_;
std::condition_variable PasteboardClient::proxyConVar_;
sptr<IRemoteObject> clientDeathObserverPtr_;

struct RadarReportIdentity {
    pid_t pid;
    int32_t errorCode;
};

bool operator==(const RadarReportIdentity &lhs, const RadarReportIdentity &rhs)
{
    return lhs.pid == rhs.pid && lhs.errorCode == rhs.errorCode;
}

PasteboardClient::PasteboardClient()
{
    Init();
};
PasteboardClient::~PasteboardClient()
{
    if (!staticDestoryMonitor_.IsDestoryed()) {
        auto pasteboardServiceProxy = GetPasteboardServiceProxy();
        if (pasteboardServiceProxy != nullptr) {
            auto remoteObject = pasteboardServiceProxy->AsObject();
            if (remoteObject != nullptr) {
                remoteObject->RemoveDeathRecipient(deathRecipient_);
            }
        }
    }
}

void PasteboardClient::Init()
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    if (clientDeathObserverPtr_ == nullptr) {
        clientDeathObserverPtr_ = new (std::nothrow) PasteboardClientDeathObserverStub();
    }
    if (clientDeathObserverPtr_ == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "create ClientDeathObserver failed.");
        return;
    }
    auto ret = proxyService->RegisterClientDeathObserver(clientDeathObserverPtr_);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "failed. ret is %{public}d", ret);
    }
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreateHtmlTextRecord(const std::string &htmlText)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New text record");
    return PasteDataRecord::NewHtmlRecord(htmlText);
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreateWantRecord(std::shared_ptr<OHOS::AAFwk::Want> want)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New want record");
    return PasteDataRecord::NewWantRecord(std::move(want));
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreatePlainTextRecord(const std::string &text)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New text record");
    return PasteDataRecord::NewPlainTextRecord(text);
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreatePixelMapRecord(std::shared_ptr<PixelMap> pixelMap)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New pixelMap record");
    return PasteDataRecord::NewPixelMapRecord(std::move(pixelMap));
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreateUriRecord(const OHOS::Uri &uri)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New uri record");
    return PasteDataRecord::NewUriRecord(uri);
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreateKvRecord(
    const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New kv record");
    return PasteDataRecord::NewKvRecord(mimeType, arrayBuffer);
}

std::shared_ptr<PasteDataRecord> PasteboardClient::CreateMultiDelayRecord(
    std::vector<std::string> mimeTypes, const std::shared_ptr<UDMF::EntryGetter> entryGetter)
{
    return PasteDataRecord::NewMultiTypeDelayRecord(mimeTypes, entryGetter);
}

std::shared_ptr<PasteData> PasteboardClient::CreateHtmlData(const std::string &htmlText)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New htmlText data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddHtmlRecord(htmlText);
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreateWantData(std::shared_ptr<OHOS::AAFwk::Want> want)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New want data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddWantRecord(std::move(want));
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreatePlainTextData(const std::string &text)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New plain data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddTextRecord(text);
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreatePixelMapData(std::shared_ptr<PixelMap> pixelMap)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New pixelMap data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddPixelMapRecord(std::move(pixelMap));
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreateUriData(const OHOS::Uri &uri)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New uri data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddUriRecord(uri);
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreateKvData(
    const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New Kv data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddKvRecord(mimeType, arrayBuffer);
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreateMultiTypeData(
    std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>> typeValueMap, const std::string &recordMimeType)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New multiType data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddRecord(PasteDataRecord::NewMultiTypeRecord(std::move(typeValueMap), recordMimeType));
    return pasteData;
}

std::shared_ptr<PasteData> PasteboardClient::CreateMultiTypeDelayData(std::vector<std::string> mimeTypes,
    std::shared_ptr<UDMF::EntryGetter> entryGetter)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "New multiTypeDelay data");
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddRecord(PasteDataRecord::NewMultiTypeDelayRecord(mimeTypes, entryGetter));
    return pasteData;
}

int32_t PasteboardClient::GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry &value)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return proxyService->GetRecordValueByType(dataId, recordId, value);
}
void PasteboardClient::Clear()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Clear start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    proxyService->Clear();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Clear end.");
    return;
}

int32_t PasteboardClient::GetPasteData(PasteData &pasteData)
{
    static DeduplicateMemory<RadarReportIdentity> reportMemory(REPORT_DUPLICATE_TIMEOUT);
    pid_t pid = getpid();
    std::string currentPid = std::to_string(pid);
    uint32_t tmpSequenceId = getSequenceId_++;
    std::string currentId = "GetPasteData_" + currentPid + "_" + std::to_string(tmpSequenceId);
    pasteData.SetPasteId(currentId);
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_GET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN, RadarReporter::CONCURRENT_ID, currentId,
        RadarReporter::PACKAGE_NAME, currentPid);
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetPasteData", HITRACE_GETPASTEDATA);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetPasteData start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_CHECK_GET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            RadarReporter::PACKAGE_NAME, currentPid, RadarReporter::ERROR_CODE,
            static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    int32_t syncTime = 0;
    int32_t ret = proxyService->GetPasteData(pasteData, syncTime);
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    RetainUri(pasteData);
    RebuildWebviewPasteData(pasteData);
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetPasteData", HITRACE_GETPASTEDATA);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetPasteData end.");
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        if (pasteData.deviceId_.empty()) {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
                RadarReporter::DIS_SYNC_TIME, syncTime, RadarReporter::PACKAGE_NAME, currentPid);
        } else {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::CONCURRENT_ID, currentId, RadarReporter::DIS_SYNC_TIME, syncTime,
                RadarReporter::PACKAGE_NAME, currentPid);
        }
    } else if (ret != static_cast<int32_t>(PasteboardError::TASK_PROCESSING) &&
               !reportMemory.IsDuplicate({.pid = pid, .errorCode = ret})) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_FAILED, RadarReporter::BIZ_STATE,
            RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId, RadarReporter::DIS_SYNC_TIME,
            syncTime, RadarReporter::PACKAGE_NAME, currentPid, RadarReporter::ERROR_CODE, ret);
    } else {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_CANCELLED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            RadarReporter::DIS_SYNC_TIME, syncTime, RadarReporter::PACKAGE_NAME, currentPid,
            RadarReporter::ERROR_CODE, ret);
    }
    return ret;
}

void PasteboardClient::CopyFile(std::shared_ptr<GetDataParams> params)
{
    #define PROGRESS_PERCENTAGE 80
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "CopyFile in.");
    std::shared_ptr<ProgressInfo> info = std::make_shared<ProgressInfo>();

    info->percentage = PROGRESS_PERCENTAGE;
    if (params->listener.ProgressNotify != nullptr) {
        params->listener.ProgressNotify(info);
    }
}

void PasteboardClient::GetFileProgressCb(std::shared_ptr<ProgressInfo> progressInfo)
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    std::string currentValue = std::to_string(progressInfo->percentage);
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "progress cb=%{public}s", currentValue.c_str());
    PasteBoardProgress::GetInstance().UpdateValue(progressKey_, currentValue);
}

int32_t PasteboardClient::PollHapSignal(std::string &signalKey)
{
    ffrtTimer_ = std::make_shared<FFRTTimer>("pasteboard_progress_abnormal");
    if (ffrtTimer_ != nullptr) {
        FFRTTask signaltask = [this, signalKey] {
            std::string defaultValue = "0";
            int32_t abnormalValue = 0;
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(PASTEBOARD_PROGRESS_SLEEP_TIME));
                PasteBoardProgress::GetInstance().GetValue(signalKey, defaultValue);
                abnormalValue = std::stoi(defaultValue);
            } while (abnormalValue == 0);
            if (abnormalValue == CANCEL_PASTE) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "progress cancel paste");
                return static_cast<int32_t>(PasteboardError::PROGRESS_CANCEL_PASTE);
            } else if (abnormalValue == PASTE_TIME_OUT) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "pasteboard progress time out");
                return static_cast<int32_t>(PasteboardError::PROGRESS_PASTE_TIME_OUT);
            } else {
                return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
            }
        };
        ffrtTimer_->SetTimer(signalKey, signaltask, 0);
    }
    return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
}

int32_t PasteboardClient::SetProgressWithoutFile(std::string &progressKey)
{
    if (progressKey.empty()) {
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    int progressValue = PASTEBOARD_PROGRESS_TWENTY_PERCENT;
    std::string currentValue = std::to_string(PASTEBOARD_PROGRESS_TWENTY_PERCENT);
    while (progressValue <= PASTEBOARD_PROGRESS_END_PERCENT) {
        PasteBoardProgress::GetInstance().UpdateValue(progressKey, currentValue);
        std::this_thread::sleep_for(std::chrono::milliseconds(PASTEBOARD_PROGRESS_SLEEP_TIME));
        progressValue += PASTEBOARD_PROGRESS_UPDATE_PERCENT;
        currentValue = std::to_string(progressValue);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardClient::ProgressSmoothToTwentyPercent(std::string &progressKey)
{
    int progressValue = 0;
    std::string currentValue = "0";
    while (progressValue <= PASTEBOARD_PROGRESS_TWENTY_PERCENT) {
        PasteBoardProgress::GetInstance().UpdateValue(progressKey, currentValue);
        std::this_thread::sleep_for(std::chrono::milliseconds(PASTEBOARD_PROGRESS_SLEEP_TIME));
        progressValue += PASTEBOARD_PROGRESS_UPDATE_PERCENT;
        currentValue = std::to_string(progressValue);
    }
}

int32_t PasteboardClient::GetPasteDataFromService(PasteData &pasteData, std::string currentPid, std::string currentId,
    pid_t pid, std::string progressKey)
{
    static DeduplicateMemory<RadarReportIdentity> reportMemory(REPORT_DUPLICATE_TIMEOUT);
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_CHECK_GET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            RadarReporter::PACKAGE_NAME, currentPid, RadarReporter::ERROR_CODE,
            static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    int32_t syncTime = 0;
    int32_t ret = proxyService->GetPasteData(pasteData, syncTime);
    if (!progressKey.empty()) {
        ProgressSmoothToTwentyPercent(progressKey);
    }
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    RetainUri(pasteData);
    RebuildWebviewPasteData(pasteData);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        if (pasteData.deviceId_.empty()) {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
                RadarReporter::DIS_SYNC_TIME, syncTime, RadarReporter::PACKAGE_NAME, currentPid);
        } else {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::CONCURRENT_ID, currentId, RadarReporter::DIS_SYNC_TIME, syncTime,
                RadarReporter::PACKAGE_NAME, currentPid);
        }
    } else if (ret != static_cast<int32_t>(PasteboardError::TASK_PROCESSING) &&
               !reportMemory.IsDuplicate({.pid = pid, .errorCode = ret})) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_FAILED, RadarReporter::BIZ_STATE,
            RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId, RadarReporter::DIS_SYNC_TIME,
            syncTime, RadarReporter::PACKAGE_NAME, currentPid, RadarReporter::ERROR_CODE, ret);
    } else {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_CANCELLED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            RadarReporter::DIS_SYNC_TIME, syncTime, RadarReporter::PACKAGE_NAME, currentPid,
            RadarReporter::ERROR_CODE, ret);
    }
    return ret;
}

int32_t PasteboardClient::GetDataWithProgress(PasteData &pasteData, std::shared_ptr<GetDataParams> params)
{
    if (params == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "params is nullptr");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    std::string progressKey;
    std::string signalKey;
    ffrtTimer_ = std::make_shared<FFRTTimer>("pasteboard_progress");
    if (params->progressIndicator == DEFAULT_PROGRESS_INDICATOR) {
        PasteBoardProgress::GetInstance().InsertValue(progressKey, PROGRESS_START_PERCENT); // 0%
        std::unique_lock<std::mutex> lock(instanceLock_);
        progressKey_ = progressKey;
        lock.unlock();
        PasteBoardProgress::GetInstance().InsertValue(signalKey, SIGNAL_KEY_DEFAULT); // 0%
        params->listener.ProgressNotify = GetFileProgressCb;
        PollHapSignal(signalKey);
        if (ffrtTimer_ != nullptr) {
            FFRTTask task = [this, progressKey, signalKey] {
                ProgressMakeMessageInfo(progressKey, signalKey);
            };
            ffrtTimer_->SetTimer(progressKey, task, HAP_PULL_UP_TIME);
        }
    }
    pid_t pid = getpid();
    std::string currentPid = std::to_string(pid);
    uint32_t tmpSequenceId = getSequenceId_++;
    std::string currentId = "GetDataWithProgress_" + currentPid + "_" + std::to_string(tmpSequenceId);
    PasteData pasteDat;
    pasteData.SetPasteId(currentId);
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_GET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN, RadarReporter::CONCURRENT_ID, currentId,
        RadarReporter::PACKAGE_NAME, currentPid);
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetDataWithProgress", HITRACE_GETPASTEDATA);
    int32_t ret = GetPasteDataFromService(pasteData, currentPid, currentId, pid, progressKey);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetPasteDataFromService is failed: ret=%{public}d.", ret);
        return ret;
    }
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetDataWithProgress", HITRACE_GETPASTEDATA);
    if (pasteData.GetPrimaryUri() != nullptr) {
        ret = CopyPasteData(pasteData, params);
    } else {
        ret = SetProgressWithoutFile(progressKey);
    }
    if (ffrtTimer_ != nullptr) {
        ffrtTimer_->CancelTimer(progressKey);
    }
    return ret;
}

int32_t PasteboardClient::GetUnifiedDataWithProgress(UDMF::UnifiedData &unifiedData,
    std::shared_ptr<GetDataParams> params)
{
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUdsdData", HITRACE_GETPASTEDATA);
    PasteData pasteData;
    int32_t ret = GetDataWithProgress(pasteData, params);
    unifiedData = *(ConvertUtils::Convert(pasteData));
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUdsdData", HITRACE_GETPASTEDATA);
    return ret;
}

int32_t PasteboardClient::GetUnifiedData(UDMF::UnifiedData &unifiedData)
{
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUnifiedData", HITRACE_GETPASTEDATA);
    PasteData pasteData;
    int32_t ret = GetPasteData(pasteData);
    unifiedData = *(PasteboardUtils::GetInstance().Convert(pasteData));
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUnifiedData", HITRACE_GETPASTEDATA);
    return ret;
}

int32_t PasteboardClient::GetUdsdData(UDMF::UnifiedData &unifiedData)
{
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUdsdData", HITRACE_GETPASTEDATA);
    PasteData pasteData;
    int32_t ret = GetPasteData(pasteData);
    unifiedData = *(ConvertUtils::Convert(pasteData));
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUdsdData", HITRACE_GETPASTEDATA);
    return ret;
}

void PasteboardClient::RefreshUri(std::shared_ptr<PasteDataRecord> &record)
{
    if (record->GetUri() == nullptr || record->GetFrom() == 0 || record->GetRecordId() == record->GetFrom()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Rebuild webview one of uri is null or not extra uri.");
        return;
    }
    std::shared_ptr<Uri> uri = record->GetUri();
    std::string puri = uri->ToString();
    std::string realUri = puri;
    if (puri.substr(0, PasteData::FILE_SCHEME_PREFIX.size()) == PasteData::FILE_SCHEME_PREFIX) {
        AppFileService::ModuleFileUri::FileUri fileUri(puri);
        realUri = PasteData::FILE_SCHEME_PREFIX + fileUri.GetRealPath();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildWebview uri is file uri: %{public}s", realUri.c_str());
    }
    if (realUri.find(PasteData::DISTRIBUTEDFILES_TAG) != std::string::npos) {
        record->SetConvertUri(realUri);
    } else {
        record->SetUri(std::make_shared<OHOS::Uri>(realUri));
    }
}

void PasteboardClient::RebuildWebviewPasteData(PasteData &pasteData)
{
    if (pasteData.GetTag() != PasteData::WEBVIEW_PASTEDATA_TAG) {
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Rebuild webview PasteData start.");
    auto justSplitHtml = false;
    auto details = std::make_shared<Details>();
    std::string textContent;
    for (auto &item : pasteData.AllRecords()) {
        justSplitHtml = justSplitHtml || item->GetFrom() > 0;
        if (!item->GetTextContent().empty() && textContent.empty()) {
            details = item->GetDetails();
            textContent = item->GetTextContent();
        }
        RefreshUri(item);
    }
    if (justSplitHtml) {
        PasteboardWebController::GetInstance().MergeExtraUris2Html(pasteData);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Rebuild webview PasteData end, merged uris into html.");
        return;
    }
    if (pasteData.GetPrimaryHtml() == nullptr) {
        return;
    }

    auto webData = std::make_shared<PasteData>(pasteData);
    PasteboardWebController::GetInstance().RebuildHtml(webData);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_HTML);
    std::shared_ptr<PasteDataRecord> pasteDataRecord = builder.SetMimeType(MIMETYPE_TEXT_HTML).
        SetPlainText(pasteData.GetPrimaryText()).SetHtmlText(webData->GetPrimaryHtml()).Build();
    if (details) {
        pasteDataRecord->SetDetails(*details);
    }
    pasteDataRecord->SetUDType(UDMF::HTML);
    pasteDataRecord->SetTextContent(textContent);
    webData->AddRecord(pasteDataRecord);
    std::size_t recordCnt = webData->GetRecordCount();
    if (recordCnt >= 1) {
        webData->RemoveRecordAt(recordCnt - 1);
    }
    pasteData = *webData;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Rebuild webview PasteData end.");
}

void PasteboardClient::RetainUri(PasteData &pasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "RetainUri start.");
    if (!pasteData.IsLocalPaste()) {
        return;
    }
    // clear convert uri
    for (size_t i = 0; i < pasteData.GetRecordCount(); ++i) {
        auto record = pasteData.GetRecordAt(i);
        if (record != nullptr) {
            record->SetConvertUri("");
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "RetainUri end.");
}

bool PasteboardClient::HasPasteData()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasPasteData start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return false;
    }
    return proxyService->HasPasteData();
}

int32_t PasteboardClient::SetPasteData(PasteData &pasteData, std::shared_ptr<PasteboardDelayGetter> delayGetter,
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "SetPasteData start.");
    RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_SET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN);
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_CHECK_SET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::ERROR_CODE,
            static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    sptr<PasteboardDelayGetterClient> delayGetterAgent;
    if (delayGetter != nullptr) {
        pasteData.SetDelayData(true);
        delayGetterAgent = new (std::nothrow) PasteboardDelayGetterClient(delayGetter);
    }
    sptr<PasteboardEntryGetterClient> entryGetterAgent;
    if (!(entryGetters.empty())) {
        pasteData.SetDelayRecord(true);
        entryGetterAgent = new (std::nothrow) PasteboardEntryGetterClient(entryGetters);
    }

    SplitWebviewPasteData(pasteData);

    auto ret = proxyService->SetPasteData(pasteData, delayGetterAgent, entryGetterAgent);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_SET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END);
    } else {
        RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_SET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::ERROR_CODE, ret);
    }
    return ret;
}

int32_t PasteboardClient::SetUnifiedData(
    const UDMF::UnifiedData &unifiedData, std::shared_ptr<PasteboardDelayGetter> delayGetter)
{
    auto pasteData = PasteboardUtils::GetInstance().Convert(unifiedData);
    return SetPasteData(*pasteData, delayGetter);
}

int32_t PasteboardClient::SetUdsdData(const UDMF::UnifiedData &unifiedData)
{
    auto pasteData = ConvertUtils::Convert(unifiedData);
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters;
    for (auto record : unifiedData.GetRecords()) {
        if (record != nullptr && record->GetEntryGetter() != nullptr) {
            entryGetters.emplace(record->GetRecordId(), record->GetEntryGetter());
        }
    }
    return SetPasteData(*pasteData, nullptr, entryGetters);
}

void PasteboardClient::SplitWebviewPasteData(PasteData &pasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteData start.");
    auto hasExtraRecord = false;
    for (const auto &record : pasteData.AllRecords()) {
        auto htmlEntry = record->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
        if (htmlEntry == nullptr) {
            continue;
        }
        std::shared_ptr<std::string> html = htmlEntry->ConvertToHtml();
        if (html == nullptr || html->empty()) {
            continue;
        }
        std::vector<std::shared_ptr<PasteDataRecord>> extraUriRecords =
                PasteboardWebController::GetInstance().SplitHtml2Records(html, record->GetRecordId());
        if (extraUriRecords.empty()) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteData extraUriRecords is empty.");
            continue;
        }
        hasExtraRecord = true;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "extraUriRecords number: %{public}zu", extraUriRecords.size());
        for (const auto &item : extraUriRecords) {
            pasteData.AddRecord(item);
        }
        record->SetFrom(record->GetRecordId());
    }
    if (hasExtraRecord) {
        pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteData end.");
}

void PasteboardClient::Subscribe(PasteboardObserverType type, sptr<PasteboardObserver> callback)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "start.");
    if (callback == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "input nullptr.");
        return;
    }
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    proxyService->SubscribeObserver(type, callback);
}

void PasteboardClient::AddPasteboardChangedObserver(sptr<PasteboardObserver> callback)
{
    Subscribe(PasteboardObserverType::OBSERVER_LOCAL, callback);
}

void PasteboardClient::AddPasteboardEventObserver(sptr<PasteboardObserver> callback)
{
    Subscribe(PasteboardObserverType::OBSERVER_EVENT, callback);
}

void PasteboardClient::Unsubscribe(PasteboardObserverType type, sptr<PasteboardObserver> callback)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    if (callback == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "remove all.");
        proxyService->UnsubscribeAllObserver(type);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "end.");
        return;
    }
    proxyService->UnsubscribeObserver(type, callback);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "end.");
}

void PasteboardClient::RemovePasteboardChangedObserver(sptr<PasteboardObserver> callback)
{
    Unsubscribe(PasteboardObserverType::OBSERVER_LOCAL, callback);
}

void PasteboardClient::RemovePasteboardEventObserver(sptr<PasteboardObserver> callback)
{
    Unsubscribe(PasteboardObserverType::OBSERVER_EVENT, callback);
}

int32_t PasteboardClient::SetGlobalShareOption(const std::map<uint32_t, ShareOption> &globalShareOptions)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return proxyService->SetGlobalShareOption(globalShareOptions);
}

int32_t PasteboardClient::RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return proxyService->RemoveGlobalShareOption(tokenIds);
}

std::map<uint32_t, ShareOption> PasteboardClient::GetGlobalShareOption(const std::vector<uint32_t> &tokenIds)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return {};
    }
    return proxyService->GetGlobalShareOption(tokenIds);
}

int32_t PasteboardClient::SetAppShareOptions(const ShareOption &shareOptions)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return proxyService->SetAppShareOptions(shareOptions);
}

int32_t PasteboardClient::RemoveAppShareOptions()
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return proxyService->RemoveAppShareOptions();
}

bool PasteboardClient::IsRemoteData()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "IsRemoteData start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return false;
    }
    auto ret = proxyService->IsRemoteData();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "IsRemoteData end.");
    return ret;
}

int32_t PasteboardClient::GetDataSource(std::string &bundleName)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetDataSource start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    int32_t ret = proxyService->GetDataSource(bundleName);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetDataSource end.");
    return ret;
}

std::vector<std::string> PasteboardClient::GetMimeTypes()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetMimeTypes start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return {};
    }
    return proxyService->GetMimeTypes();
}

bool PasteboardClient::HasDataType(const std::string &mimeType)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasDataType start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return false;
    }
    if (mimeType.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "parameter is invalid");
        return false;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "type is %{public}s", mimeType.c_str());
    bool ret = proxyService->HasDataType(mimeType);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasDataType end.");
    return ret;
}

std::set<Pattern> PasteboardClient::DetectPatterns(const std::set<Pattern> &patternsToCheck)
{
    if (!PatternDetection::IsValid(patternsToCheck)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid number in Pattern set!");
        return {};
    }

    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return {};
    }
    return proxyService->DetectPatterns(patternsToCheck);
}

sptr<IPasteboardService> PasteboardClient::GetPasteboardService()
{
    std::unique_lock<std::mutex> lock(instanceLock_);
    if (pasteboardServiceProxy_ != nullptr) {
        return pasteboardServiceProxy_;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardService start.");
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Get SystemAbilityManager failed.");
        pasteboardServiceProxy_ = nullptr;
        return nullptr;
    }
    sptr<IRemoteObject> remoteObject = samgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    if (remoteObject != nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Get PasteboardServiceProxy succeed.");
        if (deathRecipient_ == nullptr) {
            deathRecipient_ = sptr<IRemoteObject::DeathRecipient>(new PasteboardSaDeathRecipient());
        }
        remoteObject->AddDeathRecipient(deathRecipient_);
        pasteboardServiceProxy_ = iface_cast<IPasteboardService>(remoteObject);
        return pasteboardServiceProxy_;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "remoteObject is null.");
    sptr<PasteboardLoadCallback> loadCallback = new PasteboardLoadCallback();
    if (loadCallback == nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "loadCallback is nullptr.");
        return nullptr;
    }
    int32_t ret = samgrProxy->LoadSystemAbility(PASTEBOARD_SERVICE_ID, loadCallback);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Failed to load systemAbility.");
        return nullptr;
    }
    auto waitStatus = proxyConVar_.wait_for(lock, std::chrono::milliseconds(LOADSA_TIMEOUT_MS), [this]() {
        return pasteboardServiceProxy_ != nullptr;
    });
    if (!waitStatus) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Load systemAbility timeout.");
        return nullptr;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Getting PasteboardServiceProxy succeeded.");
    return pasteboardServiceProxy_;
}

sptr<IPasteboardService> PasteboardClient::GetPasteboardServiceProxy()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    return pasteboardServiceProxy_;
}

void PasteboardClient::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    if (deathRecipient_ == nullptr) {
        deathRecipient_ = sptr<IRemoteObject::DeathRecipient>(new PasteboardSaDeathRecipient());
    }
    if (remoteObject != nullptr) {
        remoteObject->AddDeathRecipient(deathRecipient_);
        pasteboardServiceProxy_ = iface_cast<IPasteboardService>(remoteObject);
    }
    proxyConVar_.notify_one();
}

void PasteboardClient::LoadSystemAbilityFail()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    pasteboardServiceProxy_ = nullptr;
    proxyConVar_.notify_one();
}

void PasteboardClient::OnRemoteSaDied(const wptr<IRemoteObject> &remote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteSaDied start.");
    std::lock_guard<std::mutex> lock(instanceLock_);
    pasteboardServiceProxy_ = nullptr;
}

void PasteboardClient::PasteStart(const std::string &pasteId)
{
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_DISTRIBUTED_FILE_START,
        RadarReporter::DFX_SUCCESS, RadarReporter::CONCURRENT_ID, pasteId);
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    proxyService->PasteStart(pasteId);
}

void PasteboardClient::PasteComplete(const std::string &deviceId, const std::string &pasteId)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    proxyService->PasteComplete(deviceId, pasteId);
}

int32_t PasteboardClient::GetRemoteDeviceName(std::string &deviceName, bool &isRemote)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return proxyService->GetRemoteDeviceName(deviceName, isRemote);
}

void PasteboardClient::ProgressMakeMessageInfo(const std::string &progressKey, const std::string &signalKey)
{
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        return;
    }
    return proxyService->ProgressMakeMessageInfo(progressKey, signalKey);
}

PasteboardSaDeathRecipient::PasteboardSaDeathRecipient() {}

void PasteboardSaDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "PasteboardSaDeathRecipient on remote systemAbility died.");
    PasteboardClient::GetInstance()->OnRemoteSaDied(object);
}
} // namespace MiscServices
} // namespace OHOS
