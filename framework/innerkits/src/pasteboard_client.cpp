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

#include <charconv>
#include <iservice_registry.h>
#include <thread>
#include <regex>

#include "convert_utils.h"
#include "ffrt/ffrt_utils.h"
#include "hitrace_meter.h"
#include "pasteboard_copy.h"
#include "pasteboard_deduplicate_memory.h"
#include "pasteboard_error.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_hilog.h"
#include "pasteboard_load_callback.h"
#include "pasteboard_pattern.h"
#include "pasteboard_progress.h"
#include "pasteboard_signal_callback.h"
#include "pasteboard_time.h"
#include "pasteboard_utils.h"
#include "pasteboard_web_controller.h"
#include "pasteboard_samgr_listener.h"
#include "pasteboard_service_loader.h"
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"
#include "nlohmann/json.hpp"
using namespace OHOS::Media;
using json = nlohmann::json;

namespace OHOS {
namespace MiscServices {
constexpr const int32_t HITRACE_GETPASTEDATA = 0;
std::string g_progressKey;
constexpr int32_t PASTEBOARD_PROGRESS_UPDATE_PERCENT = 5;
constexpr int32_t UPDATE_PERCENT_WITHOUT_FILE = 10;
constexpr int32_t PASTEBOARD_PROGRESS_TWENTY_PERCENT = 20;
constexpr int32_t PASTEBOARD_PROGRESS_FINISH_PERCENT = 100;
constexpr int32_t PASTEBOARD_PROGRESS_SLEEP_TIME = 100; // ms
constexpr int32_t SLEEP_TIME_WITHOUT_FILE = 50; // ms
constexpr int32_t PASTEBOARD_PROGRESS_RETRY_TIMES = 10;
constexpr int64_t REPORT_DUPLICATE_TIMEOUT = 2 * 60 * 1000; // 2 minutes
constexpr uint32_t JSON_INDENT = 4;
constexpr uint32_t RECORD_DISPLAY_UPPERBOUND = 3;
constexpr uint32_t MAX_SIGNAL_VALUE_SIZE = 128;
static constexpr int32_t HAP_PULL_UP_TIME = 500; // ms
static constexpr int32_t HAP_MIN_SHOW_TIME = 300; // ms
static sptr<PasteboardSaMgrListener> saCallback_ = nullptr;
constexpr const char *ERROR_CODE = "ERROR_CODE";
constexpr const char *DIS_SYNC_TIME = "DIS_SYNC_TIME";
constexpr const char *PACKAGE_NAME = "PACKAGE_NAME";
constexpr const char *PASTEDATA_SUMMARY = "PASTEDATA_SUMMARY";
std::mutex PasteboardClient::instanceLock_;
std::atomic<bool> PasteboardClient::remoteTask_(false);
std::atomic<bool> PasteboardClient::isPasting_(false);
std::atomic<uint64_t> PasteboardClient::progressStartTime_;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024; // 32K
constexpr uid_t ANCO_SERVICE_BROKER_UID = 5557;

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
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "proxyService is null");
    }
}

PasteboardClient::~PasteboardClient()
{
}

PasteboardClient *PasteboardClient::GetInstance()
{
    static PasteboardClient instance;
    return &instance;
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

int32_t PasteboardClient::GetChangeCount(uint32_t &changeCount)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetChangeCount start.");
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        changeCount = 0;
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    return ConvertErrCode(proxyService->GetChangeCount(changeCount));
}

int32_t PasteboardClient::SubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<EntityRecognitionObserver> &observer)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
        "SubscribeEntityObserver start, type is %{public}u, length is %{public}u.", static_cast<uint32_t>(entityType),
        expectedDataLength);
    if (observer == nullptr) {
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    return ConvertErrCode(proxyService->SubscribeEntityObserver(entityType, expectedDataLength, observer));
}

int32_t PasteboardClient::UnsubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<EntityRecognitionObserver> &observer)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
        "UnsubscribeEntityObserver start, type is %{public}u, length is %{public}u.", static_cast<uint32_t>(entityType),
        expectedDataLength);
    if (observer == nullptr) {
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    return ConvertErrCode(proxyService->UnsubscribeEntityObserver(entityType, expectedDataLength, observer));
}

int32_t PasteboardClient::GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry &value)
{
    return PasteboardServiceLoader::GetInstance().GetRecordValueByType(dataId, recordId, value);
}

void PasteboardClient::Clear()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Clear start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    proxyService->Clear();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Clear end.");
    return;
}

void PasteboardClient::ClearByUser(int32_t userId)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "ClearByUser start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    proxyService->ClearByUser(userId);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "ClearByUser end.");
    return;
}

void PasteboardClient::CloseSharedMemFd(int fd)
{
    if (fd >= 0) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "CloseSharedMemFd:%{public}d", fd);
        close(fd);
    }
}

