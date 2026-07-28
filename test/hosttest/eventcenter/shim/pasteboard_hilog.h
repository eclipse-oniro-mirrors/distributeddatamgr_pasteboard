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

// HOST-TEST SHIM for utils/native/include/pasteboard_hilog.h
//
// The real header pulls in hilog/log.h (an OHOS platform logging library that
// is not present on a plain host). event_center.cpp uses only three symbols
// from it, so we provide a minimal stand-in that:
//   * drops the actual logging (PASTEBOARD_HILOGE -> no-op), and
//   * PRESERVES the control-flow semantics of the check macro
//     (PASTEBOARD_CHECK_AND_RETURN_LOGE returns void when cond is false),
//     because the unit under test relies on that early return.
//
// This is the "fake seam" pattern: a device dependency is replaced by a host
// stub that keeps behaviour identical for the logic being tested.

#ifndef PASTEBOARD_HOSTTEST_SHIM_PASTEBOARD_HILOG_H
#define PASTEBOARD_HOSTTEST_SHIM_PASTEBOARD_HILOG_H

namespace OHOS {
namespace MiscServices {
// Only the label referenced by event_center.cpp is needed.
enum PasteboardModule {
    PASTEBOARD_MODULE_SERVICE = 0,
};
} // namespace MiscServices
} // namespace OHOS

// Logging is a no-op on host. The (void) cast keeps -Wunused quiet and mirrors
// the real macro accepting a label + printf-style args.
#define PASTEBOARD_HILOGE(module, fmt, ...) \
    do {                                    \
        (void)(module);                     \
    } while (0)
#define PASTEBOARD_HILOGI(module, fmt, ...) \
    do {                                    \
        (void)(module);                     \
    } while (0)
#define PASTEBOARD_HILOGD(module, fmt, ...) \
    do {                                    \
        (void)(module);                     \
    } while (0)
#define PASTEBOARD_HILOGW(module, fmt, ...) \
    do {                                    \
        (void)(module);                     \
    } while (0)

// Matches the real semantics: log then `return;` (void) when cond is falsy.
#define PASTEBOARD_CHECK_AND_RETURN_LOGE(cond, label, fmt, ...) \
    do {                                                        \
        if (!(cond)) {                                          \
            return;                                             \
        }                                                       \
    } while (0)

#endif // PASTEBOARD_HOSTTEST_SHIM_PASTEBOARD_HILOG_H
