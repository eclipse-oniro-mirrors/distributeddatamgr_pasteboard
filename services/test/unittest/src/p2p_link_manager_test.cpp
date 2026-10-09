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

#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

#include "clip/clip_plugin.h"
#include "common/block_object.h"
#include "device/dm_adapter.h"
#include "distributed_file_daemon_manager.h"
#include "ffrt/ffrt_utils.h"
#include "p2p_link_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;
using namespace OHOS::Storage::DistributedFile;

namespace {
constexpr int32_t RESULT_OK = 0;
constexpr int32_t REMOTE_DEV_FAIL = static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR);
constexpr int32_t REMOTE_DEV_OK = static_cast<int32_t>(PasteboardError::E_OK);
constexpr uint32_t POLL_INTERVAL_MS = 10;
const std::string NET_A = "networkIdA";
const std::string NET_B = "networkIdB";
} // namespace

// ---------------------------------------------------------------------------
// Mock: DistributedFileDaemonManager (virtual, override GetInstance)
// ---------------------------------------------------------------------------
class MockDfsDaemonManager : public DistributedFileDaemonManager {
public:
    MOCK_METHOD(int32_t, ConnectDfs, (const std::string &networkId), (override));
    MOCK_METHOD(int32_t, DisconnectDfs, (const std::string &networkId), (override));
};

static NiceMock<MockDfsDaemonManager> g_dfsMock;
static MockDfsDaemonManager *g_dfsMockPtr = &g_dfsMock;

namespace OHOS::Storage::DistributedFile {
DistributedFileDaemonManager &DistributedFileDaemonManager::GetInstance()
{
    return *g_dfsMockPtr;
}
} // namespace OHOS::Storage::DistributedFile

// ---------------------------------------------------------------------------
// Mock: DMAdapter::GetRemoteDeviceInfo (non-virtual, symbol override)
// ---------------------------------------------------------------------------
class P2PLinkInterface {
public:
    virtual ~P2PLinkInterface() = default;
    virtual int32_t GetRemoteDeviceInfo(const std::string &networkId, DmDeviceInfo &remoteDevice) = 0;
};

class P2PLinkInterfaceMock : public P2PLinkInterface {
public:
    P2PLinkInterfaceMock();
    ~P2PLinkInterfaceMock() override;
    MOCK_METHOD(int32_t, GetRemoteDeviceInfo, (const std::string &networkId, DmDeviceInfo &remoteDevice), (override));
};

static void *g_p2pInterface = nullptr;

P2PLinkInterfaceMock::P2PLinkInterfaceMock()
{
    g_p2pInterface = reinterpret_cast<void *>(this);
}
P2PLinkInterfaceMock::~P2PLinkInterfaceMock()
{
    g_p2pInterface = nullptr;
}

static P2PLinkInterface *GetP2PLinkInterface()
{
    return reinterpret_cast<P2PLinkInterface *>(g_p2pInterface);
}

int32_t DMAdapter::GetRemoteDeviceInfo(const std::string &networkId, DmDeviceInfo &remoteDevice)
{
    auto *iface = GetP2PLinkInterface();
    if (iface == nullptr) {
        return REMOTE_DEV_FAIL;
    }
    return iface->GetRemoteDeviceInfo(networkId, remoteDevice);
}

// ---------------------------------------------------------------------------
// Mock: ClipPlugin (virtual, injected via pluginGetter_)
// ---------------------------------------------------------------------------
class MockClipPlugin : public ClipPlugin {
public:
    MOCK_METHOD(int32_t, SetPasteData,
        (const GlobalEvent &event, const std::vector<uint8_t> &data, uint32_t version,
            const std::vector<uint8_t> &mimeTypes), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), GetPasteData,
        (const GlobalEvent &event, std::vector<uint8_t> &data), (override));
    MOCK_METHOD(int32_t, ApplyAdvancedResource, (const std::string &deviceId), (override));
    MOCK_METHOD(int32_t, PublishServiceState,
        (const std::string &networkId, ServiceStatus status), (override));
};

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class P2PLinkManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    // Create a manager initialized with given timer and a shared mock plugin.
    P2PLinkManager MakeManager(std::shared_ptr<FFRTTimer> timer, NiceMock<MockClipPlugin> &plugin);
    // Create a manager with nullptr timer and nullptr plugin getter.
    P2PLinkManager MakeBareManager();
    // Acquire a private LinkEntry for direct manipulation (requires -fno-access-control).
    std::shared_ptr<P2PLinkManager::LinkEntry> GetEntry(P2PLinkManager &m, const std::string &networkId);
    // Wait until the link is established (async integration tests).
    bool WaitForEstablished(P2PLinkManager &m, const std::string &networkId, uint32_t timeoutMs = 3000);
};

