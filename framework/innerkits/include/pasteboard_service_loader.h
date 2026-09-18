/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_SERVICE_LOADER_H
#define PASTE_BOARD_SERVICE_LOADER_H

#include "ipasteboard_service.h"
#include "paste_data_entry.h"
#include "refbase.h"
#include "api/visibility.h"

namespace OHOS {
namespace MiscServices {

class API_EXPORT PasteboardSaDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit PasteboardSaDeathRecipient();
    ~PasteboardSaDeathRecipient() = default;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;

private:
    DISALLOW_COPY_AND_MOVE(PasteboardSaDeathRecipient);
};

class API_EXPORT PasteboardServiceLoader {
public:
    static PasteboardServiceLoader &GetInstance();
    static bool IsStaticDestroyed();
    sptr<IPasteboardService> GetPasteboardService();
    sptr<IPasteboardService> GetPasteboardServiceProxy();
    void SetPasteboardServiceProxy(const sptr<IRemoteObject> &remoteObject);
    void ReleaseDeathRecipient();
    int32_t GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry& value);
    void OnRemoteSaDied(const wptr<IRemoteObject> &object);
    void LoadSystemAbilityFail();
    void LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);
    void ClearPasteboardServiceProxy();
    void CleanupResource();

private:
    PasteboardServiceLoader();
    ~PasteboardServiceLoader();
    int32_t ProcessPasteData(PasteDataEntry &data, int64_t rawDataSize, int fd,
        const std::vector<uint8_t> &recvTLV);
    static sptr<IPasteboardService> pasteboardServiceProxy_;
    static std::condition_variable proxyConVar_;
    static std::mutex instanceLock_;
    static PasteboardServiceLoader instance_;
    bool constructing_ = false;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_{ nullptr };

    class StaticDestroyMonitor {
    public:
        StaticDestroyMonitor() = default;
        ~StaticDestroyMonitor()
        {
            destroyed_ = true;
        }

        bool IsDestroyed() const
        {
            return destroyed_;
        }

    private:
        DISALLOW_COPY_AND_MOVE(StaticDestroyMonitor);
        bool destroyed_ = false;
    };
    static StaticDestroyMonitor staticDestroyMonitor_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_SERVICE_LOADER_H
