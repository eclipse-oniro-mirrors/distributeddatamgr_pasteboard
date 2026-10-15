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

#include "security_guard_adapter.h"

#include "common/block_object.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "sg_auth_client.h"

namespace OHOS {
namespace MiscServices {

class AuthCallbackImpl : public Security::SecurityGuard::AuthProviderCallback {
public:
    explicit AuthCallbackImpl(SecurityGuardAdapter &adapter) : adapter_(adapter) {}
    ~AuthCallbackImpl() override = default;
    void OnClientStatusChanged(const std::vector<Security::SecurityGuard::AuthClientInfo> &clientInfos) override
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnClientStatusChanged size=%{public}zu",
            clientInfos.size());
        if (clientInfos.empty()) {
            adapter_.hasAuthClientInfo_.store(false);
            return;
        }
        bool hasDeny = false;
        uint32_t minTimeoutMs = UINT32_MAX;
        for (const auto &info : clientInfos) {
            if (info.defaultTimeoutStrategy == Security::SecurityGuard::AuthStrategy::DENY) {
                hasDeny = true;
            }
            if (info.defaultTimeoutMs < minTimeoutMs) {
                minTimeoutMs = info.defaultTimeoutMs;
            }
        }
        adapter_.cachedDefaultStrategy_.store(
            static_cast<int32_t>(hasDeny ? Security::SecurityGuard::AuthStrategy::DENY
                                        : Security::SecurityGuard::AuthStrategy::ALLOW));
        uint32_t timeoutMs = (minTimeoutMs == 0 || minTimeoutMs > SecurityGuardAdapter::MAX_TIMEOUT_MS)
            ? SecurityGuardAdapter::MAX_TIMEOUT_MS : minTimeoutMs;
        adapter_.cachedTimeoutMs_.store(timeoutMs);
        adapter_.hasAuthClientInfo_.store(true);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "cached strategy=%{public}d, timeout=%{public}u", hasDeny ? 1 : 0, timeoutMs);
    }

private:
    SecurityGuardAdapter &adapter_;
};

SecurityGuardAdapter::SecurityGuardAdapter()
{
    authCallback_ = new AuthCallbackImpl(*this);
}

SecurityGuardAdapter::~SecurityGuardAdapter()
{
    delete authCallback_;
    authCallback_ = nullptr;
}

void SecurityGuardAdapter::OnProviderCreate()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "begin.");
    std::lock_guard<std::mutex> lock(providerMutex_);
    if (authProvider_ != nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "provider already exists.");
        return;
    }
    std::unique_ptr<Security::SecurityGuard::AuthProvider> provider;
    int32_t ret = Security::SecurityGuard::CreateAuthProvider(authCallback_, "pasteboard", provider);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "CreateAuthProvider ret=%{public}d", ret);
        return;
    }
    if (provider == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "CreateAuthProvider ok but provider null");
        return;
    }
    authProvider_ = std::move(provider);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AuthProvider created.");
}

void SecurityGuardAdapter::OnProviderDestroy()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "begin.");
    std::lock_guard<std::mutex> lock(providerMutex_);
    authProvider_.reset();
    hasAuthClientInfo_.store(false);
    cachedDefaultStrategy_.store(static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::ALLOW));
    cachedTimeoutMs_.store(MAX_TIMEOUT_MS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AuthProvider released.");
}

SecurityGuardAdapter::StrategyResult SecurityGuardAdapter::QueryStrategy(uint32_t srcTokenId, uint32_t dstTokenId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "QueryStrategy src=%{public}u dst=%{public}u", srcTokenId, dstTokenId);
    std::shared_ptr<Security::SecurityGuard::AuthProvider> providerRef;
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        providerRef = authProvider_;
    }
    if (providerRef == nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "provider null, fallback.");
        if (hasAuthClientInfo_.load()) {
            return FallbackToDefault();
        }
        return StrategyResult::ALLOW;
    }
    if (!hasAuthClientInfo_.load()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "no authClientInfo, inherit.");
        return StrategyResult::ALLOW;
    }
    Security::SecurityGuard::AuthEventInfo event;
    event.eventId = 0;
    event.version = "1.0";
    event.content = "{\"srcTokenId\":" + std::to_string(srcTokenId) +
        ",\"dstTokenId\":" + std::to_string(dstTokenId) + "}";
    uint32_t timeoutMs = cachedTimeoutMs_.load();
    if (timeoutMs == 0 || timeoutMs > SecurityGuardAdapter::MAX_TIMEOUT_MS) {
        timeoutMs = SecurityGuardAdapter::MAX_TIMEOUT_MS;
    }
    auto blockObject = std::make_shared<BlockObject<int32_t>>(
        timeoutMs, static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::TIMEOUT));
    auto callback = [blockObject](const Security::SecurityGuard::AuthResult &result) {
        blockObject->SetValue(static_cast<int32_t>(result.mergedAuthStrategy));
    };
    int32_t ret = providerRef->RequestAuthResult(event, callback);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "RequestAuthResult failed, ret=%{public}d", ret);
        return FallbackToDefault();
    }
    auto strategy = blockObject->GetValue();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "strategy result=%{public}d", strategy);
    if (strategy == static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::DENY)) {
        return StrategyResult::DENY;
    }
    if (strategy == static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::TIMEOUT)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "timeout, fallback to default.");
        return FallbackToDefault();
    }
    return StrategyResult::ALLOW;
}

bool SecurityGuardAdapter::ShouldRestrictDistributedData() const
{
    return hasAuthClientInfo_.load();
}

SecurityGuardAdapter &SecurityGuardAdapter::GetInstance()
{
    static SecurityGuardAdapter instance;
    return instance;
}

} // namespace MiscServices
} // namespace OHOS