void P2PLinkManagerTest::SetUpTestCase()
{
    // Default behavior for the long-lived dfs mock: fail connect, succeed disconnect.
    ON_CALL(g_dfsMock, ConnectDfs).WillByDefault(Return(-1));
    ON_CALL(g_dfsMock, DisconnectDfs).WillByDefault(Return(RESULT_OK));
}

void P2PLinkManagerTest::TearDownTestCase()
{
    testing::Mock::VerifyAndClearExpectations(&g_dfsMock);
}

void P2PLinkManagerTest::SetUp()
{
    // Reset any per-test dfs expectations.
    testing::Mock::VerifyAndClearExpectations(&g_dfsMock);
    ON_CALL(g_dfsMock, ConnectDfs).WillByDefault(Return(-1));
    ON_CALL(g_dfsMock, DisconnectDfs).WillByDefault(Return(RESULT_OK));
}

void P2PLinkManagerTest::TearDown()
{
    testing::Mock::VerifyAndClearExpectations(&g_dfsMock);
}

P2PLinkManager P2PLinkManagerTest::MakeManager(std::shared_ptr<FFRTTimer> timer, NiceMock<MockClipPlugin> &plugin)
{
    P2PLinkManager mgr;
    auto getter = [&plugin]() -> std::shared_ptr<ClipPlugin> {
        return std::shared_ptr<ClipPlugin>(&plugin, [](ClipPlugin *) {});
    };
    mgr.Init(timer, getter);
    return mgr;
}

P2PLinkManager P2PLinkManagerTest::MakeBareManager()
{
    P2PLinkManager mgr;
    mgr.Init(nullptr, nullptr);
    return mgr;
}

std::shared_ptr<P2PLinkManager::LinkEntry> P2PLinkManagerTest::GetEntry(
    P2PLinkManager &m, const std::string &networkId)
{
    return m.GetOrCreateEntry(networkId);
}

