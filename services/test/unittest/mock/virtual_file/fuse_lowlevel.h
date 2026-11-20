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

#ifndef MOCK_FUSE_LOWLEVEL_H
#define MOCK_FUSE_LOWLEVEL_H

// Mock for libfuse's fuse_lowlevel.h, providing the types and free functions
// that pasteboard_fuse_manager.cpp (latest commit) uses for session-based
// FUSE operation. Mirrors the mock fuse.h convention: snake_case C ABI.

#include <sys/uio.h>

struct fuse;
struct fuse_loop_config;

struct fuse_session;

struct fuse_custom_io {
    ssize_t (*read)(int fd, void *buf, size_t sz, void *userdata);
    ssize_t (*writev)(int fd, struct iovec *iov, int count, void *userdata);
};

#ifdef __cplusplus
extern "C" {
#endif

struct fuse_session *fuse_get_session(struct fuse *f);
int fuse_session_custom_io(struct fuse_session *se, void *io, size_t io_size, int fd);
int fuse_session_loop_mt(struct fuse_session *se, struct fuse_loop_config *config);

#ifdef __cplusplus
}
#endif

#endif // MOCK_FUSE_LOWLEVEL_H
