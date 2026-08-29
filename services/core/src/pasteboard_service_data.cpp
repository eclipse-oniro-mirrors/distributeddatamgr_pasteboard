/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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
#include "pasteboard_service.h"

#include <dlfcn.h>
#include <sys/mman.h>

#include "ashmem.h"
#include "accesstoken_kit.h"
#include "account_manager.h"
#include "calculate_time_consuming.h"
#include "common_event_manager.h"
#include "device/dev_profile.h"
#include "distributed_file_daemon_manager.h"
#ifdef WITH_DLP
#include "dlp_permission_kit.h"
#include "dlp_permission.h"
#endif // WITH_DLP
#include "eventcenter/pasteboard_event.h"
#include "fd_san.h"
#include "hiview_adapter.h"
#include "input_method_controller.h"
#include "ipasteboard_changed_observer.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "mem_mgr_client.h"
#include "message_parcel_warp.h"
#include "os_account_manager.h"
#include "parameters.h"
#include "pasteboard_ability_manager.h"
#include "pasteboard_common.h"
#include "common/pasteboard_common_utils.h"
#include "pasteboard_delay_manager.h"
#include "pasteboard_dialog.h"
#include "pasteboard_disposable_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "paste_data_info.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_pattern.h"
#include "pasteboard_mime_utils.h"
#include "pasteboard_time.h"
#include "pasteboard_trace.h"
#include "pasteboard_web_controller.h"
#include "permission/permission_utils.h"
#include "remote_file_share.h"
#include "res_sched_client.h"
#include "reporter.h"
#include "distributed_module_config.h"
#include "file_mount_manager.h"
#ifdef PB_SCREENLOCK_MGR_ENABLE
#include "screenlock_manager.h"
#endif // PB_SCREENLOCK_MGR_ENABLE
#include "tokenid_kit.h"
#include "uri_permission_manager_client.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif // SCENE_BOARD_ENABLE
#ifdef PB_COCKPIT_PLATFORM_ENABLE
#include "pasteboard_subprofile_subscriber.h"
#include "os_account_subprofile_client.h"
#endif // PB_COCKPIT_PLATFORM_ENABLE

namespace OHOS {
namespace MiscServices {
using namespace Rosen;
using namespace std::chrono;
using namespace Storage::DistributedFile;
using namespace RadarReporter;
using namespace UeReporter;
namespace {
constexpr const char *PASTEBOARD_SERVICE_SA_NAME = "pasteboard_service";
constexpr const char *SECURE_PASTE_PERMISSION = "ohos.permission.SECURE_PASTE";
constexpr const char *READ_PASTEBOARD_PERMISSION = "ohos.permission.READ_PASTEBOARD";
constexpr const char *GET_DATA_APP = "GET_DATA_APP";
constexpr const char *COVER_DELAY_DATA = "COVER_DELAY_DATA";
constexpr const char *UE_COPY = "DISTRIBUTED_PASTEBOARD_COPY";
constexpr const char *UE_PASTE = "DISTRIBUTED_PASTEBOARD_PASTE";
constexpr int32_t E_OK_OPERATION = 0;
constexpr float RECALCULATE_DATA_SIZE = 0.9;
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
void PasteboardService::ReportUeCopyEvent(PasteData &pasteData, int64_t dataSize, int32_t result)
{
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    auto res = (result == static_cast<int32_t>(PasteboardError::E_OK)) ? E_OK_OPERATION : result;
    UeReportInfo reportInfo;
    reportInfo.ret = res;
    reportInfo.dataType = pasteData.GenerateDataType();
    reportInfo.bundleName = appInfo.bundleName;
    reportInfo.description = pasteData.GetReportDescription();
    reportInfo.commonInfo = GetCommonState(dataSize);
    reportInfo.timestamp = pasteData.GetProperty().timestamp;
    UE_REPORT(UE_COPY, reportInfo,
        "RECORD_NUM", reportInfo.description.recordNum,
        "DATA_SIZE", reportInfo.commonInfo.dataSize,
        "CURRENT_ACCOUNT_ID", reportInfo.commonInfo.currentAccountId,
        "ENTRY_NUM", reportInfo.description.entryNum,
        "MIMETYPES", reportInfo.description.mimeTypes,
        "DATA_TIMESTAMP", reportInfo.timestamp);
}

int32_t PasteboardService::ProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo,
    PasteDataEntry &entry)
{
    const auto &targetBundle = targetAppInfo.bundleName;
    const auto &appIndex = targetAppInfo.appIndex;
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (!PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, targetAppInfo.userId)) {
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }

