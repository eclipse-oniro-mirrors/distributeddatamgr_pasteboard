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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "sg_auth_client_mock.h"
#include "security_guard_adapter.h"
#include "pasteboard_error.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;
using namespace OHOS::Security::SecurityGuard;

namespace {
constexpr uint32_t SRC_TOKEN_ID = 100;
constexpr uint32_t DST_TOKEN_ID = 200;
constexpr uint32_t TIMEOUT_MS_FAST = 10;
constexpr uint32_t TIMEOUT_MS_150 = 150;
constexpr uint32_t TIMEOUT_MS_200 = 200;
constexpr uint32_t TIMEOUT_MS_300 = 300;
constexpr uint32_t TIMEOUT_MS_EXCEED_MAX = 600;
} // namespace

class PasteboardSecurityGuardMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void PasteboardSecurityGuardMockTest::SetUpTestCase(void)
{
    SecurityGuardAdapter::GetInstance();
}

void PasteboardSecurityGuardMockTest::TearDownTestCase(void) {}

void PasteboardSecurityGuardMockTest::SetUp(void)
{
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.authProvider_.reset();
    adapter.hasAuthClientInfo_.store(false);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::ALLOW));
    adapter.cachedTimeoutMs_.store(SecurityGuardAdapter::MAX_TIMEOUT_MS);
}

void PasteboardSecurityGuardMockTest::TearDown(void)
{
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.authProvider_.reset();
    adapter.hasAuthClientInfo_.store(false);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::ALLOW));
    adapter.cachedTimeoutMs_.store(SecurityGuardAdapter::MAX_TIMEOUT_MS);
}

