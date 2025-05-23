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

#include "pasteboard_taihe_utils.h"
#include "ani_common_want.h"
#include "image_ani_utils.h"
#include "pasteboard_client.h"
#include "pasteboard_hilog.h"
#include "pasteboard_js_err.h"
#include "taihe/runtime.hpp"
#include "uri.h"

namespace OHOS {
namespace MiscServices {

void ArrayBufferStrategy::AddRecord(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData)
{
    if (!value.holds_arrayBuffer()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "arraybuffer not find");
        return;
    }
    taihe::array<uint8_t> arrayBuffer = value.get_arrayBuffer_ref();
    std::vector<uint8_t> valueVec(arrayBuffer.begin(), arrayBuffer.end());
    pasteData->AddKvRecord(mimeType, valueVec);
}

void ArrayBufferStrategy::CreateData(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> &pasteData)
{
    if (!value.holds_arrayBuffer()) {
        pasteData = nullptr;
        return;
    }
    std::vector<uint8_t> valueVec;
    taihe::array<uint8_t> arrayBuffer = value.get_arrayBuffer_ref();
    valueVec = std::vector<uint8_t>(arrayBuffer.begin(), arrayBuffer.end());
    pasteData = OHOS::MiscServices::PasteboardClient::GetInstance()->CreateKvData(mimeType, valueVec);
}

pasteboardTaihe::ValueType ArrayBufferStrategy::ConvertToValueType(
    const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry)
{
    taihe::array<uint8_t> buffer(0);
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "entry is nullptr");
        return pasteboardTaihe::ValueType::make_arrayBuffer(buffer);
    }
    std::shared_ptr<MineCustomData> customData = entry->ConvertToCustomData();
    if (customData == nullptr) {
        return pasteboardTaihe::ValueType::make_arrayBuffer(buffer);
    }
    std::map<std::string, std::vector<uint8_t>> dataMap = customData->GetItemData();
    auto item = dataMap.find(mimeType);
    if (item == dataMap.end()) {
        return pasteboardTaihe::ValueType::make_arrayBuffer(buffer);
    }
    std::vector<uint8_t> dataArray = item->second;
    buffer = taihe::array<uint8_t>(dataArray);
    return pasteboardTaihe::ValueType::make_arrayBuffer(buffer);
}

EntryValue ArrayBufferStrategy::ConvertFromValueType(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value)
{
    auto entryValue = std::make_shared<EntryValue>();
    if (!value.holds_arrayBuffer()) {
        return std::monostate {};
    }
    taihe::array<uint8_t> arrayBuffer = value.get_arrayBuffer_ref();
    std::vector<uint8_t> valueVec = std::vector<uint8_t>(arrayBuffer.begin(), arrayBuffer.end());
    return valueVec;
}

void PixelMapStrategy::AddRecord(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData)
{
    if (!value.holds_pixelMapOrWant()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PixelMap not find");
        pasteData->AddPixelMapRecord(nullptr);
        return;
    }
    uintptr_t ptr = value.get_pixelMapOrWant_ref();
    ani_object obj = reinterpret_cast<ani_object>(ptr);
    auto pixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnvSp(taihe::get_env(), obj);
    if (pixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get pixelMap failed");
    }
    pasteData->AddPixelMapRecord(pixelMap);
}

void PixelMapStrategy::CreateData(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> &pasteData)
{
    if (!value.holds_pixelMapOrWant()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PixelMap not find");
        pasteData = nullptr;
        return;
    }
    uintptr_t pixelMapPtr = value.get_pixelMapOrWant_ref();
    ani_object pixelMapObj = reinterpret_cast<ani_object>(pixelMapPtr);
    std::shared_ptr<Media::PixelMap> pixelMap =
        OHOS::Media::ImageAniUtils::GetPixelMapFromEnvSp(taihe::get_env(), pixelMapObj);
    if (pixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get pixelMap failed");
        pasteData = nullptr;
        return;
    }
    pasteData = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMap);
}

pasteboardTaihe::ValueType PixelMapStrategy::ConvertToValueType(
    const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry)
{
    uintptr_t pixelMapPtr = 0;
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "entry is nullptr");
        return pasteboardTaihe::ValueType::make_pixelMapOrWant(pixelMapPtr);
    }
    std::shared_ptr<Media::PixelMap> pixelMap = entry->ConvertToPixelMap();
    if (pixelMap == nullptr) {
        taihe::set_business_error(
            static_cast<int>(JSErrorCode::INVALID_PARAMETERS), "Parameter error. pixelMap get failed");
        return pasteboardTaihe::ValueType::make_pixelMapOrWant(pixelMapPtr);
    }
    ani_object pixelMapObj = OHOS::Media::PixelMapAni::CreatePixelMap(taihe::get_env(), pixelMap);
    pixelMapPtr = reinterpret_cast<uintptr_t>(pixelMapObj);
    return pasteboardTaihe::ValueType::make_pixelMapOrWant(pixelMapPtr);
}

