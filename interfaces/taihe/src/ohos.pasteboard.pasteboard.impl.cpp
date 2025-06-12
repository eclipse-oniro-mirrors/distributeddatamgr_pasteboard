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

#include "ohos.pasteboard.pasteboard.impl.hpp"
#include "ohos.pasteboard.pasteboard.proj.hpp"
#include "stdexcept"
#include "taihe/runtime.hpp"
#include <thread>

#include "ani_common_want.h"
#include "common/block_object.h"
#include "pixel_map_taihe_ani.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_js_err.h"
#include "pasteboard_taihe_utils.h"
#include "pasteboard_taihe_observer.h"
#include "uri.h"

using namespace OHOS::MiscServices;
using EntryValueMap = std::map<std::string, std::shared_ptr<EntryValue>>;
namespace pasteboardTaihe = ohos::pasteboard::pasteboard;
constexpr int32_t MIMETYPE_MAX_SIZE = 1024;

namespace {
constexpr uint32_t SYNC_TIMEOUT = 3500;
class PasteDataRecordImpl {
public:
    PasteDataRecordImpl()
    {
        this->record_ = std::make_shared<PasteDataRecord>();
    }

    taihe::string GetMimeType()
    {
        std::string mimeType = "";
        if (this->record_ != nullptr) {
            mimeType = this->record_->GetMimeType();
        }
        return taihe::string(mimeType);
    }

    taihe::string GetPlainText()
    {
        std::shared_ptr<std::string> plainText = std::make_shared<std::string>("");
        if (this->record_ != nullptr) {
            plainText = this->record_->GetPlainText();
        }
        return taihe::string(*plainText);
    }

    taihe::string GetUri()
    {
        std::string uriStr = "";
        std::shared_ptr<OHOS::Uri> uri = nullptr;
        if (this->record_ != nullptr) {
            uri = this->record_->GetUri();
        }
        if (uri != nullptr) {
            uriStr = uri->ToString();
        }
        return taihe::string(uriStr);
    }

    uintptr_t GetPixelMap()
    {
        uintptr_t pixelMapPtr = 0;
        if (this->record_ != nullptr) {
            std::shared_ptr<OHOS::Media::PixelMap> pixelMap = this->record_->GetPixelMap();
            ani_object pixelMapObj = OHOS::Media::PixelMapTaiheAni::CreateEtsPixelMap(taihe::get_env(), pixelMap);
            pixelMapPtr = reinterpret_cast<uintptr_t>(pixelMapObj);
        }
        return pixelMapPtr;
    }

    taihe::string ToPlainText()
    {
        if (this->record_ == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get ToPlainText object failed");
            return taihe::string("");
        }
        taihe::string text(this->record_->ConvertToText());
        return text;
    }

    pasteboardTaihe::ValueType GetRecordValueByType(taihe::string_view type)
    {
        std::string mimeType(type);
        if (mimeType.empty()) {
            taihe::set_business_error(
                static_cast<int>(JSErrorCode::INVALID_PARAMETERS), "Parameter error. mimeType cannot be empty.");
            return pasteboardTaihe::ValueType::make_string("");
        }
        std::shared_ptr<PasteDataEntry> entry = this->record_->GetEntryByMimeType(mimeType);
        if (entry == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "failed to find dataEntry");
            return pasteboardTaihe::ValueType::make_string("");
        }
        auto strategy = StrategyFactory::CreateStrategyForEntry(mimeType);
        if (strategy) {
            return strategy->ConvertToValueType(mimeType, entry);
        }
        return pasteboardTaihe::ValueType::make_string("");
    }

    int64_t GetRecordImpl()
    {
        return reinterpret_cast<int64_t>(this);
    }

    void SetRecord(std::shared_ptr<PasteDataRecord> record)
    {
        this->record_ = record;
    }

    std::shared_ptr<PasteDataRecord> GetRecord()
    {
        return this->record_;
    }

private:
    std::shared_ptr<PasteDataRecord> record_;
};

class PasteDataImpl {
public:
    PasteDataImpl()
    {
        this->pasteData_ = std::make_shared<PasteData>();
    }