/**
 * @tc.name: SaId_001
 * @tc.desc: SECURITY_GUARD_SA_ID equals 3524 (DATA_COLLECT_MANAGER_SA_ID)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, SaId_001, TestSize.Level1)
{
    EXPECT_EQ(SecurityGuardAdapter::SECURITY_GUARD_SA_ID, 3524);
}

/**
 * @tc.name: MaxTimeoutMs_001
 * @tc.desc: MAX_TIMEOUT_MS equals 400
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, MaxTimeoutMs_001, TestSize.Level1)
{
    EXPECT_EQ(SecurityGuardAdapter::MAX_TIMEOUT_MS, 400u);
}

/**
 * @tc.name: QueryStrategy_NoProvider_NoAuthClientInfo_001
 * @tc.desc: provider null + no authClientInfo → ALLOW (inherit existing behavior)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_NoProvider_NoAuthClientInfo_001, TestSize.Level1)
{
    auto ret = SecurityGuardAdapter::GetInstance().QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: QueryStrategy_NoProvider_DefaultDeny_001
 * @tc.desc: provider null + authClientInfo with default DENY → DENY
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_NoProvider_DefaultDeny_001, TestSize.Level1)
{
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::DENY));
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::DENY);
}

/**
 * @tc.name: QueryStrategy_NoProvider_DefaultAllow_001
 * @tc.desc: provider null + authClientInfo with default ALLOW → ALLOW
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_NoProvider_DefaultAllow_001, TestSize.Level1)
{
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::ALLOW));
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: QueryStrategy_NoAuthClientInfo_001
 * @tc.desc: provider exists but no authClientInfo → ALLOW (inherit)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_NoAuthClientInfo_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(false);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: QueryStrategy_Allow_001
 * @tc.desc: provider exists + authClientInfo + RequestAuthResult returns ALLOW → ALLOW
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_Allow_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _))
        .WillOnce([](const AuthEventInfo &event, const AuthResultCallback &callback) {
            AuthResult result;
            result.mergedAuthStrategy = AuthStrategy::ALLOW;
            callback(result);
            return 0;
        });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: QueryStrategy_Deny_001
 * @tc.desc: provider exists + authClientInfo + RequestAuthResult returns DENY → DENY
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_Deny_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _))
        .WillOnce([](const AuthEventInfo &event, const AuthResultCallback &callback) {
            AuthResult result;
            result.mergedAuthStrategy = AuthStrategy::DENY;
            callback(result);
            return 0;
        });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::DENY);
}

/**
 * @tc.name: TimeoutProtectionTest001
 * @tc.desc: RequestAuthResult never calls callback → timeout → default ALLOW → ALLOW
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, TimeoutProtectionTest001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _)).WillOnce(Return(0));
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::ALLOW));
    adapter.cachedTimeoutMs_.store(TIMEOUT_MS_FAST);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: TimeoutProtectionTest002
 * @tc.desc: RequestAuthResult never calls callback → timeout → default DENY → DENY
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, TimeoutProtectionTest002, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _)).WillOnce(Return(0));
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::DENY));
    adapter.cachedTimeoutMs_.store(TIMEOUT_MS_FAST);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::DENY);
}

/**
 * @tc.name: TimeoutProtectionTest003
 * @tc.desc: provider exists but no authClientInfo → timeout path not reached → ALLOW (inherit)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, TimeoutProtectionTest003, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _)).Times(0);
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(false);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: TimeoutProtectionTest004
 * @tc.desc: callback arrives within timeout with DENY → DENY (not timeout path)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, TimeoutProtectionTest004, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _))
        .WillOnce([](const AuthEventInfo &event, const AuthResultCallback &callback) {
            AuthResult result;
            result.mergedAuthStrategy = AuthStrategy::DENY;
            callback(result);
            return 0;
        });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedTimeoutMs_.store(TIMEOUT_MS_200);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::DENY);
}

/**
 * @tc.name: LateCallbackIgnoreTest001
 * @tc.desc: after timeout, late callback is ignored, no crash
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, LateCallbackIgnoreTest001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    AuthResultCallback capturedCallback;
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _))
        .WillOnce(DoAll(SaveArg<1>(&capturedCallback), Return(0)));
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::ALLOW));
    adapter.cachedTimeoutMs_.store(TIMEOUT_MS_FAST);
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
    ASSERT_NE(capturedCallback, nullptr);
    AuthResult lateResult;
    lateResult.mergedAuthStrategy = AuthStrategy::DENY;
    EXPECT_NO_FATAL_FAILURE(capturedCallback(lateResult));
}

/**
 * @tc.name: QueryStrategy_RequestFailed_DefaultAllow_001
 * @tc.desc: RequestAuthResult returns error → fallback to default ALLOW → ALLOW
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_RequestFailed_DefaultAllow_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _)).WillOnce(Return(-1));
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::ALLOW));
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::ALLOW);
}

/**
 * @tc.name: QueryStrategy_RequestFailed_DefaultDeny_001
 * @tc.desc: RequestAuthResult returns error → fallback to default DENY → DENY
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, QueryStrategy_RequestFailed_DefaultDeny_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    auto *rawProvider = mockProvider.get();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    EXPECT_CALL(*rawProvider, RequestAuthResult(_, _)).WillOnce(Return(-1));
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    adapter.cachedDefaultStrategy_.store(static_cast<int32_t>(AuthStrategy::DENY));
    adapter.OnProviderCreate();
    auto ret = adapter.QueryStrategy(SRC_TOKEN_ID, DST_TOKEN_ID);
    EXPECT_EQ(ret, SecurityGuardAdapter::StrategyResult::DENY);
}

/**
 * @tc.name: OnProviderCreate_001
 * @tc.desc: OnProviderCreate calls CreateAuthProvider, provider non-null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, OnProviderCreate_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    SecurityGuardAdapter::GetInstance().OnProviderCreate();
    EXPECT_NE(SecurityGuardAdapter::GetInstance().authProvider_, nullptr);
}

/**
 * @tc.name: OnProviderCreate_Failed_001
 * @tc.desc: CreateAuthProvider returns error → provider stays null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, OnProviderCreate_Failed_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _)).WillOnce(Return(-1));
    SecurityGuardAdapter::GetInstance().OnProviderCreate();
    EXPECT_EQ(SecurityGuardAdapter::GetInstance().authProvider_, nullptr);
}

/**
 * @tc.name: OnProviderDestroy_001
 * @tc.desc: OnProviderDestroy releases provider
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, OnProviderDestroy_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.OnProviderCreate();
    EXPECT_NE(adapter.authProvider_, nullptr);
    adapter.OnProviderDestroy();
    EXPECT_EQ(adapter.authProvider_, nullptr);
}

/**
 * @tc.name: ShouldRestrictDistributedData_NoAuthClientInfo_001
 * @tc.desc: no authClientInfo → false
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, ShouldRestrictDistributedData_NoAuthClientInfo_001, TestSize.Level1)
{
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(false);
    EXPECT_FALSE(adapter.ShouldRestrictDistributedData());
}

/**
 * @tc.name: ShouldRestrictDistributedData_HasAuthClientInfo_001
 * @tc.desc: has authClientInfo → true
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, ShouldRestrictDistributedData_HasAuthClientInfo_001, TestSize.Level1)
{
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.hasAuthClientInfo_.store(true);
    EXPECT_TRUE(adapter.ShouldRestrictDistributedData());
}

/**
 * @tc.name: OnClientStatusChanged_MultipleClients_DenyPriority_001
 * @tc.desc: multiple clientInfos, one DENY → cached DENY, min timeout
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, OnClientStatusChanged_MultipleClients_DenyPriority_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.OnProviderCreate();
    std::vector<AuthClientInfo> clientInfos;
    AuthClientInfo info1{};
    info1.defaultTimeoutStrategy = AuthStrategy::ALLOW;
    info1.defaultTimeoutMs = TIMEOUT_MS_300;
    AuthClientInfo info2{};
    info2.defaultTimeoutStrategy = AuthStrategy::DENY;
    info2.defaultTimeoutMs = TIMEOUT_MS_200;
    clientInfos.push_back(info1);
    clientInfos.push_back(info2);
    adapter.authCallback_->OnClientStatusChanged(clientInfos);
    EXPECT_EQ(adapter.cachedDefaultStrategy_.load(), static_cast<int32_t>(AuthStrategy::DENY));
    EXPECT_EQ(adapter.cachedTimeoutMs_.load(), TIMEOUT_MS_200);
    EXPECT_TRUE(adapter.hasAuthClientInfo_.load());
}

/**
 * @tc.name: OnClientStatusChanged_AllAllow_MinTimeout_001
 * @tc.desc: all ALLOW → cached ALLOW, min timeout
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, OnClientStatusChanged_AllAllow_MinTimeout_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.OnProviderCreate();
    std::vector<AuthClientInfo> clientInfos;
    AuthClientInfo info1{};
    info1.defaultTimeoutStrategy = AuthStrategy::ALLOW;
    info1.defaultTimeoutMs = TIMEOUT_MS_300;
    AuthClientInfo info2{};
    info2.defaultTimeoutStrategy = AuthStrategy::ALLOW;
    info2.defaultTimeoutMs = TIMEOUT_MS_150;
    clientInfos.push_back(info1);
    clientInfos.push_back(info2);
    adapter.authCallback_->OnClientStatusChanged(clientInfos);
    EXPECT_EQ(adapter.cachedDefaultStrategy_.load(), static_cast<int32_t>(AuthStrategy::ALLOW));
    EXPECT_EQ(adapter.cachedTimeoutMs_.load(), TIMEOUT_MS_150);
}

/**
 * @tc.name: OnClientStatusChanged_TimeoutCapped_001
 * @tc.desc: minTimeoutMs > MAX_TIMEOUT_MS → capped to 400
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSecurityGuardMockTest, OnClientStatusChanged_TimeoutCapped_001, TestSize.Level1)
{
    NiceMock<SgAuthClientMock> sgMock;
    auto mockProvider = std::make_unique<MockAuthProvider>();
    EXPECT_CALL(sgMock, CreateAuthProvider(_, _, _))
        .WillOnce([&mockProvider](const AuthProviderCallback *, const std::string &,
            std::unique_ptr<AuthProvider> &provider) { provider = std::move(mockProvider); return 0; });
    auto &adapter = SecurityGuardAdapter::GetInstance();
    adapter.OnProviderCreate();
    std::vector<AuthClientInfo> clientInfos;
    AuthClientInfo info{};
    info.defaultTimeoutStrategy = AuthStrategy::ALLOW;
    info.defaultTimeoutMs = TIMEOUT_MS_EXCEED_MAX;
    clientInfos.push_back(info);
    adapter.authCallback_->OnClientStatusChanged(clientInfos);
    EXPECT_EQ(adapter.cachedTimeoutMs_.load(), SecurityGuardAdapter::MAX_TIMEOUT_MS);
}
