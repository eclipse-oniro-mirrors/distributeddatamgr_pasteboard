/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef COMPOSITE_KEY_H
#define COMPOSITE_KEY_H

#include <functional>

namespace OHOS::MiscServices {

constexpr int32_t DEFAULT_SUBSPACE_ID = -1;

struct CompositeKey {
    int32_t userId;
    int32_t subspaceId;

    CompositeKey() : userId(-1), subspaceId(DEFAULT_SUBSPACE_ID) {}
    CompositeKey(int32_t uid, int32_t sid) : userId(uid), subspaceId(sid) {}

    bool operator==(const CompositeKey& other) const {
        return userId == other.userId && subspaceId == other.subspaceId;
    }

    bool operator<(const CompositeKey& other) const {
        if (userId != other.userId) return userId < other.userId;
        return subspaceId < other.subspaceId;
    }
};

struct CompositeKeyHash {
    size_t operator()(const CompositeKey& key) const {
        return std::hash<int32_t>()(key.userId) ^
               (std::hash<int32_t>()(key.subspaceId) << 1);
    }
};

} // namespace OHOS::MiscServices

#endif // COMPOSITE_KEY_H