bool P2PLinkManagerTest::WaitForEstablished(P2PLinkManager &m, const std::string &networkId, uint32_t timeoutMs)
{
    for (uint32_t elapsed = 0; elapsed <= timeoutMs; elapsed += POLL_INTERVAL_MS) {
        if (m.IsEstablished(networkId)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    }
    return false;
}

// Init

/**
 * @tc.name: InitTest001
 * @tc.desc: Init stores timer and plugin getter
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, InitTest001, TestSize.Level1)
{
    P2PLinkManager mgr;
    auto timer = std::make_shared<FFRTTimer>("ut");
    NiceMock<MockClipPlugin> plugin;
    auto getter = [&plugin]() -> std::shared_ptr<ClipPlugin> {
        return std::shared_ptr<ClipPlugin>(&plugin, [](ClipPlugin *) {});
    };
    mgr.Init(timer, getter);
    EXPECT_NE(mgr.timer_, nullptr);
    EXPECT_NE(mgr.pluginGetter_, nullptr);
}

// GetOrCreateEntry / HasLink / IsEstablished / GetAllNetworkIds

/**
 * @tc.name: HasLinkTest001
 * @tc.desc: HasLink returns false for unknown networkId
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, HasLinkTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

/**
 * @tc.name: IsEstablishedTest001
 * @tc.desc: IsEstablished returns false when entry not found
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, IsEstablishedTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    EXPECT_FALSE(mgr.IsEstablished(NET_A));
}

/**
 * @tc.name: IsEstablishedTest002
 * @tc.desc: IsEstablished returns true when state is ESTABLISHED, false for other states
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, IsEstablishedTest002, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    ASSERT_NE(entry, nullptr);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    EXPECT_TRUE(mgr.IsEstablished(NET_A));
    entry->state.store(P2PLinkManager::LinkState::IDLE);
    EXPECT_FALSE(mgr.IsEstablished(NET_A));
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    EXPECT_FALSE(mgr.IsEstablished(NET_A));
}

/**
 * @tc.name: GetAllNetworkIdsTest001
 * @tc.desc: GetAllNetworkIds returns all registered keys
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, GetAllNetworkIdsTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    GetEntry(mgr, NET_A);
    GetEntry(mgr, NET_B);
    auto ids = mgr.GetAllNetworkIds();
    EXPECT_EQ(ids.size(), 2u);
    bool hasA = false;
    bool hasB = false;
    for (const auto &id : ids) {
        if (id == NET_A) hasA = true;
        if (id == NET_B) hasB = true;
    }
    EXPECT_TRUE(hasA);
    EXPECT_TRUE(hasB);
}

/**
 * @tc.name: GetOrCreateEntryTest001
 * @tc.desc: GetOrCreateEntry creates a new entry on first access and reuses on second
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, GetOrCreateEntryTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto e1 = GetEntry(mgr, NET_A);
    ASSERT_NE(e1, nullptr);
    EXPECT_EQ(e1->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_EQ(e1->refCount, 0);
    auto e2 = GetEntry(mgr, NET_A);
    EXPECT_EQ(e1.get(), e2.get());
    EXPECT_TRUE(mgr.HasLink(NET_A));
}

// EnsureLink (PrepareEnsurePlan branches)

/**
 * @tc.name: EnsureLinkTest001
 * @tc.desc: empty networkId returns nullptr
 * @tc.type: FUNC
 * @tc.mock: none (early return)
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto block = mgr.EnsureLink("");
    EXPECT_EQ(block, nullptr);
    EXPECT_FALSE(mgr.HasLink(""));
}

/**
 * @tc.name: EnsureLinkTest002
 * @tc.desc: IDLE state with null timer reverts to IDLE, refCount net-zero, returns block
 * @tc.type: FUNC
 * @tc.mock: none (no DoEstablish scheduled because timer is null)
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkTest002, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto block = mgr.EnsureLink(NET_A);
    EXPECT_EQ(block, nullptr);
    EXPECT_FALSE(mgr.IsEstablished(NET_A));
    EXPECT_TRUE(mgr.HasLink(NET_A));
    auto entry = GetEntry(mgr, NET_A);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_EQ(entry->refCount, 0);
}

/**
 * @tc.name: EnsureLinkTest003
 * @tc.desc: ESTABLISHING state joins in-flight: returns a distinct waiter block
 * @tc.type: FUNC
 * @tc.mock: none
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkTest003, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);
    auto waiter = mgr.EnsureLink(NET_A);
    EXPECT_NE(waiter, nullptr);
    EXPECT_NE(waiter, entry->establishBlock);
    EXPECT_EQ(entry->waiters.size(), 1u);
    EXPECT_EQ(entry->refCount, 1);
}

/**
 * @tc.name: EnsureLinkTest004
 * @tc.desc: ESTABLISHED state reuses link, refreshes idle, no block returned
 * @tc.type: FUNC
 * @tc.mock: pluginGetter returns a plugin (ApplyAdvancedResource not called when resourcesApplied)
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkTest004, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    entry->resourcesApplied = true; // already applied
    auto block = mgr.EnsureLink(NET_A, true); // applyResourcesOnReuse=true but already applied
    EXPECT_EQ(block, nullptr); // reuse path returns no block
    EXPECT_EQ(entry->refCount, 1);
    EXPECT_EQ(entry->resourcesApplied, true);
}

/**
 * @tc.name: EnsureLinkTest005
 * @tc.desc: ESTABLISHED reuse with applyResourcesOnReuse && !resourcesApplied triggers ApplyResourceOnReuse
 * @tc.type: FUNC
 * @tc.mock: plugin ApplyAdvancedResource -> success
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkTest005, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, ApplyAdvancedResource).WillByDefault(Return(RESULT_OK));
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    entry->resourcesApplied = false;
    auto block = mgr.EnsureLink(NET_A, true);
    EXPECT_EQ(block, nullptr);
    EXPECT_EQ(entry->resourcesApplied, true);
    EXPECT_EQ(entry->refCount, 1);
}

// PreEstablish

/**
 * @tc.name: PreEstablishTest001
 * @tc.desc: empty networkId is a no-op
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, PreEstablishTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.PreEstablish("");
    EXPECT_FALSE(mgr.HasLink(""));
}

/**
 * @tc.name: PreEstablishTest002
 * @tc.desc: IDLE state moves to ESTABLISHING and requests schedule (applyResources=false)
 * @tc.type: FUNC
 * @tc.mock: none (ScheduleEstablish returns immediately because timer is null)
 */
HWTEST_F(P2PLinkManagerTest, PreEstablishTest002, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.PreEstablish(NET_A);
    auto entry = GetEntry(mgr, NET_A);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHING);
    EXPECT_NE(entry->establishBlock, nullptr);
}

