/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_P2P_LINK_MANAGER_H
#define PASTEBOARD_P2P_LINK_MANAGER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "common/block_object.h"
#include "common/concurrent_map.h"
#include "ffrt/ffrt_utils.h"

namespace OHOS {
namespace MiscServices {

class ClipPlugin;

class P2PLinkManager {
public:
    static constexpr int32_t MIN_TRANMISSION_TIME = 30 * 1000; // ms
    static constexpr int32_t IDLE_TIMEOUT = 2 * 60 * 1000; // ms
    static constexpr int32_t SET_VALUE_SUCCESS = 1;
    static constexpr int32_t SET_VALUE_FAILURE = 0;

    P2PLinkManager() = default;
    ~P2PLinkManager() = default;

    void Init(std::shared_ptr<FFRTTimer> timer, std::function<std::shared_ptr<ClipPlugin>()> pluginGetter);

    std::shared_ptr<BlockObject<int32_t>> EnsureLink(const std::string &networkId, bool applyResourcesOnReuse = true);

    void PreEstablish(const std::string &networkId);

    void Release(const std::string &networkId);

    void ForceClose(const std::string &networkId);

    void CloseAll();

    bool IsEstablished(const std::string &networkId);

    bool HasLink(const std::string &networkId);

    void RemoveLink(const std::string &networkId);

    std::vector<std::string> GetAllNetworkIds();

private:
    enum class LinkState : int32_t {
        IDLE,
        ESTABLISHING,
        ESTABLISHED,
    };

    struct LinkEntry {
        std::atomic<LinkState> state{LinkState::IDLE};
        int32_t refCount{0};
        bool resourcesApplied{false};
        std::shared_ptr<BlockObject<int32_t>> establishBlock;
        std::vector<std::shared_ptr<BlockObject<int32_t>>> waiters;
        FFRTMutex mutex;
    };

    ConcurrentMap<std::string, std::shared_ptr<LinkEntry>> links_;
    std::shared_ptr<FFRTTimer> timer_;
    std::function<std::shared_ptr<ClipPlugin>()> pluginGetter_;

    std::shared_ptr<LinkEntry> GetOrCreateEntry(const std::string &networkId);
    void ScheduleEstablish(const std::string &networkId, std::shared_ptr<LinkEntry> entry, bool applyResources);
    void ApplyResourceOnReuse(const std::string &networkId, std::shared_ptr<LinkEntry> entry);
    void DoEstablish(const std::string &networkId, std::shared_ptr<LinkEntry> entry, bool applyResources);
    bool ApplyAdvancedResource(const std::string &networkId, std::shared_ptr<ClipPlugin> plugin,
        std::shared_ptr<LinkEntry> entry);
    bool ConnectDfsAndPublish(const std::string &networkId, std::shared_ptr<ClipPlugin> plugin,
        std::shared_ptr<LinkEntry> entry, bool applyResources);
    void DoCloseLink(const std::string &networkId);
    void StartIdleTimer(const std::string &networkId);
    void CancelIdleTimer(const std::string &networkId);
    void OnIdleTimeout(const std::string &networkId);
    void NotifyWaiters(std::shared_ptr<LinkEntry> entry, int32_t value);
    void CleanupEntryIfIdle(const std::string &networkId, std::shared_ptr<LinkEntry> entry);
    static std::string IdleTimerName(const std::string &networkId);
    static std::string EstablishTimerName(const std::string &networkId);
};

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_P2P_LINK_MANAGER_H