int32_t PasteboardClient::GetPasteData(PasteData &pasteData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "enter");
    pid_t pid = getpid();
    std::string currentPid = std::to_string(pid);
    std::string currentId = PasteData::CreatePasteId("GetPasteData", getSequenceId_++);
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_GET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN, RadarReporter::CONCURRENT_ID, currentId,
        PACKAGE_NAME, currentPid);
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetPasteData", HITRACE_GETPASTEDATA);
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_CHECK_GET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            PACKAGE_NAME, currentPid, ERROR_CODE, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    int32_t syncTime = 0;
    int32_t realErrCode = 0;
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> recvTLV;
    int32_t ret = proxyService->GetPasteData(fd, rawDataSize, recvTLV, currentId, syncTime, realErrCode);
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    ret = ConvertErrCode(realErrCode);
    int32_t result = ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    PasteboardWebController::GetInstance().RetainUri(pasteData);
    PasteboardWebController::GetInstance().RemoveInvalidUri(pasteData);
    PasteboardWebController::GetInstance().RebuildWebviewPasteData(pasteData);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        GetDataReport(pasteData, syncTime, currentId, currentPid, ret);
        return ret;
    } else if (result == static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR)) {
        GetDataReport(pasteData, syncTime, currentId, currentPid, result);
        return result;
    }
    GetDataReport(pasteData, syncTime, currentId, currentPid, ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardClient::GetDataReport(PasteData &pasteData, int32_t syncTime, const std::string &currentId,
    const std::string &currentPid, int32_t ret)
{
    static DeduplicateMemory<RadarReportIdentity> reportMemory(REPORT_DUPLICATE_TIMEOUT);
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetPasteData", HITRACE_GETPASTEDATA);
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        if (pasteData.deviceId_.empty()) {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
                PACKAGE_NAME, currentPid, DIS_SYNC_TIME, syncTime,
                PASTEDATA_SUMMARY, pasteDataInfoSummary);
        } else {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::CONCURRENT_ID, currentId, PACKAGE_NAME, currentPid,
                DIS_SYNC_TIME, syncTime, PASTEDATA_SUMMARY,
                pasteDataInfoSummary);
        }
    } else if (ret != static_cast<int32_t>(PasteboardError::TASK_PROCESSING) &&
               !reportMemory.IsDuplicate({.pid = getpid(), .errorCode = ret})) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            PACKAGE_NAME, currentPid, DIS_SYNC_TIME, syncTime,
            ERROR_CODE, ret, PASTEDATA_SUMMARY, pasteDataInfoSummary);
    } else {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_CANCELLED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentId,
            PACKAGE_NAME, currentPid, DIS_SYNC_TIME, syncTime,
            ERROR_CODE, ret, PASTEDATA_SUMMARY, pasteDataInfoSummary);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "leave, ret=%{public}d", ret);
}

void PasteboardClient::GetProgressByProgressInfo(std::shared_ptr<GetDataParams> params)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(params != nullptr, PASTEBOARD_MODULE_CLIENT, "params is null!");

    if (params->info == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "params->info is null!");
        return;
    }
    std::unique_lock<std::mutex> lock(instanceLock_);
    std::string progressKey = g_progressKey;
    lock.unlock();
    std::string currentValue = std::to_string(params->info->percentage);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "pasteboard progress percent = %{public}s", currentValue.c_str());
    PasteBoardProgress::UpdateValue(progressKey, currentValue);
}

int32_t PasteboardClient::SetProgressWithoutFile(std::string &progressKey, std::shared_ptr<GetDataParams> params)
{
    int progressValue = PASTEBOARD_PROGRESS_TWENTY_PERCENT;
    while (progressValue < PASTEBOARD_PROGRESS_FINISH_PERCENT && !remoteTask_.load()) {
        uint64_t currentTimeMicros = PasteBoardTime::GetCurrentTimeMicros();
        if (currentTimeMicros >= progressStartTime_) {
            uint64_t duration = currentTimeMicros - progressStartTime_;
            if (duration >= (HAP_PULL_UP_TIME + HAP_MIN_SHOW_TIME) || duration < HAP_PULL_UP_TIME) {
                UpdateProgress(params, PASTEBOARD_PROGRESS_FINISH_PERCENT);
                break;
            }
        }
        if (ProgressSignalClient::GetInstance().CheckCancelIfNeed()) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "progress cancel success!");
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_WITHOUT_FILE));
        progressValue += UPDATE_PERCENT_WITHOUT_FILE;
        UpdateProgress(params, progressValue);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardClient::ProgressSmoothToTwentyPercent(PasteData &pasteData, std::string &progressKey,
    std::shared_ptr<GetDataParams> params)
{
    if (pasteData.GetRecordCount() <= 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "no pasteData, progress no need to twenty");
        return;
    }
    int progressValue = 0;
    bool hasUri = (pasteData.GetPrimaryUri() != nullptr);
    while (progressValue < PASTEBOARD_PROGRESS_TWENTY_PERCENT && !remoteTask_.load()) {
        uint64_t currentTimeMicros = PasteBoardTime::GetCurrentTimeMicros();
        if (currentTimeMicros >= progressStartTime_) {
            uint64_t duration = currentTimeMicros - progressStartTime_;
            if (duration >= (HAP_PULL_UP_TIME + HAP_MIN_SHOW_TIME) || duration < HAP_PULL_UP_TIME) {
                UpdateProgress(
                    params, hasUri ? PASTEBOARD_PROGRESS_TWENTY_PERCENT : PASTEBOARD_PROGRESS_FINISH_PERCENT);
                break;
            }
        }
        if (ProgressSignalClient::GetInstance().CheckCancelIfNeed()) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(PASTEBOARD_PROGRESS_SLEEP_TIME));
        progressValue += PASTEBOARD_PROGRESS_UPDATE_PERCENT;
        UpdateProgress(params, progressValue);
    }
}

