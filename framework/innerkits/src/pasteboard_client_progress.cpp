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

#include "pasteboard_client.h"

#include <charconv>
#include <iservice_registry.h>
#include <thread>
#include <regex>

#include "common/block_object.h"
#include "convert_utils.h"
#include "fd_san.h"
#include "ffrt/ffrt_utils.h"
#include "hitrace_meter.h"
#include "pasteboard_common.h"
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
constexpr int32_t HITRACE_GETPASTEDATA = 0;
std::string g_progressKey;
constexpr int32_t PASTEBOARD_PROGRESS_UPDATE_PERCENT = 5;
constexpr int32_t UPDATE_PERCENT_WITHOUT_FILE = 10;
constexpr int32_t PASTEBOARD_PROGRESS_TWENTY_PERCENT = 20;
constexpr int32_t PASTEBOARD_PROGRESS_FINISH_PERCENT = 100;
constexpr int32_t PASTEBOARD_PROGRESS_SLEEP_TIME = 100; // ms
constexpr int32_t SLEEP_TIME_WITHOUT_FILE = 50; // ms
constexpr int64_t REPORT_DUPLICATE_TIMEOUT = 2 * 60 * 1000; // 2 minutes
constexpr uint32_t MAX_SIGNAL_VALUE_SIZE = 128;
static constexpr int32_t HAP_PULL_UP_TIME = 500; // ms
static constexpr int32_t HAP_MIN_SHOW_TIME = 300; // ms
constexpr const char *ERROR_CODE = "ERROR_CODE";
constexpr const char *DIS_SYNC_TIME = "DIS_SYNC_TIME";
constexpr const char *PACKAGE_NAME = "PACKAGE_NAME";
constexpr const char *PASTEDATA_SUMMARY = "PASTEDATA_SUMMARY";

struct RadarReportIdentity {
    pid_t pid;
    int32_t errorCode;
};

inline bool operator==(const RadarReportIdentity &lhs, const RadarReportIdentity &rhs)
{
    return lhs.pid == rhs.pid && lhs.errorCode == rhs.errorCode;
}

void PasteboardClient::GetProgressByProgressInfo(std::shared_ptr<GetDataParams> params)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(params != nullptr, PASTEBOARD_MODULE_CLIENT, "params is null!");

    PASTEBOARD_CHECK_AND_RETURN_LOGE(params->info != nullptr, PASTEBOARD_MODULE_CLIENT, "params->info is null!");
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
    PASTEBOARD_CHECK_AND_RETURN_LOGE(params != nullptr, PASTEBOARD_MODULE_CLIENT, "params is null!");
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

void PasteboardClient::ProcessRadarReport(int32_t ret, PasteData &pasteData,
    PasteDataFromServiceInfo &pasteDataFromServiceInfo, int32_t syncTime)
{
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    static DeduplicateMemory<RadarReportIdentity> reportMemory(REPORT_DUPLICATE_TIMEOUT);
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    std::string currentIdStr = std::to_string(pasteDataFromServiceInfo.currentSeqId);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        if (pasteData.deviceId_.empty()) {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID,
                currentIdStr, PACKAGE_NAME, pasteDataFromServiceInfo.currentPid,
                DIS_SYNC_TIME, syncTime, PASTEDATA_SUMMARY,
                pasteDataInfoSummary);
        } else {
            RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_SUCCESS,
                RadarReporter::CONCURRENT_ID, currentIdStr, PACKAGE_NAME,
                pasteDataFromServiceInfo.currentPid, DIS_SYNC_TIME, syncTime,
                PASTEDATA_SUMMARY, pasteDataInfoSummary);
        }
    } else if (ret != static_cast<int32_t>(PasteboardError::TASK_PROCESSING) &&
               !reportMemory.IsDuplicate({.pid = pasteDataFromServiceInfo.pid, .errorCode = ret})) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_FAILED, RadarReporter::BIZ_STATE,
            RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, currentIdStr,
            PACKAGE_NAME, pasteDataFromServiceInfo.currentPid, DIS_SYNC_TIME, syncTime,
            ERROR_CODE, ret, PASTEDATA_SUMMARY, pasteDataInfoSummary);
    } else {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, bizStage, RadarReporter::DFX_CANCELLED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID,
            currentIdStr, PACKAGE_NAME, pasteDataFromServiceInfo.currentPid,
            DIS_SYNC_TIME, syncTime, ERROR_CODE, ret, PASTEDATA_SUMMARY,
            pasteDataInfoSummary);
    }
}

void PasteboardClient::ProgressRadarReport(PasteData &pasteData, PasteDataFromServiceInfo &pasteDataFromServiceInfo)
{
    pasteDataFromServiceInfo.pid = getpid();
    pasteDataFromServiceInfo.currentPid = std::to_string(pasteDataFromServiceInfo.pid);
    pasteDataFromServiceInfo.currentSeqId = getSequenceId_++;
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_GET_BIZ_SCENE, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_BEGIN,
        RadarReporter::CONCURRENT_ID, std::to_string(pasteDataFromServiceInfo.currentSeqId),
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
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(params != nullptr,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR),
        PASTEBOARD_MODULE_CLIENT, "Invalid param!");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!isPasting_,
        static_cast<int32_t>(PasteboardError::TASK_PROCESSING),
        PASTEBOARD_MODULE_CLIENT, "task copying!");
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

int32_t PasteboardClient::HandleSignalValue(const std::string &signalValue)
{
    int32_t progressStatusValue = 0;
    std::shared_ptr<ProgressReportListener> progressReport = std::make_shared<ProgressReportListener>();
    progressReport->OnProgressFail = OnProgressAbnormal;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(signalValue.size() <= MAX_SIGNAL_VALUE_SIZE,
        static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), PASTEBOARD_MODULE_CLIENT,
        "progress invalid signalValue: %{public}s", signalValue.c_str());
    static const std::regex numberRegex(R"(^[+-]?\d+$)");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(std::regex_match(signalValue, numberRegex),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR),
        PASTEBOARD_MODULE_CLIENT, "progressStatusValue invalid = %{public}s", signalValue.c_str());
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

} // namespace MiscServices
} // namespace OHOS
