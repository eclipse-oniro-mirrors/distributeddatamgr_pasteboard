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

// HOST-TEST FAKE for dataclassification dev_slinfo_mgr.h (DEVSL C API).
// security_level.cpp calls DATASL_OnStart / DATASL_GetHighestSecLevel /
// DATASL_OnStop. This fake mirrors the real macros/struct and adds test hooks
// (g_devslResult, g_devslLevel) so the security-level branches can be driven
// deterministically without the real dataclassification service.

#ifndef PASTEBOARD_HOSTTEST_FAKE_DEV_SLINFO_MGR_H
#define PASTEBOARD_HOSTTEST_FAKE_DEV_SLINFO_MGR_H

#include <cstdint>

#define MAX_UDID_LENGTH 64
#define DATA_SEC_LEVEL0 0
#define DATA_SEC_LEVEL1 1
#define DATA_SEC_LEVEL2 2
#define DATA_SEC_LEVEL3 3
#define DATA_SEC_LEVEL4 4

typedef struct {
    uint8_t udid[MAX_UDID_LENGTH];
    uint32_t udidLen;
} DEVSLQueryParams;

enum {
    DEVSL_SUCCESS = 0,
    DEVSL_ERROR = 1,
};

// Test hooks (defined in the test TU).
namespace hosttest_devsl {
extern int32_t g_result;   // value DATASL_GetHighestSecLevel returns
extern uint32_t g_level;   // level it writes out
}

inline int32_t DATASL_OnStart(void)
{
    return DEVSL_SUCCESS;
}

inline void DATASL_OnStop(void) {}

inline int32_t DATASL_GetHighestSecLevel(DEVSLQueryParams *queryParams, uint32_t *levelInfo)
{
    (void)queryParams;
    // The real API only writes a meaningful level on success; on failure it
    // leaves the caller's value untouched. Mirror that so failure paths see
    // the caller's initial level (0), not a stale injected level.
    if (levelInfo != nullptr && hosttest_devsl::g_result == DEVSL_SUCCESS) {
        *levelInfo = hosttest_devsl::g_level;
    }
    return hosttest_devsl::g_result;
}

#endif // PASTEBOARD_HOSTTEST_FAKE_DEV_SLINFO_MGR_H