    PasteData tmp;
    bool isRemoteData = data.IsRemote();
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    tmp.SetRemote(isRemoteData);
    SetLocalPasteFlag(tmp.IsRemote(), targetAppInfo.tokenId, tmp);
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, targetAppInfo.userId);
    PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
    PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);

    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(tmp, std::make_pair(targetBundle, appIndex));
    int32_t ret = GrantUriPermission(grantUris, targetAppInfo.tokenId, isRemoteData);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "grant to %{public}s:%{public}d failed, ret=%{public}d", targetBundle.c_str(),
        appIndex, ret);

    return PostProcessDelayHtmlEntry(tmp, targetAppInfo, entry);
}

int32_t PasteboardService::PostProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo,
    PasteDataEntry &entry)
{
    PasteboardWebController::GetInstance().RetainUri(data);
    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    PasteboardWebController::GetInstance().RebuildWebviewPasteData(data, targetAppInfo.bundleName,
        targetAppInfo.appIndex);

    std::shared_ptr<std::string> html = data.GetPrimaryHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::REBUILD_HTML_FAILED),
        PASTEBOARD_MODULE_SERVICE, "rebuild html failed");

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(*html);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::HTML_CONTENT] = *html;
        entry.SetValue(newObject);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData &pasteData)
{
    pasteData.SetLocalPasteFlag(!isCrossPaste && tokenId == pasteData.GetTokenId());
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "isLocalPaste = %{public}d.", pasteData.IsLocalPaste());
}

int32_t PasteboardService::ShowProgress(const std::string &progressKey, const sptr<IRemoteObject> &observer)
{
    if (!HasPasteData()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "not pastedata, no need to show progress.");
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (!IsFocusedApp(tokenId)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "not focused app, no need to show progress.");
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    }
    PasteboardDialog::ProgressMessageInfo message;
    std::string deviceName = "";
    bool isRemote = false;
    auto result = (GetRemoteDeviceName(deviceName, isRemote) == ERR_OK);
    if (result && isRemote) {
        message.promptText = "PromptText_PasteBoard_Remote";
        message.remoteDeviceName = deviceName;
    } else {
        message.promptText = "PromptText_PasteBoard_Local";
        message.remoteDeviceName = "";
    }
    message.isRemote = isRemote;
    message.progressKey = progressKey;

    FocusedAppInfo appInfo = GetFocusedAppInfo();
    message.left = appInfo.left;
    message.top = appInfo.top;
    message.width = static_cast<int32_t>(appInfo.width);
    message.height = static_cast<int32_t>(appInfo.height);
    message.callerToken = appInfo.abilityToken;
    message.clientCallback = observer;
    PasteboardDialog::ShowProgress(message);
    return ERR_OK;
}

bool PasteboardService::WriteRawData(const void *data, int64_t size, int &serFd)
{
    MessageParcelWarp messageData;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(data != nullptr, false, PASTEBOARD_MODULE_SERVICE, "data is null");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(0 < size && size <= messageData.GetRawDataSize(), false,
        PASTEBOARD_MODULE_SERVICE, "size invalid, size:%{public}" PRId64, size);

    int fd = AshmemCreate("WriteRawData Ashmem", size);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(fd >= 0, false, PASTEBOARD_MODULE_SERVICE, "ashmem create failed");
    fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);

    int32_t result = AshmemSetProt(fd, PROT_READ | PROT_WRITE);
    if (result < 0) {
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ashmem set prot failed");
        return false;
    }
    void *ptr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "mmap failed, fd:%{public}d", fd);
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
        return false;
    }
    if (!messageData.MemcpyData(ptr, static_cast<size_t>(size), data, size)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "memcpy_s failed, fd:%{public}d", fd);
        ::munmap(ptr, size);
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
        return false;
    }
    ::munmap(ptr, size);
    serFd = fd;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Write data end. fd:%{public}d size:%{public}" PRId64, serFd, size);
    return true;
}

