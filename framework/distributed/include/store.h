/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_STORE_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_STORE_H

#include <shared_mutex>
#include "single_kvstore.h"
#include "visibility.h"

namespace OHOS::MiscServices {
using namespace OHOS::DistributedKv;
class API_EXPORT Store final {
public:
    using KvStore = DistributedKv::SingleKvStore;
    using Status = DistributedKv::Status;
    using SubscribeType = DistributedKv::SubscribeType;
    using KvStoreObserver = DistributedKv::KvStoreObserver;

    explicit Store();
    ~Store();
    Status RemoveDeviceData(const std::string &device);
    Status PutBatch(const std::vector<Entry> &entries);
    Status DeleteBatch(const std::vector<Key> &deleteKeys);
    Status GetEntries(const DataQuery &query, std::vector<Entry> &entries);
    Status Put(const Key &key, std::string &value);
    Status Get(const Key &key, Value &value);
    Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
        bool isRetry, std::shared_ptr<KvStoreSyncCallback> callback);
    Status SubscribeKvStore(SubscribeType type, std::shared_ptr<KvStoreObserver> observer);
    Status UnSubscribeKvStore(SubscribeType type, std::shared_ptr<KvStoreObserver> observer);
    Status Initialize();
    Status GetResultSet(const Key &prefix, std::shared_ptr<KvStoreResultSet> &resultSet);
    Status RebuildKvStore();
    bool RemoveDirtyData(const Key &prefix, const Key &exceptPrefix);
    bool RemoveLocalData(const Key &prefix);
    Status OpenKvStore(int32_t userId);
    bool isSubscibe_ = false;

private:
    std::shared_ptr<KvStore> store_;
    std::recursive_mutex mutex;
    bool isNeedRemoveKvStore_ = true;
    bool RemoveData(const Key &prefix, bool isFilter, const Key &exceptPrefix = Key());
    static constexpr const char *APP_ID = "pasteboard_service";
    static constexpr const char *STORE_ID = "distributed_store";
    static constexpr const char *OLD_BASE_DIR = "/data/service/el1/public/database/pasteboard_service";
    static constexpr const char *BASE_DIR = "/data/service/el2/100/database/pasteboard_service";
    static constexpr const char *BASE_DIR_PREFIX =  "/data/service/el2/";
    static constexpr const char *BASE_DIR_SUFFIX =  "/database/pasteboard_service";
    static constexpr uint32_t RETRY_INTERVAL = 100;
    static constexpr uint32_t MAX_RETRY_TIMES = 2;
};
} // namespace OHOS::MiscServices
#endif // DISTRIBUTED_PASTEBOARD_FRAMEWORK_STORE_H
