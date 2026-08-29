/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
constexpr int32_t MAX_REMOTE_FILE_MANAGER_URI_COUNT = 256;
constexpr const char *FILE_DOCS_URI_PREFIX = "file://docs/";
constexpr const char *FILEMANAGER_KEY = "filemanager";
constexpr int32_t SET_VALUE_SUCCESS = 1;
constexpr uid_t ANCO_SERVICE_BROKER_UID = 5557;
constexpr float RECALCULATE_DATA_SIZE = 0.9;
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;

void PasteboardService::ClearP2PEstablishTaskInfo()
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pEstablishInfo_.networkId.clear();
    p2pEstablishInfo_.pasteBlock = nullptr;
}

void PasteboardService::OpenP2PLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
#endif
    auto plugin = GetClipPlugin();
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
    int32_t status = plugin->ApplyAdvancedResource(networkId);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == RESULT_OK, PASTEBOARD_MODULE_SERVICE,
        "apply resource failed, deviceId=%{public}.5s, status=%{public}d", networkId.c_str(), status);

    status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == RESULT_OK, PASTEBOARD_MODULE_SERVICE,
        "publish CONNECT_SUCC failed, deviceId=%{public}.5s, status=%{public}d", networkId.c_str(), status);

#ifdef PB_DEVICE_MANAGER_ENABLE
    status = DistributedFileDaemonManager::GetInstance().ConnectDfs(networkId);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status:%{public}d", status);
        plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
#endif
}

void PasteboardService::EstablishP2PLink(const std::string &networkId, const std::string &pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Compute(networkId, [pasteId, callPid](const auto &key, auto &value) {
            value.Compute(pasteId, [callPid](const auto &key, auto &value) {
                value.callPid = callPid;
                value.isSuccess = false;
                return true;
            });
            return true;
        });
    }
    if (ffrtTimer_) {
        FFRTTask task = [this, networkId, pasteId] {
            std::thread thread([=]() {
                PasteComplete(networkId, pasteId);
            });
            PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete01");
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    OpenP2PLink(networkId);
#endif
}

std::shared_ptr<BlockObject<int32_t>> PasteboardService::CheckAndReuseP2PLink(
    const std::string &networkId, const std::string &pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [pasteId, callPid](const auto &key, auto &value) {
        value.Compute(pasteId, [callPid](const auto &key, auto &value) {
            value.callPid = callPid;
            value.isSuccess = false;
            return true;
        });
        return true;
    });
    if (ffrtTimer_) {
        FFRTTask task = [this, networkId, pasteId] {
            std::thread thread([=]() {
                PasteComplete(networkId, pasteId);
            });
            PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete02");
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    auto p2pNetwork = p2pMap_.Find(networkId);
    bool isP2pSuccess = p2pNetwork.first && p2pNetwork.second.Find(P2P_PRESYNC_ID).first &&
        p2pNetwork.second.Find(P2P_PRESYNC_ID).second.isSuccess == true;
    if (isP2pSuccess) {
        if (ffrtTimer_) {
            std::string taskName = P2P_PRESYNC_ID + networkId;
            ffrtTimer_->CancelTimer(taskName);
        }
        p2pMap_.ComputeIfPresent(networkId, [this](const auto &key, auto &value) {
            value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
                return false;
            });
            return true;
        });
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "No Need P2pEstablish");
        std::shared_ptr<BlockObject<int32_t>> result = nullptr;
        auto p2pIter = preSyncP2pMap_.find(networkId);
        if (p2pIter != preSyncP2pMap_.end()) {
            result = p2pIter->second;
        }
        preSyncP2pMap_.erase(networkId);
        return result;
    }
    return nullptr;
#else
    return nullptr;
#endif
}

bool PasteboardService::IsContainUri(const Event &evt)
{
    if (evt.notNeedLink && !evt.isDelay) {
        return false;
    }
    std::vector<std::string> keyVecs;
    keyVecs.push_back(MIMETYPE_TEXT_URI);
    keyVecs.push_back(MIMETYPE_TEXT_HTML);
    bool result = std::any_of(keyVecs.begin(), keyVecs.end(), [dataType = evt.dataType](const std::string &key) {
        return std::find(dataType.begin(), dataType.end(), key) != dataType.end();
    });
    return result;
}

