/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_DP_DISTRIBUTED_DEVICE_PROFILE_ERRORS_H
#define OHOS_DP_DISTRIBUTED_DEVICE_PROFILE_ERRORS_H

namespace OHOS {
namespace DistributedDeviceProfile {
constexpr int32_t DP_SUCCESS = 0;
constexpr int32_t DP_GET_SERVICE_FAILED = 98566147;
constexpr int32_t DP_CACHE_EXIST = 98566164;
constexpr int32_t DP_EXCEED_MAX_SIZE_FAIL = 98566201;
constexpr int32_t MAX_SUBSCRIBE_INFO_SIZE = 500;
const std::string SEPARATOR = "#";
} // namespace DistributedDeviceProfile
} // namespace OHOS
#endif // OHOS_DP_DISTRIBUTED_DEVICE_PROFILE_ERRORS_H
