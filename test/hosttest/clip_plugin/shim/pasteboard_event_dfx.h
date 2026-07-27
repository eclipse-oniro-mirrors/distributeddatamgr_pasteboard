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

// HOST-TEST SHIM for pasteboard_event_dfx.h.
// The real header pulls in hisysevent.h (OHOS system-event lib). clip_plugin.cpp
// uses only the RadarReporter enums + the RADAR_REPORT macro, so this shim
// provides those and turns RADAR_REPORT into a no-op.

#ifndef PASTEBOARD_HOSTTEST_SHIM_PASTEBOARD_EVENT_DFX_H
#define PASTEBOARD_HOSTTEST_SHIM_PASTEBOARD_EVENT_DFX_H

namespace OHOS {
namespace MiscServices {
namespace RadarReporter {
enum BizScene : int {
    DFX_PLUGIN_CREATE_DESTROY = 8,
};
enum PluginCreateDestroyStage : int {
    DFX_PLUGIN_CREATE = 1,
    DFX_PLUGIN_DESTROY = 2,
};
enum StageRes : int {
    DFX_IDLE = 0,
    DFX_SUCCESS = 1,
    DFX_FAILED = 2,
    DFX_CANCELLED = 3,
};
} // namespace RadarReporter
} // namespace MiscServices
} // namespace OHOS

// no-op: host tests don't emit system events
#define RADAR_REPORT(bizScene, bizStage, stageRes, ...) \
    do {                                                \
        (void)(bizScene);                               \
        (void)(bizStage);                               \
        (void)(stageRes);                               \
    } while (0)

#endif // PASTEBOARD_HOSTTEST_SHIM_PASTEBOARD_EVENT_DFX_H
