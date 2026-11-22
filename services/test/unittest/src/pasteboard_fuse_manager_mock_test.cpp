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
#include <utility>

#include "fuse_api_mock.h"
#include "fuse_mount_api_mock.h"
#include "ipasteboard_fuse_getter.h"
#include "iremote_object.h"
#include "pasteboard_fuse_manager.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

constexpr int32_t TEST_USER_ID = 100;
constexpr int32_t TEST_PID = 1234;
constexpr int32_t UNAUTHORIZED_PID = 9999;
constexpr uint32_t TEST_DATA_ID = 1;
constexpr uint32_t TEST_RECORD_ID = 10;
const std::string TEST_FILE_ID = "file_001";
const std::string TEST_FILE_NAME = "test.txt";

class MockFuseGetter : public IPasteboardFuseGetter {
public:
    OHOS::sptr<OHOS::IRemoteObject> AsObject() override { return nullptr; }
    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override
    {
        data = readData_;
        return readRet_;
    }
    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override
    {
        stat = stat_;
        return statRet_;
    }
    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override
    {
        cancelCalled_ = true;
        return cancelRet_;
    }

    std::vector<uint8_t> readData_{1, 2, 3, 4, 5};
    int32_t readRet_ = 0;
    FuseFileStat stat_{};
    int32_t statRet_ = 0;
    int32_t cancelRet_ = 0;
    bool cancelCalled_ = false;
};

// Callback signature matches fuse_fill_dir_t; parameter names follow libfuse conventions.
// NOLINTNEXTLINE
static int TestFiller(void *buf, const char *name, const struct stat *stbuf, off_t off, enum fuse_fill_dir_flags flags)
{
    return 0;
}

namespace {

class PasteboardFuseManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    NiceMock<FuseMountApiMock> *mountMock_ = nullptr;
    NiceMock<FuseApiMock> *fuseMock_ = nullptr;
    PasteboardFuseManager *fuse_ = nullptr;
    PasteboardFuseManager::SessionContext sessionCtx_{};

    void SetupAuthorizedContext(int32_t pid);
    void SetupUnauthorizedContext();
};

void PasteboardFuseManagerTest::SetUpTestCase(void) {}
void PasteboardFuseManagerTest::TearDownTestCase(void) {}

void PasteboardFuseManagerTest::SetUp(void)
{
    mountMock_ = new NiceMock<FuseMountApiMock>();
    fuseMock_ = new NiceMock<FuseApiMock>();
    fuse_ = new PasteboardFuseManager();
    sessionCtx_ = { fuse_, TEST_USER_ID };
}

void PasteboardFuseManagerTest::TearDown(void)
{
    delete std::exchange(fuse_, nullptr);
    delete std::exchange(fuseMock_, nullptr);
    delete std::exchange(mountMock_, nullptr);
}

void PasteboardFuseManagerTest::SetupAuthorizedContext(int32_t pid)
{
    fuseMock_->mockContext_.pid = pid;
    fuseMock_->mockContext_.uid = static_cast<uid_t>(pid);
    fuseMock_->mockContext_.private_data = &sessionCtx_;
    fuse_->authorizedPastePids_[TEST_USER_ID].insert(pid);
    fuse_->authorizedPasteUids_[TEST_USER_ID].insert(static_cast<uid_t>(pid));
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(&fuseMock_->mockContext_));
}

void PasteboardFuseManagerTest::SetupUnauthorizedContext()
{
    fuseMock_->mockContext_.pid = UNAUTHORIZED_PID;
    fuseMock_->mockContext_.uid = static_cast<uid_t>(UNAUTHORIZED_PID);
    fuseMock_->mockContext_.private_data = &sessionCtx_;
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(&fuseMock_->mockContext_));
}

// ===== Lifecycle Tests =====