void PasteboardService::OnEstablishP2PLinkTask(const std::string &networkId,
    std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteBlock != nullptr, PASTEBOARD_MODULE_SERVICE, "block is nullptr");
    OpenP2PLink(networkId);
    pasteBlock->SetValue(SET_VALUE_SUCCESS);
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    auto findResult = p2pMap_.Find(networkId);
    if (!findResult.first || findResult.second.Empty()) {
        CloseP2PLink(networkId);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "P2pEstablish Finish");
}

std::shared_ptr<BlockObject<int32_t>> PasteboardService::EstablishP2PLinkTask(
    const std::string &pasteId, const ClipPlugin::GlobalEvent &event)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    const std::string &networkId = event.deviceId;
    if (networkId.empty() || networkId == DMAdapter::GetInstance().GetLocalNetworkId()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "local device");
        return nullptr;
    }
    if (!IsContainUri(event)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "no MIMETYPE_TEXT_URI and no MIMETYPE_TEXT_HTML");
        return nullptr;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLinkTask enter");
    std::shared_ptr<BlockObject<int32_t>> result = CheckAndReuseP2PLink(networkId, pasteId);
    if (result) {
        return result;
    }
    if (!ffrtTimer_) {
        return nullptr;
    }
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
    if (!pasteBlock) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc BlockObject");
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pEstablishInfo_.networkId = networkId;
        p2pEstablishInfo_.pasteBlock = pasteBlock;
    }
    FFRTTask p2pTask = [networkId, pasteBlock, this] {
        std::thread thread([=]() {
            OnEstablishP2PLinkTask(networkId, pasteBlock);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "OnEstablishP2P");
        thread.detach();
    };
    std::string taskName = pasteId + P2P_ESTABLISH_STR;
    ffrtTimer_->SetTimer(taskName, p2pTask);
    return pasteBlock;
#else
    return nullptr;
#endif
}

void PasteboardService::CloseP2PLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseP2PLink enter");
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
        return;
    }
    std::thread thread([networkId]() {
        auto status = DistributedFileDaemonManager::GetInstance().DisconnectDfs(networkId);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "close p2p error, status:%{public}d", status);
        }
    });
    thread.detach();
    auto plugin = GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(plugin != nullptr, PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
    auto status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state idle error, status:%{public}d", status);
    }
#endif
}

int32_t PasteboardService::PasteStart(const std::string &pasteId)
{
    if (ffrtTimer_) {
        ffrtTimer_->CancelTimer(pasteId);
    }
    return ERR_OK;
}

int32_t PasteboardService::PasteComplete(const std::string &deviceId, const std::string &pasteId)
{
    if (deviceId.empty()) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "deviceId is empty");
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "deviceId is %{public}.6s, taskId is %{public}s", deviceId.c_str(),
        pasteId.c_str());
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_DISTRIBUTED_FILE_END, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, pasteId);
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(deviceId, [pasteId, deviceId, this](const auto &key, auto &value) {
        value.ComputeIfPresent(pasteId, [deviceId](const auto &key, auto &value) {
            return false;
        });
        if (value.Empty()) {
            CloseP2PLink(deviceId);
            return false;
        }
        return true;
    });
    return ERR_OK;
}

int32_t PasteboardService::GetRemoteDeviceName(std::string &deviceName, bool &isRemote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    auto event = GetValidDistributeEvent(appInfo.userId);
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    if (!event.second.deviceId.empty()) {
        auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(event.second.deviceId, remoteDevice);
        if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
            return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
        }
        deviceName = remoteDevice.deviceName;
        isRemote = true;
    } else {
        deviceName = "local";
        isRemote = false;
    }
#endif
    if (deviceName.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to get remote device name");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    return ERR_OK;
}

void PasteboardService::RemoveInvalidRemoteUri(std::vector<Uri> &grantUris)
{
    auto newEnd = std::remove_if(grantUris.begin(), grantUris.end(),
        [](const Uri& uri) {
            std::string puri = uri.ToString();
            return puri.find("networkid=") == std::string::npos;
        });
    grantUris.erase(newEnd, grantUris.end());
}