CommonInfo PasteboardService::GetCommonState(int64_t dataSize)
{
    CommonInfo commonInfo;
    commonInfo.currentAccountId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    commonInfo.deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    commonInfo.dataSize = dataSize;
    return commonInfo;
}

void PasteboardService::SetRadarEvent(const AppInfo &appInfo, PasteData &data, bool isPeerOnline,
    RadarReportInfo &radarReportInfo, const std::string &peerNetId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(peerNetId, remoteDevice);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        DeviceManager::GetInstance().GetNetworkTypeByNetworkId(PASTEBOARD_SERVICE_SA_NAME, peerNetId,
            radarReportInfo.pasteInfo.networkType);
    }
#endif
    std::string peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
    radarReportInfo.stageRes = DFX_SUCCESS;
    radarReportInfo.bundleName = appInfo.bundleName;
    radarReportInfo.description = data.GetReportDescription();
    radarReportInfo.pasteInfo.onlineDevNum = DMAdapter::GetInstance().GetDeviceNum();
    radarReportInfo.pasteInfo.peerNetId = PasteboardDfxUntil::GetAnonymousID(peerNetId);
    radarReportInfo.pasteInfo.peerUdid = PasteboardDfxUntil::GetAnonymousID(peerUdid);
    radarReportInfo.pasteInfo.peerBundleName = data.GetOriginAuthority().first;
    radarReportInfo.pasteInfo.isPeerOnline = isPeerOnline;
}

void PasteboardService::SetUeEvent(const AppInfo &appInfo, PasteData &data, bool isPeerOnline,
    UeReportInfo &ueReportInfo, const std::string &peerNetId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(peerNetId, remoteDevice);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        DeviceManager::GetInstance().GetNetworkTypeByNetworkId(PASTEBOARD_SERVICE_SA_NAME, peerNetId,
            ueReportInfo.pasteInfo.networkType);
    }
#endif
    ueReportInfo.bundleName = appInfo.bundleName;
    ueReportInfo.dataType = data.GenerateDataType();
    ueReportInfo.pasteInfo.peerBundleName = data.GetOriginAuthority().first;
    ueReportInfo.pasteInfo.isDistributed = data.IsRemote();
    ueReportInfo.pasteInfo.isPeerOnline = isPeerOnline;
    ueReportInfo.pasteInfo.onlineDevNum = DMAdapter::GetInstance().GetDeviceNum();
    ueReportInfo.description = data.GetReportDescription();
    ueReportInfo.timestamp = data.GetProperty().timestamp;
}

int32_t PasteboardService::GetPasteData(int &fd, int64_t &size, std::vector<uint8_t> &rawData,
    uint32_t pasteSeqId, int32_t &syncTime, int32_t &realErrCode)
{
    fd = -1;
    std::string pasteId = PasteData::CreatePasteId("GetPasteData", pasteSeqId, IPCSkeleton::GetCallingPid());
    UeReportInfo ueReportInfo;
    int32_t ret = GetPasteDataInner(fd, size, rawData, pasteId, syncTime, ueReportInfo);
    if (fd == -1) {
        fd = AshmemCreate("GetPasteData Ashmem", 1);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(fd >= 0, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "ashmem create failed");
        fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);
    }
    ueReportInfo.ret = (ret == static_cast<int32_t>(PasteboardError::E_OK) ? E_OK_OPERATION : ret);
    ueReportInfo.commonInfo = GetCommonState(size);
    UE_REPORT(UE_PASTE, ueReportInfo,
        "IS_DISTRIBUTED_PASTEBOARD", ueReportInfo.pasteInfo.isDistributed,
        "RECORD_NUM", ueReportInfo.description.recordNum,
        "DATA_SIZE", ueReportInfo.commonInfo.dataSize,
        "CURRENT_ACCOUNT_ID", ueReportInfo.commonInfo.currentAccountId,
        "PEER_BUNDLE_NAME", ueReportInfo.pasteInfo.peerBundleName,
        "IS_PEER_ONLINE", ueReportInfo.pasteInfo.isPeerOnline,
        "ONLINE_DEV_NUM", ueReportInfo.pasteInfo.onlineDevNum,
        "NETWORK_TYPE", ueReportInfo.pasteInfo.networkType,
        "ENTRY_NUM", ueReportInfo.description.entryNum,
        "MIMETYPES", ueReportInfo.description.mimeTypes,
        "DATA_TIMESTAMP", ueReportInfo.timestamp);
    realErrCode = ret;
    return ERR_OK;
}

