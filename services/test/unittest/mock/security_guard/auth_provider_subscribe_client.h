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

#ifndef SECURITY_GUARD_AUTH_PROVIDER_SUBSCRIBE_CLIENT_H
#define SECURITY_GUARD_AUTH_PROVIDER_SUBSCRIBE_CLIENT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace OHOS::Security::SecurityGuard {

enum AuthStrategy {
    ALLOW = 0,
    DENY = 1,
    TIMEOUT = 2,
};

constexpr int64_t AUTH_RESULT_TIMEOUT_MS = 400;

class AuthClientInfo {
public:
    AuthClientInfo() = default;
    ~AuthClientInfo() = default;
    AuthStrategy GetDefaultTimeoutStrategy() const { return defaultTimeoutStrategy_; }
    int32_t GetDefaultTimeoutMs() const { return defaultTimeoutMs_; }
    void SetDefaultTimeoutStrategy(AuthStrategy strategy) { defaultTimeoutStrategy_ = strategy; }
    void SetDefaultTimeoutMs(int32_t timeouts) { defaultTimeoutMs_ = timeouts; }

private:
    AuthStrategy defaultTimeoutStrategy_ { ALLOW };
    int32_t defaultTimeoutMs_ { static_cast<int32_t>(AUTH_RESULT_TIMEOUT_MS) };
};

class AuthResult {
public:
    AuthResult() = default;
    ~AuthResult() = default;
    AuthStrategy GetMergedAuthStrategy() const { return mergedAuthStrategy_; }
    void SetMergedAuthStrategy(AuthStrategy strategy) { mergedAuthStrategy_ = strategy; }

private:
    AuthStrategy mergedAuthStrategy_ { ALLOW };
};

class AuthEventInfo {
public:
    AuthEventInfo() = default;
    AuthEventInfo(int64_t eventId, const std::string &version = "", const std::string &content = "")
        : eventId_(eventId), version_(version), content_(content) {}
    ~AuthEventInfo() = default;
    int64_t GetEventId() const { return eventId_; }
    void SetEventId(int64_t eventId) { eventId_ = eventId; }
    void SetVersion(const std::string &version) { version_ = version; }
    void SetContent(const std::string &content) { content_ = content; }

private:
    int64_t eventId_ = 0;
    std::string version_;
    std::string content_;
};

using AuthProviderCallback = std::function<void(const std::vector<AuthClientInfo> &clientsInfo)>;
using AuthResultCallback = std::function<void(const AuthResult &result)>;

class AuthProviderSubscribeClient {
public:
    virtual ~AuthProviderSubscribeClient() = default;
    virtual int32_t RequestAuthResult(const AuthEventInfo &event, AuthResultCallback callback) { return 0; }
    virtual void DeleteProvider() {}
    static int32_t CreateProvider(AuthProviderCallback callback, const std::string &serviceId,
        std::shared_ptr<AuthProviderSubscribeClient> &provider);
};

} // namespace OHOS::Security::SecurityGuard

#endif // SECURITY_GUARD_AUTH_PROVIDER_SUBSCRIBE_CLIENT_H