    void AddRecord(pasteboardTaihe::weak::PasteDataRecord record)
    {
        PasteDataRecordImpl *implPtr = reinterpret_cast<PasteDataRecordImpl *>(record->GetRecordImpl());
        std::shared_ptr<PasteDataRecord> pasteDataRecord = implPtr->GetRecord();
        this->pasteData_->AddRecord(pasteDataRecord);
        implPtr = nullptr;
    }

    void CreateAndAddRecord(taihe::string_view mimeType, const pasteboardTaihe::ValueType &value)
    {
        std::string mimeTypeStr = std::string(mimeType);
        auto strategy = StrategyFactory::CreateStrategyForRecord(value, mimeTypeStr);
        if (strategy) {
            strategy->AddRecord(mimeTypeStr, value, this->pasteData_);
        }
    }

    taihe::array<taihe::string> GetMimeTypes()
    {
        std::vector<std::string> mimeTypesVec = this->pasteData_->GetMimeTypes();
        if (mimeTypesVec.empty()) {
            return taihe::array<taihe::string>(nullptr, 0);
        }
        std::vector<taihe::string> mimeTypes;
        for (auto mimeType : mimeTypesVec) {
            mimeTypes.push_back(taihe::string(mimeType));
        }
        taihe::array<taihe::string> mimeTypeArr(mimeTypes);
        return mimeTypeArr;
    }

    taihe::string GetPrimaryHtml()
    {
        std::shared_ptr<std::string> htmlPtr = this->pasteData_->GetPrimaryHtml();
        if (htmlPtr == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get GetPrimaryHtml failed");
            return taihe::string("");
        }
        taihe::string result(*htmlPtr);
        return result;
    }

    uintptr_t GetPrimaryWant()
    {
        std::shared_ptr<OHOS::AAFwk::Want> want = this->pasteData_->GetPrimaryWant();
        if (want == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get GetPrimaryWant want failed");
            return 0;
        }
        ani_object wantObj = OHOS::AppExecFwk::WrapWant(taihe::get_env(), *want);
        uintptr_t wantPtr = reinterpret_cast<uintptr_t>(wantObj);
        return wantPtr;
    }

    taihe::string GetPrimaryMimeType()
    {
        std::shared_ptr<std::string> mimeTypePtr = this->pasteData_->GetPrimaryMimeType();
        if (mimeTypePtr == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get GetPrimaryMimeType failed");
            return taihe::string("");
        }
        taihe::string result(*mimeTypePtr);
        return result;
    }

    taihe::string GetPrimaryText()
    {
        std::shared_ptr<std::string> textPtr = this->pasteData_->GetPrimaryText();
        if (textPtr == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get GetPrimaryText failed");
            return taihe::string("");
        }
        taihe::string result(*textPtr);
        return result;
    }

    taihe::string GetPrimaryUri()
    {
        std::shared_ptr<OHOS::Uri> uriPtr = this->pasteData_->GetPrimaryUri();
        if (uriPtr == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get GetPrimaryUri failed");
            return taihe::string("");
        }
        taihe::string result(uriPtr->ToString());
        return result;
    }

    pasteboardTaihe::PasteDataProperty GetProperty()
    {
        PasteDataProperty dataProperty = this->pasteData_->GetProperty();
        pasteboardTaihe::ShareOption shareOption = ShareOptionAdapter::ToTaihe(dataProperty.shareOption);
        pasteboardTaihe::PasteDataProperty property = { shareOption, dataProperty.timestamp,
            taihe::string(dataProperty.tag) };
        return property;
    }

    void SetProperty(const pasteboardTaihe::PasteDataProperty &property)
    {
        PasteDataProperty dataProperty;
        ShareOption shareOption = ShareOptionAdapter::FromTaihe(property.shareOption);
        dataProperty.shareOption = shareOption;
        dataProperty.timestamp = property.timestamp;
        dataProperty.tag = std::string(property.tag);
        this->pasteData_->SetProperty(dataProperty);
    }

