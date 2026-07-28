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

// HOST-TEST SHIM for distributed_file_daemon_manager.h.
// pasteboard_progress_signal.cpp #includes this header but never uses any symbol
// from it (an unnecessary heavy include that drags in dfs_service on-device).
// The shim is intentionally empty so the .cpp compiles host-side without the
// real distributed-file-daemon dependency.

#ifndef PASTEBOARD_HOSTTEST_SHIM_DISTRIBUTED_FILE_DAEMON_MANAGER_H
#define PASTEBOARD_HOSTTEST_SHIM_DISTRIBUTED_FILE_DAEMON_MANAGER_H

// (empty on purpose — the unit under test references nothing here)

#endif // PASTEBOARD_HOSTTEST_SHIM_DISTRIBUTED_FILE_DAEMON_MANAGER_H