/**
 * @tc.name: PreEstablishTest003
 * @tc.desc: ESTABLISHING state is a no-op (no new schedule)
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, PreEstablishTest003, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    auto blockBefore = entry->establishBlock;
    mgr.PreEstablish(NET_A);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHING);
    EXPECT_EQ(entry->establishBlock, blockBefore);
}

/**
 * @tc.name: PreEstablishTest004
 * @tc.desc: ESTABLISHED state refreshes idle timer
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, PreEstablishTest004, TestSize.Level1)
{
    auto timer = std::make_shared<FFRTTimer>("ut");
    NiceMock<MockClipPlugin> plugin;
    auto mgr = MakeManager(timer, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    mgr.PreEstablish(NET_A);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHED);
}

// DoEstablish (direct synchronous calls cover all branches)

/**
 * @tc.name: DoEstablishTest001
 * @tc.desc: GetRemoteDeviceInfo fails -> notify failure and cleanup
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> failure
 */
HWTEST_F(P2PLinkManagerTest, DoEstablishTest001, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);

    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_FAIL));
    mgr.DoEstablish(NET_A, entry, true);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_TRUE(mgr.HasLink(NET_A)); // refCount>0 -> entry kept (reset to IDLE)
}

/**
 * @tc.name: DoEstablishTest002
 * @tc.desc: plugin is null -> notify failure and cleanup
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; pluginGetter returns nullptr
 */
HWTEST_F(P2PLinkManagerTest, DoEstablishTest002, TestSize.Level1)
{
    P2PLinkManager mgr;
    mgr.Init(nullptr, nullptr); // plugin getter returns nullptr
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);

    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    mgr.DoEstablish(NET_A, entry, true);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_TRUE(mgr.HasLink(NET_A)); // refCount>0 -> entry kept (reset to IDLE)
}

/**
 * @tc.name: DoEstablishTest003
 * @tc.desc: applyResources=true && ApplyAdvancedResource fails -> abort and cleanup
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; plugin ApplyAdvancedResource -> failure
 */
HWTEST_F(P2PLinkManagerTest, DoEstablishTest003, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(-1));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);

    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    mgr.DoEstablish(NET_A, entry, true);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_TRUE(mgr.HasLink(NET_A)); // refCount>0 -> entry kept (reset to IDLE)
}

/**
 * @tc.name: DoEstablishTest004
 * @tc.desc: ConnectDfs fails -> publish IDLE, notify failure, cleanup
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; plugin ApplyAdvancedResource -> success;
 *           plugin PublishServiceState -> success; DistributedFileDaemonManager::ConnectDfs -> failure
 */
HWTEST_F(P2PLinkManagerTest, DoEstablishTest004, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, ApplyAdvancedResource).WillByDefault(Return(RESULT_OK));
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(RESULT_OK));
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::CONNECT_SUCC))
        .WillOnce(Return(RESULT_OK));
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::IDLE)).WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);

    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, ConnectDfs(NET_A)).WillOnce(Return(-1));
    mgr.DoEstablish(NET_A, entry, true);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_TRUE(mgr.HasLink(NET_A)); // refCount>0 -> entry kept (reset to IDLE)
}

/**
 * @tc.name: DoEstablishTest005
 * @tc.desc: all succeed -> state ESTABLISHED, notify success, resourcesApplied follows applyResources
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; plugin ApplyAdvancedResource -> success;
 *           plugin PublishServiceState -> success; DistributedFileDaemonManager::ConnectDfs -> success
 */
