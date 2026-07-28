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

// HOST-TEST FAKE for framework device/dm_adapter.h.
// security_level.cpp only calls DMAdapter::GetInstance().GetLocalDeviceUdid().
// The real DMAdapter is a device-manager-backed singleton; this fake provides
// just that one method with a settable udid (hosttest_dm::g_udid).

#ifndef PASTEBOARD_HOSTTEST_FAKE_DEVICE_DM_ADAPTER_H
#define PASTEBOARD_HOSTTEST_FAKE_DEVICE_DM_ADAPTER_H

#include <string>

namespace OHOS {
namespace MiscServices {

namespace hosttest_dm {
extern std::string g_udid; // settable by tests
}

class DMAdapter {
public:
    static DMAdapter &GetInstance()
    {
        static DMAdapter instance;
        return instance;
    }
    const std::string &GetLocalDeviceUdid()
    {
        return hosttest_dm::g_udid;
    }
};

} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_HOSTTEST_FAKE_DEVICE_DM_ADAPTER_H
