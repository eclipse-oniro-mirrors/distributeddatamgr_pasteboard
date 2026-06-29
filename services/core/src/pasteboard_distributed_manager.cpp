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

#include "pasteboard_distributed_manager.h"
#include "pasteboard_hilog.h"
#include "pasteboard_common_utils.h"
#include "pasteboard_error.h"
#include "device/dm_adapter.h"
#include "account_account_manager.h"
#include "reporter.h"
#include "security_level.h"
#include "pasteboard_time.h"
#include "loader.h"
#include "pasteboard_web_controller.h"
#include "pasteboard_common.h"
#include "distributed_file_daemon_manager.h"
#include "os_account_manager.h"
#include "pasteboard_event_dfx.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace MiscServices {
using namespace RadarReporter;

namespace {
constexpr int32_t CONSTRAINT = 0;
constexpr pid_t DEVICE_COLLABORATION_UID = 5100;
constexpr int32_t SET_DISTRIBUTED_DATA_INTERVAL = 100;
constexpr int32_t MAX_TRANSFER_SIZE = 20 * 1024 * 1024;
constexpr const char *PLUGIN_NAME = "distributed_clip";
constexpr const char *MIMETYPE_TEXT_URI = "text/uri-list";
constexpr const char *MIMETYPE_TEXT_HTML = "text/html";
constexpr const char *P2P_PRESYNC_ID = "preSync";
constexpr int32_t WIFI_DISABLED = 0;
constexpr int64_t SIZE_K = 1024;
}

PasteboardDistributedManager::PasteboardDistributedManager(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDistributedManager constructed.");
}

PasteboardDistributedManager::~PasteboardDistributedManager()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDistributedManager destructed.");
}

std::pair<std::shared_ptr<PasteData>, PasteDateResult> PasteboardDistributedManager::GetDistributedData(
    const Event &event, int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    PasteDateResult pasteDateResult;
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR);
        return std::make_pair(nullptr, pasteDateResult);
    }
    std::vector<uint8_t> rawData;
    auto result = clipPlugin->GetPasteData(event, rawData);
    if (result.first != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get data failed");
        Reporter::GetInstance().PasteboardFault().Report({ user, "GET_REMOTE_DATA_FAILED" });
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = result.first;
        return std::make_pair(nullptr, pasteDateResult);
    }
    if (static_cast<int64_t>(rawData.size()) > service_.maxLocalCapacity_.load()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote dataSize exceeded, dataSize=%{public}zu", rawData.size());
        pasteDateResult.syncTime = 0;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED);
        return std::make_pair(nullptr, pasteDateResult);
    }
    service_.SetCurrentEvent(std::move(event));
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->Decode(rawData);
    pasteData->SetOriginAuthority(std::make_pair(pasteData->GetBundleName(), pasteData->GetAppIndex()));
    pasteData->rawDataSize_ = static_cast<int64_t>(rawData.size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set remote data, dataSize=%{public}" PRId64, pasteData->rawDataSize_);
    for (size_t i = 0; i < pasteData->GetRecordCount(); i++) {
        auto item = pasteData->GetRecordAt(i);
        if (item == nullptr || item->GetConvertUri().empty()) {
            continue;
        }
        if (item->GetOriginUri() == nullptr) {
            item->SetConvertUri("");
            continue;
        }
        item->isConvertUriFromRemote = true;
    }
    pasteDateResult.syncTime = result.second;
    pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::E_OK);
    return std::make_pair(pasteData, pasteDateResult);
}

bool PasteboardDistributedManager::IsConstraintEnabled(int32_t user)
{
    bool isConstraintEnabled = false;
    ErrCode err = AccountSA::OsAccountManager::CheckOsAccountConstraintEnabled(user, CONSTRAINT, isConstraintEnabled);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        err == ERR_OK, false, PASTEBOARD_MODULE_SERVICE, "CheckOsAccountConstraintEnabled failed, %{public}d", err);
    return isConstraintEnabled;
}

bool PasteboardDistributedManager::IsDisallowDistributed()
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    if (uid == DEVICE_COLLABORATION_UID) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uid from device collaboration");
        return true;
    }
    return false;
}

