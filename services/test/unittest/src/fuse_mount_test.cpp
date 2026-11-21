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

#include <cerrno>
#include <cstring>
#include <memory>
#include <string>
#include <sys/stat.h>

#include "securec.h"

#include "fuse_mount.h"
#include "fuse_mount_sys_mock.h"
#include "system_ability_manager_client_mock.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MiscServices {
namespace {
constexpr int32_t TEST_USER_ID = 100;
// The path string itself is never consumed by the real filesystem: mkdir/lstat
// are overridden by fuse_mount_sys_mock.cpp, and the StorageManager proxy is
// unreachable (samgr forced to nullptr). It only needs to be a non-empty value
// that satisfies the std::string parameter contract.
const std::string TEST_MOUNT_PATH = "/data/pasteboard/fuse/100";
// Sentinel written into fuseFd before each Mount call to prove that
// MountUdmfClipFuse always resets fuseFd to -1 on every return path.
constexpr int32_t FUSE_FD_SENTINEL = 0x7FCADE;

class FuseMountTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    // Forces SystemAbilityManagerClient::GetSystemAbilityManager() to return
    // nullptr so GetStorageManagerProxy() deterministically returns nullptr and
    // the proxy-unavailable branch of Mount/UMount is exercised regardless of
    // whether a real samgr/storage_manager is running on the test device.
    // Matches the pattern used by PasteboardClientMockTest
    // (framework/test/src/pasteboard_client_proxy_mock_test.cpp).
    static std::shared_ptr<SystemAbilityManagerClientMock> samgrClientMock_;
    // Drives every PrepareMountPoint branch via the link-time mkdir/lstat
    // overrides defined in fuse_mount_sys_mock.cpp.
    NiceMock<FuseMountSysMock> *sysMock_ = nullptr;
};

std::shared_ptr<SystemAbilityManagerClientMock> FuseMountTest::samgrClientMock_;

void FuseMountTest::SetUpTestCase(void)
{
    samgrClientMock_ = std::make_shared<SystemAbilityManagerClientMock>();
    PasteSystemAbilityManagerClient::pasteSystemAbilityManagerClient = samgrClientMock_;
}

void FuseMountTest::TearDownTestCase(void)
{
    PasteSystemAbilityManagerClient::pasteSystemAbilityManagerClient = nullptr;
    samgrClientMock_ = nullptr;
}

void FuseMountTest::SetUp(void)
{
    sysMock_ = new NiceMock<FuseMountSysMock>();
}

void FuseMountTest::TearDown(void)
{
    delete sysMock_;
    sysMock_ = nullptr;
    if (samgrClientMock_ != nullptr) {
        // Isolate per-test samgr expectations so a Times(0)/WillRepeatedly set
        // in one test does not leak into the next.
        Mock::VerifyAndClear(samgrClientMock_.get());
    }
}

// ===== MountUdmfClipFuse: PrepareMountPoint branches =====

/**
 * @tc.name: MountUdmfClipFuse_001
 * @tc.desc: mkdir succeeds (path created) -> PrepareMountPoint returns 0, then
 *           StorageManager proxy is unavailable -> returns -1. fuseFd is reset.
 * @tc.type: FUNC
 * @tc.mock:
 *   mkdir -> returns 0 (success); lstat -> not called (Times(0));
 *   SystemAbilityManagerClient::GetSystemAbilityManager -> nullptr (proxy null).
 */
