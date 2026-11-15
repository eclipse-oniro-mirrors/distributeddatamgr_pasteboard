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

#include <string>
#include <vector>

#include "common/block_object.h"
#include "pasteboard_hilog.h"
#include "auth_provider_subscribe_client.h"

namespace OHOS {
namespace MiscServices {

namespace {
int32_t RequestBlockStrategy(
    const std::shared_ptr<Security::SecurityGuard::AuthProviderSubscribeClient> &provider,
    uint32_t srcTokenId, uint32_t dstTokenId, uint32_t timeoutMs)
{
    std::string content = "{\"srcTokenId\":" + std::to_string(srcTokenId) +
        ",\"dstTokenId\":" + std::to_string(dstTokenId) + "}";
    Security::SecurityGuard::AuthEventInfo event(
        SecurityGuardAdapter::PASTEBOARD_EVENT_ID, SecurityGuardAdapter::AUTH_EVENT_VERSION, content);
    auto blockObject = std::make_shared<BlockObject<int32_t>>(
        timeoutMs, static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::TIMEOUT));
    auto resultCallback = [blockObject](const Security::SecurityGuard::AuthResult &result) {
        blockObject->SetValue(static_cast<int32_t>(result.GetMergedAuthStrategy()));
    };
    int32_t ret = provider->RequestAuthResult(event, resultCallback);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "RequestAuthResult failed, ret=%{public}d", ret);
        return static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::TIMEOUT);
    }
    return blockObject->GetValue();
}
} // namespace

SecurityGuardAdapter::SecurityGuardAdapter() = default;

SecurityGuardAdapter::~SecurityGuardAdapter()
{
    std::shared_ptr<Security::SecurityGuard::AuthProviderSubscribeClient> provider;
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        destroyed_.store(true);
        provider = std::move(authProvider_);
    }
    if (provider != nullptr) {
        provider->DeleteProvider();
    }
}

void SecurityGuardAdapter::OnProviderCreate()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "begin.");
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        if (authProvider_ != nullptr) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "provider already exists.");
            return;
        }
        destroyed_.store(false);
    }
    Security::SecurityGuard::AuthProviderCallback callback =
        [this](const std::vector<Security::SecurityGuard::AuthClientInfo> &clientInfos) {
            HandleClientStatus(clientInfos);
        };
    std::shared_ptr<Security::SecurityGuard::AuthProviderSubscribeClient> provider;
    int32_t ret = Security::SecurityGuard::AuthProviderSubscribeClient::CreateProvider(
        callback, SERVICE_ID, provider);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "CreateProvider ret=%{public}d", ret);
        return;
    }
    if (provider == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "CreateProvider ok but provider null");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        if (authProvider_ != nullptr) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "provider created concurrently, cleanup duplicate.");
            provider->DeleteProvider();
            return;
        }
        authProvider_ = std::move(provider);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AuthProvider created.");
}

void SecurityGuardAdapter::HandleClientStatus(
    const std::vector<Security::SecurityGuard::AuthClientInfo> &clientInfos)
{
    if (clientInfos.empty()) {
        std::lock_guard<std::mutex> lock(providerMutex_);
        if (destroyed_.load()) {
            return;
        }
        hasAuthClientInfo_.store(false);
        return;
    }
    bool hasDeny = false;
    int32_t minTimeoutMs = clientInfos.front().GetDefaultTimeoutMs();
    for (const auto &info : clientInfos) {
        if (info.GetDefaultTimeoutStrategy() == Security::SecurityGuard::AuthStrategy::DENY) {
            hasDeny = true;
        }
        int32_t timeout = info.GetDefaultTimeoutMs();
        if (timeout < minTimeoutMs) {
            minTimeoutMs = timeout;
        }
    }
    int32_t strategy = static_cast<int32_t>(hasDeny ? Security::SecurityGuard::AuthStrategy::DENY
                                                     : Security::SecurityGuard::AuthStrategy::ALLOW);
    uint32_t timeoutMs = (minTimeoutMs <= 0 || static_cast<uint32_t>(minTimeoutMs) > MAX_TIMEOUT_MS)
        ? MAX_TIMEOUT_MS : static_cast<uint32_t>(minTimeoutMs);
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        if (destroyed_.load()) {
            return;
        }
        cachedDefaultStrategy_.store(strategy);
        cachedTimeoutMs_.store(timeoutMs);
        hasAuthClientInfo_.store(true);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "cached strategy=%{public}d, timeout=%{public}u", hasDeny ? 1 : 0, timeoutMs);
}

void SecurityGuardAdapter::OnProviderDestroy()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "begin.");
    std::shared_ptr<Security::SecurityGuard::AuthProviderSubscribeClient> provider;
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        destroyed_.store(true);
        provider = std::move(authProvider_);
        hasAuthClientInfo_.store(false);
        cachedDefaultStrategy_.store(static_cast<int32_t>(StrategyResult::ALLOW));
        cachedTimeoutMs_.store(MAX_TIMEOUT_MS);
    }
    if (provider != nullptr) {
        provider->DeleteProvider();
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AuthProvider released.");
}

SecurityGuardAdapter::StrategyResult SecurityGuardAdapter::QueryStrategy(uint32_t srcTokenId, uint32_t dstTokenId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "QueryStrategy src=%{public}u dst=%{public}u", srcTokenId, dstTokenId);
    std::shared_ptr<Security::SecurityGuard::AuthProviderSubscribeClient> providerRef;
    bool hasAuthClientInfo = false;
    int32_t cachedDefaultStrategy = 0;
    uint32_t cachedTimeoutMs = MAX_TIMEOUT_MS;
    {
        std::lock_guard<std::mutex> lock(providerMutex_);
        providerRef = authProvider_;
        hasAuthClientInfo = hasAuthClientInfo_.load();
        cachedDefaultStrategy = cachedDefaultStrategy_.load();
        cachedTimeoutMs = cachedTimeoutMs_.load();
    }
    if (providerRef == nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "provider null, fallback.");
        if (hasAuthClientInfo) {
            return FallbackToDefault(cachedDefaultStrategy);
        }
        return StrategyResult::ALLOW;
    }
    if (!hasAuthClientInfo) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "no authClientInfo, inherit.");
        return StrategyResult::ALLOW;
    }
    if (cachedTimeoutMs == 0 || cachedTimeoutMs > MAX_TIMEOUT_MS) {
        cachedTimeoutMs = MAX_TIMEOUT_MS;
    }
    auto strategy = RequestBlockStrategy(providerRef, srcTokenId, dstTokenId, cachedTimeoutMs);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "strategy result=%{public}d", strategy);
    if (strategy == static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::DENY)) {
        return StrategyResult::DENY;
    }
    if (strategy == static_cast<int32_t>(Security::SecurityGuard::AuthStrategy::TIMEOUT)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "timeout, fallback to default.");
        return FallbackToDefault(cachedDefaultStrategy);
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
