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

#include "dev_slinfo_mgr_mock.h"
#include "dm_adapter_mock.h"
#include "security_level.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

namespace {
const std::string TEST_UDID = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
}

namespace OHOS::MiscServices {
extern bool InitDEVSLQueryParams(DEVSLQueryParams *params, const std::string &udid);
}

class SecurityLevelTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
};

void SecurityLevelTest::SetUpTestCase(void) {}
void SecurityLevelTest::TearDownTestCase(void) {}
void SecurityLevelTest::SetUp(void) {}
void SecurityLevelTest::TearDown(void) {}

HWTEST_F(SecurityLevelTest, Constructor_001, TestSize.Level1)
{
    SecurityLevel securityLevel;
    EXPECT_EQ(securityLevel.securityLevel_, DATA_SEC_LEVEL0);
}

HWTEST_F(SecurityLevelTest, IsSupportedDistributed_LevelAboveThreshold_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL4), Return(DEVSL_SUCCESS)));
    EXPECT_CALL(devSlMock, OnStop()).WillRepeatedly(Return());

    SecurityLevel securityLevel;
    EXPECT_TRUE(securityLevel.IsSupportedDistributed(false));
}

HWTEST_F(SecurityLevelTest, IsSupportedDistributed_LevelBelowThreshold_NeedLog_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL1), Return(DEVSL_SUCCESS)));
    EXPECT_CALL(devSlMock, OnStop()).WillRepeatedly(Return());

    SecurityLevel securityLevel;
    EXPECT_FALSE(securityLevel.IsSupportedDistributed(true));
}

HWTEST_F(SecurityLevelTest, IsSupportedDistributed_LevelBelowThreshold_NoLog_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL1), Return(DEVSL_SUCCESS)));
    EXPECT_CALL(devSlMock, OnStop()).WillRepeatedly(Return());

    SecurityLevel securityLevel;
    EXPECT_FALSE(securityLevel.IsSupportedDistributed(false));
}

HWTEST_F(SecurityLevelTest, GetDeviceSecurityLevel_CachedLevel_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillOnce(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillOnce(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL4), Return(DEVSL_SUCCESS)));
    EXPECT_CALL(devSlMock, OnStop()).WillOnce(Return());

    SecurityLevel securityLevel;
    EXPECT_TRUE(securityLevel.IsSupportedDistributed(false));
    EXPECT_EQ(securityLevel.securityLevel_, DATA_SEC_LEVEL4);

    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _)).Times(0);
    EXPECT_CALL(devSlMock, OnStop()).Times(0);

    EXPECT_TRUE(securityLevel.IsSupportedDistributed(false));
}

HWTEST_F(SecurityLevelTest, GetSensitiveLevel_Success_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL3), Return(DEVSL_SUCCESS)));
    EXPECT_CALL(devSlMock, OnStop()).WillRepeatedly(Return());

    SecurityLevel securityLevel;
    EXPECT_TRUE(securityLevel.IsSupportedDistributed(false));
    EXPECT_EQ(securityLevel.securityLevel_, DATA_SEC_LEVEL3);
}

HWTEST_F(SecurityLevelTest, GetSensitiveLevel_Failed_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL0), Return(DEVSL_ERROR)));
    EXPECT_CALL(devSlMock, OnStop()).WillRepeatedly(Return());

    SecurityLevel securityLevel;
    EXPECT_FALSE(securityLevel.IsSupportedDistributed(false));
}

HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_NullParams_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).Times(0);
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _)).Times(0);
    EXPECT_CALL(devSlMock, OnStop()).Times(0);

    SecurityLevel securityLevel;
    EXPECT_EQ(securityLevel.securityLevel_, DATA_SEC_LEVEL0);
}

HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_EmptyUdid_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    static std::string emptyUdid;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(emptyUdid));
    EXPECT_CALL(devSlMock, OnStart()).Times(0);
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _)).Times(0);
    EXPECT_CALL(devSlMock, OnStop()).Times(0);

    SecurityLevel securityLevel;
    EXPECT_FALSE(securityLevel.IsSupportedDistributed(false));
    EXPECT_EQ(securityLevel.securityLevel_, DATA_SEC_LEVEL0);
}

HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_Success_001, TestSize.Level1)
{
    DEVSLQueryParams params = {};
    bool ret = InitDEVSLQueryParams(&params, TEST_UDID);
    EXPECT_TRUE(ret);
    EXPECT_EQ(params.udidLen, static_cast<uint32_t>(MAX_UDID_LENGTH));
}

HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_NullParams_002, TestSize.Level1)
{
    bool ret = InitDEVSLQueryParams(nullptr, TEST_UDID);
    EXPECT_FALSE(ret);
}

HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_EmptyUdid_002, TestSize.Level1)
{
    DEVSLQueryParams params = {};
    bool ret = InitDEVSLQueryParams(&params, "");
    EXPECT_FALSE(ret);
}

HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_ShortUdid_001, TestSize.Level1)
{
    DEVSLQueryParams params = {};
    static std::string shortUdid = "abc";
    bool ret = InitDEVSLQueryParams(&params, shortUdid);
    EXPECT_TRUE(ret);
    EXPECT_EQ(params.udid[0], 'a');
    EXPECT_EQ(params.udid[1], 'b');
    EXPECT_EQ(params.udid[2], 'c');
    EXPECT_EQ(params.udidLen, static_cast<uint32_t>(MAX_UDID_LENGTH));
}

HWTEST_F(SecurityLevelTest, GetSensitiveLevel_InitFailed_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    static std::string emptyUdid;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(emptyUdid));
    EXPECT_CALL(devSlMock, OnStart()).Times(0);
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _)).Times(0);
    EXPECT_CALL(devSlMock, OnStop()).Times(0);

    SecurityLevel securityLevel;
    EXPECT_FALSE(securityLevel.IsSupportedDistributed(false));
}

HWTEST_F(SecurityLevelTest, IsSupportedDistributed_Level3_001, TestSize.Level1)
{
    NiceMock<DevSlInfoMgrMock> devSlMock;
    NiceMock<DMAdapterMock> dmMock;
    EXPECT_CALL(dmMock, GetLocalDeviceUdid()).WillRepeatedly(ReturnRef(TEST_UDID));
    EXPECT_CALL(devSlMock, OnStart()).WillRepeatedly(Return(DEVSL_SUCCESS));
    EXPECT_CALL(devSlMock, GetHighestSecLevel(_, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(DATA_SEC_LEVEL3), Return(DEVSL_SUCCESS)));
    EXPECT_CALL(devSlMock, OnStop()).WillRepeatedly(Return());

    SecurityLevel securityLevel;
    EXPECT_TRUE(securityLevel.IsSupportedDistributed(false));
}