    pasteboardTaihe::PasteDataRecord GetRecord(int32_t index)
    {
        pasteboardTaihe::PasteDataRecord record =
            taihe::make_holder<PasteDataRecordImpl, pasteboardTaihe::PasteDataRecord>();
        if (index < 0 || index >= this->GetRecordCount()) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::OUT_OF_RANGE), "index out of range.");
            return record;
        }
        std::shared_ptr<PasteDataRecord> dataRecord = this->pasteData_->GetRecordAt(index);
        if (dataRecord == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "invalid parameter record");
            return record;
        }

        PasteDataRecordImpl *recordImpl = reinterpret_cast<PasteDataRecordImpl *>(record->GetRecordImpl());
        recordImpl->SetRecord(dataRecord);
        recordImpl = nullptr;
        return record;
    }

    int32_t GetRecordCount()
    {
        return static_cast<int32_t>(this->pasteData_->GetRecordCount());
    }

    taihe::string GetTag()
    {
        taihe::string tag(this->pasteData_->GetTag());
        return tag;
    }

    int64_t GetPasteDataImpl()
    {
        return reinterpret_cast<int64_t>(this);
    }

    void SetPasteData(std::shared_ptr<PasteData> pasteData)
    {
        this->pasteData_ = pasteData;
    }

    std::shared_ptr<PasteData> GetPasteData()
    {
        return this->pasteData_;
    }

private:
    std::shared_ptr<PasteData> pasteData_;
};

class SystemPasteboardImpl {
public:
    SystemPasteboardImpl() {}