/**
 * @tc.name: OnGetData_001
 * @tc.desc: should return error when fuse_session_new returns nullptr after mount success
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnGetData_001, TestSize.Level1)
{
    EXPECT_CALL(*mountMock_, MountUdmfClipFuse(TEST_USER_ID, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(99), Return(0)));
    EXPECT_CALL(*fuseMock_, fuse_session_new(_, _, _, _)).WillOnce(Return(nullptr));
    int32_t ret = fuse_->OnGetData(TEST_USER_ID);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: OnGetData_MountFailed_001
 * @tc.desc: should return error when MountUdmfClipFuse fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnGetData_MountFailed_001, TestSize.Level1)
{
    EXPECT_CALL(*mountMock_, MountUdmfClipFuse(TEST_USER_ID, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(-1), Return(-1)));
    int32_t ret = fuse_->OnGetData(TEST_USER_ID);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: OnRemoveData_001
 * @tc.desc: should remove provider and clean up resources when OnRemoveData is called
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnRemoveData_001, TestSize.Level1)
{
    fuse_->OnRemoveData(TEST_USER_ID);
    EXPECT_TRUE(fuse_->providers_.find(TEST_USER_ID) == fuse_->providers_.end());
}

/**
 * @tc.name: SetProvider_001
 * @tc.desc: should return false from HasProvider when provider is null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, SetProvider_001, TestSize.Level1)
{
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, {});
    EXPECT_FALSE(fuse_->HasProvider(TEST_USER_ID));
}

/**
 * @tc.name: SetProvider_002
 * @tc.desc: should return true from HasProvider when provider is non-null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, SetProvider_002, TestSize.Level1)
{
    auto *getter = new MockFuseGetter();
    fuse_->SetProvider(TEST_USER_ID, getter, TEST_DATA_ID, {});
    EXPECT_TRUE(fuse_->HasProvider(TEST_USER_ID));
}

/**
 * @tc.name: HasProvider_001
 * @tc.desc: should return false when no provider has been set
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, HasProvider_001, TestSize.Level1)
{
    EXPECT_FALSE(fuse_->HasProvider(TEST_USER_ID));
}

/**
 * @tc.name: HasProvider_002
 * @tc.desc: should return false when provider is set to null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, HasProvider_002, TestSize.Level1)
{
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, {});
    EXPECT_FALSE(fuse_->HasProvider(TEST_USER_ID));
}

/**
 * @tc.name: OnProviderDied_001
 * @tc.desc: should remove provider, clear fdMap, and clean fileInfos when provider died
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnProviderDied_001, TestSize.Level1)
{
    auto *getter = new MockFuseGetter();
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 1024, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, getter, TEST_DATA_ID, infos);
    fuse_->fdMap_[1001] = {static_cast<uid_t>(TEST_PID), std::make_shared<FileInfo>(fuse_->fileInfos_[TEST_USER_ID][0])};
    fuse_->OnProviderDied(TEST_USER_ID);
    EXPECT_FALSE(fuse_->HasProvider(TEST_USER_ID));
    EXPECT_EQ(fuse_->fdMap_.count(1001), 0u);
}

/**
 * @tc.name: OnAppExit_PasteApp_001
 * @tc.desc: should remove authorized paste PID when paste app exits
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnAppExit_PasteApp_001, TestSize.Level1)
{
    fuse_->authorizedPastePids_[TEST_USER_ID].insert(TEST_PID);
    fuse_->OnAppExit(TEST_PID, TEST_USER_ID);
    EXPECT_EQ(fuse_->authorizedPastePids_[TEST_USER_ID].count(TEST_PID), 0u);
}

/**
 * @tc.name: OnAged_001
 * @tc.desc: should remove provider and clear fileInfos when data is aged
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnAged_001, TestSize.Level1)
{
    auto *getter = new MockFuseGetter();
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 1024, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, getter, TEST_DATA_ID, infos);
    fuse_->OnAged(TEST_USER_ID);
    EXPECT_FALSE(fuse_->HasProvider(TEST_USER_ID));
    EXPECT_EQ(fuse_->fileInfos_.count(TEST_USER_ID), 0u);
}

// ===== CheckAuthorization Tests (AC-04.1, AC-04.2) =====

/**
 * @tc.name: CheckAuthorization_Authorized_001
 * @tc.desc: should return true when PID is in authorized record (AC-04.1)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, CheckAuthorization_Authorized_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    EXPECT_TRUE(fuse_->CheckAuthorization(TEST_USER_ID));
}

/**
 * @tc.name: CheckAuthorization_Unauthorized_001
 * @tc.desc: should return false when PID is not in authorized record (AC-04.2)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, CheckAuthorization_Unauthorized_001, TestSize.Level1)
{
    SetupUnauthorizedContext();
    EXPECT_FALSE(fuse_->CheckAuthorization(TEST_USER_ID));
}

/**
 * @tc.name: CheckAuthorization_NoPidRecord_001
 * @tc.desc: should return false when no PID record exists for the given userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, CheckAuthorization_NoPidRecord_001, TestSize.Level1)
{
    SetupUnauthorizedContext();
    fuse_->authorizedPastePids_.clear();
    EXPECT_FALSE(fuse_->CheckAuthorization(TEST_USER_ID));
}

// ===== StatHandler Tests (AC-04.3, AC-04.2) =====

/**
 * @tc.name: StatHandler_Unauthorized_001
 * @tc.desc: should return -EACCES when unauthorized access (AC-04.2)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_Unauthorized_001, TestSize.Level1)
{
    SetupUnauthorizedContext();
    struct stat stbuf;
    int ret = PasteboardFuseManager::StatHandler("/test", &stbuf, nullptr);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: StatHandler_Root_001
 * @tc.desc: should return 0 and set S_IFDIR mode when stat root path "/" (AC-04.3)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_Root_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct stat stbuf;
    int ret = PasteboardFuseManager::StatHandler("/", &stbuf, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(stbuf.st_mode & S_IFDIR);
}

/**
 * @tc.name: StatHandler_NotFound_001
 * @tc.desc: should return -ENOENT when file is not found
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_NotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct stat stbuf;
    int ret = PasteboardFuseManager::StatHandler("/nonexistent", &stbuf, nullptr);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: StatHandler_File_001
 * @tc.desc: should return 0 and set S_IFREG mode with correct size when stat a file (AC-04.3)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_File_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 1024, 5000, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct stat stbuf;
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/" + TEST_FILE_NAME;
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(stbuf.st_mode & S_IFREG);
    EXPECT_EQ(stbuf.st_size, 1024);
}

/**
 * @tc.name: StatHandler_Dir_001
 * @tc.desc: should return 0 and set S_IFDIR mode when stat a directory (AC-04.3)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_Dir_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, "dir_001", "mydir", 0, 0, true, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct stat stbuf;
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/mydir";
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(stbuf.st_mode & S_IFDIR);
}

// ===== OpenHandler Tests (AC-04.4, AC-04.2) =====

/**
 * @tc.name: OpenHandler_Unauthorized_001
 * @tc.desc: should return -EACCES when unauthorized access (AC-04.2)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_Unauthorized_001, TestSize.Level1)
{
    SetupUnauthorizedContext();
    struct fuse_file_info fi{};
    int ret = PasteboardFuseManager::OpenHandler("/test", &fi);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: OpenHandler_NotFound_001
 * @tc.desc: should return -ENOENT when file is not found
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_NotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct fuse_file_info fi{};
    int ret = PasteboardFuseManager::OpenHandler("/nonexistent", &fi);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: OpenHandler_Success_001
 * @tc.desc: should return 0 and set internal fd in fi->fh when open succeeds (AC-04.4)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_Success_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 1024, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct fuse_file_info fi{};
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/" + TEST_FILE_NAME;
    int ret = PasteboardFuseManager::OpenHandler(path.c_str(), &fi);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(fi.fh, 0u);
}

// ===== ReadHandler Tests (AC-04.5, AC-04.2, EX-03, EX-05) =====

/**
 * @tc.name: ReadHandler_Unauthorized_001
 * @tc.desc: should return -EACCES when unauthorized access (AC-04.2)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_Unauthorized_001, TestSize.Level1)
{
    SetupUnauthorizedContext();
    struct fuse_file_info fi{};
    char buf[1024];
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 1024, 0, &fi);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: ReadHandler_InvalidFd_001
 * @tc.desc: should return -ENOENT when fd is not in fdMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_InvalidFd_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct fuse_file_info fi{};
    fi.fh = 9999;
    char buf[1024];
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 1024, 0, &fi);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: ReadHandler_NullProvider_001
 * @tc.desc: should return -EIO when provider is null (EX-03)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_NullProvider_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    auto info = std::make_shared<FileInfo>();
    info->userId = TEST_USER_ID;
    info->fileId = TEST_FILE_ID;
    info->provider = nullptr;
    int fd = fuse_->nextFd_++;
    fuse_->fdMap_[fd] = {static_cast<uid_t>(TEST_PID), info};
    struct fuse_file_info fi{};
    fi.fh = static_cast<uint64_t>(fd);
    char buf[1024];
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 1024, 0, &fi);
    EXPECT_EQ(ret, -EIO);
}

/**
 * @tc.name: ReadHandler_ProviderError_001
 * @tc.desc: should return -EIO when OnRead callback returns error (EX-03)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_ProviderError_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    auto *getter = new MockFuseGetter();
    getter->readRet_ = -1;
    auto info = std::make_shared<FileInfo>();
    info->userId = TEST_USER_ID;
    info->fileId = TEST_FILE_ID;
    info->provider = getter;
    int fd = fuse_->nextFd_++;
    fuse_->fdMap_[fd] = {static_cast<uid_t>(TEST_PID), info};
    struct fuse_file_info fi{};
    fi.fh = static_cast<uint64_t>(fd);
    char buf[1024];
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 1024, 0, &fi);
    EXPECT_EQ(ret, -EIO);
}

/**
 * @tc.name: ReadHandler_Success_001
 * @tc.desc: should return data bytes and copy data to buf when OnRead succeeds (AC-04.5)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_Success_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    auto *getter = new MockFuseGetter();
    getter->readData_ = {10, 20, 30};
    getter->readRet_ = 0;
    auto info = std::make_shared<FileInfo>();
    info->userId = TEST_USER_ID;
    info->fileId = TEST_FILE_ID;
    info->provider = getter;
    int fd = fuse_->nextFd_++;
    fuse_->fdMap_[fd] = {static_cast<uid_t>(TEST_PID), info};
    struct fuse_file_info fi{};
    fi.fh = static_cast<uint64_t>(fd);
    char buf[1024] = {0};
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 1024, 0, &fi);
    EXPECT_EQ(ret, 3);
    EXPECT_EQ(buf[0], 10);
    EXPECT_EQ(buf[1], 20);
    EXPECT_EQ(buf[2], 30);
}

// ===== ReleaseHandler Tests (AC-04.6) =====

/**
 * @tc.name: ReleaseHandler_Success_001
 * @tc.desc: should return 0 and remove fd from fdMap when close succeeds (AC-04.6)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReleaseHandler_Success_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    auto info = std::make_shared<FileInfo>();
    int fd = fuse_->nextFd_++;
    fuse_->fdMap_[fd] = {static_cast<uid_t>(TEST_PID), info};
    struct fuse_file_info fi{};
    fi.fh = static_cast<uint64_t>(fd);
    int ret = PasteboardFuseManager::ReleaseHandler("/test", &fi);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fuse_->fdMap_.count(fd), 0u);
}

/**
 * @tc.name: ReleaseHandler_InvalidFd_001
 * @tc.desc: should return 0 when fd is not in fdMap (no-op for invalid fd)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReleaseHandler_InvalidFd_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct fuse_file_info fi{};
    fi.fh = 9999;
    int ret = PasteboardFuseManager::ReleaseHandler("/test", &fi);
    EXPECT_EQ(ret, 0);
}

// ===== ReaddirHandler Tests (AC-03.4, AC-04.2) =====

/**
 * @tc.name: ReaddirHandler_Unauthorized_001
 * @tc.desc: should return -EACCES when unauthorized access (AC-04.2)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_Unauthorized_001, TestSize.Level1)
{
    SetupUnauthorizedContext();
    int ret = PasteboardFuseManager::ReaddirHandler("/", nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: ReaddirHandler_Root_001
 * @tc.desc: should return 0 and list files when readdir on root path "/" (AC-03.4)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_Root_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {
        {0, 0, TEST_RECORD_ID, "file_a", "a.txt", 100, 0, false, nullptr},
        {0, 0, TEST_RECORD_ID, "file_b", "b.txt", 200, 0, false, nullptr}
    };
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    int ret = PasteboardFuseManager::ReaddirHandler("/", nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: ReaddirHandler_NonRoot_001
 * @tc.desc: should return -ENOENT when readdir on non-root path
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_NonRoot_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    int ret = PasteboardFuseManager::ReaddirHandler("/subdir", nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -ENOENT);
}

// ===== GetMountPath Test =====

/**
 * @tc.name: GetMountPath_001
 * @tc.desc: should return path containing userId and pasteboard directory name
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, GetMountPath_001, TestSize.Level1)
{
    std::string path = fuse_->GetMountPath(TEST_USER_ID);
    EXPECT_NE(path.find("clipboard"), std::string::npos);
    EXPECT_NE(path.find(std::to_string(TEST_USER_ID)), std::string::npos);
}

// ===== Null fuse_get_context Tests (AC: null pointer safety) =====

/**
 * @tc.name: StatHandler_NullContext_001
 * @tc.desc: should return -EACCES when fuse_get_context returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_NullContext_001, TestSize.Level1)
{
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(nullptr));
    struct stat stbuf;
    int ret = PasteboardFuseManager::StatHandler("/test", &stbuf, nullptr);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: OpenHandler_NullContext_001
 * @tc.desc: should return -EACCES when fuse_get_context returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_NullContext_001, TestSize.Level1)
{
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(nullptr));
    struct fuse_file_info fi{};
    int ret = PasteboardFuseManager::OpenHandler("/test", &fi);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: ReadHandler_NullContext_001
 * @tc.desc: should return -EACCES when fuse_get_context returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_NullContext_001, TestSize.Level1)
{
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(nullptr));
    struct fuse_file_info fi{};
    char buf[1024];
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 1024, 0, &fi);
    EXPECT_EQ(ret, -EACCES);
}

/**
 * @tc.name: ReleaseHandler_NullContext_001
 * @tc.desc: should return 0 when fuse_get_context returns nullptr (no-op for release)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReleaseHandler_NullContext_001, TestSize.Level1)
{
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(nullptr));
    struct fuse_file_info fi{};
    int ret = PasteboardFuseManager::ReleaseHandler("/test", &fi);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: ReaddirHandler_NullContext_001
 * @tc.desc: should return -EACCES when fuse_get_context returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_NullContext_001, TestSize.Level1)
{
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(nullptr));
    int ret = PasteboardFuseManager::ReaddirHandler("/", nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -EACCES);
}

// ===== OpenHandler TOCTOU Tests (AC: atomic auth + fdMap) =====

/**
 * @tc.name: OpenHandler_ToctouSafety_001
 * @tc.desc: should atomically check auth and assign fd under the same lock
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_ToctouSafety_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 1024, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct fuse_file_info fi{};
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/" + TEST_FILE_NAME;
    int ret = PasteboardFuseManager::OpenHandler(path.c_str(), &fi);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(fi.fh, 0u);
    int32_t assignedFd = static_cast<int32_t>(fi.fh);
    EXPECT_EQ(fuse_->fdMap_.count(assignedFd), 1u);
}

// ===== Multi-User Isolation Tests (AC: per-session user scoping) =====

/**
 * @tc.name: MultiUser_Isolation_001
 * @tc.desc: should deny cross-user pid authorization and cross-user file access via another session's context
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, MultiUser_Isolation_001, TestSize.Level1)
{
    constexpr int32_t USER_A = 100;
    constexpr int32_t USER_B = 200;
    constexpr int32_t PID_A = 1111;
    constexpr int32_t PID_B = 2222;
    constexpr uint32_t DATA_ID_A = 1;
    constexpr uint32_t DATA_ID_B = 2;
    fuse_->authorizedPastePids_[USER_A].insert(PID_A);
    fuse_->authorizedPastePids_[USER_B].insert(PID_B);
    std::vector<FileInfo> infosA = {{0, 0, TEST_RECORD_ID, "file_a", "a.txt", 100, 0, false, nullptr}};
    std::vector<FileInfo> infosB = {{0, 0, TEST_RECORD_ID, "file_b", "b.txt", 200, 0, false, nullptr}};
    fuse_->SetProvider(USER_A, nullptr, DATA_ID_A, infosA);
    fuse_->SetProvider(USER_B, nullptr, DATA_ID_B, infosB);
    PasteboardFuseManager::SessionContext ctxA{ fuse_, USER_A };
    fuseMock_->mockContext_.pid = PID_B;
    fuseMock_->mockContext_.private_data = &ctxA;
    EXPECT_CALL(*fuseMock_, fuse_get_context()).WillRepeatedly(Return(&fuseMock_->mockContext_));
    struct stat stbuf;
    std::string pathA = "/" + std::to_string(DATA_ID_A) + "/" + std::to_string(TEST_RECORD_ID) + "/a.txt";
    int ret = PasteboardFuseManager::StatHandler(pathA.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, -EACCES);
    fuseMock_->mockContext_.pid = PID_A;
    std::string pathB = "/" + std::to_string(DATA_ID_B) + "/" + std::to_string(TEST_RECORD_ID) + "/b.txt";
    ret = PasteboardFuseManager::StatHandler(pathB.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: GetFileInfo_ScopedByUser_001
 * @tc.desc: should only return files belonging to the given userId, not another user's file with the same fileId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, GetFileInfo_ScopedByUser_001, TestSize.Level1)
{
    constexpr int32_t USER_A = 100;
    constexpr int32_t USER_B = 200;
    constexpr uint32_t DATA_ID_A = 1;
    constexpr uint32_t DATA_ID_B = 2;
    constexpr uint32_t SHARED_RECORD_ID = 5;
    const std::string SAME_FILE_NAME = "shared.txt";
    std::vector<FileInfo> infosA = {{0, 0, SHARED_RECORD_ID, "shared_file", SAME_FILE_NAME, 111, 0, false, nullptr}};
    std::vector<FileInfo> infosB = {{0, 0, SHARED_RECORD_ID, "shared_file", SAME_FILE_NAME, 222, 0, false, nullptr}};
    fuse_->SetProvider(USER_A, nullptr, DATA_ID_A, infosA);
    fuse_->SetProvider(USER_B, nullptr, DATA_ID_B, infosB);
    std::lock_guard<std::mutex> lock(fuse_->mutex_);
    auto infoA = fuse_->GetFileInfoLocked(USER_A, DATA_ID_A, SHARED_RECORD_ID, SAME_FILE_NAME);
    ASSERT_NE(infoA, nullptr);
    EXPECT_EQ(infoA->userId, USER_A);
    EXPECT_EQ(infoA->fileSize, 111);
    auto infoB = fuse_->GetFileInfoLocked(USER_B, DATA_ID_B, SHARED_RECORD_ID, SAME_FILE_NAME);
    ASSERT_NE(infoB, nullptr);
    EXPECT_EQ(infoB->userId, USER_B);
    EXPECT_EQ(infoB->fileSize, 222);
    auto infoMiss = fuse_->GetFileInfoLocked(USER_A, DATA_ID_A, SHARED_RECORD_ID, "nonexistent.txt");
    EXPECT_EQ(infoMiss, nullptr);
}

// ===== SetProvider Populates fileInfos_ Tests =====

/**
 * @tc.name: SetProvider_PopulatesFileInfos_001
 * @tc.desc: SetProvider should populate fileInfos_ so handlers find files via /<dataId>/<recordId>/<fileName>
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, SetProvider_PopulatesFileInfos_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    auto *getter = new MockFuseGetter();
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 2048, 9000, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, getter, TEST_DATA_ID, infos);
    EXPECT_EQ(fuse_->fileInfos_.count(TEST_USER_ID), 1u);
    EXPECT_EQ(fuse_->fileInfos_[TEST_USER_ID].size(), 1u);
    EXPECT_EQ(fuse_->fileInfos_[TEST_USER_ID][0].dataId, TEST_DATA_ID);
    EXPECT_EQ(fuse_->fileInfos_[TEST_USER_ID][0].recordId, TEST_RECORD_ID);
    EXPECT_EQ(fuse_->fileInfos_[TEST_USER_ID][0].fileName, TEST_FILE_NAME);
    struct stat stbuf;
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/" + TEST_FILE_NAME;
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(stbuf.st_size, 2048);
}

/**
 * @tc.name: StatHandler_DataIdDir_001
 * @tc.desc: should return 0 and set S_IFDIR mode when stat a dataId directory /<dataId>
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_DataIdDir_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct stat stbuf;
    std::string path = "/" + std::to_string(TEST_DATA_ID);
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(stbuf.st_mode & S_IFDIR);
}

/**
 * @tc.name: ReaddirHandler_DataIdDir_001
 * @tc.desc: should return 0 and list recordId directories when readdir on /<dataId>
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_DataIdDir_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {
        {0, 0, TEST_RECORD_ID, "file_a", "a.txt", 100, 0, false, nullptr},
        {0, 0, TEST_RECORD_ID, "file_b", "b.txt", 200, 0, false, nullptr}
    };
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    std::string path = "/" + std::to_string(TEST_DATA_ID);
    int ret = PasteboardFuseManager::ReaddirHandler(path.c_str(), nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, 0);
}

// ===== ParsePath Comprehensive Branch Tests =====

/**
 * @tc.name: ParsePath_001
 * @tc.desc: ParsePath returns PATH_INVALID for nullptr and non-slash-prefixed paths
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ParsePath_001, TestSize.Level1)
{
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    EXPECT_EQ(PasteboardFuseManager::ParsePath(nullptr, dataId, recordId, fileName),
        PasteboardFuseManager::PATH_INVALID);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("noslash", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_INVALID);
}

/**
 * @tc.name: ParsePath_002
 * @tc.desc: ParsePath returns PATH_ROOT for "/" and PATH_DATA_DIR for /<dataId> forms
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ParsePath_002, TestSize.Level1)
{
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_ROOT);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_DATA_DIR);
    EXPECT_EQ(dataId, 123u);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123/", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_DATA_DIR);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123/456", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_DATA_DIR);
    EXPECT_EQ(dataId, 123u);
    EXPECT_EQ(recordId, 456u);
}

/**
 * @tc.name: ParsePath_003
 * @tc.desc: ParsePath returns PATH_RECORD_DIR for /<dataId>/<recordId>/ and PATH_FILE for file path
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ParsePath_003, TestSize.Level1)
{
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123/456/", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_RECORD_DIR);
    EXPECT_EQ(dataId, 123u);
    EXPECT_EQ(recordId, 456u);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123/456/file.txt", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_FILE);
    EXPECT_EQ(fileName, "file.txt");
}

/**
 * @tc.name: ParsePath_004
 * @tc.desc: ParsePath returns PATH_INVALID for non-numeric dataId or recordId segments
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ParsePath_004, TestSize.Level1)
{
    uint32_t dataId = 0;
    uint32_t recordId = 0;
    std::string fileName;
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/abc", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_INVALID);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123/abc", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_INVALID);
    EXPECT_EQ(PasteboardFuseManager::ParsePath("/123/abc/file", dataId, recordId, fileName),
        PasteboardFuseManager::PATH_INVALID);
}

// ===== StatHandler PATH_RECORD_DIR and DataIdDir NotFound Branches =====

/**
 * @tc.name: StatHandler_RecordIdDir_001
 * @tc.desc: should return 0 and set S_IFDIR when stat a recordId directory /<dataId>/<recordId>/
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_RecordIdDir_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct stat stbuf;
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/";
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(stbuf.st_mode & S_IFDIR);
}

/**
 * @tc.name: StatHandler_RecordIdDir_NotFound_001
 * @tc.desc: should return -ENOENT when stat a recordId directory that has no matching records
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_RecordIdDir_NotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct stat stbuf;
    int ret = PasteboardFuseManager::StatHandler("/999/888/", &stbuf, nullptr);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: StatHandler_DataIdDir_NotFound_001
 * @tc.desc: should return -ENOENT when stat a dataId directory that has no matching data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_DataIdDir_NotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct stat stbuf;
    int ret = PasteboardFuseManager::StatHandler("/999", &stbuf, nullptr);
    EXPECT_EQ(ret, -ENOENT);
}

// ===== OpenHandler EISDIR Branch Tests =====

/**
 * @tc.name: OpenHandler_DirPath_001
 * @tc.desc: should return -EISDIR when opening a dataId directory path
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_DirPath_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    struct fuse_file_info fi{};
    std::string path = "/" + std::to_string(TEST_DATA_ID);
    int ret = PasteboardFuseManager::OpenHandler(path.c_str(), &fi);
    EXPECT_EQ(ret, -EISDIR);
}

/**
 * @tc.name: OpenHandler_RecordIdDir_001
 * @tc.desc: should return -EISDIR when opening a recordId directory path
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OpenHandler_RecordIdDir_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    struct fuse_file_info fi{};
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/";
    int ret = PasteboardFuseManager::OpenHandler(path.c_str(), &fi);
    EXPECT_EQ(ret, -EISDIR);
}

// ===== ReaddirHandler PATH_RECORD_DIR Branch Tests =====

/**
 * @tc.name: ReaddirHandler_RecordIdDir_001
 * @tc.desc: should return 0 and list files when readdir on /<dataId>/<recordId>/
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_RecordIdDir_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {
        {0, 0, TEST_RECORD_ID, "file_a", "a.txt", 100, 0, false, nullptr},
        {0, 0, TEST_RECORD_ID, "file_b", "b.txt", 200, 0, false, nullptr}
    };
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/";
    int ret = PasteboardFuseManager::ReaddirHandler(path.c_str(), nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: ReaddirHandler_RecordIdDir_NotFound_001
 * @tc.desc: should return -ENOENT when readdir on a recordId directory with no matching data
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_RecordIdDir_NotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::string path = "/999/888/";
    int ret = PasteboardFuseManager::ReaddirHandler(path.c_str(), nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -ENOENT);
}

// ===== OnAppExit Copy-App Path and Paste-App Remaining-Pid Branches =====

/**
 * @tc.name: OnAppExit_CopyApp_001
 * @tc.desc: when the exiting app is the copy app (provider exists), cleanup provider + stop session
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnAppExit_CopyApp_001, TestSize.Level1)
{
    auto *getter = new MockFuseGetter();
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 1024, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, getter, TEST_DATA_ID, infos);
    fuse_->authorizedPastePids_[TEST_USER_ID].insert(TEST_PID);
    fuse_->OnAppExit(TEST_PID, TEST_USER_ID);
    EXPECT_FALSE(fuse_->HasProvider(TEST_USER_ID));
    EXPECT_EQ(fuse_->authorizedPastePids_.count(TEST_USER_ID), 0u);
    EXPECT_EQ(fuse_->fileInfos_.count(TEST_USER_ID), 0u);
}

/**
 * @tc.name: OnAppExit_PasteApp_RemainingPids_001
 * @tc.desc: when a paste app exits but other authorized pids remain, do NOT clear the pid set
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnAppExit_PasteApp_RemainingPids_001, TestSize.Level1)
{
    constexpr int32_t OTHER_PID = 5555;
    fuse_->authorizedPastePids_[TEST_USER_ID].insert(TEST_PID);
    fuse_->authorizedPastePids_[TEST_USER_ID].insert(OTHER_PID);
    fuse_->OnAppExit(TEST_PID, TEST_USER_ID);
    EXPECT_EQ(fuse_->authorizedPastePids_[TEST_USER_ID].count(TEST_PID), 0u);
    EXPECT_EQ(fuse_->authorizedPastePids_[TEST_USER_ID].count(OTHER_PID), 1u);
}

// ===== OnGetData Session-Exists and StopSessionLoop No-Session Branches =====

/**
 * @tc.name: OnGetData_SessionExists_001
 * @tc.desc: should return 0 without mounting when a FUSE session already exists for the userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, OnGetData_SessionExists_001, TestSize.Level1)
{
    fuse_->authorizedPastePids_[TEST_USER_ID].insert(TEST_PID);
    fuse_->fuseSessions_[TEST_USER_ID].running.store(false);
    EXPECT_CALL(*mountMock_, MountUdmfClipFuse(_, _, _)).Times(0);
    int32_t ret = fuse_->OnGetData(TEST_USER_ID);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: StopSessionLoop_NoSession_001
 * @tc.desc: StopSessionLoop should be a no-op (no crash) when no session exists for the userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StopSessionLoop_NoSession_001, TestSize.Level1)
{
    fuse_->StopSessionLoop(TEST_USER_ID);
    SUCCEED();
}

// ===== ReadHandler Zero-Size Branch =====

/**
 * @tc.name: ReadHandler_ZeroSize_001
 * @tc.desc: should return 0 immediately when requested read size is 0
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReadHandler_ZeroSize_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    auto *getter = new MockFuseGetter();
    auto info = std::make_shared<FileInfo>();
    info->userId = TEST_USER_ID;
    info->fileId = TEST_FILE_ID;
    info->provider = getter;
    int fd = fuse_->nextFd_++;
    fuse_->fdMap_[fd] = {static_cast<uid_t>(TEST_PID), info};
    struct fuse_file_info fi{};
    fi.fh = static_cast<uint64_t>(fd);
    char buf[16];
    int ret = PasteboardFuseManager::ReadHandler("/test", buf, 0, 0, &fi);
    EXPECT_EQ(ret, 0);
}

// ===== GetFileInfoLocked NotFound and NoUser Branch =====

/**
 * @tc.name: GetFileInfo_NoUser_001
 * @tc.desc: GetFileInfoLocked should return nullptr when userId has no file infos
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, GetFileInfo_NoUser_001, TestSize.Level1)
{
    std::lock_guard<std::mutex> lock(fuse_->mutex_);
    auto info = fuse_->GetFileInfoLocked(TEST_USER_ID, TEST_DATA_ID, TEST_RECORD_ID, TEST_FILE_NAME);
    EXPECT_EQ(info, nullptr);
}

// ===== New-Commit Branch Coverage: dataId/recordId existence validation =====

/**
 * @tc.name: ReaddirHandler_RecordIdDir_DataIdNotFound_001
 * @tc.desc: ReaddirHandler on /<dataId>/<recordId>/ should return -ENOENT when fileInfos exist
 *           but no entry matches the requested dataId (new validation branch added in latest commit)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_RecordIdDir_DataIdNotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    constexpr uint32_t ABSENT_DATA_ID = 999;
    std::string path = "/" + std::to_string(ABSENT_DATA_ID) + "/" + std::to_string(TEST_RECORD_ID) + "/";
    int ret = PasteboardFuseManager::ReaddirHandler(path.c_str(), nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: ReaddirHandler_RecordIdDir_RecordIdNotFound_001
 * @tc.desc: ReaddirHandler on /<dataId>/<recordId>/ should return -ENOENT when fileInfos exist
 *           and dataId matches but recordId does not (new validation branch)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_RecordIdDir_RecordIdNotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    constexpr uint32_t ABSENT_RECORD_ID = 888;
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(ABSENT_RECORD_ID) + "/";
    int ret = PasteboardFuseManager::ReaddirHandler(path.c_str(), nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: ReaddirHandler_DataIdDir_DataIdNotFound_001
 * @tc.desc: ReadDataIdDirLocked should return -ENOENT when fileInfos exist but no entry
 *           matches the requested dataId (new validation branch added in latest commit)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, ReaddirHandler_DataIdDir_DataIdNotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    constexpr uint32_t ABSENT_DATA_ID = 999;
    std::string path = "/" + std::to_string(ABSENT_DATA_ID);
    int ret = PasteboardFuseManager::ReaddirHandler(path.c_str(), nullptr,
        reinterpret_cast<void *>(&TestFiller), 0, nullptr, 0);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: StatHandler_DataIdDir_DataIdNotFound_001
 * @tc.desc: StatHandler on /<dataId> should return -ENOENT when fileInfos exist but no entry
 *           matches the requested dataId (existing second-level validation, not previously covered)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_DataIdDir_DataIdNotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    constexpr uint32_t ABSENT_DATA_ID = 999;
    struct stat stbuf;
    std::string path = "/" + std::to_string(ABSENT_DATA_ID);
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, -ENOENT);
}

/**
 * @tc.name: StatHandler_RecordIdDir_RecordIdNotFound_001
 * @tc.desc: StatHandler on /<dataId>/<recordId>/ should return -ENOENT when fileInfos exist
 *           and dataId matches but recordId does not (existing second-level validation)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardFuseManagerTest, StatHandler_RecordIdDir_RecordIdNotFound_001, TestSize.Level1)
{
    SetupAuthorizedContext(TEST_PID);
    std::vector<FileInfo> infos = {{0, 0, TEST_RECORD_ID, TEST_FILE_ID, TEST_FILE_NAME, 100, 0, false, nullptr}};
    fuse_->SetProvider(TEST_USER_ID, nullptr, TEST_DATA_ID, infos);
    constexpr uint32_t ABSENT_RECORD_ID = 888;
    struct stat stbuf;
    std::string path = "/" + std::to_string(TEST_DATA_ID) + "/" + std::to_string(ABSENT_RECORD_ID) + "/";
    int ret = PasteboardFuseManager::StatHandler(path.c_str(), &stbuf, nullptr);
    EXPECT_EQ(ret, -ENOENT);
}
} // namespace