EntryValue PixelMapStrategy::ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value)
{
    if (!value.holds_pixelMapOrWant()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PixelMap not find");
        return std::monostate {};
    }
    uintptr_t pixelMapPtr = value.get_pixelMapOrWant_ref();
    ani_object pixelMapObj = reinterpret_cast<ani_object>(pixelMapPtr);
    std::shared_ptr<Media::PixelMap> pixelMap =
        OHOS::Media::ImageAniUtils::GetPixelMapFromEnvSp(taihe::get_env(), pixelMapObj);
    if (pixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Get pixelMap failed");
    }
    return pixelMap;
}

void WantStrategy::AddRecord(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData)
{
    uintptr_t ptr = 0;
    ani_object obj = nullptr;
    if (value.holds_pixelMapOrWant()) {
        ptr = value.get_pixelMapOrWant_ref();
        obj = reinterpret_cast<ani_object>(ptr);
    }
    OHOS::AAFwk::Want want;
    OHOS::AppExecFwk::UnwrapWant(taihe::get_env(), obj, want);
    pasteData->AddWantRecord(std::make_shared<OHOS::AAFwk::Want>(want));
}

void WantStrategy::CreateData(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> &pasteData)
{
    if (!value.holds_pixelMapOrWant()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Want not find");
        pasteData = nullptr;
        return;
    }
    uintptr_t wantPtr = value.get_pixelMapOrWant_ref();
    ani_object wantObj = reinterpret_cast<ani_object>(wantPtr);
    OHOS::AAFwk::Want want;
    bool ret = OHOS::AppExecFwk::UnwrapWant(taihe::get_env(), wantObj, want);
    if (!ret) {
        taihe::set_business_error(
            static_cast<int>(JSErrorCode::INVALID_PARAMETERS), "Parameter error. want get failed");
        pasteData = nullptr;
        return;
    }
    pasteData = PasteboardClient::GetInstance()->CreateWantData(std::make_shared<OHOS::AAFwk::Want>(want));
}

pasteboardTaihe::ValueType WantStrategy::ConvertToValueType(
    const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry)
{
    uintptr_t wantPtr = 0;
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "entry is nullptr");
        return pasteboardTaihe::ValueType::make_pixelMapOrWant(wantPtr);
    }
    std::shared_ptr<AAFwk::Want> want = entry->ConvertToWant();
    if (want == nullptr) {
        taihe::set_business_error(
            static_cast<int>(JSErrorCode::INVALID_PARAMETERS), "Parameter error. want get failed");
        return pasteboardTaihe::ValueType::make_pixelMapOrWant(wantPtr);
    }
    ani_object wantObj = OHOS::AppExecFwk::WrapWant(taihe::get_env(), *want);
    wantPtr = reinterpret_cast<uintptr_t>(wantObj);
    return pasteboardTaihe::ValueType::make_pixelMapOrWant(wantPtr);
}

EntryValue WantStrategy::ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value)
{
    if (!value.holds_pixelMapOrWant()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Want not find");
        return std::monostate {};
    }
    uintptr_t wantRef = value.get_pixelMapOrWant_ref();
    ani_object wantObj = reinterpret_cast<ani_object>(wantRef);
    OHOS::AAFwk::Want want;
    bool ret = OHOS::AppExecFwk::UnwrapWant(taihe::get_env(), wantObj, want);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Unwrap want failed");
    }
    auto wantPtr = std::make_shared<OHOS::AAFwk::Want>(want);
    return wantPtr;
}

void HtmlStrategy::AddRecord(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "html not find");
        return;
    }
    std::string html(value.get_string_ref());
    pasteData->AddHtmlRecord(html);
}

void HtmlStrategy::CreateData(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> &pasteData)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "html not find");
        pasteData = nullptr;
        return;
    }
    std::string html(value.get_string_ref());
    pasteData = PasteboardClient::GetInstance()->CreateHtmlData(html);
}

pasteboardTaihe::ValueType HtmlStrategy::ConvertToValueType(
    const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry)
{
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "entry is nullptr");
        return pasteboardTaihe::ValueType::make_string("");
    }
    std::shared_ptr<std::string> str = entry->ConvertToHtml();
    if (str == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "html not find");
        return pasteboardTaihe::ValueType::make_string("");
    }
    return pasteboardTaihe::ValueType::make_string(*str);
}

EntryValue HtmlStrategy::ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "html not find");
        return std::monostate {};
    }
    std::string html(value.get_string_ref());
    return html;
}

void TextStrategy::AddRecord(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "text not find");
        return;
    }
    std::string text(value.get_string_ref());
    pasteData->AddTextRecord(text);
}

void TextStrategy::CreateData(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> &pasteData)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "text not find");
        pasteData = nullptr;
        return;
    }
    std::string text(value.get_string_ref());
    pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(text);
}

