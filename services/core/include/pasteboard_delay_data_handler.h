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

#ifndef PASTEBOARD_DELAY_DATA_HANDLER_H
#define PASTEBOARD_DELAY_DATA_HANDLER_H

#include "pasteboard_service.h"
#include "ipasteboard_delay_getter.h"
#include "ipasteboard_entry_getter.h"

namespace OHOS {
namespace MiscServices {

class PasteboardDelayDataHandler {
public:
    class DelayGetterDeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        explicit DelayGetterDeathRecipient(int32_t userId, PasteboardDelayDataHandler& handler);
        virtual ~DelayGetterDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject>& remote) override;
    private:
        int32_t userId_ = ERROR_USERID;
        PasteboardDelayDataHandler& handler_;
    };

    class EntryGetterDeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        explicit EntryGetterDeathRecipient(int32_t userId, PasteboardDelayDataHandler& handler);
        virtual ~EntryGetterDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject>& remote) override;
    private:
        int32_t userId_ = ERROR_USERID;
        PasteboardDelayDataHandler& handler_;
    };

    explicit PasteboardDelayDataHandler(PasteboardService& service);
    ~PasteboardDelayDataHandler();

    int32_t GetDelayPasteRecord(int32_t userId, PasteData& data);
    void GetDelayPasteData(int32_t userId, PasteData& data);
    int32_t GetFullDelayPasteData(int32_t userId, PasteData& data);
    int32_t SyncDelayedData();

    int32_t ProcessDelayHtmlEntry(PasteData& data, const AppInfo& targetAppInfo, PasteDataEntry& entry);
    int32_t PostProcessDelayHtmlEntry(PasteData& data, const AppInfo& targetAppInfo, PasteDataEntry& entry);

    int32_t GetLocalEntryValue(int32_t userId, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t GetRemoteEntryValue(const AppInfo& appInfo, PasteData& data, PasteDataRecord& record,
        PasteDataEntry& entry);
    int32_t ProcessRemoteDelayUri(const std::string& deviceId, const AppInfo& appInfo,
        PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t ProcessRemoteDelayHtml(const std::string& remoteDeviceId, const AppInfo& appInfo,
        const std::vector<uint8_t>& rawData, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry);
    int32_t ProcessRemoteDelayHtmlInner(const std::string& remoteDeviceId, const AppInfo& appInfo,
        PasteData& tmpData, PasteData& data, PasteDataEntry& entry);

    void NotifyDelayGetterDied(int32_t userId);
    void NotifyEntryGetterDied(int32_t userId);

private:
    PasteboardService& service_;
    ConcurrentMap<int32_t, std::pair<sptr<IPasteboardDelayGetter>, sptr<DelayGetterDeathRecipient>>> delayGetters_;
    ConcurrentMap<int32_t, std::pair<sptr<IPasteboardEntryGetter>, sptr<EntryGetterDeathRecipient>>> entryGetters_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_DELAY_DATA_HANDLER_H