int32_t PasteboardService::GetPasteDataInner(int &fd, int64_t &size, std::vector<uint8_t> &rawData,
    const std::string &pasteId, int32_t &syncTime, UeReportInfo &ueReportInfo)
{
    PasteboardTrace tracer("PasteboardService GetPasteData");
    PasteData data{};
    data.SetPasteId(pasteId);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = GetAppInfo(tokenId);
    bool developerMode = OHOS::system::GetBoolParameter("const.security.developermode.state", false);
    bool isTestServerSetPasteData = developerMode && setPasteDataUId_.load() == TEST_SERVER_UID;
    if (!VerifyPermission(tokenId) && !isTestServerSetPasteData) {
        RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_CHECK_GET_AUTHORITY, DFX_SUCCESS, GET_DATA_APP, appInfo.bundleName,
            RadarReporter::CONCURRENT_ID, data.GetPasteId());
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "check permission failed, callingPid is %{public}d", callPid);
        HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE,
            static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    bool isPeerOnline = false;
    std::string peerNetId = "";
    std::string peerUdid = "";
    RadarReportInfo radarReportInfo;
    radarReportInfo.pasteInfo.pasteId = data.GetPasteId();
    auto ret = GetData(tokenId, data, syncTime, isPeerOnline, peerNetId, peerUdid);
    data.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    SetUeEvent(appInfo, data, isPeerOnline, ueReportInfo, peerNetId);
    SetRadarEvent(appInfo, data, isPeerOnline, radarReportInfo, peerNetId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "data is invalid, ret is %{public}d, callPid is %{public}d, tokenId is %{public}d", ret, callPid, tokenId);
        HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE, ret);
        radarReportInfo.commonInfo = GetCommonState(-1);
        PASTE_RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_GET_DATA_INFO, radarReportInfo);
        return ret;
    }
    delayDataId_ = data.GetDataId();
    delayTokenId_ = tokenId;

    ret = DealData(fd, size, rawData, data);
    radarReportInfo.commonInfo = GetCommonState(size);
    PASTE_RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_GET_DATA_INFO, radarReportInfo);
    return ret;
}

int32_t PasteboardService::DealData(int &fd, int64_t &size, std::vector<uint8_t> &rawData, PasteData &data)
{
    std::vector<uint8_t> pasteDataTlv(0);
    {
        std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
        if (!data.Encode(pasteDataTlv)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to encode pastedata in TLV");
            HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE, ERR_INVALID_VALUE);
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
    }
    int64_t tlvSize = static_cast<int64_t>(pasteDataTlv.size());
    int serviceFd = -1;
    if (tlvSize > MIN_ASHMEM_DATA_SIZE) {
        bool res = WriteRawData(pasteDataTlv.data(), tlvSize, serviceFd);
        if (!res) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to WriteRawData:%{public}" PRId64, tlvSize);
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
        pasteDataTlv.clear();
    } else {
        serviceFd = AshmemCreate("DealData Ashmem", 1);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(serviceFd >= 0,
            static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "create fd failed");
        fdsan_exchange_owner_tag(serviceFd, 0, PASTEBOARD_FD_TAG);
    }
    size = tlvSize;
    fd = serviceFd;
    rawData = std::move(pasteDataTlv);
    HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE, ERR_OK);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "DealData fd:%{public}d, size:%{public}" PRId64, fd, size);
    return ERR_OK;
}

