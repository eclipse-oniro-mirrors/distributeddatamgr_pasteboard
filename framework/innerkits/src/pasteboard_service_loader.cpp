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

#include <if_system_ability_manager.h>
#include <iservice_registry.h>

#include "message_parcel_warp.h"
#include "pasteboard_client_death_observer_stub.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_load_callback.h"
#include "pasteboard_service_loader.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace MiscServices {

constexpr int32_t LOADSA_TIMEOUT_MS = 4000;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024; // 32K
sptr<IPasteboardService> PasteboardServiceLoader::pasteboardServiceProxy_;
std::condition_variable PasteboardServiceLoader::proxyConVar_;
PasteboardServiceLoader::StaticDestroyMonitor PasteboardServiceLoader::staticDestroyMonitor_;
sptr<IRemoteObject> clientDeathObserverPtr_;
std::mutex PasteboardServiceLoader::instanceLock_;

PasteboardServiceLoader::PasteboardServiceLoader()
{
}

PasteboardServiceLoader::~PasteboardServiceLoader()
{
    if (staticDestroyMonitor_.IsDestroyed()) {
        return;
    }
    auto pasteboardServiceProxy = GetPasteboardServiceProxy();
    if (pasteboardServiceProxy == nullptr) {
        return;
    }
    auto remoteObject = pasteboardServiceProxy->AsObject();
    if (remoteObject == nullptr) {
        return;
    }
    remoteObject->RemoveDeathRecipient(deathRecipient_);
}

void PasteboardServiceLoader::CleanupResource()
{
    if (staticDestroyMonitor_.IsDestroyed()) {
        return;
    }
    auto pasteboardServiceProxy = GetPasteboardService();
    if (pasteboardServiceProxy == nullptr) {
        return;
    }
    auto remoteObject = pasteboardServiceProxy->AsObject();
    if (remoteObject == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(instanceLock_);
    remoteObject->RemoveDeathRecipient(deathRecipient_);
}

extern "C" __attribute__((destructor)) void CleanUp()
{
    PasteboardServiceLoader::GetInstance().CleanupResource();
}

PasteboardServiceLoader &PasteboardServiceLoader::GetInstance()
{
    static PasteboardServiceLoader serviceLoader;
    return serviceLoader;
}

sptr<IPasteboardService> PasteboardServiceLoader::GetPasteboardService()
{
    std::unique_lock<std::mutex> lock(instanceLock_);
    if (pasteboardServiceProxy_ != nullptr) {
        return pasteboardServiceProxy_;
    }
    if (constructing_) {
        auto waitStatus = proxyConVar_.wait_for(lock, std::chrono::milliseconds(LOADSA_TIMEOUT_MS), [this]() {
            return pasteboardServiceProxy_ != nullptr;
        });
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(waitStatus, nullptr, PASTEBOARD_MODULE_CLIENT, "Load SA timeout");
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Getting PasteboardServiceProxy succeeded.");
        return pasteboardServiceProxy_;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteboardService start.");
    sptr<ISystemAbilityManager> saMgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saMgrProxy == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Get SystemAbilityManager failed.");
        pasteboardServiceProxy_ = nullptr;
        return nullptr;
    }
    sptr<IRemoteObject> remoteObject = saMgrProxy->CheckSystemAbility(PASTEBOARD_SERVICE_ID);
    if (remoteObject != nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Get PasteboardServiceProxy succeed.");
        SetPasteboardServiceProxy(remoteObject);
        return pasteboardServiceProxy_;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "remoteObject is null.");
    sptr<PasteboardLoadCallback> loadCallback = new PasteboardLoadCallback();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(loadCallback != nullptr, nullptr, PASTEBOARD_MODULE_CLIENT, "loadCb is null");
    int32_t ret = saMgrProxy->LoadSystemAbility(PASTEBOARD_SERVICE_ID, loadCallback);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == ERR_OK, nullptr, PASTEBOARD_MODULE_CLIENT, "Failed to load SA");
    constructing_ = true;
    auto waitStatus = proxyConVar_.wait_for(lock, std::chrono::milliseconds(LOADSA_TIMEOUT_MS), [this]() {
        return pasteboardServiceProxy_ != nullptr;
    });
    constructing_ = false;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(waitStatus, nullptr, PASTEBOARD_MODULE_CLIENT, "Load SA timeout");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Getting PasteboardServiceProxy succeeded.");
    return pasteboardServiceProxy_;
}

sptr<IPasteboardService> PasteboardServiceLoader::GetPasteboardServiceProxy()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    return pasteboardServiceProxy_;
}

void PasteboardServiceLoader::ClearPasteboardServiceProxy()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    pasteboardServiceProxy_ = nullptr;
}

