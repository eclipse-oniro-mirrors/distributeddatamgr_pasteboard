/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef PASTEBOARD_HILOG_H
#define PASTEBOARD_HILOG_H

#include "hilog/log.h"

namespace OHOS {
namespace MiscServices {
// param of log interface, such as PASTEBOARD_HILOGF.
enum PasteboardSubModule {
    PASTEBOARD_MODULE_INNERKIT = 0,
    PASTEBOARD_MODULE_CLIENT,
    PASTEBOARD_MODULE_SERVICE,
    PASTEBOARD_MODULE_JAVAKIT, // java kit, defined to avoid repeated use of domain.
    PASTEBOARD_MODULE_JNI,
    PASTEBOARD_MODULE_COMMON,
    PASTEBOARD_MODULE_JS_NAPI,
    PASTEBOARD_MODULE_CAPI,
    PASTEBOARD_MODULE_JS_ANI,
    PASTEBOARD_MODULE_BUTT,
};

// 0xD001C00: subsystem:PASTEBOARD module:PasteboardManager, 8 bits reserved.
static constexpr unsigned int BASE_PASTEBOARD_DOMAIN_ID = 0xD001C00;

enum PasteboardDomainId {
    PASTEBOARD_INNERKIT_DOMAIN = BASE_PASTEBOARD_DOMAIN_ID + PASTEBOARD_MODULE_INNERKIT,
    PASTEBOARD_CLIENT_DOMAIN,
    PASTEBOARD_SERVICE_DOMAIN,
    PASTEBOARD_JAVAKIT_DOMAIN,
    PASTEBOARD_JNI_DOMAIN,
    PASTEBOARD_COMMON_DOMAIN,
    PASTEBOARD_JS_NAPI,
    PASTEBOARD_CAPI,
    PASTEBOARD_JS_ANI,
    PASTEBOARD_BUTT,
};

static constexpr OHOS::HiviewDFX::HiLogLabel PASTEBOARD[PASTEBOARD_MODULE_BUTT] = {
    { LOG_CORE, PASTEBOARD_INNERKIT_DOMAIN, "PBIK" },
    { LOG_CORE, PASTEBOARD_CLIENT_DOMAIN, "PBC" },
    { LOG_CORE, PASTEBOARD_SERVICE_DOMAIN, "PBS" },
    { LOG_CORE, PASTEBOARD_JAVAKIT_DOMAIN, "PBJK" },
    { LOG_CORE, PASTEBOARD_JNI_DOMAIN, "PBJN" },
    { LOG_CORE, PASTEBOARD_COMMON_DOMAIN, "PBCM" },
    { LOG_CORE, PASTEBOARD_JS_NAPI, "PBJS" },
    { LOG_CORE, PASTEBOARD_CAPI, "PBCA" },
    { LOG_CORE, PASTEBOARD_JS_ANI, "PBANI" },
};

// In order to improve performance, do not check the module range.
// Besides, make sure module is less than PASTEBOARD_MODULE_BUTT.
#define PASTEBOARD_HILOGF(module, ...)                                                                \
    do {                                                                                              \
        if (HiLogIsLoggable(PASTEBOARD[module].domain, PASTEBOARD[module].tag, LOG_FATAL)) {          \
            ((void)HILOG_IMPL(LOG_CORE, LOG_FATAL, PASTEBOARD[module].domain, PASTEBOARD[module].tag, \
                "%{public}s# " fmt, __FUNCTION__, ##__VA_ARGS__));                                    \
        }                                                                                             \
    } while (0)
#define PASTEBOARD_HILOGE(module, fmt, ...)                                                           \
    do {                                                                                              \
        if (HiLogIsLoggable(PASTEBOARD[module].domain, PASTEBOARD[module].tag, LOG_ERROR)) {          \
            ((void)HILOG_IMPL(LOG_CORE, LOG_ERROR, PASTEBOARD[module].domain, PASTEBOARD[module].tag, \
                "%{public}s# " fmt, __FUNCTION__, ##__VA_ARGS__));                                    \
        }                                                                                             \
    } while (0)
#define PASTEBOARD_HILOGW(module, fmt, ...)                                                          \
    do {                                                                                             \
        if (HiLogIsLoggable(PASTEBOARD[module].domain, PASTEBOARD[module].tag, LOG_WARN)) {          \
            ((void)HILOG_IMPL(LOG_CORE, LOG_WARN, PASTEBOARD[module].domain, PASTEBOARD[module].tag, \
                "%{public}s# " fmt, __FUNCTION__, ##__VA_ARGS__));                                   \
        }                                                                                            \
    } while (0)
#define PASTEBOARD_HILOGI(module, fmt, ...)                                                          \
    do {                                                                                             \
        if (HiLogIsLoggable(PASTEBOARD[module].domain, PASTEBOARD[module].tag, LOG_INFO)) {          \
            ((void)HILOG_IMPL(LOG_CORE, LOG_INFO, PASTEBOARD[module].domain, PASTEBOARD[module].tag, \
                "%{public}s# " fmt, __FUNCTION__, ##__VA_ARGS__));                                   \
        }                                                                                            \
    } while (0)
#define PASTEBOARD_HILOGD(module, fmt, ...)                                                           \
    do {                                                                                              \
        if (HiLogIsLoggable(PASTEBOARD[module].domain, PASTEBOARD[module].tag, LOG_DEBUG)) {          \
            ((void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, PASTEBOARD[module].domain, PASTEBOARD[module].tag, \
                "%{public}s# " fmt, __FUNCTION__, ##__VA_ARGS__));                                    \
        }                                                                                             \
    } while (0)

// check then return ret and print log
#define CHECK_AND_RETURN_RET_LOG_INNER(cond, ret, log, label, fmt, ...) \
    do {                                                                \
        if (!(cond)) {                                                  \
            log(label, fmt, ##__VA_ARGS__);                             \
            return ret;                                                 \
        }                                                               \
    } while (0)

// check then return and print log
#define CHECK_AND_RETURN_LOG_INNER(cond, log, label, fmt, ...) \
    do {                                                       \
        if (!(cond)) {                                         \
            log(label, fmt, ##__VA_ARGS__);                    \
            return;                                            \
        }                                                      \
    } while (0)

#define PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(cond, ret, label, fmt, ...) \
    CHECK_AND_RETURN_RET_LOG_INNER(cond, ret, PASTEBOARD_HILOGD, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(cond, ret, label, fmt, ...) \
    CHECK_AND_RETURN_RET_LOG_INNER(cond, ret, PASTEBOARD_HILOGI, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_RET_LOGW(cond, ret, label, fmt, ...) \
    CHECK_AND_RETURN_RET_LOG_INNER(cond, ret, PASTEBOARD_HILOGW, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(cond, ret, label, fmt, ...) \
    CHECK_AND_RETURN_RET_LOG_INNER(cond, ret, PASTEBOARD_HILOGE, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_LOGD(cond, label, fmt, ...) \
    CHECK_AND_RETURN_LOG_INNER(cond, PASTEBOARD_HILOGD, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_LOGI(cond, label, fmt, ...) \
    CHECK_AND_RETURN_LOG_INNER(cond, PASTEBOARD_HILOGI, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_LOGW(cond, label, fmt, ...) \
    CHECK_AND_RETURN_LOG_INNER(cond, PASTEBOARD_HILOGW, label, fmt, ##__VA_ARGS__)
#define PASTEBOARD_CHECK_AND_RETURN_LOGE(cond, label, fmt, ...) \
    CHECK_AND_RETURN_LOG_INNER(cond, PASTEBOARD_HILOGE, label, fmt, ##__VA_ARGS__)
} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_HILOG_H