void PasteboardClient::UpdateProgress(std::shared_ptr<GetDataParams> params, int progressValue)
{
    if (params == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "params is null!");
        return;
    }
    if (params->info != nullptr) {
        params->info->percentage = progressValue;
    }
    if (params->listener.ProgressNotify != nullptr) {
        params->listener.ProgressNotify(params);
    }
}

void PasteboardClient::OnProgressAbnormal(int32_t result)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "The progress is reported abnormal.");
    remoteTask_.store(true);
}

int32_t PasteboardClient::GetPasteDataFromService(PasteData &pasteData,
    PasteDataFromServiceInfo &pasteDataFromServiceInfo, std::string progressKey, std::shared_ptr<GetDataParams> params)
{
    auto proxyService = GetPasteboardService();
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_CHECK_GET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID,
            pasteDataFromServiceInfo.currentId, PACKAGE_NAME, pasteDataFromServiceInfo.currentPid,
            ERROR_CODE, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
            PASTEDATA_SUMMARY, pasteDataInfoSummary);
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    int32_t syncTime = 0;
    int32_t realErrCode = 0;
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> recvTLV(0);
    std::string pasteId = pasteDataFromServiceInfo.currentId;
    int32_t ret = proxyService->GetPasteData(fd, rawDataSize, recvTLV, pasteId, syncTime, realErrCode);
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    ret = ConvertErrCode(realErrCode);
    int32_t result = ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    ProgressSmoothToTwentyPercent(pasteData, progressKey, params);
    PasteboardWebController::GetInstance().RetainUri(pasteData);
    PasteboardWebController::GetInstance().RemoveInvalidUri(pasteData);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        ProcessRadarReport(ret, pasteData, pasteDataFromServiceInfo, syncTime);
        return ret;
    } else if (result == static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR)) {
        ProcessRadarReport(result, pasteData, pasteDataFromServiceInfo, syncTime);
        return result;
    }
    ProcessRadarReport(ret, pasteData, pasteDataFromServiceInfo, syncTime);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

template<typename T>
int32_t PasteboardClient::ProcessPasteData(T &data, int64_t rawDataSize, int fd,
    const std::vector<uint8_t> &recvTLV)
{
    int32_t ret = static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR);
    if (fd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "fail fd:%{public}d", fd);
        return ret;
    }
    MessageParcelWarp messageReply;
    if (rawDataSize <= 0 || rawDataSize > messageReply.GetRawDataSize()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid raw data size:%{public}" PRId64, rawDataSize);
        CloseSharedMemFd(fd);
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
    }
    bool result = false;
    MessageParcel parcelData;
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        parcelData.WriteInt64(rawDataSize);
        parcelData.WriteFileDescriptor(fd);
        CloseSharedMemFd(fd);
        const uint8_t *rawData =
            reinterpret_cast<const uint8_t *>(messageReply.ReadRawData(parcelData, static_cast<size_t>(rawDataSize)));
        if (rawData == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "mmap failed, size=%{public}" PRId64, rawDataSize);
            return ret;
        }
        std::vector<uint8_t> pasteDataTlv(rawData, rawData + rawDataSize);
        result = data.Decode(pasteDataTlv);
    } else {
        result = data.Decode(recvTLV);
        CloseSharedMemFd(fd);
    }
    if (!result) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to decode pastedata in TLV");
        return ret;
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardClient::ProcessRadarReport(int32_t ret, PasteData &pasteData,
    PasteDataFromServiceInfo &pasteDataFromServiceInfo, int32_t syncTime)
{
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    static DeduplicateMemory<RadarReportIdentity> reportMemory(REPORT_DUPLICATE_TIMEOUT);
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        if (pasteData.deviceId_.empty()) {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID,
                pasteDataFromServiceInfo.currentId, PACKAGE_NAME, pasteDataFromServiceInfo.currentPid,
                DIS_SYNC_TIME, syncTime, PASTEDATA_SUMMARY,
                pasteDataInfoSummary);
        } else {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::CONCURRENT_ID, pasteDataFromServiceInfo.currentId, PACKAGE_NAME,
                pasteDataFromServiceInfo.currentPid, DIS_SYNC_TIME, syncTime,
                PASTEDATA_SUMMARY, pasteDataInfoSummary);
        }
    } else if (ret != static_cast<int32_t>(PasteboardError::TASK_PROCESSING) &&
               !reportMemory.IsDuplicate({.pid = pasteDataFromServiceInfo.pid, .errorCode = ret})) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_FAILED, RadarReporter::BIZ_STATE,
            RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, pasteDataFromServiceInfo.currentId,
            PACKAGE_NAME, pasteDataFromServiceInfo.currentPid, DIS_SYNC_TIME, syncTime,
            ERROR_CODE, ret, PASTEDATA_SUMMARY, pasteDataInfoSummary);
    } else {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_CANCELLED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID,
            pasteDataFromServiceInfo.currentId, PACKAGE_NAME, pasteDataFromServiceInfo.currentPid,
            DIS_SYNC_TIME, syncTime, ERROR_CODE, ret, PASTEDATA_SUMMARY,
            pasteDataInfoSummary);
    }
}

