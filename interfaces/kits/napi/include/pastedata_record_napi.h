/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef N_NAPI_PASTEDATA_RECORD_H
#define N_NAPI_PASTEDATA_RECORD_H

#include "napi_pasteboard_common.h"

namespace OHOS {
namespace MiscServicesNapi {
class PastedataRecordEntryGetterInstance : public std::enable_shared_from_this<PastedataRecordEntryGetterInstance> {
public:
    class PastedataRecordEntryGetterImpl : public UDMF::EntryGetter {
    public:
        explicit PastedataRecordEntryGetterImpl() = default;
        UDMF::ValueType GetValueByType(const std::string &utdId) override;
        void SetEntryGetterWrapper(const std::shared_ptr<PastedataRecordEntryGetterInstance> instance);
    private:
        std::shared_ptr<PastedataRecordEntryGetterInstance> wrapper_;
    };
    explicit PastedataRecordEntryGetterInstance(const napi_env &env, const napi_ref &ref);
    ~PastedataRecordEntryGetterInstance();

    UDMF::ValueType GetValueByType(const std::string &utdId);

    napi_env GetEnv()
    {
        return env_;
    }

    napi_ref GetRef()
    {
        return ref_;
    }

    std::shared_ptr<PastedataRecordEntryGetterImpl> GetStub()
    {
        return stub_;
    }

private:
    napi_env env_ = nullptr;
    napi_ref ref_ = nullptr;
    std::shared_ptr<PastedataRecordEntryGetterImpl> stub_ = nullptr;
};

struct PasteboardEntryGetterWorker {
    std::string utdId;
    std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetter = nullptr;
    std::shared_ptr<UDMF::ValueType> entryValue = nullptr;
    bool complete = false;
    bool clean = false;
    std::condition_variable cv;
    std::mutex mutex;
};

class PasteDataRecordNapi {
public:
    static napi_value PasteDataRecordInit(napi_env env, napi_value exports);
    static napi_value New(napi_env env, napi_callback_info info);
    static napi_status NewInstance(napi_env env, napi_value &instance);
    static void Destructor(napi_env env, void *nativeObject, void *finalize_hint);
    static bool NewInstanceByRecord(
        napi_env env, napi_value &instance, const std::shared_ptr<MiscServices::PasteDataRecord> &record);
    static bool NewHtmlTextRecordInstance(napi_env env, const std::string &text, napi_value &instance);
    static bool NewPlainTextRecordInstance(napi_env env, const std::string &text, napi_value &instance);
    static bool NewPixelMapRecordInstance(
        napi_env env, const std::shared_ptr<OHOS::Media::PixelMap> pixelMap, napi_value &instance);
    static bool NewUriRecordInstance(napi_env env, const std::string &text, napi_value &instance);
    static bool NewWantRecordInstance(
        napi_env env, const std::shared_ptr<OHOS::AAFwk::Want> want, napi_value &instance);
    static bool NewKvRecordInstance(
        napi_env env, const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer, napi_value &instance);
    static bool NewEntryGetterRecordInstance(
        const std::vector<std::string> &mimeTypes,
        std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetter,
        napi_value &instance);
    static napi_value CreatKvData(napi_env env, std::shared_ptr<MiscServices::MineCustomData> customData);
    static std::shared_ptr<MiscServices::MineCustomData> GetNativeKvData(napi_env env, napi_value napiValue);
    static napi_value CreateInstance(napi_env env, std::shared_ptr<MiscServices::PasteDataRecord> record);
    napi_value SetNapiKvData(napi_env env, std::shared_ptr<MiscServices::MineCustomData> customData);
    PasteDataRecordNapi();
    ~PasteDataRecordNapi();

    static napi_value ConvertToText(napi_env env, napi_callback_info info);
    static napi_value ConvertToTextV9(napi_env env, napi_callback_info info);
    static napi_value ToPlainText(napi_env env, napi_callback_info info);
    static napi_value AddEntry(napi_env env, napi_callback_info info);
    static napi_value GetValidTypes(napi_env env, napi_callback_info info);
    static napi_value GetRecordData(napi_env env, napi_callback_info info);

    std::shared_ptr<MiscServices::PasteDataRecord> value_;

private:
    void JSFillInstance(napi_env env, napi_value &instance);
    void SetNamedPropertyByStr(napi_env env, napi_value &instance, const char *propName, const char *propValue);
    std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetter_;
    napi_env env_;
};

extern "C" {
    API_EXPORT napi_value GetEtsPasteDataRecord(napi_env env, std::shared_ptr<MiscServices::PasteDataRecord> record);
}
} // namespace MiscServicesNapi
} // namespace OHOS
#endif