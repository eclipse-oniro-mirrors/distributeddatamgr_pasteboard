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

#ifndef NAPI_PASTEBOARD_ASSERT_H
#define NAPI_PASTEBOARD_ASSERT_H

#include "napi_init.h"

#define PASTEBOARD_RETVAL_NOTHING

#define NAPI_CREATE_AND_THROW(env, code, message)                                                 \
    do {                                                                                          \
        napi_value errorObj = nullptr;                                                            \
        napi_value errorCode = nullptr;                                                           \
        napi_value errorMessage = nullptr;                                                        \
        napi_create_int32(env, code, &errorCode);                                                 \
        napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &errorMessage);                   \
        napi_create_error(env, errorCode, errorMessage, &errorObj);                               \
        napi_throw(env, errorObj);                                                                \
    } while (0)

#define PASTEBOARD_ASSERT_BASE(env, assertion, message, code, retVal)                             \
    do {                                                                                          \
        if (!(assertion)) {                                                                       \
            NAPI_CREATE_AND_THROW(env, code, message);                                            \
            return retVal;                                                                        \
        }                                                                                         \
    } while (0)

#define PASTEBOARD_ASSERT(env, assertion, message, code) PASTEBOARD_ASSERT_BASE(env, assertion, message, code, nullptr)

#define PASTEBOARD_ASSERT_RETURN_VOID(env, assertion, message, code) \
        PASTEBOARD_ASSERT_BASE(env, assertion, message, code, PASTEBOARD_RETVAL_NOTHING)

#define PASTEBOARD_CALL_BASE(theCall, retVal)                                                                \
    do {                                                                                                     \
        if ((theCall) != napi_ok) {                                                                          \
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi call failed, theCall: %{public}s", #theCall); \
            return retVal;                                                                                   \
        }                                                                                                    \
    } while (0)

#define PASTEBOARD_CALL(theCall) PASTEBOARD_CALL_BASE(theCall, nullptr)

#define PASTEBOARD_CALL_RETURN_VOID(theCall) PASTEBOARD_CALL_BASE(theCall, PASTEBOARD_RETVAL_NOTHING)

#endif // NAPI_PASTEBOARD_ASSERT_H