void PasteboardClient::ProgressRadarReport(PasteData &pasteData, PasteDataFromServiceInfo &pasteDataFromServiceInfo)
{
    pasteDataFromServiceInfo.pid = getpid();
    pasteDataFromServiceInfo.currentPid = std::to_string(pasteDataFromServiceInfo.pid);
    pasteDataFromServiceInfo.currentId = PasteData::CreatePasteId("GetDataWithProgress", getSequenceId_++);
    pasteData.SetPasteId(pasteDataFromServiceInfo.currentId);
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_GET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN,
        RadarReporter::CONCURRENT_ID, pasteDataFromServiceInfo.currentId,
        PACKAGE_NAME, pasteDataFromServiceInfo.currentPid);
}

int32_t PasteboardClient::ProgressAfterTwentyPercent(PasteData &pasteData, std::shared_ptr<GetDataParams> params,
    std::string progressKey)
{
    if (pasteData.GetRecordCount() <= 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "no pasteData, no need progress");
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    int32_t ret = 0;
    bool hasUri = (pasteData.GetPrimaryUri() != nullptr);
    if (hasUri) {
        ret = PasteBoardCopyFile::GetInstance().CopyPasteData(pasteData, params);
    } else {
        ret = SetProgressWithoutFile(progressKey, params);
    }
    return ret;
}

int32_t PasteboardClient::CheckProgressParam(std::shared_ptr<GetDataParams> params)
{
    if (params == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid param!");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    if (isPasting_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "task copying!");
        return static_cast<int32_t>(PasteboardError::TASK_PROCESSING);
    }
    ProgressSignalClient::GetInstance().Init();
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardClient::GetDataWithProgress(PasteData &pasteData, std::shared_ptr<GetDataParams> params)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetDataWithProgress start.");
    int32_t ret = CheckProgressParam(params);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        return ret;
    }
    progressStartTime_ = PasteBoardTime::GetCurrentTimeMicros();
    isPasting_.store(true);
    std::string progressKey;
    std::string keyDefaultValue = "0";
    std::shared_ptr<FFRTTimer> ffrtTimer;
    ffrtTimer = std::make_shared<FFRTTimer>("pasteboard_progress");
    if (params->progressIndicator != NONE_PROGRESS_INDICATOR) {
        PasteBoardProgress::InsertValue(progressKey, keyDefaultValue); // 0%
        std::unique_lock<std::mutex> lock(instanceLock_);
        g_progressKey = progressKey;
        lock.unlock();
        params->listener.ProgressNotify = GetProgressByProgressInfo;
        if (ffrtTimer != nullptr) {
            FFRTTask task = [this, progressKey] {
                ShowProgress(progressKey);
            };
            ffrtTimer->SetTimer(progressKey, task, HAP_PULL_UP_TIME);
        }
    }
    PasteDataFromServiceInfo pasteDataFromServiceInfo;
    ProgressRadarReport(pasteData, pasteDataFromServiceInfo);
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetDataWithProgress", HITRACE_GETPASTEDATA);
    ret = GetPasteDataFromService(pasteData, pasteDataFromServiceInfo, progressKey, params);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetPasteDataFromService is failed: ret=%{public}d.", ret);
        remoteTask_.store(false);
        isPasting_.store(false);
        return ret;
    }
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetDataWithProgress", HITRACE_GETPASTEDATA);
    ret = ProgressAfterTwentyPercent(pasteData, params, progressKey);
    PasteboardWebController::GetInstance().RebuildWebviewPasteData(pasteData);
    if (ffrtTimer != nullptr) {
        ffrtTimer->CancelTimer(progressKey);
    }
    if (remoteTask_.load()) {
        ret = static_cast<int32_t>(PasteboardError::PROGRESS_ABNORMAL);
        remoteTask_.store(false);
    }
    isPasting_.store(false);
    return ret;
}