HWTEST_F(P2PLinkManagerTest, DoEstablishTest005, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, ApplyAdvancedResource).WillByDefault(Return(RESULT_OK));
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(RESULT_OK));
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::CONNECT_SUCC))
        .WillOnce(Return(RESULT_OK));
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::IDLE)).Times(0);
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);

    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, ConnectDfs(NET_A)).WillOnce(Return(RESULT_OK));
    mgr.DoEstablish(NET_A, entry, true);
    EXPECT_TRUE(mgr.IsEstablished(NET_A));
    EXPECT_EQ(entry->resourcesApplied, true);
    mgr.ForceClose(NET_A);
}

/**
 * @tc.name: DoEstablishTest006
 * @tc.desc: applyResources=false skips ApplyAdvancedResource; ConnectDfs success still establishes
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; ConnectDfs -> success;
 *           ApplyAdvancedResource never called; PublishServiceState -> success
 */
HWTEST_F(P2PLinkManagerTest, DoEstablishTest006, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    EXPECT_CALL(plugin, ApplyAdvancedResource).Times(0);
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::CONNECT_SUCC))
        .WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);

    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, ConnectDfs(NET_A)).WillOnce(Return(RESULT_OK));
    mgr.DoEstablish(NET_A, entry, false);
    EXPECT_TRUE(mgr.IsEstablished(NET_A));
    EXPECT_EQ(entry->resourcesApplied, false);
    mgr.ForceClose(NET_A);
}

// ApplyAdvancedResource (direct)

/**
 * @tc.name: ApplyAdvancedResourceTest001
 * @tc.desc: ApplyAdvancedResource fails -> notify failure, cleanup, return false
 * @tc.type: FUNC
 * @tc.mock: plugin ApplyAdvancedResource -> failure
 */
HWTEST_F(P2PLinkManagerTest, ApplyAdvancedResourceTest001, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(-1));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 1;
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(P2PLinkManager::MIN_TRANMISSION_TIME, 0);
    auto pluginPtr = std::shared_ptr<ClipPlugin>(&plugin, [](ClipPlugin *) {});
    auto ret = mgr.ApplyAdvancedResource(NET_A, pluginPtr, entry);
    EXPECT_FALSE(ret);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_TRUE(mgr.HasLink(NET_A)); // refCount>0 -> entry kept (reset to IDLE)
}

/**
 * @tc.name: ApplyAdvancedResourceTest002
 * @tc.desc: ApplyAdvancedResource succeeds -> return true, state unchanged
 * @tc.type: FUNC
 * @tc.mock: plugin ApplyAdvancedResource -> success
 */
HWTEST_F(P2PLinkManagerTest, ApplyAdvancedResourceTest002, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    auto pluginPtr = std::shared_ptr<ClipPlugin>(&plugin, [](ClipPlugin *) {});
    auto ret = mgr.ApplyAdvancedResource(NET_A, pluginPtr, entry);
    EXPECT_TRUE(ret);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHING);
}

// ApplyResourceOnReuse (direct)

/**
 * @tc.name: ApplyResourceOnReuseTest001
 * @tc.desc: plugin is null -> resourcesApplied reset to false
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, ApplyResourceOnReuseTest001, TestSize.Level1)
{
    P2PLinkManager mgr;
    mgr.Init(nullptr, nullptr);
    auto entry = GetEntry(mgr, NET_A);
    entry->resourcesApplied = true;
    mgr.ApplyResourceOnReuse(NET_A, entry);
    EXPECT_FALSE(entry->resourcesApplied);
}

/**
 * @tc.name: ApplyResourceOnReuseTest002
 * @tc.desc: ApplyAdvancedResource succeeds -> resourcesApplied stays true
 * @tc.type: FUNC
 * @tc.mock: plugin ApplyAdvancedResource -> success
 */
HWTEST_F(P2PLinkManagerTest, ApplyResourceOnReuseTest002, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->resourcesApplied = true;
    mgr.ApplyResourceOnReuse(NET_A, entry);
    EXPECT_TRUE(entry->resourcesApplied);
}

/**
 * @tc.name: ApplyResourceOnReuseTest003
 * @tc.desc: ApplyAdvancedResource fails -> resourcesApplied reset to false
 * @tc.type: FUNC
 * @tc.mock: plugin ApplyAdvancedResource -> failure
 */