bool PasteboardService::IsFileManagerApp(const std::string &bundleName)
{
    auto fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    return IsSystemAppByFullTokenID(fullTokenId) && bundleName.find(FILEMANAGER_KEY) != std::string::npos;
}

bool PasteboardService::StartWith(const std::string &str, const std::string &prefix)
{
    if (prefix.size() > str.size()) {
        return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
}

int32_t PasteboardService::CheckRemoteFileDocsUriLimit(const std::vector<Uri> &grantUris, const std::string &bundleName)
{
    if (IsFileManagerApp(bundleName) || grantUris.size() <= MAX_REMOTE_FILE_MANAGER_URI_COUNT) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }
    for (const auto &uri : grantUris) {
        if (StartWith(uri.ToString(), FILE_DOCS_URI_PREFIX)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "remote uri count %{public}zu bundleName is %{public}s", grantUris.size(), bundleName.c_str());
            return static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED);
        }
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId)
{
    size_t offset = 0;
    size_t length = grantUris.size();
    size_t count = PasteData::URI_BATCH_SIZE;
    bool hasGranted = false;
    int32_t permissionCode = 0;
    int32_t ret = 0;
    auto appInfo = GetAppInfo(targetTokenId);
    int32_t userId = appInfo.userId;
    auto [hasData, data] = clips_.Find(userId);
    uint32_t srcTokenId = (hasData && data) ? data->GetTokenId() : 0;
    if (isRemoteData && CheckRemoteFileDocsUriLimit(grantUris, appInfo.bundleName) !=
        static_cast<int32_t>(PasteboardError::E_OK)) {
        return ret;
    }
    while (length > offset) {
        if (length - offset < PasteData::URI_BATCH_SIZE) {
            count = length - offset;
        }
        auto sendValues = std::vector<Uri>(grantUris.begin() + offset, grantUris.begin() + offset + count);
        if (isRemoteData) {
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermissionPrivileged(
                sendValues, permFlag, appInfo.bundleName, appInfo.appIndex);
        } else {
            std::vector<std::string> uriStrVec;
            for (auto &uri : sendValues) {
                uriStrVec.emplace_back(uri.ToString());
            }
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermission(
                uriStrVec, permFlag, targetTokenId, srcTokenId);
        }
        hasGranted = hasGranted || (permissionCode == 0);
        ret = permissionCode == 0 ? ret : permissionCode;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "permissionCode is %{public}d", permissionCode);
        offset += count;
    }
    if (hasGranted) {
        std::lock_guard<std::mutex> lock(readBundleMutex_);
        if (readBundles_.count(targetTokenId) == 0) {
            readBundles_.insert(targetTokenId);
        }
    }
    return ret;
}

int32_t PasteboardService::GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, bool isRemoteData)
{
    std::vector<Uri> readUris = grantUris[PasteDataRecord::READ_PERMISSION];
    std::vector<Uri> writeUris = grantUris[PasteDataRecord::READ_WRITE_PERMISSION];
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(!readUris.empty() ||
        !writeUris.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no uri");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "readUris=%{public}zu, writeUris=%{public}zu, targetTokenId=%{public}u",
        readUris.size(), writeUris.size(), targetTokenId);
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(callingUid != ANCO_SERVICE_BROKER_UID,
        static_cast<int32_t>(PasteboardError::E_OK), PASTEBOARD_MODULE_SERVICE, "callingUid = ANCO_SERVICE_BROKER_UID");
    int32_t ret = 0;
    if (isRemoteData) {
        RemoveInvalidRemoteUri(readUris);
        RemoveInvalidRemoteUri(writeUris);
    }
    auto permFlag = PasteDataRecord::READ_PERMISSION;
    ret = GrantPermission(readUris, permFlag, isRemoteData, targetTokenId);
    if (!isRemoteData) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "NeedPersistance, permFlag is %{public}d", permFlag);
        permFlag = PasteDataRecord::READ_WRITE_PERMISSION;
    }
    auto result = GrantPermission(writeUris, permFlag, isRemoteData, targetTokenId);
    ret = result == 0 ? ret : result;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