    void DeleteObserver(const OHOS::sptr<PasteboardTaiheObserver> &observer)
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "observer == null: %{public}d, size: %{public}zu",
            observer == nullptr, observers_.size());
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Invalid ani environment.");
            return;
        }
        std::vector<OHOS::sptr<PasteboardTaiheObserver>> observers;
        {
            for (auto it = observers_.begin(); it != observers_.end();) {
                if (it->second == observer) {
                    observers.push_back(observer);
                    env->GlobalReference_Delete(it->first);
                    it = observers_.erase(it);
                    break;
                }
                if (observer == nullptr) {
                    observers.push_back(it->second);
                    env->GlobalReference_Delete(it->first);
                    it = observers_.erase(it);
                } else {
                    it++;
                }
            }
        }
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_ANI, "Delete observer size: %{public}zu", observers.size());
        for (auto &delObserver : observers) {
            PasteboardClient::GetInstance()->Unsubscribe(PasteboardObserverType::OBSERVER_LOCAL,
                delObserver);
        }
    }

    void OnUpdate(::taihe::callback_view<void()> callback, uintptr_t cb)
    {
        ani_object callbackObj = reinterpret_cast<ani_object>(cb);
        ani_ref callbackRef;
        ani_env *env = taihe::get_env();
        if (env == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Failed to register, get environment failed");
            return;
        }
        if (ANI_OK != env->GlobalReference_Create(callbackObj, &callbackRef)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Failed to register, create reference failed");
            return;
        }

        OHOS::sptr<PasteboardTaiheObserver> observer = nullptr;
        ani_boolean isEqual = false;
        for (const auto &[refKey, observerValue] : observers_) {
            env->Reference_StrictEquals(refKey, callbackRef, &isEqual);
            if (isEqual) {
                observer = observerValue;
                break;
            }
        }

        if (observer != nullptr) {
            env->GlobalReference_Delete(callbackRef);
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Already registered.");
            return;
        }

        auto callbackPtr = std::make_shared<::taihe::callback<void()>>(callback);
        observer = OHOS::sptr<PasteboardTaiheObserver>::MakeSptr(callbackPtr);
        if (observer == nullptr) {
            env->GlobalReference_Delete(callbackRef);
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Malloc observer failed.");
            return;
        }

        PasteboardClient::GetInstance()->Subscribe(PasteboardObserverType::OBSERVER_LOCAL, observer);
        observers_[callbackRef] = observer;
    }

    void OffUpdate(::taihe::optional_view<uintptr_t> cb)
    {
        OHOS::sptr<PasteboardTaiheObserver> observer = nullptr;
        if (cb.has_value()) {
            ani_object callbackObj = reinterpret_cast<ani_object>(cb.value());
            ani_ref callbackRef;
            ani_env *env = taihe::get_env();
            if (env == nullptr) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Failed to register, get environment failed");
                return;
            }
            if (ANI_OK != env->GlobalReference_Create(callbackObj, &callbackRef)) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Failed to register, create reference failed");
                return;
            }
            ani_boolean isEqual = false;
            for (const auto &[refKey, observerValue] : observers_) {
                env->Reference_StrictEquals(refKey, callbackRef, &isEqual);
                if (isEqual) {
                    observer = observerValue;
                    break;
                }
            }
            env->GlobalReference_Delete(callbackRef);
            if (!isEqual) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Unregister failed, please register first.");
                return;
            }
        }

        DeleteObserver(observer);
    }

    taihe::string GetDataSource()
    {
        auto block =
            std::make_shared<OHOS::BlockObject<std::shared_ptr<std::pair<int32_t, std::string>>>>(SYNC_TIMEOUT);
        std::thread thread([block]() mutable {
            std::string bundleName;
            int32_t ret = PasteboardClient::GetInstance()->GetDataSource(bundleName);
            auto value = std::make_shared<std::pair<int32_t, std::string>>(ret, bundleName);
            block->SetValue(value);
        });
        thread.detach();
        auto value = block->GetValue();
        if (value == nullptr) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::REQUEST_TIME_OUT), "Request timed out.");
            return taihe::string("");
        }
        if (value->first != static_cast<int>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "GetDataSource, failed, ret = %{public}d", value->first);
            return taihe::string("");
        }
        taihe::string bundleNameTH(value->second);
        return bundleNameTH;
    }

    bool HasDataType(taihe::string_view mimeType)
    {
        std::string mimeTypeStr = std::string(mimeType);
        auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<bool>>>(SYNC_TIMEOUT);
        std::thread thread([block, mimeTypeStr]() mutable {
            bool ret = PasteboardClient::GetInstance()->HasDataType(mimeTypeStr);
            auto ptr = std::make_shared<bool>(ret);
            block->SetValue(ptr);
        });
        thread.detach();
        std::shared_ptr<bool> value = block->GetValue();
        if (value == nullptr) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::REQUEST_TIME_OUT), "Request timed out.");
            return false;
        }
        return *value;
    }

    void ClearDataSync()
    {
        auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
        std::thread thread([block]() {
            PasteboardClient::GetInstance()->Clear();
            auto ptr = std::make_shared<int>(0);
            block->SetValue(ptr);
        });
        thread.detach();
        auto value = block->GetValue();
        if (value == nullptr) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::REQUEST_TIME_OUT), "Request timed out.");
        }
    }

    void ClearDataImpl()
    {
        PasteboardClient::GetInstance()->Clear();
    }

    pasteboardTaihe::PasteData GetDataSync()
    {
        auto pasteData = std::make_shared<PasteData>();
        auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int32_t>>>(SYNC_TIMEOUT);
        std::thread thread([block, pasteData]() {
            int32_t ret = PasteboardClient::GetInstance()->GetPasteData(*pasteData);
            auto ptr = std::make_shared<int32_t>(ret);
            block->SetValue(ptr);
        });
        thread.detach();
        auto value = block->GetValue();
        if (value == nullptr) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::REQUEST_TIME_OUT), "Request timed out.");
            return taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
        }
        pasteboardTaihe::PasteData pasteDataTH = taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
        int64_t implRawPtr = pasteDataTH->GetPasteDataImpl();
        PasteDataImpl *implPtr = reinterpret_cast<PasteDataImpl *>(implRawPtr);
        implPtr->SetPasteData(pasteData);
        implPtr = nullptr;
        return pasteDataTH;
    }

    pasteboardTaihe::PasteData GetDataImpl()
    {
        auto pasteData = std::make_shared<PasteData>();
        int32_t ret = PasteboardClient::GetInstance()->GetPasteData(*pasteData);
        if (ret == static_cast<int32_t>(PasteboardError::TASK_PROCESSING)) {
            taihe::set_business_error(ret, "Another copy or paste operation is in progress.");
            return taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
        }
        pasteboardTaihe::PasteData pasteDataTH = taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
        int64_t implRawPtr = pasteDataTH->GetPasteDataImpl();
        PasteDataImpl *implPtr = reinterpret_cast<PasteDataImpl *>(implRawPtr);
        implPtr->SetPasteData(pasteData);
        implPtr = nullptr;
        return pasteDataTH;
    }

    bool HasDataSync()
    {
        auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<bool>>>(SYNC_TIMEOUT);
        std::thread thread([block]() {
            bool ret = PasteboardClient::GetInstance()->HasPasteData();
            auto ptr = std::make_shared<bool>(ret);
            block->SetValue(ptr);
        });
        thread.detach();
        std::shared_ptr<bool> value = block->GetValue();
        if (value == nullptr) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::REQUEST_TIME_OUT), "Request timed out.");
            return false;
        }
        return *value;
    }

    bool HasDataImpl()
    {
        bool ret = PasteboardClient::GetInstance()->HasPasteData();
        return ret;
    }

    void SetDataSync(pasteboardTaihe::weak::PasteData data)
    {
        PasteDataImpl *implPtr = reinterpret_cast<PasteDataImpl *>(data->GetPasteDataImpl());
        std::shared_ptr<PasteData> pasteData = implPtr->GetPasteData();
        implPtr = nullptr;
        if (pasteData == nullptr) {
            return;
        }
        auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int32_t>>>(SYNC_TIMEOUT);
        std::thread thread([block, pasteData]() {
            int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
            auto ptr = std::make_shared<int32_t>(ret);
            block->SetValue(ptr);
        });
        thread.detach();
        std::shared_ptr<int32_t> value = block->GetValue();
        if (value == nullptr) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::REQUEST_TIME_OUT), "Request timed out.");
        }
    }

    void SetDataImpl(pasteboardTaihe::weak::PasteData data)
    {
        PasteDataImpl *implPtr = reinterpret_cast<PasteDataImpl *>(data->GetPasteDataImpl());
        std::shared_ptr<PasteData> pasteData = implPtr->GetPasteData();
        implPtr = nullptr;
        if (pasteData == nullptr) {
            return;
        }
        std::map<uint32_t, std::shared_ptr<OHOS::UDMF::EntryGetter>> entryGetters;
        for (auto record : pasteData->AllRecords()) {
            if (record != nullptr && record->GetEntryGetter() != nullptr) {
                entryGetters.emplace(record->GetRecordId(), record->GetEntryGetter());
            }
        }
        int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData, nullptr, entryGetters);
        if (ret == static_cast<int>(PasteboardError::TASK_PROCESSING)) {
            taihe::set_business_error(ret, "Another copy or paste operation is in progress.");
        } else if (ret == static_cast<int>(PasteboardError::PROHIBIT_COPY)) {
            taihe::set_business_error(ret, "Replication is prohibited.");
        }
    }

    taihe::array<taihe::string> GetMimeTypesSync()
    {
        std::vector<std::string> mimeTypesVec = PasteboardClient::GetInstance()->GetMimeTypes();
        std::vector<taihe::string> mimeTypes;
        for (auto mimeType : mimeTypesVec) {
            mimeTypes.push_back(taihe::string(mimeType));
        }
        taihe::array<taihe::string> mimeTypeArr(mimeTypes);
        return mimeTypeArr;
    }

