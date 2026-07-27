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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_FRAMEWORK_COMMON_HISTOGRAM_WRAPPER_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_FRAMEWORK_COMMON_HISTOGRAM_WRAPPER_H

#include <atomic>
#include "histogram_enum.h"

#ifdef PASTEBOARD_API_METRICS_ENABLED
#include "histogram_plugin_macros.h"

#define HISTOGRAM_BOOLEAN_SAMPLED(name, value) \
    do { \
        static std::atomic<uint32_t> counter{0}; \
        uint32_t count = ++counter; \
        if (count == 1 || (count - 1) % 100 == 0) { \
            HISTOGRAM_BOOLEAN(name, value); \
        } \
    } while (0)

#define HISTOGRAM_ENUMERATION_SAMPLED(name, sample, boundary) \
    do { \
        static std::atomic<uint32_t> counter{0}; \
        uint32_t count = ++counter; \
        if (count == 1 || (count - 1) % 100 == 0) { \
            HISTOGRAM_ENUMERATION(name, sample, boundary); \
        } \
    } while (0)

#define HISTOGRAM_BOOLEAN_SAMPLED_1K(name, value) \
    do { \
        static std::atomic<uint32_t> counter{0}; \
        uint32_t count = ++counter; \
        if (count == 1 || (count - 1) % 2000 == 0) { \
            HISTOGRAM_BOOLEAN(name, value); \
        } \
    } while (0)

#else
#define HISTOGRAM_BOOLEAN(name, value) ((void)0)
#define HISTOGRAM_BOOLEAN_SAMPLED(name, value) ((void)0)
#define HISTOGRAM_BOOLEAN_SAMPLED_1K(name, value) ((void)0)
#define HISTOGRAM_ENUMERATION(name, sample, boundary) ((void)0)
#define HISTOGRAM_ENUMERATION_SAMPLED(name, sample, boundary) ((void)0)
#endif

#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_FRAMEWORK_COMMON_HISTOGRAM_WRAPPER_H