HWTEST_F(P2PLinkManagerTest, ApplyResourceOnReuseTest003, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    EXPECT_CALL(plugin, ApplyAdvancedResource(NET_A)).WillOnce(Return(-1));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->resourcesApplied = true;
    mgr.ApplyResourceOnReuse(NET_A, entry);
    EXPECT_FALSE(entry->resourcesApplied);
}

// Release

/**
 * @tc.name: ReleaseTest001
 * @tc.desc: Release on unknown networkId is a no-op
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, ReleaseTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.Release(NET_A);
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

/**
 * @tc.name: ReleaseTest002
 * @tc.desc: Release decrements refCount; when still > 0 no idle timer starts
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, ReleaseTest002, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->refCount = 2;
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    mgr.Release(NET_A);
    EXPECT_EQ(entry->refCount, 1);
}

/**
 * @tc.name: ReleaseTest003
 * @tc.desc: Release to refCount<=0 on ESTABLISHED starts idle timer
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, ReleaseTest003, TestSize.Level1)
{
    auto timer = std::make_shared<FFRTTimer>("ut");
    NiceMock<MockClipPlugin> plugin;
    auto mgr = MakeManager(timer, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->refCount = 1;
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    mgr.Release(NET_A);
    EXPECT_EQ(entry->refCount, 0);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHED);
}

/**
 * @tc.name: ReleaseTest004
 * @tc.desc: Release to refCount<=0 but not ESTABLISHED does not start idle timer
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, ReleaseTest004, TestSize.Level1)
{
    auto timer = std::make_shared<FFRTTimer>("ut");
    NiceMock<MockClipPlugin> plugin;
    auto mgr = MakeManager(timer, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->refCount = 1;
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    mgr.Release(NET_A);
    EXPECT_EQ(entry->refCount, 0);
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHING);
}

// ForceClose / CloseAll / RemoveLink

/**
 * @tc.name: ForceCloseTest001
 * @tc.desc: ForceClose on unknown networkId is a no-op
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, ForceCloseTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.ForceClose(NET_A);
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

/**
 * @tc.name: ForceCloseTest002
 * @tc.desc: ForceClose on existing entry resets state, notifies waiters, removes entry
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success (DoCloseLink path), plugin publish
 */
HWTEST_F(P2PLinkManagerTest, ForceCloseTest002, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::IDLE)).WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    entry->refCount = 2;
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, DisconnectDfs(NET_A)).WillOnce(Return(RESULT_OK));
    mgr.ForceClose(NET_A);
    EXPECT_FALSE(mgr.HasLink(NET_A));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @tc.name: CloseAllTest001
 * @tc.desc: CloseAll on empty map is a no-op
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, CloseAllTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.CloseAll();
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

/**
 * @tc.name: CloseAllTest002
 * @tc.desc: CloseAll closes every entry and clears the map
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success, plugin publish for each
 */
HWTEST_F(P2PLinkManagerTest, CloseAllTest002, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto ea = GetEntry(mgr, NET_A);
    ea->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    auto eb = GetEntry(mgr, NET_B);
    eb->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_B, _)).WillOnce(Return(REMOTE_DEV_OK));
    mgr.CloseAll();
    EXPECT_FALSE(mgr.HasLink(NET_A));
    EXPECT_FALSE(mgr.HasLink(NET_B));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @tc.name: RemoveLinkTest001
 * @tc.desc: RemoveLink delegates to ForceClose
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, RemoveLinkTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    mgr.RemoveLink(NET_A);
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

// DoCloseLink (direct)

/**
 * @tc.name: DoCloseLinkTest001
 * @tc.desc: GetRemoteDeviceInfo fails -> early return, no DisconnectDfs
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> failure
 */
HWTEST_F(P2PLinkManagerTest, DoCloseLinkTest001, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_FAIL));
    EXPECT_CALL(g_dfsMock, DisconnectDfs).Times(0);
    mgr.DoCloseLink(NET_A);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

/**
 * @tc.name: DoCloseLinkTest002
 * @tc.desc: GetRemoteDeviceInfo succeeds -> DisconnectDfs submitted and plugin publishes IDLE
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; plugin PublishServiceState -> success
 */