HWTEST_F(FuseMountTest, MountUdmfClipFuse_001, TestSize.Level1)
{
    EXPECT_CALL(*sysMock_, mkdir(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*sysMock_, lstat(_, _)).Times(0);
    EXPECT_CALL(*samgrClientMock_, GetSystemAbilityManager()).WillRepeatedly(Return(nullptr));

    int32_t fuseFd = FUSE_FD_SENTINEL;
    int32_t ret = MountUdmfClipFuse(TEST_USER_ID, TEST_MOUNT_PATH, fuseFd);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(fuseFd, -1);
}

/**
 * @tc.name: MountUdmfClipFuse_002
 * @tc.desc: mkdir fails with a non-EEXIST errno -> PrepareMountPoint returns
 *           -errno and MountUdmfClipFuse returns it without reaching the proxy.
 * @tc.type: FUNC
 * @tc.mock:
 *   mkdir -> returns -1, errno=EACCES (non-EEXIST); lstat -> not called;
 *   SystemAbilityManagerClient::GetSystemAbilityManager -> not called (Times(0)).
 */
HWTEST_F(FuseMountTest, MountUdmfClipFuse_002, TestSize.Level1)
{
    EXPECT_CALL(*sysMock_, mkdir(_, _))
        .WillOnce(Invoke([](const char *, mode_t) {
            errno = EACCES;
            return -1;
        }));
    EXPECT_CALL(*sysMock_, lstat(_, _)).Times(0);
    EXPECT_CALL(*samgrClientMock_, GetSystemAbilityManager()).Times(0);

    int32_t fuseFd = FUSE_FD_SENTINEL;
    int32_t ret = MountUdmfClipFuse(TEST_USER_ID, TEST_MOUNT_PATH, fuseFd);
    EXPECT_EQ(ret, -EACCES);
    EXPECT_EQ(fuseFd, -1);
}

/**
 * @tc.name: MountUdmfClipFuse_003
 * @tc.desc: mkdir returns EEXIST and lstat reports a directory ->
 *           PrepareMountPoint returns 0, then proxy unavailable -> returns -1.
 * @tc.type: FUNC
 * @tc.mock:
 *   mkdir -> returns -1, errno=EEXIST; lstat -> returns 0, st_mode=S_IFDIR;
 *   SystemAbilityManagerClient::GetSystemAbilityManager -> nullptr (proxy null).
 */
HWTEST_F(FuseMountTest, MountUdmfClipFuse_003, TestSize.Level1)
{
    EXPECT_CALL(*sysMock_, mkdir(_, _))
        .WillOnce(Invoke([](const char *, mode_t) {
            errno = EEXIST;
            return -1;
        }));
    EXPECT_CALL(*sysMock_, lstat(_, _))
        .WillOnce(Invoke([](const char *, struct stat *buf) {
            memset_s(buf, sizeof(*buf), 0, sizeof(*buf));
            buf->st_mode = S_IFDIR;
            return 0;
        }));
    EXPECT_CALL(*samgrClientMock_, GetSystemAbilityManager()).WillRepeatedly(Return(nullptr));

    int32_t fuseFd = FUSE_FD_SENTINEL;
    int32_t ret = MountUdmfClipFuse(TEST_USER_ID, TEST_MOUNT_PATH, fuseFd);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(fuseFd, -1);
}

/**
 * @tc.name: MountUdmfClipFuse_004
 * @tc.desc: mkdir returns EEXIST but lstat reports a non-directory (regular
 *           file) -> PrepareMountPoint returns -ENOTDIR and MountUdmfClipFuse
 *           returns it without reaching the proxy.
 * @tc.type: FUNC
 * @tc.mock:
 *   mkdir -> returns -1, errno=EEXIST; lstat -> returns 0, st_mode=S_IFREG;
 *   SystemAbilityManagerClient::GetSystemAbilityManager -> not called (Times(0)).
 */
HWTEST_F(FuseMountTest, MountUdmfClipFuse_004, TestSize.Level1)
{
    EXPECT_CALL(*sysMock_, mkdir(_, _))
        .WillOnce(Invoke([](const char *, mode_t) {
            errno = EEXIST;
            return -1;
        }));
    EXPECT_CALL(*sysMock_, lstat(_, _))
        .WillOnce(Invoke([](const char *, struct stat *buf) {
            memset_s(buf, sizeof(*buf), 0, sizeof(*buf));
            buf->st_mode = S_IFREG;
            return 0;
        }));
    EXPECT_CALL(*samgrClientMock_, GetSystemAbilityManager()).Times(0);

    int32_t fuseFd = FUSE_FD_SENTINEL;
    int32_t ret = MountUdmfClipFuse(TEST_USER_ID, TEST_MOUNT_PATH, fuseFd);
    EXPECT_EQ(ret, -ENOTDIR);
    EXPECT_EQ(fuseFd, -1);
}

/**
 * @tc.name: MountUdmfClipFuse_005
 * @tc.desc: mkdir returns EEXIST but lstat fails -> PrepareMountPoint returns
 *           -errno and MountUdmfClipFuse returns it without reaching the proxy.
 * @tc.type: FUNC
 * @tc.mock:
 *   mkdir -> returns -1, errno=EEXIST; lstat -> returns -1, errno=ENOENT;
 *   SystemAbilityManagerClient::GetSystemAbilityManager -> not called (Times(0)).
 */
HWTEST_F(FuseMountTest, MountUdmfClipFuse_005, TestSize.Level1)
{
    EXPECT_CALL(*sysMock_, mkdir(_, _))
        .WillOnce(Invoke([](const char *, mode_t) {
            errno = EEXIST;
            return -1;
        }));
    EXPECT_CALL(*sysMock_, lstat(_, _))
        .WillOnce(Invoke([](const char *, struct stat *) {
            errno = ENOENT;
            return -1;
        }));
    EXPECT_CALL(*samgrClientMock_, GetSystemAbilityManager()).Times(0);

    int32_t fuseFd = FUSE_FD_SENTINEL;
    int32_t ret = MountUdmfClipFuse(TEST_USER_ID, TEST_MOUNT_PATH, fuseFd);
    EXPECT_EQ(ret, -ENOENT);
    EXPECT_EQ(fuseFd, -1);
}

// ===== UMountUdmfClipFuse: proxy-unavailable branch =====

/**
 * @tc.name: UMountUdmfClipFuse_001
 * @tc.desc: StorageManager proxy is unavailable -> UMountUdmfClipFuse returns
 *           -1 without touching the filesystem (no mkdir/lstat).
 * @tc.type: FUNC
 * @tc.mock:
 *   mkdir -> not called (Times(0)); lstat -> not called (Times(0));
 *   SystemAbilityManagerClient::GetSystemAbilityManager -> nullptr (proxy null).
 */
HWTEST_F(FuseMountTest, UMountUdmfClipFuse_001, TestSize.Level1)
{
    EXPECT_CALL(*sysMock_, mkdir(_, _)).Times(0);
    EXPECT_CALL(*sysMock_, lstat(_, _)).Times(0);
    EXPECT_CALL(*samgrClientMock_, GetSystemAbilityManager()).WillRepeatedly(Return(nullptr));

    int32_t ret = UMountUdmfClipFuse(TEST_USER_ID, TEST_MOUNT_PATH);
    EXPECT_EQ(ret, -1);
}
} // namespace
} // namespace MiscServices
} // namespace OHOS