void PasteboardService::AddPermissionRecord(uint32_t tokenId, bool isReadGrant, bool isSecureGrant)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return;
    }
    bool isGrant = isReadGrant || isSecureGrant;
    if (!isGrant) {
        return;
    }
    auto permUsedType = static_cast<PermissionUsedType>(AccessTokenKit::GetPermissionUsedType(
        tokenId, isSecureGrant ? SECURE_PASTE_PERMISSION : READ_PASTEBOARD_PERMISSION));
    AddPermParamInfo info;
    info.tokenId = tokenId;
    info.permissionName = READ_PASTEBOARD_PERMISSION;
    info.successCount = 1;
    info.failCount = 0;
    info.type = permUsedType;
    int32_t result = PrivacyKit::AddPermissionUsedRecord(info);
    if (result != RET_SUCCESS) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "add record failed, result is %{public}d", result);
    }
    return;
}

int32_t PasteboardService::GetData(uint32_t tokenId, PasteData &data, int32_t &syncTime, bool &isPeerOnline,
    std::string &peerNetId, std::string &peerUdid)
{
    CalculateTimeConsuming::SetBeginTime();
    auto appInfo = GetAppInfo(tokenId);
    int32_t result = static_cast<int32_t>(PasteboardError::E_OK);
    std::string pasteId = data.GetPasteId();
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = nullptr;
    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
    if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
        auto isPasting = taskMgr_.IsRemoteDataPasting(distEvt);
        if (isPasting) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "wait remote data, seqId=%{public}u", distEvt.seqId);
            taskMgr_.WaitRemoteData(distEvt);
        }
    }
    if (distRet != static_cast<int32_t>(PasteboardError::E_OK) ||
        GetScreenStatus(appInfo.userId) != ScreenEvent::ScreenUnlocked) {
        auto currentEvent = GetCurrentEvent();
        pasteBlock = EstablishP2PLinkTask(pasteId, currentEvent);
        result = GetLocalData(appInfo, data);
        if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
            peerNetId = currentEvent.deviceId;
            peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
        }
    } else {
        pasteBlock = EstablishP2PLinkTask(pasteId, distEvt);
        result = GetRemoteData(appInfo.userId, distEvt, data, syncTime);
        if (result == static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED)) {
            HandleGetDataError(result, pasteBlock, distEvt.deviceId, pasteId);
            result = GetLocalData(appInfo, data);
        } else {
            peerNetId = distEvt.deviceId;
            peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
        }
    }
    HandleNotificationsAndStatusChecks(appInfo, data, peerNetId, isPeerOnline);
    PublishServiceState(data, syncTime, peerNetId, pasteBlock);

    if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
        HandleGetDataError(result, pasteBlock, distEvt.deviceId, pasteId);
        return result;
    }
    return CheckAndGrantRemoteUri(data, appInfo, pasteId, pasteBlock);
}

