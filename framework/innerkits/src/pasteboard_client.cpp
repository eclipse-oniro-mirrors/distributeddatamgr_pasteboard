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
constexpr int32_t PASTEBOARD_PROGRESS_RETRY_TIMES = 10;
static sptr<PasteboardSaMgrListener> saCallback_ = nullptr;
std::mutex PasteboardClient::instanceLock_;
std::atomic<bool> PasteboardClient::remoteTask_(false);
std::atomic<bool> PasteboardClient::isPasting_(false);
std::atomic<uint64_t> PasteboardClient::progressStartTime_;

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
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
    }
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

bool PasteboardClient::HasRemoteData()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "HasRemoteData start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    bool ret = false;
    int32_t errCode = proxyService->HasRemoteData(ret);
    if (errCode != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "has remote data failed, ret=%{public}d", errCode);
        return false;
    }
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
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGW(callback != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "callback is null");
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
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == ERR_OK, {},
        PASTEBOARD_MODULE_CLIENT, "GetGlobalShareOption failed, ret=%{public}d", ret);
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
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == ERR_OK, {},
        PASTEBOARD_MODULE_CLIENT, "GetMimeTypes failed, ret=%{public}d", ret);
    return mimeTypes;
}

int32_t PasteboardClient::GetPasteDataInfo(PasteDataInfo &pasteDataInfo)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "GetPasteDataInfo start.");
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    int32_t ret = proxyService->GetPasteDataInfo(pasteDataInfo);
    ret = ConvertErrCode(ret);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetPasteDataInfo failed, ret=%{public}d", ret);
        return ret;
    }
    return ret;
}

bool PasteboardClient::HasDataType(const std::string &mimeType, uint32_t timeout)
{
    auto block = std::make_shared<BlockObject<std::shared_ptr<int32_t>>>(timeout);
    ffrt::submit([block, mimeType]() {
        bool ret = PasteboardClient::GetInstance()->HasDataType(mimeType);
        std::shared_ptr<int32_t> value = std::make_shared<int32_t>(ret ? 1 : 0);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(block != nullptr, PASTEBOARD_MODULE_CLIENT, "block is null");
        block->SetValue(value);
        }, {}, {}, ffrt::task_attr().qos(static_cast<int32_t>(ffrt::qos_user_interactive)));

    auto value = block->GetValue();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(value != nullptr, false, PASTEBOARD_MODULE_CLIENT, "async task timeout");
    return (*value) == 1;
}

void PasteboardClient::HasDataType(const std::string &mimeType, std::function<void(bool)> callback)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(callback != nullptr, PASTEBOARD_MODULE_CLIENT, "callback is null");

    std::thread thread([mimeType, callback] {
        bool ret = PasteboardClient::GetInstance()->HasDataType(mimeType);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(callback != nullptr, PASTEBOARD_MODULE_CLIENT, "callback is null");
        callback(ret);
    });
    thread.detach();
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
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(PatternDetection::IsValid(patternsToCheck), {},
        PASTEBOARD_MODULE_CLIENT, "Invalid number in Pattern set!");

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

int32_t PasteboardClient::SyncDelayedData()
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    return proxyService->SyncDelayedData();
}

} // namespace MiscServices
} // namespace OHOS
