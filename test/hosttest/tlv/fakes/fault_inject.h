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

// Test-only fault-injection switch for the TLV fakes.
//
// TLVUtils::Raw2Parcel has three failure branches: allocation failure, safe-copy
// failure, and Parcel::ParseFrom failure. The first two are reachable from the
// tests with nothing but a hostile bufferLen (SIZE_MAX, and a value above
// securec's SECUREC_MEM_MAX_LEN), so they need no seam -- and the suite links
// the real securec rather than a test-local substitute for a sanctioned
// safe function. Only the ParseFrom branch needs a switch, so that the test can
// exercise it deterministically without changing tlv_utils.cpp itself.

#ifndef PASTEBOARD_HOSTTEST_FAULT_INJECT_H
#define PASTEBOARD_HOSTTEST_FAULT_INJECT_H

namespace hosttest_fault {
inline bool g_forceParseFromFail = false;
} // namespace hosttest_fault

#endif // PASTEBOARD_HOSTTEST_FAULT_INJECT_H