std::map<uint32_t, std::vector<Uri>> PasteboardService::CheckUriPermission(PasteData &data,
    const std::pair<std::string, int32_t> &targetBundleAndIndex)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter");
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    std::map<uint32_t, std::vector<Uri>> result;
    std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
    for (size_t i = 0; i < data.GetRecordCount(); i++) {
        auto item = data.GetRecordAt(i);
        if (item == nullptr || (!data.IsRemote() && targetBundleAndIndex == data.GetOriginAuthority())) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "local dev & local app");
            continue;
        }
        std::shared_ptr<OHOS::Uri> uri = nullptr;
        if (!item->isConvertUriFromRemote && !item->GetConvertUri().empty()) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "clear local disUri");
            item->SetConvertUri("");
        }
        if (item->isConvertUriFromRemote && !item->GetConvertUri().empty()) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "get remote disUri");
            uri = std::make_shared<OHOS::Uri>(item->GetConvertUri());
        } else if (!item->isConvertUriFromRemote && item->GetOriginUri() != nullptr) {
            uri = item->GetOriginUri();
        }
        if (uri == nullptr) {
            continue;
        }
        auto hasGrantUriPermission = item->HasGrantUriPermission();
        const std::string &bundleName = data.GetOriginAuthority().first;
        if (!IsBundleOwnUriPermission(bundleName, *uri) && !hasGrantUriPermission) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri:%{private}s, bundleName:%{public}s, appIndex:%{public}d,"
                " has grant:%{public}d", uri->ToString().c_str(), bundleName.c_str(), data.GetOriginAuthority().second,
                hasGrantUriPermission);
            continue;
        }
        if (data.IsRemote()) {
            readUris.emplace_back(*uri);
            continue;
        }
        auto uriPermission = item->GetUriPermission();
        if (uriPermission == PasteDataRecord::READ_PERMISSION) {
            readUris.emplace_back(*uri);
        } else if (uriPermission == PasteDataRecord::READ_WRITE_PERMISSION) {
            writeUris.emplace_back(*uri);
        }
    }
    result[PasteDataRecord::READ_PERMISSION] = readUris;
    result[PasteDataRecord::READ_WRITE_PERMISSION] = writeUris;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, readUris:%{public}zu, writeUris:%{public}zu",
        readUris.size(), writeUris.size());
    return result;
}

bool PasteboardService::IsBundleOwnUriPermission(const std::string &bundleName, Uri &uri)
{
    return (bundleName.compare(uri.GetAuthority()) == 0);
}

int32_t PasteboardService::HasPasteData(bool &funcResult)
{
    funcResult = HasPasteData();
    return ERR_OK;
}

bool PasteboardService::HasPasteData()
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }

    if (GetScreenStatus(userId) == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            return true;
        }
    }

    auto it = clips_.Find(userId);
    if (it.first && (it.second != nullptr)) {
        auto tokenId = IPCSkeleton::GetCallingTokenID();
        auto ret = IsDataValid(*(it.second), tokenId, userId);
        if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "pasteData is invalid, tokenId: %{public}d, userId: %{public}d,"
                "ret is %{public}d", tokenId, userId, ret);
            return false;
        }
        return true;
    }
    return false;
}

bool PasteboardService::HasRemoteUri(std::shared_ptr<PasteData> data)
{
    for (const auto &record : data->AllRecords()) {
        if (record == nullptr) {
            continue;
        }
        auto recordTypes = record->GetMimeTypes();
        if (recordTypes.find(MIMETYPE_TEXT_URI) == recordTypes.end()) {
            continue;
        }
        auto convertUri = record->GetConvertUri();
        if (!convertUri.empty() && convertUri.find(PasteboardImgExtractor::FILE_SCHEME_PREFIX) == 0 &&
            convertUri.find("networkid=") != std::string::npos) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "record has convert uri");
            return true;
        }
        auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_URI);
        if (entry == nullptr) {
            continue;
        }
        if (!entry->HasContentByMimeType(MIMETYPE_TEXT_URI)) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "uri is delay, has no content");
            return true;
        }
        auto uri = entry->ConvertToUri();
        if (uri == nullptr) {
            continue;
        }
        auto uriStr = uri->ToString();
        if (!uriStr.empty() && uriStr.find(PasteboardImgExtractor::FILE_SCHEME_PREFIX) == 0 &&
            uriStr.find("networkid=") != std::string::npos) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "has remote uri");
            return true;
        }
    }
    return false;
}

