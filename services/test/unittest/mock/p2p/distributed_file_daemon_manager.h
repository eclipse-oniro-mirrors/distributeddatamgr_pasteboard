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

#ifndef DISTRIBUTED_FILE_DAEMON_MANAGER_H
#define DISTRIBUTED_FILE_DAEMON_MANAGER_H

#include <string>

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class DistributedFileDaemonManager {
public:
    static DistributedFileDaemonManager &GetInstance();
    virtual int32_t ConnectDfs(const std::string &networkId) = 0;
    virtual int32_t DisconnectDfs(const std::string &networkId) = 0;
    virtual ~DistributedFileDaemonManager() = default;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // DISTRIBUTED_FILE_DAEMON_MANAGER_H
