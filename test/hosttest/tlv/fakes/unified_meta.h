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

// HOST-TEST FAKE for udmf's unified_meta.h (as included by tlv_utils.h).
//
// The real header drags in string_ex.h + unified_key.h and a chain of UDMF
// types. tlv_utils.{h,cpp} only needs API_EXPORT from this include (it does not
// use any UDMF::ValueType / Object here). So the fake provides just the macro
// and the common stdlib headers the TLV headers expect to be in scope.

#ifndef PASTEBOARD_HOSTTEST_FAKE_UNIFIED_META_H
#define PASTEBOARD_HOSTTEST_FAKE_UNIFIED_META_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifndef API_EXPORT
#define API_EXPORT __attribute__((visibility("default")))
#endif

// tlv_utils.h references Media::PixelMap in method signatures BEFORE the .cpp
// includes pixel_map.h. In the real build the type is visible here through the
// include chain; the fake provides a forward declaration so the header parses.
// The full fake definition lives in fakes/pixel_map.h (included by the .cpp).
namespace OHOS {
namespace Media {
class PixelMap;
} // namespace Media
} // namespace OHOS

#endif // PASTEBOARD_HOSTTEST_FAKE_UNIFIED_META_H