int32_t PasteboardClient::GetUnifiedDataWithProgress(UDMF::UnifiedData &unifiedData,
    std::shared_ptr<GetDataParams> params)
{
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUnifiedDataWithProgress", HITRACE_GETPASTEDATA);
    PasteData pasteData;
    int32_t ret = GetDataWithProgress(pasteData, params);
    unifiedData = *(ConvertUtils::Convert(pasteData));
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUnifiedDataWithProgress", HITRACE_GETPASTEDATA);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "enter");
    StartAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUdsdData", HITRACE_GETPASTEDATA);
    PasteData pasteData;
    int32_t ret = GetPasteData(pasteData);
    unifiedData = *(ConvertUtils::Convert(pasteData));
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetUdsdData", HITRACE_GETPASTEDATA);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "leave, ret=%{public}d", ret);
    return ret;
}

bool PasteboardClient::HasPasteData()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasPasteData start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    bool ret = false;
    int32_t errCode = proxyService->HasPasteData(ret);
    if (errCode != ERR_OK) {
        return false;
    }
    return ret;
}

void PasteboardClient::CreateGetterAgent(sptr<PasteboardDelayGetterClient> &delayGetterAgent,
    std::shared_ptr<PasteboardDelayGetter> &delayGetter, sptr<PasteboardEntryGetterClient> &entryGetterAgent,
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> &entryGetters, PasteData &pasteData)
{
    if (delayGetter != nullptr) {
        pasteData.SetDelayData(true);
        delayGetterAgent = new (std::nothrow) PasteboardDelayGetterClient(delayGetter);
    }
    if (!(entryGetters.empty())) {
        pasteData.SetDelayRecord(true);
        entryGetterAgent = new (std::nothrow) PasteboardEntryGetterClient(entryGetters);
    }
    if (pasteData.IsDelayData() && delayGetterAgent == nullptr) {
        pasteData.SetDelayData(false);
    }
    if (pasteData.IsDelayRecord() && entryGetterAgent == nullptr) {
        pasteData.SetDelayRecord(false);
    }
}

int32_t PasteboardClient::WritePasteData(PasteData &pasteData, std::vector<uint8_t> &buffer, int &fd,
    int64_t &tlvSize, MessageParcelWarp &messageData, MessageParcel &parcelPata)
{
    tlvSize = static_cast<int64_t>(pasteData.Count());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(0 < tlvSize && tlvSize <= MessageParcelWarp::GetRawDataSize(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), PASTEBOARD_MODULE_CLIENT,
        "invalid data size, dataSize=%{public}" PRId64, tlvSize);
    std::vector<uint8_t> pasteDataTlv(0);
    bool result = pasteData.Encode(tlvSize, pasteDataTlv);
    if (!result) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "paste data encode failed.");
        return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
    }
    if (tlvSize > MIN_ASHMEM_DATA_SIZE) {
        if (!messageData.WriteRawData(parcelPata, pasteDataTlv.data(), pasteDataTlv.size())) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to WriteRawData");
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
        fd = messageData.GetWriteDataFd();
        pasteDataTlv.clear();
    } else {
        fd = messageData.CreateTmpFd();
        if (fd < 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to create tmp fd");
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
    }
    buffer = std::move(pasteDataTlv);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "set: fd:%{public}d, size:%{public}" PRId64, fd, tlvSize);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardClient::SetPasteData(PasteData &pasteData, std::shared_ptr<PasteboardDelayGetter> delayGetter,
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "enter");
    RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_SET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN);
    auto proxyService = GetPasteboardService();
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_CHECK_SET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, ERROR_CODE,
            static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    sptr<PasteboardDelayGetterClient> delayGetterAgent;
    sptr<PasteboardEntryGetterClient> entryGetterAgent;
    CreateGetterAgent(delayGetterAgent, delayGetter, entryGetterAgent, entryGetters, pasteData);
    std::vector<uint8_t> pasteDataTlv(0);
    int fd = -1;
    int64_t tlvSize = 0;
    MessageParcelWarp messageData;
    MessageParcel parcelPata;
    int32_t ret = WritePasteData(pasteData, pasteDataTlv, fd, tlvSize, messageData, parcelPata);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Write data failed, size=%{public}" PRId64, tlvSize);
        return ret;
    }
    if (delayGetterAgent != nullptr && entryGetterAgent != nullptr) {
        ret = proxyService->SetPasteData(fd, tlvSize, pasteDataTlv, delayGetterAgent, entryGetterAgent);
    } else if (delayGetterAgent != nullptr && entryGetterAgent == nullptr) {
        ret = proxyService->SetPasteDataDelayData(fd, tlvSize, pasteDataTlv, delayGetterAgent);
    } else if (delayGetterAgent == nullptr && entryGetterAgent != nullptr) {
        ret = proxyService->SetPasteDataEntryData(fd, tlvSize, pasteDataTlv, entryGetterAgent);
    } else {
        ret = proxyService->SetPasteDataOnly(fd, tlvSize, pasteDataTlv);
    }
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    ret = ConvertErrCode(ret);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_SET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, PASTEDATA_SUMMARY, pasteDataInfoSummary);
    } else {
        RADAR_REPORT(RadarReporter::DFX_SET_PASTEBOARD, RadarReporter::DFX_SET_BIZ_SCENE, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END,
            ERROR_CODE, ret, PASTEDATA_SUMMARY, pasteDataInfoSummary);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "leave, ret=%{public}d", ret);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "enter");
    auto pasteData = ConvertUtils::Convert(unifiedData);
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters;
    for (auto record : unifiedData.GetRecords()) {
        if (record != nullptr && record->GetEntryGetter() != nullptr) {
            entryGetters.emplace(record->GetRecordId(), record->GetEntryGetter());
        }
    }
    int32_t ret = SetPasteData(*pasteData, nullptr, entryGetters);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "leave, ret=%{public}d", ret);
    return ret;
}