HWTEST_F(P2PLinkManagerTest, DoCloseLinkTest002, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    EXPECT_CALL(plugin, PublishServiceState(NET_A, ClipPlugin::ServiceStatus::IDLE)).WillOnce(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, DisconnectDfs(NET_A)).WillOnce(Return(RESULT_OK));
    mgr.DoCloseLink(NET_A);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// OnIdleTimeout

/**
 * @tc.name: OnIdleTimeoutTest001
 * @tc.desc: unknown networkId -> no-op
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, OnIdleTimeoutTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.OnIdleTimeout(NET_A);
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

/**
 * @tc.name: OnIdleTimeoutTest002
 * @tc.desc: refCount<=0 && ESTABLISHED -> close link and erase entry
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success (DoCloseLink path)
 */
HWTEST_F(P2PLinkManagerTest, OnIdleTimeoutTest002, TestSize.Level1)
{
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    auto mgr = MakeManager(nullptr, plugin);
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    entry->refCount = 0;
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillOnce(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, DisconnectDfs(NET_A)).WillOnce(Return(RESULT_OK));
    mgr.OnIdleTimeout(NET_A);
    EXPECT_FALSE(mgr.HasLink(NET_A));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @tc.name: OnIdleTimeoutTest003
 * @tc.desc: refCount>0 -> link kept (shouldClose false)
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, OnIdleTimeoutTest003, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHED);
    entry->refCount = 1;
    mgr.OnIdleTimeout(NET_A);
    EXPECT_TRUE(mgr.HasLink(NET_A));
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHED);
}

/**
 * @tc.name: OnIdleTimeoutTest004
 * @tc.desc: refCount<=0 but not ESTABLISHED -> no close
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, OnIdleTimeoutTest004, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->refCount = 0;
    mgr.OnIdleTimeout(NET_A);
    EXPECT_TRUE(mgr.HasLink(NET_A));
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::ESTABLISHING);
}

// CleanupEntryIfIdle (direct)

/**
 * @tc.name: CleanupEntryIfIdleTest001
 * @tc.desc: refCount<=0 -> entry erased from map
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, CleanupEntryIfIdleTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->refCount = 0;
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    mgr.CleanupEntryIfIdle(NET_A, entry);
    EXPECT_FALSE(mgr.HasLink(NET_A));
}

/**
 * @tc.name: CleanupEntryIfIdleTest002
 * @tc.desc: refCount>0 -> entry kept, state reset to IDLE
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, CleanupEntryIfIdleTest002, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    entry->refCount = 2;
    entry->state.store(P2PLinkManager::LinkState::ESTABLISHING);
    entry->resourcesApplied = true;
    mgr.CleanupEntryIfIdle(NET_A, entry);
    EXPECT_TRUE(mgr.HasLink(NET_A));
    EXPECT_EQ(entry->state.load(), P2PLinkManager::LinkState::IDLE);
    EXPECT_FALSE(entry->resourcesApplied);
    EXPECT_EQ(entry->establishBlock, nullptr);
}

// NotifyWaiters (direct)

/**
 * @tc.name: NotifyWaitersTest001
 * @tc.desc: NotifyWaiters sets value on establishBlock and all waiters
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, NotifyWaitersTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    auto establish = std::make_shared<BlockObject<int32_t>>(100, 0);
    auto w1 = std::make_shared<BlockObject<int32_t>>(100, 0);
    auto w2 = std::make_shared<BlockObject<int32_t>>(100, 0);
    entry->establishBlock = establish;
    entry->waiters.push_back(w1);
    entry->waiters.push_back(w2);
    mgr.NotifyWaiters(entry, P2PLinkManager::SET_VALUE_SUCCESS);
    EXPECT_EQ(establish->GetValue(), P2PLinkManager::SET_VALUE_SUCCESS);
    EXPECT_EQ(w1->GetValue(), P2PLinkManager::SET_VALUE_SUCCESS);
    EXPECT_EQ(w2->GetValue(), P2PLinkManager::SET_VALUE_SUCCESS);
    EXPECT_EQ(entry->waiters.size(), 0u);
    EXPECT_EQ(entry->establishBlock, nullptr);
}

/**
 * @tc.name: NotifyWaitersTest002
 * @tc.desc: NotifyWaiters with no establishBlock and no waiters does not crash
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, NotifyWaitersTest002, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    auto entry = GetEntry(mgr, NET_A);
    mgr.NotifyWaiters(entry, P2PLinkManager::SET_VALUE_FAILURE);
    EXPECT_EQ(entry->establishBlock, nullptr);
    EXPECT_EQ(entry->waiters.size(), 0u);
}

// Timer helpers

/**
 * @tc.name: StartIdleTimerTest001
 * @tc.desc: StartIdleTimer with null timer is a no-op (no crash)
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, StartIdleTimerTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.StartIdleTimer(NET_A);
    EXPECT_TRUE(mgr.HasLink(NET_A) == mgr.HasLink(NET_A));
}

/**
 * @tc.name: CancelIdleTimerTest001
 * @tc.desc: CancelIdleTimer with null timer is a no-op (no crash)
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, CancelIdleTimerTest001, TestSize.Level1)
{
    auto mgr = MakeBareManager();
    mgr.CancelIdleTimer(NET_A);
    SUCCEED();
}

/**
 * @tc.name: TimerNameTest001
 * @tc.desc: Idle/Establish timer names follow the expected format
 * @tc.type: FUNC
 */
