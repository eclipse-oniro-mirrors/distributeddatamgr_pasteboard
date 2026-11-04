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

#ifndef SECURITY_GUARD_ADAPTER_H
#define SECURITY_GUARD_ADAPTER_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace OHOS {
namespace Security {
namespace SecurityGuard {
class AuthProviderSubscribeClient;
class AuthClientInfo;
} // namespace SecurityGuard
} // namespace Security

namespace MiscServices {

class SecurityGuardAdapter {
public:
    enum StrategyResult : int32_t {
        ALLOW = 0,
        DENY = 1,
    };

    static constexpr int32_t SECURITY_GUARD_SA_ID = 3524;
    static constexpr int64_t PASTEBOARD_EVENT_ID = 0x27810000;
    static constexpr const char *AUTH_EVENT_VERSION = "1.0";
    static constexpr const char *SERVICE_ID = "pasteboard_service";

    static SecurityGuardAdapter &GetInstance();

    void OnProviderCreate();
    void OnProviderDestroy();
    StrategyResult QueryStrategy(uint32_t srcTokenId, uint32_t dstTokenId);
    bool ShouldRestrictDistributedData() const;

    ~SecurityGuardAdapter();

private:
    static constexpr uint32_t MAX_TIMEOUT_MS = 400;

    SecurityGuardAdapter();

    StrategyResult FallbackToDefault(int32_t cachedStrategy) const
    {
        return cachedStrategy == static_cast<int32_t>(StrategyResult::DENY)
            ? StrategyResult::DENY : StrategyResult::ALLOW;
    }

    void HandleClientStatus(const std::vector<Security::SecurityGuard::AuthClientInfo> &clientInfos);

    mutable std::mutex providerMutex_;
    std::atomic<bool> hasAuthClientInfo_{false};
    std::atomic<int32_t> cachedDefaultStrategy_{0};
    std::atomic<uint32_t> cachedTimeoutMs_{MAX_TIMEOUT_MS};
    std::atomic<bool> destroyed_{false};

    std::shared_ptr<Security::SecurityGuard::AuthProviderSubscribeClient> authProvider_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // SECURITY_GUARD_ADAPTER_H