int32_t PasteboardService::GetLocalData(const AppInfo &appInfo, PasteData &data)
{
    std::string pasteId = data.GetPasteId();
    auto it = clips_.Find(appInfo.userId);
    auto tempTime = copyTime_.Find(appInfo.userId);
    if (!it.first || !tempTime.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "no data userId is %{public}d.", appInfo.userId);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    bool isDelayData = false;
    bool isDelayRecord = false;
    std::string originBundleName;
    {
        std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
        auto ret = IsDataValid(*(it.second), appInfo.tokenId, appInfo.userId);
        if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "paste data is invalid. ret = %{public}d "
                "appInfo.userId = %{public}d", ret, appInfo.userId);
            return ret;
        }
        data = *(it.second);
        originBundleName = it.second->GetBundleName();
        isDelayData = it.second->IsDelayData();
        isDelayRecord = it.second->IsDelayRecord();
    }
    if (isDelayData) {
        GetDelayPasteData(appInfo.userId, data);
        RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_CHECK_GET_DELAY_PASTE, DFX_SUCCESS, CONCURRENT_ID, pasteId);
    }
    if (isDelayRecord) {
        GetDelayPasteRecord(appInfo.userId, data);
    }
    data.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    auto result = copyTime_.Find(appInfo.userId);
    if (!result.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId not found userId is %{public}d", appInfo.userId);
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    auto curTime = result.second;
    UpdateClipOnRead(appInfo.userId, data, originBundleName, tempTime.second, curTime);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteData success. appInfo.userId = %{public}d", appInfo.userId);
    SetLocalPasteFlag(data.IsRemote(), appInfo.tokenId, data);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::UpdateClipOnRead(int32_t userId, const PasteData &data,
    const std::string &originBundleName, uint64_t startTime, uint64_t curTime)
{
    if (startTime != curTime) {
        return;
    }
    bool isNotify = false;
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        clips_.ComputeIfPresent(userId, [&data, &isNotify](auto &key, auto &value) {
            if (value->IsDelayData()) {
                value = std::make_shared<PasteData>(data);
                isNotify = true;
            }
            if (value->IsDelayRecord()) {
                value = std::make_shared<PasteData>(data);
            }
            return true;
        });
    }
    if (isNotify) {
        NotifyObservers(originBundleName, userId, PasteboardEventStatus::PASTEBOARD_WRITE);
    }
}

void PasteboardService::GetDelayPasteData(int32_t userId, PasteData &data)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "get delay data start");
    delayGetters_.ComputeIfPresent(userId, [this, &data, userId](auto, auto &delayGetter) {
        PasteData delayData;
        if (delayGetter.first != nullptr) {
            delayGetter.first->GetPasteData("", delayData);
        }
        if (delayGetter.second != nullptr && delayGetter.first != nullptr) {
            delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
        }
        delayData.SetDelayData(false);
        delayData.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
        delayData.SetOriginAuthority(data.GetOriginAuthority());
        delayData.SetTime(data.GetTime());
        delayData.SetTokenId(data.GetTokenId());
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(delayData, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(delayData, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(delayData);
        data = delayData;
        return false;
    });
}

int32_t PasteboardService::GetDelayPasteRecord(int32_t userId, PasteData &data)
{
    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    auto delayEntryInfos = DelayManager::GetPrimaryDelayEntryInfo(data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!delayEntryInfos.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no delay entry");
    DelayManager::GetLocalEntryValue(delayEntryInfos, getter.first, data);
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::SaveData(PasteData &pasteData, int64_t dataSize,
    const sptr<IPasteboardDelayGetter> delayGetter, const sptr<IPasteboardEntryGetter> entryGetter)
{
    PasteboardTrace tracer("PasteboardService, SetPasteData");
    auto tokenId = pasteData.GetTokenId();
    if (!IsCopyable(tokenId)) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_SET_AUTHORITY, DFX_SUCCESS);
        return static_cast<int32_t>(PasteboardError::PROHIBIT_COPY);
    }
    if (setting_.exchange(true)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "is setting.");
        return static_cast<int32_t>(PasteboardError::TASK_PROCESSING);
    }
    CalculateTimeConsuming::SetBeginTime();
    auto appInfo = GetAppInfo(tokenId);
    if (appInfo.userId == ERROR_USERID) {
        setting_.store(false);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    pasteData.userId_ = appInfo.userId;
    pasteData.deviceId_ = DMAdapter::GetInstance().GetLocalNetworkId();
    SetPasteDataInfo(pasteData, appInfo);
    auto authority = std::make_pair(appInfo.bundleName, appInfo.appIndex);
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authority);
    bool hasSplited = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex,
        appInfo.userId);
    PasteboardWebController::GetInstance().SetWebviewPasteData(pasteData, bundleIndex);
    PasteboardWebController::GetInstance().CheckAppUriPermission(pasteData);
    if (hasSplited || dataSize > static_cast<int64_t>(maxLocalCapacity_.load() * RECALCULATE_DATA_SIZE)) {
        int64_t newDataSize = static_cast<int64_t>(pasteData.Count());
        if (newDataSize > maxLocalCapacity_.load()) {
            setting_.store(false);
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid data size, dataSize=%{public}" PRId64, newDataSize);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
        }
        pasteData.rawDataSize_ = newDataSize;
    }
    setPasteDataUId_.store(IPCSkeleton::GetCallingUid());
    RemovePasteData(appInfo);
    clips_.InsertOrAssign(appInfo.userId, std::make_shared<PasteData>(pasteData));
    IncreaseChangeCount(appInfo.userId);
    RadarReportInfo radarReportInfo;
    radarReportInfo.stageRes = static_cast<int32_t>(pasteData.IsDelayData());
    radarReportInfo.bundleName = appInfo.bundleName;
    radarReportInfo.description = pasteData.GetReportDescription();
    radarReportInfo.commonInfo = GetCommonState(dataSize);
    COPY_RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_SET_DELAY_COPY, radarReportInfo);
    HandleDelayDataAndRecord(pasteData, delayGetter, entryGetter, appInfo);
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    copyTime_.InsertOrAssign(appInfo.userId, curTime);
    SetDataExpirationTimer(appInfo.userId);
    if (!(pasteData.IsDelayData())) {
        SetDistributedData(appInfo.userId, pasteData);
        NotifyObservers(appInfo.bundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_WRITE);
    }
    SetPasteDataDot(pasteData, appInfo.userId);
    setting_.store(false);
    SubscribeKeyboardEvent();
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::SetPasteDataInfo(PasteData &pasteData, const AppInfo &appInfo)
{
    pasteData.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    pasteData.SetOriginAuthority(std::make_pair(appInfo.bundleName, appInfo.appIndex));
    pasteData.SetTime(GetTime());
    pasteData.SetScreenStatus(GetScreenStatus(appInfo.userId));
    auto dataId = ++dataId_;
    pasteData.SetDataId(dataId);
    for (auto &record : pasteData.AllRecords()) {
        record->SetDataId(dataId);
    }
    if (pasteData.GetRecordCount() != 0) {
        size_t counts = pasteData.GetRecordCount() - 1;
        std::shared_ptr<PasteDataRecord> records = pasteData.GetRecordAt(counts);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(records != nullptr, PASTEBOARD_MODULE_SERVICE,
            "records[%{public}d] is nullptr.", static_cast<int32_t>(counts));
        std::string text = records->ConvertToText();
        pasteData.SetTextSize(text.size());
    }
}

