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

/**
 * @tc.name: Constructor_001
 * @tc.desc: Test SecurityLevel default constructor initializes securityLevel_ to DATA_SEC_LEVEL0
 * @tc.type: FUNC
 */
HWTEST_F(SecurityLevelTest, Constructor_001, TestSize.Level1)
{
    SecurityLevel securityLevel;
    EXPECT_EQ(securityLevel.securityLevel_, DATA_SEC_LEVEL0);
}

/**
 * @tc.name: IsSupportedDistributed_LevelAboveThreshold_001
 * @tc.desc: Test IsSupportedDistributed returns true when security level is above threshold
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: IsSupportedDistributed_LevelBelowThreshold_NeedLog_001
 * @tc.desc: Test IsSupportedDistributed returns false when level below threshold and needLog is true
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: IsSupportedDistributed_LevelBelowThreshold_NoLog_001
 * @tc.desc: Test IsSupportedDistributed returns false when level below threshold and needLog is false
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: GetDeviceSecurityLevel_CachedLevel_001
 * @tc.desc: Test cached security level is reused on second IsSupportedDistributed call without re-querying
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: GetSensitiveLevel_Success_001
 * @tc.desc: Test GetSensitiveLevel succeeds with DATA_SEC_LEVEL3 and IsSupportedDistributed returns true
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: GetSensitiveLevel_Failed_001
 * @tc.desc: Test GetSensitiveLevel fails with DEVSL_ERROR and IsSupportedDistributed returns false
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: InitDEVSLQueryParams_NullParams_001
 * @tc.desc: Test SecurityLevel default state when DEVSL query params are not initialized
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: InitDEVSLQueryParams_EmptyUdid_001
 * @tc.desc: Test IsSupportedDistributed returns false when device UDID is empty
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: InitDEVSLQueryParams_Success_001
 * @tc.desc: Test InitDEVSLQueryParams succeeds with valid UDID and sets correct udidLen
 * @tc.type: FUNC
 */
HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_Success_001, TestSize.Level1)
{
    DEVSLQueryParams params = {};
    bool ret = InitDEVSLQueryParams(&params, TEST_UDID);
    EXPECT_TRUE(ret);
    EXPECT_EQ(params.udidLen, static_cast<uint32_t>(MAX_UDID_LENGTH));
}

/**
 * @tc.name: InitDEVSLQueryParams_NullParams_002
 * @tc.desc: Test InitDEVSLQueryParams returns false when params pointer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_NullParams_002, TestSize.Level1)
{
    bool ret = InitDEVSLQueryParams(nullptr, TEST_UDID);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InitDEVSLQueryParams_EmptyUdid_002
 * @tc.desc: Test InitDEVSLQueryParams returns false when UDID string is empty
 * @tc.type: FUNC
 */
HWTEST_F(SecurityLevelTest, InitDEVSLQueryParams_EmptyUdid_002, TestSize.Level1)
{
    DEVSLQueryParams params = {};
    bool ret = InitDEVSLQueryParams(&params, "");
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: InitDEVSLQueryParams_ShortUdid_001
 * @tc.desc: Test InitDEVSLQueryParams succeeds with short UDID and copies partial content correctly
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: GetSensitiveLevel_InitFailed_001
 * @tc.desc: Test IsSupportedDistributed returns false when UDID is empty causing init to fail
 * @tc.type: FUNC
 */
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

/**
 * @tc.name: IsSupportedDistributed_Level3_001
 * @tc.desc: Test IsSupportedDistributed returns true with DATA_SEC_LEVEL3 security level
 * @tc.type: FUNC
 */
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