private:
    static thread_local std::map<ani_ref, OHOS::sptr<PasteboardTaiheObserver>> observers_;
};
thread_local std::map<ani_ref, OHOS::sptr<PasteboardTaiheObserver>> SystemPasteboardImpl::observers_;

pasteboardTaihe::PasteDataRecord MakePasteDataRecord()
{
    return taihe::make_holder<PasteDataRecordImpl, pasteboardTaihe::PasteDataRecord>();
}

pasteboardTaihe::PasteData CreatePasteData()
{
    return taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
}

pasteboardTaihe::SystemPasteboard CreateSystemPasteboard()
{
    return taihe::make_holder<SystemPasteboardImpl, pasteboardTaihe::SystemPasteboard>();
}

pasteboardTaihe::PasteData CreateDataByValue(
    taihe::string_view mimeType, const pasteboardTaihe::ValueType &value)
{
    pasteboardTaihe::PasteData data = taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
    if (mimeType.empty()) {
        taihe::set_business_error(
            static_cast<int>(JSErrorCode::INVALID_PARAMETERS), "Parameter error. mimeType cannot be empty.");
        return data;
    }
    if (mimeType.size() > MIMETYPE_MAX_SIZE) {
        taihe::set_business_error(static_cast<int>(JSErrorCode::INVALID_PARAMETERS),
            "Parameter error. The length of mimeType cannot be greater than 1024 bytes.");
        return data;
    }
    std::shared_ptr<PasteData> pasteData;
    std::string mimeTypeStr = std::string(mimeType);
    auto strategy = StrategyFactory::CreateStrategyForData(mimeTypeStr);
    if (strategy) {
        strategy->CreateData(mimeTypeStr, value, pasteData);
    }
    if (pasteData == nullptr) {
        return data;
    }
    PasteDataImpl *dataImpl = reinterpret_cast<PasteDataImpl *>(data->GetPasteDataImpl());
    dataImpl->SetPasteData(pasteData);
    dataImpl = nullptr;
    return data;
}