bool PasteboardDistributedManager::SetDistributedData(int32_t user, PasteData &data)
{
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!networkId.empty(), false, PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
    Event event;
    event.user = user;
    event.seqId = ++service_.sequenceId_;
    auto expiration = PasteBoardTime::GetBootTimeMs() + PasteboardService::EXPIRATION_INTERVAL;
    event.expiration = static_cast<uint64_t>(expiration);
    event.deviceId = networkId;
    event.account = AccountManager::GetInstance().GetCurrentAccount();
    event.status = ClipPlugin::EVT_NORMAL;
    event.dataType = data.GetMimeTypes();
    event.isDelay = data.IsDelayRecord();
    event.dataId = data.GetDataId();
    service_.SetCurrentEvent(event);

    if (IsConstraintEnabled(user) || IsDisallowDistributed()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "not allowed to send, user:%{public}d", user);
        return false;
    }
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", data.GetDataId());
        return false;
    }
    ShareOption shareOpt = data.GetShareOption();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::InApp, false, PASTEBOARD_MODULE_SERVICE,
        "data share option is in app, dataId:%{public}u", data.GetDataId());
    if (service_.CheckMdmShareOption(data)) {
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::LocalDevice, false, PASTEBOARD_MODULE_SERVICE,
            "data share option is local device, dataId:%{public}u", data.GetDataId());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, isDelay:%{public}d,"
        "expiration:%{public}" PRIu64, event.dataId, event.seqId, event.isDelay, event.expiration);
    return SetCurrentDistributedData(data, event);
}