void PasteboardClient::SubscribePasteboardSA()
{
    PASTEBOARD_CHECK_AND_RETURN_LOGD(
        getuid() != ANCO_SERVICE_BROKER_UID, PASTEBOARD_MODULE_CLIENT, "ignore,uid:%{public}u.", getuid());
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(samgrProxy != nullptr, PASTEBOARD_MODULE_CLIENT, "get samgr fail.");
    std::lock_guard<std::mutex> lock(saListenerMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(!isSubscribeSa_, PASTEBOARD_MODULE_CLIENT, "already subscribe sa.");
    if (saCallback_ == nullptr) {
        saCallback_ = sptr<PasteboardSaMgrListener>::MakeSptr();
    }
    PASTEBOARD_CHECK_AND_RETURN_LOGE(saCallback_ != nullptr, PASTEBOARD_MODULE_CLIENT, "Create saCallback failed!");
    auto ret = samgrProxy->SubscribeSystemAbility(PASTEBOARD_SERVICE_ID, saCallback_);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(
        ret == ERR_OK, PASTEBOARD_MODULE_CLIENT, "subscribe pasteboard sa failed! ret %{public}d.", ret);
    isSubscribeSa_ = true;
}

void PasteboardClient::UnSubscribePasteboardSA()
{
    PASTEBOARD_CHECK_AND_RETURN_LOGD(
        getuid() != ANCO_SERVICE_BROKER_UID, PASTEBOARD_MODULE_CLIENT, "ignore,uid:%{public}u.", getuid());
    std::lock_guard<std::mutex> lock(saListenerMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(saCallback_ != nullptr, PASTEBOARD_MODULE_CLIENT, "saCallback is nullptr");
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    auto tempCallback = saCallback_;
    saCallback_ = nullptr;
    isSubscribeSa_ = false;
    PASTEBOARD_CHECK_AND_RETURN_LOGE(samgrProxy != nullptr, PASTEBOARD_MODULE_CLIENT, "get samgr fail");
    int32_t ret = samgrProxy->UnSubscribeSystemAbility(PASTEBOARD_SERVICE_ID, tempCallback);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(
        ret == ERR_OK, PASTEBOARD_MODULE_CLIENT, "unSubscribe pasteboard sa failed! ret %{public}d.", ret);
}

void PasteboardClient::ReleaseSaListener()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start.");
    UnSubscribePasteboardSA();
    PasteboardServiceLoader::GetInstance().ReleaseDeathRecipient();
}

int32_t PasteboardClient::DetachPasteboard()
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        proxyService != nullptr, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is null");
    return proxyService->DetachPasteboard();
}

void PasteboardClient::Resubscribe()
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT,
        "proxyService is null");
    std::lock_guard<std::mutex> lock(observerSetMutex_);
    for (auto it = observerSet_.begin(); it != observerSet_.end(); ++it) {
        proxyService->ResubscribeObserver(it->first, it->second);
    }
}