void PasteboardService::HandleDelayDataAndRecord(PasteData &pasteData, const sptr<IPasteboardDelayGetter> delayGetter,
    const sptr<IPasteboardEntryGetter> entryGetter, const AppInfo &appInfo)
{
    if (pasteData.IsDelayData() && delayGetter != nullptr) {
        sptr<DelayGetterDeathRecipient> deathRecipient = new (std::nothrow)
            DelayGetterDeathRecipient(appInfo.userId, *this);
        delayGetter->AsObject()->AddDeathRecipient(deathRecipient);
        delayGetters_.InsertOrAssign(appInfo.userId, std::make_pair(delayGetter, deathRecipient));
    }
    if (pasteData.IsDelayRecord() && entryGetter != nullptr) {
        sptr<EntryGetterDeathRecipient> deathRecipient = new (std::nothrow)
            EntryGetterDeathRecipient(appInfo.userId, *this);
        entryGetter->AsObject()->AddDeathRecipient(deathRecipient);
        entryGetters_.InsertOrAssign(appInfo.userId, std::make_pair(entryGetter, deathRecipient));
    }
}

bool PasteboardService::IsBasicType(const std::string &mimeType)
{
    if (mimeType == MIMETYPE_TEXT_HTML || mimeType == MIMETYPE_TEXT_PLAIN || mimeType == MIMETYPE_TEXT_URI ||
        mimeType == MIMETYPE_PIXELMAP || mimeType == MIMETYPE_AUTOFILL_SECURE) {
        return true;
    }
    return false;
}