int32_t PasteboardService::HasRemoteData(bool &funcResult)
{
    funcResult = HasRemoteData();
    return ERR_OK;
}

bool PasteboardService::HasRemoteData()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(GetScreenStatus(appInfo.userId) == ScreenEvent::ScreenUnlocked, false,
        PASTEBOARD_MODULE_SERVICE, "screen is locked.");
    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
    if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
        return true;
    }
    if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
        auto isPasting = taskMgr_.IsRemoteDataPasting(distEvt);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!isPasting, true, PASTEBOARD_MODULE_SERVICE, "remote data is pasting.");
    }
    auto [hasData, data] = clips_.Find(appInfo.userId);
    if (!hasData || data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "local data is null");
        return false;
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(data->IsRemote(), false,
        PASTEBOARD_MODULE_SERVICE, "not contains remote data.");
    bool hasRemoteUri = HasRemoteUri(data);
    return hasRemoteUri;
}

int32_t PasteboardService::GetDataTokenId(PasteData &pasteData)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto originTokenId = pasteData.GetOriginTokenId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(
        originTokenId != PasteData::INVALID_TOKEN_ID, tokenId, PASTEBOARD_MODULE_SERVICE, "originTokenId invalid");
    auto isUriProxyGrant = PermissionUtils::IsPermissionGranted(
        PermissionUtils::PERMISSION_PROXY_AUTHORIZATION_URI, tokenId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(
        isUriProxyGrant, tokenId, PASTEBOARD_MODULE_SERVICE, "No permission, callingTokenId= %{public}u", tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "originTokenId= %{public}u.", originTokenId);
    return originTokenId;
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

void PasteboardService::ClearAgedData(int32_t userId)
{
    auto data = clips_.Find(userId);
    if (data.first) {
        clips_.Erase(userId);
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    copyTime_.Erase(userId);
    RefreshCriticalState();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "data is out of the time");
    RADAR_REPORT(DFX_CLEAR_PASTEBOARD, DFX_AUTO_CLEAR, DFX_SUCCESS);
}

void PasteboardService::SetDataExpirationTimer(int32_t userId)
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }

    FFRTTask task = [this, userId]() {
        std::thread thread([=]() {
            ClearAgedData(userId);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "ClearAgedData");
        thread.detach();
    };

    std::string taskName = "data_expiration[userId=" + std::to_string(userId) + "]";
    ffrtTimer_->SetTimer(taskName, task, static_cast<uint32_t>(agedTime_.load()));
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

int32_t PasteboardService::GetMimeTypes(std::vector<std::string> &funcResult)
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (GetScreenStatus(userId) == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            if (distEvt.version != ClipPlugin::InfoType::DEFAULT) {
                return GetRemoteMimeTypes(funcResult, distEvt);
            }
            PasteData data;
            int32_t syncTime = 0;
            int32_t ret = GetRemoteData(userId, distEvt, data, syncTime);
            PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK),
                static_cast<int32_t>(PasteboardError::GET_REMOTE_DATA_ERROR),
                PASTEBOARD_MODULE_SERVICE, "get remote data failed, ret=%{public}d", ret);
        }
    }
    funcResult = GetLocalMimeTypes();
    return ERR_OK;
}