pasteboardTaihe::PasteData CreateDataByRecord(
    taihe::map_view<taihe::string, pasteboardTaihe::ValueType> typeValueMap)
{
    pasteboardTaihe::PasteData pasteDataTH = taihe::make_holder<PasteDataImpl, pasteboardTaihe::PasteData>();
    if (typeValueMap.size() == 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "valueType is empty");
        return pasteDataTH;
    }
    std::shared_ptr<EntryValueMap> entryValueMap = std::make_shared<EntryValueMap>();
    std::string primaryMimeType = "";
    for (const auto &item : typeValueMap) {
        if (item.first.empty()) {
            taihe::set_business_error(
                static_cast<int>(JSErrorCode::INVALID_PARAMETERS), "Parameter error. mimeType cannot be empty.");
            return pasteDataTH;
        }
        if (item.first.size() > MIMETYPE_MAX_SIZE) {
            taihe::set_business_error(static_cast<int>(JSErrorCode::INVALID_PARAMETERS),
                "Parameter error. The length of mimeType cannot be greater than 1024 bytes.");
            return pasteDataTH;
        }
        if (primaryMimeType.empty()) {
            primaryMimeType = std::string(item.first);
        }
        std::string mimeTypeStr = std::string(item.first);
        std::shared_ptr<EntryValue> entry;
        auto strategy = StrategyFactory::CreateStrategyForRecord(item.second, mimeTypeStr);
        if (!strategy) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
                "data type or mimeType is not supported, mimeType is %{public}s", mimeTypeStr.c_str());
            return pasteDataTH;
        }
        EntryValue entryValue = strategy->ConvertFromValueType(mimeTypeStr, item.second);
        if (std::holds_alternative<std::monostate>(entryValue)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "data type or mimeType is invalid, mimeType is %{public}s",
                mimeTypeStr.c_str());
            return pasteDataTH;
        }
        entry = std::make_shared<EntryValue>(entryValue);
        entryValueMap->emplace(std::make_pair(mimeTypeStr, entry));
    }
    std::shared_ptr<PasteData> pasteData =
        PasteboardClient::GetInstance()->CreateMultiTypeData(entryValueMap, primaryMimeType);
    if (pasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Create multiType data failed");
        return pasteDataTH;
    }
    PasteDataImpl *dataImpl = reinterpret_cast<PasteDataImpl *>(pasteDataTH->GetPasteDataImpl());
    dataImpl->SetPasteData(pasteData);
    dataImpl = nullptr;
    return pasteDataTH;
}

pasteboardTaihe::SystemPasteboard GetSystemPasteboard()
{
    return taihe::make_holder<SystemPasteboardImpl, pasteboardTaihe::SystemPasteboard>();
}
} // namespace

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_MakePasteDataRecord(MakePasteDataRecord);
TH_EXPORT_CPP_API_CreatePasteData(CreatePasteData);
TH_EXPORT_CPP_API_CreateSystemPasteboard(CreateSystemPasteboard);
TH_EXPORT_CPP_API_CreateDataByValue(CreateDataByValue);
TH_EXPORT_CPP_API_CreateDataByRecord(CreateDataByRecord);
TH_EXPORT_CPP_API_GetSystemPasteboard(GetSystemPasteboard);
// NOLINTEND