bool PasteboardDistributedManager::SetCurrentDistributedData(PasteData &data, Event event)
{
    std::thread thread([this, data, event]() mutable {
        {
            std::lock_guard<std::mutex> lock(service_.setDistributedMemory_.mutex);
            service_.setDistributedMemory_.latestEvent = event;
            service_.setDistributedMemory_.latestData = std::make_shared<PasteData>(data);
            if (service_.setDistributedMemory_.isRunning) {
                return;
            }
            service_.setDistributedMemory_.isRunning = true;
        }
        bool isNeedCheck = false;
        while (true) {
            auto block = std::make_shared<BlockObject<bool>>(SET_DISTRIBUTED_DATA_INTERVAL, false);
            {
                std::lock_guard<std::mutex> lock(service_.setDistributedMemory_.mutex);
                if ((service_.setDistributedMemory_.currentEvent.seqId == service_.setDistributedMemory_.latestEvent.seqId
                        && isNeedCheck) || service_.setDistributedMemory_.latestData == nullptr) {
                    service_.setDistributedMemory_.latestData = nullptr;
                    service_.setDistributedMemory_.isRunning = false;
                    break;
                }
            }
            if (!isNeedCheck) {
                isNeedCheck = true;
            }
            std::thread innerThread([this, block]() mutable {
                auto result = SetCurrentData();
                block->SetValue(true);
            });
            PasteBoardCommonUtils::SetThreadTaskName(innerThread, "SetCurrentData");
            innerThread.detach();
            bool ret = block->GetValue();
            PASTEBOARD_CHECK_AND_RETURN_LOGE(ret, PASTEBOARD_MODULE_SERVICE, "timeout,seqId:%{public}hu", event.seqId);
        }
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "SetDistributeDa");
    thread.detach();
    return true;
}

bool PasteboardDistributedManager::SetCurrentData()
{
    PasteData currentData;
    Event currentEvent;
    {
        std::lock_guard<std::mutex> lock(service_.setDistributedMemory_.mutex);
        if (service_.setDistributedMemory_.latestData == nullptr) {
            return false;
        }
        service_.setDistributedMemory_.currentEvent = service_.setDistributedMemory_.latestEvent;
        currentEvent = service_.setDistributedMemory_.currentEvent;
        currentData = *service_.setDistributedMemory_.latestData;
    }
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(
            PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", currentData.GetDataId());
        return false;
    }
    RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_LOAD_DISTRIBUTED_PLUGIN, DFX_SUCCESS);
    bool needFull = currentData.IsDelayRecord() &&
        moduleConfig_.GetRemoteDeviceMinVersion() == DistributedModuleConfig::Version::VERSION_FOUR;
    if (needFull) {
        service_.GetFullDelayPasteData(currentEvent.user, currentData);
        currentEvent.isDelay = false;
        {
            std::unique_lock<std::shared_mutex> write(service_.pasteDataMutex_);
            std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(currentData.GetOriginAuthority());
            PasteboardWebController::GetInstance().SplitWebviewPasteData(
                currentData, bundleIndex, currentData.userId_);
            PasteboardWebController::GetInstance().SetWebviewPasteData(currentData, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(currentData);
        }
    }
    service_.GenerateDistributedUri(currentData);
    currentEvent.notNeedLink = !service_.IsNeedLink(currentData);
    std::vector<uint8_t> rawData;
    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    {
        std::shared_lock<std::shared_mutex> read(service_.pasteDataMutex_);
        if (!currentData.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "distributed data encode failed, dataId:%{public}u, seqId:%{public}hu",
                currentEvent.dataId, currentEvent.seqId);
            return false;
        }
    }
    if (currentData.IsDelayRecord() && !needFull) {
        clipPlugin->RegisterDelayCallback(
            std::bind(&PasteboardDistributedManager::GetDistributedDelayData, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3),
            std::bind(&PasteboardDistributedManager::GetDistributedDelayEntry, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }
    std::vector<uint8_t> rawMimeTypes;
    if (rawData.size() > MAX_TRANSFER_SIZE) {
        auto mimeTypes = currentData.GetMimeTypes();
        rawMimeTypes = EncodeMimeTypes(mimeTypes);
    }
    clipPlugin->SetPasteData(currentEvent, rawData, remoteVersionMin, rawMimeTypes);
    return true;
}

int32_t PasteboardDistributedManager::GetDistributedDelayEntry(const Event &evt, uint32_t recordId, const std::string &utdId,
    std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64
        ", recordId:%{public}u, type:%{public}s", evt.dataId, evt.seqId, evt.expiration, recordId, utdId.c_str());
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = service_.clips_.Find(service_.GetCompositeKey(evt.user));
#else
    auto [hasData, data] = service_.clips_.Find(evt.user);
#endif
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    auto record = data->GetRecordById(recordId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID),
        PASTEBOARD_MODULE_SERVICE, "recordId=%{public}u invalid, max=%{public}zu", recordId, data->GetRecordCount());

    PasteDataEntry entry;
    entry.SetUtdId(utdId);
    int32_t ret = service_.GetLocalEntryValue(evt.user, *data, *record, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, seqId=%{public}hu, dataId=%{public}u, recordId=%{public}u"
        ", type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_URI) {
        ret = ProcessDistributedDelayUri(evt.user, *data, entry, rawData);
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessDistributedDelayHtml(*data, entry, rawData);
    } else {
        ret = ProcessDistributedDelayEntry(entry, rawData);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "process distributed entry failed, seqId=%{public}hu, dataId=%{public}u, "
        "recordId=%{public}u, type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "type=%{public}s, size=%{public}zu", utdId.c_str(), rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDistributedManager::ProcessDistributedDelayUri(int32_t userId, PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");

    {
        std::unique_lock<std::shared_mutex> write(service_.pasteDataMutex_);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    std::string localUri = uri->ToString();
    std::vector<std::string> localUris = { localUri };
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    int32_t ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(localUris, userId, dfsUris);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE,
        "generate distributed uri failed, uri=%{private}s", localUri.c_str());

    auto dfsUri = dfsUris.find(localUri);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(dfsUri != dfsUris.end(), static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "dfsUris is null");
    std::string distributedUri = dfsUri->second.uriStr;
    size_t fileSize = dfsUri->second.fileSize;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri: %{private}s -> %{private}s, fileSize=%{public}zu",
        localUri.c_str(), distributedUri.c_str(), fileSize);

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(distributedUri);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::FILE_URI_PARAM] = distributedUri;
        entry.SetValue(newObject);
        entry.SetFileSize(static_cast<int64_t>(fileSize));
    }

    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode uri failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDistributedManager::ProcessDistributedDelayHtml(PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    {
        std::unique_lock<std::shared_mutex> write(service_.pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_)) {
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }

    PasteData tmp;
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
    tmp.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    if (PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, data.userId_)) {
        PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);
        service_.GenerateDistributedUri(tmp);
    }

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    bool encodeSucc = tmp.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode html failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDistributedManager::ProcessDistributedDelayEntry(PasteDataEntry &entry, std::vector<uint8_t> &rawData)
{
    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode entry failed, type=%{public}s", entry.GetUtdId().c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDistributedManager::GetDistributedDelayData(const Event &evt, uint8_t version, std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64,
        evt.dataId, evt.seqId, evt.expiration);
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = service_.clips_.Find(service_.GetCompositeKey(evt.user));
#else
    auto [hasData, data] = service_.clips_.Find(evt.user);
