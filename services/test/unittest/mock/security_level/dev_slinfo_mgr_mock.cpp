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

#include "dev_slinfo_mgr_mock.h"

DevSlInfoMgrMock::DevSlInfoMgrMock()
{
    DevSlInfoMgrMock::mock_ = this;
}

DevSlInfoMgrMock::~DevSlInfoMgrMock()
{
    DevSlInfoMgrMock::mock_ = nullptr;
}

DevSlInfoMgrMock *DevSlInfoMgrMock::GetMock()
{
    return DevSlInfoMgrMock::mock_;
}

int32_t DATASL_OnStart(void)
{
    auto mock = DevSlInfoMgrMock::GetMock();
    if (mock == nullptr) {
        return DEVSL_SUCCESS;
    }
    return mock->OnStart();
}

int32_t DATASL_GetHighestSecLevel(DEVSLQueryParams *queryParams, uint32_t *levelInfo)
{
    auto mock = DevSlInfoMgrMock::GetMock();
    if (mock == nullptr) {
        if (levelInfo != nullptr) {
            *levelInfo = DATA_SEC_LEVEL0;
        }
        return DEVSL_SUCCESS;
    }
    return mock->GetHighestSecLevel(queryParams, levelInfo);
}

void DATASL_OnStop(void)
{
    auto mock = DevSlInfoMgrMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->OnStop();
}