void PasteboardServiceLoader::SetPasteboardServiceProxy(const sptr<IRemoteObject> &remoteObject)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(remoteObject != nullptr, PASTEBOARD_MODULE_CLIENT, "remoteObject is null.");
    if (deathRecipient_ == nullptr) {
        deathRecipient_ = sptr<IRemoteObject::DeathRecipient>(new PasteboardSaDeathRecipient());
    }
    PASTEBOARD_CHECK_AND_RETURN_LOGE(
        deathRecipient_ != nullptr, PASTEBOARD_MODULE_CLIENT, "deathRecipient_ is null.");
    if (!remoteObject->AddDeathRecipient(deathRecipient_)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "AddDeathRecipient failed.");
        deathRecipient_ = nullptr;
        return;
    }
    pasteboardServiceProxy_ = iface_cast<IPasteboardService>(remoteObject);
    if (clientDeathObserverPtr_ == nullptr) {
        clientDeathObserverPtr_ = sptr<PasteboardClientDeathObserverStub>::MakeSptr();
    }
    if (clientDeathObserverPtr_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "clientDeathObserverPtr_ is null.");
        remoteObject->RemoveDeathRecipient(deathRecipient_);
        deathRecipient_ = nullptr;
        return;
    }
    auto ret = pasteboardServiceProxy_->RegisterClientDeathObserver(clientDeathObserverPtr_);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "RegisterClientDeathObserver failed.");
        remoteObject->RemoveDeathRecipient(deathRecipient_);
        deathRecipient_ = nullptr;
        clientDeathObserverPtr_ = nullptr;
    }
}

void PasteboardServiceLoader::ReleaseDeathRecipient()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(deathRecipient_ != nullptr, PASTEBOARD_MODULE_CLIENT, "deathRecipient is null.");
    bool notNull = pasteboardServiceProxy_ != nullptr && pasteboardServiceProxy_->AsObject() != nullptr;
    PASTEBOARD_CHECK_AND_RETURN_LOGE(notNull, PASTEBOARD_MODULE_CLIENT, "pasteboardServiceProxy is null.");
    pasteboardServiceProxy_->AsObject()->RemoveDeathRecipient(deathRecipient_);
    deathRecipient_ = nullptr;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReleaseDeathRecipient end.");
}

int32_t PasteboardServiceLoader::GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry &value)
{
    auto proxyService = GetPasteboardService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(proxyService != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR),
        PASTEBOARD_MODULE_CLIENT, "proxyService is nullptr");
    std::vector<uint8_t> sendTLV(0);
    if (!value.Encode(sendTLV)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "fail encode entry value");
        return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
    }
    int fd = -1;
    int64_t tlvSize = static_cast<int64_t>(sendTLV.size());
    MessageParcelWarp messageData;
    MessageParcel parcelData;
    if (tlvSize > MIN_ASHMEM_DATA_SIZE) {
        if (!messageData.WriteRawData(parcelData, sendTLV.data(), sendTLV.size())) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "WriteRawData Failed, size:%{public}" PRId64, tlvSize);
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
        fd = messageData.GetWriteDataFd();
        std::vector<uint8_t>().swap(sendTLV);
    } else {
        fd = messageData.CreateTmpFd();
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(fd >= 0, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
            PASTEBOARD_MODULE_CLIENT, "CreateTmpFd failed:%{public}d", fd);
    }
    int32_t ret = proxyService->GetRecordValueByType(dataId, recordId, tlvSize, sendTLV, fd);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "GetRecordValueByType failed, ret:%{public}d", ret);
        return ret;
    }

    return ProcessPasteData(value, tlvSize, fd, sendTLV);
}

int32_t PasteboardServiceLoader::ProcessPasteData(PasteDataEntry &data, int64_t rawDataSize, int fd,
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
        close(fd);
        return ret;
    }
    bool result = false;
    MessageParcel parcelData;
    PasteDataEntry entryValue;
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        parcelData.WriteInt64(rawDataSize);
        parcelData.WriteFileDescriptor(fd);
        close(fd);
        const uint8_t *rawData =
            reinterpret_cast<const uint8_t *>(messageReply.ReadRawData(parcelData, static_cast<size_t>(rawDataSize)));
        if (rawData == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "mmap failed, size=%{public}" PRId64, rawDataSize);
            return ret;
        }
        std::vector<uint8_t> pasteDataTlv(rawData, rawData + rawDataSize);
        result = entryValue.Decode(pasteDataTlv);
    } else {
        result = entryValue.Decode(recvTLV);
        close(fd);
    }
    if (!result) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to decode pastedata in TLV");
        return ret;
    }
    data = std::move(entryValue);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardServiceLoader::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    SetPasteboardServiceProxy(remoteObject);
    proxyConVar_.notify_all();
}

void PasteboardServiceLoader::LoadSystemAbilityFail()
{
    std::lock_guard<std::mutex> lock(instanceLock_);
    pasteboardServiceProxy_ = nullptr;
    proxyConVar_.notify_all();
}

void PasteboardServiceLoader::OnRemoteSaDied(const wptr<IRemoteObject> &remote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OnRemoteSaDied start.");
    std::lock_guard<std::mutex> lock(instanceLock_);
    pasteboardServiceProxy_ = nullptr;
}

PasteboardSaDeathRecipient::PasteboardSaDeathRecipient() {}

void PasteboardSaDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "PasteboardSaDeathRecipient on remote systemAbility died.");
    PasteboardServiceLoader::GetInstance().OnRemoteSaDied(object);
}
} // namespace MiscServices
} // namespace OHOS