#endif
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    if (version == 0) {
        ret = service_.GetFullDelayPasteData(evt.user, *data);
    } else if (version == 1) {
        ret = service_.GetDelayPasteRecord(evt.user, *data);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get delay data failed, version=%{public}hhu", version);

    auto authorityInfo = data->GetOriginAuthority();
    data->SetBundleInfo(authorityInfo.first, authorityInfo.second);
    {
        std::unique_lock<std::shared_mutex> write(service_.pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authorityInfo);
        PasteboardWebController::GetInstance().SplitWebviewPasteData(*data, bundleIndex, evt.user);
        PasteboardWebController::GetInstance().SetWebviewPasteData(*data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(*data);
    }
    service_.GenerateDistributedUri(*data);

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    std::shared_lock<std::shared_mutex> read(service_.pasteDataMutex_);
    bool encodeSucc = data->Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode data failed, dataId:%{public}u, seqId:%{public}hu", evt.dataId, evt.seqId);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "size=%{public}zu", rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

std::shared_ptr<ClipPlugin> PasteboardDistributedManager::GetClipPlugin()
{
    auto isOn = moduleConfig_.IsOn();
    if (isOn) {
        auto isSupported = service_.securityLevel_.IsSupportedDistributed(false);
        if (!isSupported) {
            return nullptr;
        }
    }
    std::lock_guard<decltype(service_.mutex)> lockGuard(service_.mutex);
    if (!isOn || clipPlugin_ != nullptr) {
        return clipPlugin_;
    }
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    InitPlugin(clipPlugin_);
    return clipPlugin_;
}

void PasteboardDistributedManager::InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin)
{
    if (!clipPlugin) {
        return;
    }
    clipPlugin->RegisterPreSyncCallback(std::bind(&PasteboardService::PreEstablishP2PLinkCallback,
        &service_, std::placeholders::_1, std::placeholders::_2));
    clipPlugin->RegisterPreSyncMonitorCallback(std::bind(&PasteboardService::PreSyncSwitchMonitorCallback, &service_));
    clipPlugin->SetMaxLocalCapacity(service_.maxLocalCapacity_.load() / SIZE_K / SIZE_K);
}

void PasteboardDistributedManager::CleanDistributedData(int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->Clear(user);
}

bool PasteboardDistributedManager::IsValidCurrentEvent()
{
    auto expiration = PasteBoardTime::GetBootTimeMs();
    auto currentEvent = service_.GetCurrentEvent();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(static_cast<uint64_t>(expiration) < currentEvent.expiration,
        false, PASTEBOARD_MODULE_SERVICE, "event is invalid");
    return true;
}

void PasteboardDistributedManager::CloseDistributedStore(int32_t user, bool isNeedClear)
{
    std::lock_guard<decltype(service_.mutex)> lockGuard(service_.mutex);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
    if (isNeedClear) {
        clipPlugin_->Clear(user);
    }
    clipPlugin_->Close(user);
}

void PasteboardDistributedManager::ChangeStoreStatus(int32_t userId)
{
    PasteboardService::currentUserId_.store(userId);
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->ChangeStoreStatus(userId);
}

} // namespace MiscServices
} // namespace OHOS