void PasteboardService::CloseSharedMemFd(int fd)
{
    if (fd >= 0) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Close fd:%{public}d", fd);
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
    }
}

int32_t PasteboardService::WritePasteData(
    int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer, PasteData &pasteData, bool &hasData)
{
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        auto actualSize = AshmemGetSize(fd);
        if (actualSize < 0 || rawDataSize > actualSize) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "rawDataSize invalid, actualSize=%{public}d, rawDataSize:%{public}" PRId64, actualSize, rawDataSize);
            CloseSharedMemFd(fd);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
        }
        void *ptr = ::mmap(nullptr, rawDataSize, PROT_READ, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "mmap failed, size:%{public}" PRId64, rawDataSize);
            CloseSharedMemFd(fd);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
        }
        const uint8_t *rawData = reinterpret_cast<const uint8_t *>(ptr);
        if (rawData == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "rawData is nullptr, size:%{public}" PRId64, rawDataSize);
            ::munmap(ptr, rawDataSize);
            CloseSharedMemFd(fd);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
        }
        std::vector<uint8_t> pasteDataTlv(rawData, rawData + rawDataSize);
        hasData = pasteData.Decode(pasteDataTlv);
        ::munmap(ptr, rawDataSize);
    } else {
        hasData = pasteData.Decode(buffer);
    }
    CloseSharedMemFd(fd);
    pasteData.rawDataSize_ = rawDataSize;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set local data, dataSize=%{public}" PRId64, rawDataSize);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::RemovePasteData(const AppInfo &appInfo)
{
    delayGetters_.ComputeIfPresent(appInfo.userId, [](auto, auto &delayGetter) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_SET_DELAY_COPY, DFX_SUCCESS, COVER_DELAY_DATA, DFX_SUCCESS);
        if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
            delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
        }
        return false;
    });
    entryGetters_.ComputeIfPresent(appInfo.userId, [](auto, auto &entryGetter) {
        if (entryGetter.first != nullptr && entryGetter.second != nullptr) {
            entryGetter.first->AsObject()->RemoveDeathRecipient(entryGetter.second);
        }
        return false;
    });
}

void PasteboardService::SetPasteDataDot(PasteData &pasteData, const int32_t &userId)
{
    auto bundleName = pasteData.GetBundleName();
    HistoryInfo info{ pasteData.GetTime(), bundleName, "set", "", userId };
    SetPasteboardHistory(info);

    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData Report!");
    Reporter::GetInstance().PasteboardBehaviour().Report(
        { static_cast<int>(BehaviourPasteboardState::BPS_COPY_STATE), bundleName });

    int state = static_cast<int>(StatisticPasteboardState::SPS_COPY_STATE);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData GetTextSize!");
    size_t dataSize = pasteData.GetTextSize();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData timeC!");
    CalculateTimeConsuming timeC(dataSize, state);
}

void PasteboardService::GetPasteDataDot(PasteData &pasteData, const std::string &bundleName, const int32_t &userId)
{
    std::string remote;
    if (pasteData.IsRemote()) {
        remote = "remote";
    }
    std::string time = GetTime();
    HistoryInfo info{ time, bundleName, "get", remote, userId };
    SetPasteboardHistory(info);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetPasteData Report!");
    int pState = StatisticPasteboardState::SPS_INVALID_STATE;
    int bState = BehaviourPasteboardState::BPS_INVALID_STATE;
    if (pasteData.IsRemote()) {
        pState = static_cast<int>(StatisticPasteboardState::SPS_REMOTE_PASTE_STATE);
        bState = static_cast<int>(BehaviourPasteboardState::BPS_REMOTE_PASTE_STATE);
    } else {
        pState = static_cast<int>(StatisticPasteboardState::SPS_PASTE_STATE);
        bState = static_cast<int>(BehaviourPasteboardState::BPS_PASTE_STATE);
    };

    Reporter::GetInstance().PasteboardBehaviour().Report({ bState, bundleName });
    size_t dataSize = pasteData.GetTextSize();
    CalculateTimeConsuming timeC(dataSize, pState);
}
} // namespace MiscServices
} // namespace OHOS
