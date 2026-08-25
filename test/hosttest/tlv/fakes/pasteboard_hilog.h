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

// HOST-TEST FAKE for pasteboard_hilog.h (TLV suite).
// Drops device logging; preserves the control-flow semantics of the check
// macros that tlv_utils.cpp relies on (return / return ret on false cond).

#ifndef PASTEBOARD_HOSTTEST_FAKE_PASTEBOARD_HILOG_H
#define PASTEBOARD_HOSTTEST_FAKE_PASTEBOARD_HILOG_H

namespace OHOS {
namespace MiscServices {
enum PasteboardModule {
    PASTEBOARD_MODULE_COMMON = 0,
    PASTEBOARD_MODULE_SERVICE,
};
} // namespace MiscServices
} // namespace OHOS

#define PASTEBOARD_HILOGE(module, fmt, ...) do { (void)(module); } while (0)
#define PASTEBOARD_HILOGI(module, fmt, ...) do { (void)(module); } while (0)
#define PASTEBOARD_HILOGD(module, fmt, ...) do { (void)(module); } while (0)
#define PASTEBOARD_HILOGW(module, fmt, ...) do { (void)(module); } while (0)

// return; on false cond (matches CHECK_AND_RETURN_LOG_INNER).
#define PASTEBOARD_CHECK_AND_RETURN_LOGE(cond, label, fmt, ...) \
    do {                                                        \
        if (!(cond)) {                                          \
            return;                                             \
        }                                                       \
    } while (0)

// return ret; on false cond (matches CHECK_AND_RETURN_RET_LOG_INNER).
#define PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(cond, ret, label, fmt, ...) \
    do {                                                                \
        if (!(cond)) {                                                  \
            return ret;                                                 \
        }                                                               \
    } while (0)

#endif // PASTEBOARD_HOSTTEST_FAKE_PASTEBOARD_HILOG_H
