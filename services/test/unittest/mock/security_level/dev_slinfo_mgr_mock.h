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

#ifndef DEV_SLINFO_MGR_MOCK_H
#define DEV_SLINFO_MGR_MOCK_H

#include <gmock/gmock.h>

#include "dev_slinfo_mgr.h"

class IDevSlInfoMgr {
public:
    virtual int32_t OnStart() = 0;
    virtual int32_t GetHighestSecLevel(DEVSLQueryParams *queryParams, uint32_t *levelInfo) = 0;
    virtual void OnStop() = 0;
};

class DevSlInfoMgrMock : public IDevSlInfoMgr {
public:
    DevSlInfoMgrMock();
    ~DevSlInfoMgrMock();

    MOCK_METHOD(int32_t, OnStart, (), (override));
    MOCK_METHOD(int32_t, GetHighestSecLevel, (DEVSLQueryParams * queryParams, uint32_t *levelInfo), (override));
    MOCK_METHOD(void, OnStop, (), (override));

    static DevSlInfoMgrMock *GetMock();

private:
    static inline DevSlInfoMgrMock *mock_ = nullptr;
};
#endif // DEV_SLINFO_MGR_MOCK_H