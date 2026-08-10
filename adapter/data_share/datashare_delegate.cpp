/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include "datashare_delegate.h"

#include "iservice_registry.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
const constexpr char *SETTING_COLUMN_KEYWORD = "KEYWORD";
const constexpr char *SETTING_COLUMN_VALUE = "VALUE";
const constexpr char *SETTING_URI_PROXY_PREFIX = "datashare:///com.ohos.settingsdata/entry/settingsdata/"
                                          "USER_SETTINGSDATA_SECURE_";
const constexpr char *SETTING_URI_PROXY_SUFFIX = "?Proxy=true";
constexpr const char *SETTINGS_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
const constexpr char *SETTING_URI_WIFI_PROXY =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
constexpr const int32_t PASTEBOARD_SA_ID = 3701;

DataShareDelegate &DataShareDelegate::GetInstance()
{
    static DataShareDelegate instance;
    return instance;
}

sptr<IRemoteObject> GetSystemAbilitySafe(int32_t saId)
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    return samgr ? samgr->GetSystemAbility(saId) : nullptr;
}

std::shared_ptr<DataShare::DataShareHelper> DataShareDelegate::CreateDataShareHelper(const std::string &key)
{
    std::string SETTING_URI_PROXY;
    if (key == DEVICE_COLLAB_SWITCH) {
        SETTING_URI_PROXY = std::string(SETTING_URI_WIFI_PROXY);
    } else {
        std::lock_guard<std::mutex> lock(userIdMutex_);
        SETTING_URI_PROXY = std::string(SETTING_URI_PROXY_PREFIX) + userId_ + std::string(SETTING_URI_PROXY_SUFFIX);
    }
    auto remoteObj = GetSystemAbilitySafe(PASTEBOARD_SA_ID);
    if (!remoteObj) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get sa manager return nullptr");
        return nullptr;
    }
    
    auto [ret, helper] = DataShare::DataShareHelper::Create(remoteObj, SETTING_URI_PROXY, SETTINGS_DATA_EXT_URI);
    remoteObj = nullptr;
    return helper;
}

void DataShareDelegate::SetUserId(int32_t userId)
{
    std::lock_guard<std::mutex> lock(userIdMutex_);
    this->userId_ = std::to_string(userId);
}

bool DataShareDelegate::ReleaseDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> helper)
{
    if (helper == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "helper is nullptr");
        return false;
    }
    if (!helper->Release()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "release helper fail");
        return false;
    }
    return true;
}

int32_t DataShareDelegate::GetValue(const std::string &key, std::string &value)
{
    auto helper = CreateDataShareHelper(key);
    if (helper == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "helper is nullptr");
        return static_cast<int32_t>(PasteboardError::CREATE_DATASHARE_SERVICE_ERROR);
    }
    std::vector<std::string> columns = { SETTING_COLUMN_VALUE };
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(SETTING_COLUMN_KEYWORD, key);
    Uri uri = MakeUri(key);
    auto resultSet = helper->Query(uri, predicates, columns);
    ReleaseDataShareHelper(helper);
    if (resultSet == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Query failed key=%{public}s", key.c_str());
        return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
    }
    int32_t count = 0;
    int32_t ret = resultSet->GetRowCount(count);
    if (count == 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "no value, key=%{public}s, ret=%{public}d", key.c_str(), ret);
        resultSet->Close();
        return static_cast<int32_t>(PasteboardError::QUERY_SETTING_NO_DATA_ERROR);
    }
    int32_t index = 0;
    resultSet->GoToRow(index);
    ret = resultSet->GetString(index, value);
    if (ret != DataShare::E_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get value failed, ret=%{public}d", ret);
        resultSet->Close();
        return ret;
    }
    resultSet->Close();
    return static_cast<int32_t>(PasteboardError::E_OK);
}

Uri DataShareDelegate::MakeUri(const std::string &key)
{
    std::string uriString;
    if (key == DEVICE_COLLAB_SWITCH) {
        uriString = std::string(SETTING_URI_WIFI_PROXY) + "&key=" + key;
    } else {
        std::lock_guard<std::mutex> lock(userIdMutex_);
        uriString = std::string(SETTING_URI_PROXY_PREFIX) + userId_ +
            std::string(SETTING_URI_PROXY_SUFFIX) + "&key=" + key;
    }
    Uri uri(uriString);
    return uri;
}

int32_t DataShareDelegate::RegisterObserver(const std::string &key, sptr<AAFwk::IDataAbilityObserver> observer)
{
    auto uri = MakeUri(key);
    auto helper = CreateDataShareHelper(key);
    if (helper == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "helper is nullptr");
        return ERR_NO_INIT;
    }
    helper->RegisterObserver(uri, observer);
    ReleaseDataShareHelper(helper);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "register observer %{public}s", uri.ToString().c_str());
    return ERR_OK;
}

int32_t DataShareDelegate::UnregisterObserver(const std::string &key, sptr<AAFwk::IDataAbilityObserver> observer)
{
    auto uri = MakeUri(key);
    auto helper = CreateDataShareHelper(key);
    if (helper == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "helper is nullptr");
        return ERR_NO_INIT;
    }
    helper->UnregisterObserver(uri, observer);
    ReleaseDataShareHelper(helper);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "unregister observer %{public}s", uri.ToString().c_str());
    return ERR_OK;
}
} // namespace OHOS::MiscServices