pasteboardTaihe::ValueType TextStrategy::ConvertToValueType(
    const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry)
{
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "entry is nullptr");
        return pasteboardTaihe::ValueType::make_string("");
    }
    std::shared_ptr<std::string> str = entry->ConvertToPlainText();
    if (str == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "text not find");
        return pasteboardTaihe::ValueType::make_string("");
    }
    return pasteboardTaihe::ValueType::make_string(*str);
}

EntryValue TextStrategy::ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "text not find");
        return std::monostate {};
    }
    std::string text(value.get_string_ref());
    return text;
}

void UriStrategy::AddRecord(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> pasteData)
{
    if (!value.holds_string()) {
        return;
    }
    std::string uriStr(value.get_string_ref());
    pasteData->AddUriRecord(OHOS::Uri(uriStr));
}

void UriStrategy::CreateData(
    const std::string &mimeType, const pasteboardTaihe::ValueType &value, std::shared_ptr<PasteData> &pasteData)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "uri not find");
        pasteData = nullptr;
        return;
    }
    std::string uriStr(value.get_string_ref());
    pasteData = PasteboardClient::GetInstance()->CreateUriData(OHOS::Uri(uriStr));
}

pasteboardTaihe::ValueType UriStrategy::ConvertToValueType(
    const std::string &mimeType, std::shared_ptr<PasteDataEntry> entry)
{
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "entry is nullptr");
        return pasteboardTaihe::ValueType::make_string("");
    }
    std::shared_ptr<Uri> uri = entry->ConvertToUri();
    if (uri == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "uri not find");
        return pasteboardTaihe::ValueType::make_string("");
    }
    std::string uriStr = uri->ToString();
    return pasteboardTaihe::ValueType::make_string(uriStr);
}

EntryValue UriStrategy::ConvertFromValueType(const std::string &mimeType, const pasteboardTaihe::ValueType &value)
{
    if (!value.holds_string()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "uri not find");
        return std::monostate {};
    }
    std::string uriStr(value.get_string_ref());
    return uriStr;
}

std::unique_ptr<ValueTypeStrategy> StrategyFactory::CreateStrategyForRecord(
    const pasteboardTaihe::ValueType &value, const std::string &mimeType)
{
    if (mimeType == "") {
        return nullptr;
    }
    if (value.holds_arrayBuffer()) {
        return std::make_unique<ArrayBufferStrategy>();
    }
    if (mimeType == MIMETYPE_PIXELMAP) {
        return std::make_unique<PixelMapStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_WANT) {
        return std::make_unique<WantStrategy>();
    }
    if (!value.holds_string()) {
        return nullptr;
    }
    if (mimeType == MIMETYPE_TEXT_HTML) {
        return std::make_unique<HtmlStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_PLAIN) {
        return std::make_unique<TextStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_URI) {
        return std::make_unique<UriStrategy>();
    }
    return nullptr;
}

std::unique_ptr<ValueTypeStrategy> StrategyFactory::CreateStrategyForData(const std::string &mimeType)
{
    if (mimeType == "") {
        return nullptr;
    }
    if (mimeType == MIMETYPE_PIXELMAP) {
        return std::make_unique<PixelMapStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_WANT) {
        return std::make_unique<WantStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_HTML) {
        return std::make_unique<HtmlStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_PLAIN) {
        return std::make_unique<TextStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_URI) {
        return std::make_unique<UriStrategy>();
    }
    return std::make_unique<ArrayBufferStrategy>();
}

std::unique_ptr<ValueTypeStrategy> StrategyFactory::CreateStrategyForEntry(const std::string &mimeType)
{
    if (mimeType == MIMETYPE_TEXT_URI) {
        return std::make_unique<UriStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_PLAIN) {
        return std::make_unique<TextStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_HTML) {
        return std::make_unique<HtmlStrategy>();
    }
    if (mimeType == MIMETYPE_PIXELMAP) {
        return std::make_unique<PixelMapStrategy>();
    }
    if (mimeType == MIMETYPE_TEXT_WANT) {
        return std::make_unique<WantStrategy>();
    }
    return std::make_unique<ArrayBufferStrategy>();
}

ShareOption ShareOptionAdapter::FromTaihe(pasteboardTaihe::ShareOption value)
{
    switch (value.get_key()) {
        case pasteboardTaihe::ShareOption::key_t::INAPP:
            return ShareOption::InApp;
        default:
            return ShareOption::LocalDevice;
    }
}

pasteboardTaihe::ShareOption ShareOptionAdapter::ToTaihe(ShareOption value)
{
    switch (value) {
        case ShareOption::InApp:
            return pasteboardTaihe::ShareOption::key_t::INAPP;
        default:
            return pasteboardTaihe::ShareOption::key_t::LOCALDEVICE;
    }
}
} // namespace MiscServices
} // namespace OHOS