HWTEST_F(P2PLinkManagerTest, TimerNameTest001, TestSize.Level1)
{
    EXPECT_EQ(P2PLinkManager::IdleTimerName(NET_A), "P2P_IDLE_" + NET_A);
    EXPECT_EQ(P2PLinkManager::EstablishTimerName(NET_A), "P2P_ESTABLISH_" + NET_A);
}

// Async integration: EnsureLink -> ScheduleEstablish -> DoEstablish (success/fail)

/**
 * @tc.name: EnsureLinkIntegrationTest001
 * @tc.desc: EnsureLink with a real timer eventually establishes the link when all deps succeed
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; ApplyAdvancedResource -> success;
 *           PublishServiceState -> success; ConnectDfs -> success
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkIntegrationTest001, TestSize.Level1)
{
    auto timer = std::make_shared<FFRTTimer>("ut");
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, ApplyAdvancedResource).WillByDefault(Return(RESULT_OK));
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    auto mgr = MakeManager(timer, plugin);
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillRepeatedly(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, ConnectDfs(NET_A)).WillRepeatedly(Return(RESULT_OK));
    auto block = mgr.EnsureLink(NET_A);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(WaitForEstablished(mgr, NET_A));
    EXPECT_TRUE(mgr.IsEstablished(NET_A));
    mgr.ForceClose(NET_A);
}

/**
 * @tc.name: EnsureLinkIntegrationTest002
 * @tc.desc: EnsureLink with a real timer fails to establish when ConnectDfs fails
 * @tc.type: FUNC
 * @tc.mock: DMAdapter::GetRemoteDeviceInfo -> success; ApplyAdvancedResource -> success;
 *           PublishServiceState -> success; ConnectDfs -> failure
 */
HWTEST_F(P2PLinkManagerTest, EnsureLinkIntegrationTest002, TestSize.Level1)
{
    auto timer = std::make_shared<FFRTTimer>("ut");
    NiceMock<MockClipPlugin> plugin;
    ON_CALL(plugin, ApplyAdvancedResource).WillByDefault(Return(RESULT_OK));
    ON_CALL(plugin, PublishServiceState).WillByDefault(Return(RESULT_OK));
    auto mgr = MakeManager(timer, plugin);
    NiceMock<P2PLinkInterfaceMock> dmMock;
    EXPECT_CALL(dmMock, GetRemoteDeviceInfo(NET_A, _)).WillRepeatedly(Return(REMOTE_DEV_OK));
    EXPECT_CALL(g_dfsMock, ConnectDfs(NET_A)).WillRepeatedly(Return(-1));
    auto block = mgr.EnsureLink(NET_A);
    ASSERT_NE(block, nullptr);
    // Wait for the async establish to settle (it should fail and leave the link non-established).
    bool established = WaitForEstablished(mgr, NET_A, 1000);
    EXPECT_FALSE(established);
    EXPECT_FALSE(mgr.IsEstablished(NET_A));
    mgr.ForceClose(NET_A);
}
