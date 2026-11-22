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

#include "fuse_api_mock.h"

// Mock implementations of the libfuse C API; snake_case names are required.
// NOLINTBEGIN

FuseApiMock::FuseApiMock()
{
    FuseApiMock::mock_ = this;
    mockContext_.pid = getpid();
}

FuseApiMock::~FuseApiMock()
{
    FuseApiMock::mock_ = nullptr;
}

FuseApiMock *FuseApiMock::GetMock()
{
    return FuseApiMock::mock_;
}

struct fuse *fuse_new(struct fuse_args *args, const struct fuse_operations *op, size_t op_size, void *user_data)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return nullptr;
    }
    return mock->fuse_new(args, op, op_size, user_data);
}

int fuse_mount(struct fuse *f, const char *mountpoint)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return 0;
    }
    return mock->fuse_mount(f, mountpoint);
}

void fuse_unmount(struct fuse *f)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_unmount(f);
}

int fuse_loop_mt(struct fuse *f, struct fuse_loop_config *config)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return 0;
    }
    return mock->fuse_loop_mt(f, config);
}

struct fuse_loop_config *fuse_loop_cfg_create(void)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return nullptr;
    }
    return mock->fuse_loop_cfg_create();
}

void fuse_loop_cfg_set_max_threads(struct fuse_loop_config *config, unsigned int max_threads)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_loop_cfg_set_max_threads(config, max_threads);
}

void fuse_loop_cfg_destroy(struct fuse_loop_config *config)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_loop_cfg_destroy(config);
}

void fuse_destroy(struct fuse *f)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_destroy(f);
}

void fuse_exit(struct fuse *f)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_exit(f);
}

struct fuse_context *fuse_get_context(void)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return nullptr;
    }
    return mock->fuse_get_context();
}

int fuse_opt_add_arg(struct fuse_args *args, const char *arg)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return 0;
    }
    return mock->fuse_opt_add_arg(args, arg);
}

void fuse_opt_free_args(struct fuse_args *args)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_opt_free_args(args);
}

struct fuse_session *fuse_get_session(struct fuse *f)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return nullptr;
    }
    return mock->fuse_get_session(f);
}

int fuse_session_custom_io(struct fuse_session *se, void *io, size_t io_size, int fd)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return -1;
    }
    return mock->fuse_session_custom_io(se, io, io_size, fd);
}

int fuse_session_loop_mt(struct fuse_session *se, struct fuse_loop_config *config)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return -1;
    }
    return mock->fuse_session_loop_mt(se, config);
}

struct fuse_session *fuse_session_new(struct fuse_args *args, const struct fuse_lowlevel_ops *ops,
    size_t op_size, void *userdata)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return nullptr;
    }
    return mock->fuse_session_new(args, ops, op_size, userdata);
}

void fuse_session_destroy(struct fuse_session *se)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_session_destroy(se);
}

void fuse_session_exit(struct fuse_session *se)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_session_exit(se);
}

void fuse_loop_cfg_set_idle_threads(struct fuse_loop_config *config, unsigned int idle_threads)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_loop_cfg_set_idle_threads(config, idle_threads);
}

void fuse_loop_cfg_set_clone_fd(struct fuse_loop_config *config, int clone_fd)
{
    auto mock = FuseApiMock::GetMock();
    if (mock == nullptr) {
        return;
    }
    mock->fuse_loop_cfg_set_clone_fd(config, clone_fd);
}
// NOLINTEND