bool PasteboardClient::Subscribe(PasteboardObserverType type, sptr<PasteboardObserver> callback)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "start.");
    if (callback == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "callback is null");
        return false;
    }
    auto proxyService = GetPasteboardService();
    {
        std::lock_guard<std::mutex> lock(observerSetMutex_);
        observerSet_.insert(std::make_pair(type, callback));
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "proxyService is null");
    int32_t ret = proxyService->SubscribeObserver(type, callback);
    SubscribePasteboardSA();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == ERR_OK, false, PASTEBOARD_MODULE_CLIENT,
        "subscribe failed, ret=%{public}d", ret);
    return true;
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
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    if (callback == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "remove all.");
        {
            std::lock_guard<std::mutex> lock(observerSetMutex_);
            observerSet_.clear();
        }
        proxyService->UnsubscribeAllObserver(type);
        UnSubscribePasteboardSA();
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "end.");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(observerSetMutex_);
        observerSet_.erase(std::make_pair(type, callback));
        if (observerSet_.size() == 0) {
            UnSubscribePasteboardSA();
        }
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

int32_t PasteboardClient::SubscribeDisposableObserver(const sptr<PasteboardDisposableObserver> &observer,
    int32_t targetWindowId, DisposableType type, uint32_t maxLength)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(observer != nullptr,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_CLIENT,
        "param invalid, observer is null");
    int32_t typeInt = static_cast<int32_t>(type);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(typeInt >= 0 && typeInt < static_cast<int32_t>(DisposableType::MAX),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_CLIENT,
        "param invalid, type=%{public}d", typeInt);

    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR), PASTEBOARD_MODULE_CLIENT,
        "proxyService is null");

    int32_t ret = proxyService->SubscribeDisposableObserver(observer, targetWindowId, type, maxLength);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == ERR_OK, ret, PASTEBOARD_MODULE_CLIENT,
        "subscribe failed, ret=%{public}d", ret);
    return ERR_OK;
}

int32_t PasteboardClient::SetGlobalShareOption(const std::map<uint32_t, ShareOption> &globalShareOptions)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    std::unordered_map<uint32_t, int32_t> shareOptions = {};
    for (const auto &pair : globalShareOptions) {
        shareOptions[pair.first] = static_cast<int32_t>(pair.second);
    }
    int32_t ret = proxyService->SetGlobalShareOption(shareOptions);
    ret = ConvertErrCode(ret);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        ret = ERR_OK;
    }
    return ret;
}

int32_t PasteboardClient::RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    int32_t ret = proxyService->RemoveGlobalShareOption(tokenIds);
    ret = ConvertErrCode(ret);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        ret = ERR_OK;
    }
    return ret;
}

std::map<uint32_t, ShareOption> PasteboardClient::GetGlobalShareOption(const std::vector<uint32_t> &tokenIds)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, {},
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    std::unordered_map<uint32_t, int32_t> funcResult = {};
    int32_t ret = proxyService->GetGlobalShareOption(tokenIds, funcResult);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetGlobalShareOption failed, ret=%{public}d", ret);
        return {};
    }
    std::map<uint32_t, ShareOption> result;
    for (const auto &pair : funcResult) {
        result[pair.first] = static_cast<ShareOption>(pair.second);
    }
    return result;
}

int32_t PasteboardClient::SetAppShareOptions(const ShareOption &shareOptions)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    return proxyService->SetAppShareOptions(shareOptions);
}

int32_t PasteboardClient::RemoveAppShareOptions()
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    int32_t ret = proxyService->RemoveAppShareOptions();
    ret = ConvertErrCode(ret);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        ret = ERR_OK;
    }
    return ret;
}

bool PasteboardClient::IsRemoteData()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "IsRemoteData start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    bool ret = false;
    int32_t retCode = proxyService->IsRemoteData(ret);
    if (retCode != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "IsRemoteData failed, retCode=%{public}d", retCode);
        return false;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "IsRemoteData end.");
    return ret;
}

int32_t PasteboardClient::GetDataSource(std::string &bundleName)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetDataSource start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    int32_t ret = proxyService->GetDataSource(bundleName);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetDataSource end.");
    return ConvertErrCode(ret);
}

std::vector<std::string> PasteboardClient::GetMimeTypes()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetMimeTypes start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, {},
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    std::vector<std::string> mimeTypes = {};
    int32_t ret = proxyService->GetMimeTypes(mimeTypes);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetMimeTypes failed, ret=%{public}d", ret);
        return {};
    }
    return mimeTypes;
}

bool PasteboardClient::HasDataType(const std::string &mimeType)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasDataType start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!mimeType.empty(), false, PASTEBOARD_MODULE_CLIENT, "parameter is invalid");
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "type is %{public}s", mimeType.c_str());
    bool ret = false;
    int32_t retCode = proxyService->HasDataType(mimeType, ret);
    if (retCode != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "HasDataType failed, retCode=%{public}d", retCode);
        return false;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasDataType end.");
    return ret;
}

bool PasteboardClient::HasUtdType(const std::string &utdType)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasUtdType start.");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!utdType.empty(), false, PASTEBOARD_MODULE_CLIENT, "parameter is invalid");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "type is %{public}s", utdType.c_str());
    bool ret = false;
    int32_t retCode = proxyService->HasUtdType(utdType, ret);
    if (retCode != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "HasUtdType failed, retCode=%{public}d, type=%{public}s",
            retCode, utdType.c_str());
        return false;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasUtdType end.");
    return ret;
}