int32_t PasteboardService::GetPasteDataInfo(PasteDataInfo &pasteDataInfo)
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetDataInfo userId: %{public}d, clips_ find: %{public}d",
        userId, clips_.Find(userId).first);
    auto it = clips_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(it.first, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "Can not find data. userId: %{public}d", userId);
    if (it.second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is nullptr. userId: %{public}d", userId);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    auto &pasteData = *(it.second);
    auto ret = IsDataValid(pasteData, IPCSkeleton::GetCallingTokenID(), userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "pasteData is invalid, ret is %{public}d", ret);
    
    pasteDataInfo.isDelayedData = pasteData.IsDelayData();
    pasteDataInfo.isDelayedRecord = pasteData.IsDelayRecord();
    pasteDataInfo.mimeTypes = pasteData.GetMimeTypes();
    pasteDataInfo.rawDataSize = pasteData.rawDataSize_;

    int32_t textSize = 0;
    int32_t htmlSize = 0;
    for (size_t i = 0; i < pasteData.GetRecordCount(); ++i) {
        auto record = pasteData.GetRecordAt(i);
        if (record == nullptr) {
            continue;
        }
        auto plainText = record->GetPlainTextV0();
        if (plainText != nullptr) {
            textSize += static_cast<int32_t>(plainText->size());
        }
        auto htmlText = record->GetHtmlTextV0();
        if (htmlText != nullptr) {
            htmlSize += static_cast<int32_t>(htmlText->size());
        }
    }
    pasteDataInfo.textDataSize = textSize;
    pasteDataInfo.htmlDataSize = htmlSize;

    return ERR_OK;
}

int32_t PasteboardService::HasDataType(const std::string &mimeType, bool &funcResult)
{
    auto ret = PasteBoardCommon::IsValidMimeType(mimeType);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR),
        PASTEBOARD_MODULE_SERVICE, "Parameter error. MimeType size=%{public}zu.", mimeType.size());
    funcResult = HasDataType(mimeType);
    return ERR_OK;
}

bool PasteboardService::HasDataType(const std::string &mimeType)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto userId = GetAppInfo(tokenId).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }
    if (GetScreenStatus(userId) == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            auto it = std::find(distEvt.dataType.begin(), distEvt.dataType.end(), mimeType);
            if (it != distEvt.dataType.end()) {
                return true;
            }
            if (IsBasicType(mimeType)) {
                return false;
            }
            if (distEvt.version != ClipPlugin::InfoType::DEFAULT) {
                return HasRemoteDataType(mimeType, distEvt);
            }
            PasteData data;
            int32_t syncTime = 0;
            if (GetRemoteData(userId, distEvt, data, syncTime) != static_cast<int32_t>(PasteboardError::E_OK)) {
                return false;
            }
        }
    }
    return HasLocalDataType(mimeType, tokenId, userId);
}

int32_t PasteboardService::HasUtdType(const std::string &utdType, bool &funcResult)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!utdType.empty(), false, PASTEBOARD_MODULE_SERVICE, "parameter is invalid");
    funcResult = HasUtdType(utdType);
    return ERR_OK;
}

bool PasteboardService::HasUtdType(const std::string &utdType)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    auto userId = appInfo.userId;
    auto screenStatus = GetScreenStatus(appInfo.userId);
    PasteData data;
    if (screenStatus == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            int32_t syncTime = 0;
            if (GetRemoteData(userId, distEvt, data, syncTime) != static_cast<int32_t>(PasteboardError::E_OK)) {
                return false;
            }
            return data.HasUtdType(utdType);
        }
    }
    auto it = clips_.Find(userId);
    if (!it.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "can not find data. userId: %{public}d, utdType: %{public}s",
            userId, utdType.c_str());
        return false;
    }
    if (it.second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is nullptr. userId: %{public}d, utdType: %{public}s",
            userId, utdType.c_str());
        return false;
    }
    auto ret = IsDataValid(*(it.second), tokenId, appInfo.userId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "pasteData is invalid, tokenId is %{public}d, userId: %{public}d,"
            "utdType: %{public}s, ret is %{public}d",
            tokenId, userId, utdType.c_str(), ret);
        return false;
    }
    if (it.second->GetScreenStatus() > screenStatus) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "current screen is %{public}d, set data screen is %{public}d."
            "userId: %{public}d, utdType: %{public}s",
            screenStatus, it.second->GetScreenStatus(), userId, utdType.c_str());
        return false;
    }
    return it.second->HasUtdType(utdType);
}
} // namespace MiscServices
} // namespace OHOS
