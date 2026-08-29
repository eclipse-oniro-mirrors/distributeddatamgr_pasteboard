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
constexpr int64_t REPORT_DUPLICATE_TIMEOUT = 2 * 60 * 1000; // 2 minutes
constexpr uint32_t JSON_INDENT = 4;
constexpr uint32_t RECORD_DISPLAY_UPPERBOUND = 3;
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

int32_t PasteboardClient::GetPasteData(PasteData &pasteData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "enter");
    pid_t pid = getpid();
    std::string currentPid = std::to_string(pid);
    uint32_t seqId = getSequenceId_++;
    std::string currentId = std::to_string(seqId);
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
    int32_t ret = proxyService->GetPasteData(fd, rawDataSize, recvTLV, seqId, syncTime, realErrCode);
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    ret = ConvertErrCode(realErrCode);
    int32_t result = ProcessPasteData<PasteData>(pasteData, rawDataSize, fd, recvTLV);
    PasteboardWebController::GetInstance().RetainUri(pasteData);
    PasteboardWebController::GetInstance().RemoveInvalidUri(pasteData);
    PasteboardWebController::GetInstance().RebuildWebviewPasteData(pasteData);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        GetDataReport(pasteData, syncTime, seqId, currentPid, ret);
        return ret;
    } else if (result == static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR)) {
        GetDataReport(pasteData, syncTime, seqId, currentPid, result);
        return result;
    }
    GetDataReport(pasteData, syncTime, seqId, currentPid, ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardClient::GetDataReport(PasteData &pasteData, int32_t syncTime, uint32_t currentSeqId,
    const std::string &currentPid, int32_t ret)
{
    static DeduplicateMemory<RadarReportIdentity> reportMemory(REPORT_DUPLICATE_TIMEOUT);
    int32_t bizStage = (syncTime == 0) ? RadarReporter::DFX_LOCAL_PASTE_END : RadarReporter::DFX_DISTRIBUTED_PASTE_END;
    FinishAsyncTrace(HITRACE_TAG_MISC, "PasteboardClient::GetPasteData", HITRACE_GETPASTEDATA);
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    std::string currentId = std::to_string(currentSeqId);
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

int32_t PasteboardClient::GetPasteDataFromService(PasteData &pasteData,
    PasteDataFromServiceInfo &pasteDataFromServiceInfo, std::string progressKey, std::shared_ptr<GetDataParams> params)
{
    auto proxyService = GetPasteboardService();
    std::string pasteDataInfoSummary = GetPasteDataInfoSummary(pasteData);
    if (proxyService == nullptr) {
        RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_CHECK_GET_SERVER, RadarReporter::DFX_FAILED,
            RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID,
            std::to_string(pasteDataFromServiceInfo.currentSeqId), PACKAGE_NAME, pasteDataFromServiceInfo.currentPid,
            ERROR_CODE, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
            PASTEDATA_SUMMARY, pasteDataInfoSummary);
        return static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR);
    }
    int32_t syncTime = 0;
    int32_t realErrCode = 0;
    int fd = -1;
    int64_t rawDataSize = 0;
    std::vector<uint8_t> recvTLV(0);
    uint32_t pasteSeqId = pasteDataFromServiceInfo.currentSeqId;
    int32_t ret = proxyService->GetPasteData(fd, rawDataSize, recvTLV, pasteSeqId, syncTime, realErrCode);
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
    fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);
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