std::set<Pattern> PasteboardClient::DetectPatterns(const std::set<Pattern> &patternsToCheck)
{
    if (!PatternDetection::IsValid(patternsToCheck)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Invalid number in Pattern set!");
        return {};
    }

    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, {},
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    std::vector<Pattern> patterns(patternsToCheck.begin(), patternsToCheck.end());
    std::vector<Pattern> funcResult = {};
    int32_t ret = proxyService->DetectPatterns(patterns, funcResult);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "DetectPatterns failed, ret=%{public}d", ret);
        return {};
    }
    std::set<Pattern> result(funcResult.begin(), funcResult.end());
    return result;
}

sptr<IPasteboardService> PasteboardClient::GetPasteboardService()
{
    return PasteboardServiceLoader::GetInstance().GetPasteboardService();
}

void PasteboardClient::PasteStart(const std::string &pasteId)
{
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_DISTRIBUTED_FILE_START,
        RadarReporter::DFX_SUCCESS, RadarReporter::CONCURRENT_ID, pasteId);
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    proxyService->PasteStart(pasteId);
}

void PasteboardClient::PasteComplete(const std::string &deviceId, const std::string &pasteId)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    proxyService->PasteComplete(deviceId, pasteId);
}

int32_t PasteboardClient::HandleSignalValue(const std::string &signalValue)
{
    int32_t progressStatusValue = 0;
    std::shared_ptr<ProgressReportListener> progressReport = std::make_shared<ProgressReportListener>();
    progressReport->OnProgressFail = OnProgressAbnormal;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(signalValue.size() <= MAX_SIGNAL_VALUE_SIZE,
        static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), PASTEBOARD_MODULE_CLIENT,
        "progress invalid signalValue: %{public}s", signalValue.c_str());
    static const std::regex numberRegex(R"(^[+-]?\d+$)");
    if (!std::regex_match(signalValue, numberRegex)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "progressStatusValue invalid = %{public}s", signalValue.c_str());
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    auto ret = std::from_chars(signalValue.data(), signalValue.data() + signalValue.size(), progressStatusValue);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret.ec == std::errc(),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_CLIENT,
        "progress invalid status: %{public}s", signalValue.c_str());

    if (progressStatusValue == NORMAL_PASTE) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }
    if (progressStatusValue == CANCEL_PASTE) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "progress cancel paste");
        ProgressSignalClient::GetInstance().Cancel();
    } else if (progressStatusValue == PASTE_TIME_OUT) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "progress time out");
        progressReport->OnProgressFail(static_cast<int32_t>(PasteboardError::PROGRESS_PASTE_TIME_OUT));
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "progress invalid status: %{public}s", signalValue.c_str());
        progressReport->OnProgressFail(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardClient::ShowProgress(const std::string &progressKey)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(proxyService != nullptr, PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    sptr<PasteboardSignalCallback> callback = new PasteboardSignalCallback();
    proxyService->ShowProgress(progressKey, callback);
}

int32_t PasteboardClient::SyncDelayedData()
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    return proxyService->SyncDelayedData();
}

std::string PasteboardClient::GetPasteDataInfoSummary(const PasteData &pasteData)
{
    // Deal with pasteData info
    json RadarReportInfoInJson = {
        {"PasteBundle", pasteData.GetBundleName().empty() ? "/" : pasteData.GetBundleName()},
        {"PasteDataSize", pasteData.CountTLV()},
        {"RecordCount", pasteData.GetRecordCount()},
        {"IsRemote", pasteData.IsRemote()},
        {"IsDelayData", pasteData.IsDelayData()},
        {"IsDelayRecord", pasteData.IsDelayRecord()}
    };

    // Deal with Records info
    RadarReportInfoInJson["recordList"] = json::array();
    for (size_t i = 0; i < pasteData.GetRecordCount(); ++i) {
        // set a record display upperbound
        if (i >= RECORD_DISPLAY_UPPERBOUND) {
            break;
        }
        auto record = pasteData.GetRecordAt(i);
        if (record == nullptr) {
            break;
        }
        json recordInfo = {
            {"PrimaryType", record->GetMimeType()},
            {"MimeTypes", record->GetMimeTypes()}
        };
        RadarReportInfoInJson["recordList"].emplace_back(recordInfo);
    }

    // To string and return
    return RadarReportInfoInJson.dump(JSON_INDENT);
}

int32_t PasteboardClient::ConvertErrCode(int32_t errCode)
{
    switch (errCode) {
        case ERR_INVALID_VALUE: // fall-through
        case ERR_INVALID_DATA:
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        case ERR_OK:
            return static_cast<int32_t>(PasteboardError::E_OK);
        default:
            return errCode;
    }
}
} // namespace MiscServices
} // namespace OHOS
