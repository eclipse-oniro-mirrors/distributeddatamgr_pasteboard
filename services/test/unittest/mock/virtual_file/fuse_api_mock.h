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

#ifndef FUSE_API_MOCK_H
#define FUSE_API_MOCK_H

// This file mocks the libfuse C API; snake_case names match the real ABI.
// NOLINTBEGIN
#include <gmock/gmock.h>

#include <fuse.h>

class IFuseApi {
public:
    virtual ~IFuseApi() = default;
    virtual struct fuse *fuse_new(struct fuse_args *args, const struct fuse_operations *op,
        size_t op_size, void *user_data) = 0;
    virtual int fuse_mount(struct fuse *f, const char *mountpoint) = 0;
    virtual void fuse_unmount(struct fuse *f) = 0;
    virtual int fuse_loop_mt(struct fuse *f, struct fuse_loop_config *config) = 0;
    virtual struct fuse_loop_config *fuse_loop_cfg_create() = 0;
    virtual void fuse_loop_cfg_set_max_threads(struct fuse_loop_config *config, unsigned int max_threads) = 0;
    virtual void fuse_loop_cfg_destroy(struct fuse_loop_config *config) = 0;
    virtual void fuse_destroy(struct fuse *f) = 0;
    virtual void fuse_exit(struct fuse *f) = 0;
    virtual struct fuse_context *fuse_get_context() = 0;
    virtual int fuse_opt_add_arg(struct fuse_args *args, const char *arg) = 0;
    virtual void fuse_opt_free_args(struct fuse_args *args) = 0;
};

class FuseApiMock : public IFuseApi {
public:
    FuseApiMock();
    ~FuseApiMock() override;
    MOCK_METHOD(struct fuse *, fuse_new,
        (struct fuse_args * args, const struct fuse_operations *op, size_t op_size, void *user_data), (override));
    MOCK_METHOD(int, fuse_mount, (struct fuse * f, const char *mountpoint), (override));
    MOCK_METHOD(void, fuse_unmount, (struct fuse * f), (override));
    MOCK_METHOD(int, fuse_loop_mt, (struct fuse * f, struct fuse_loop_config *config), (override));
    MOCK_METHOD(struct fuse_loop_config *, fuse_loop_cfg_create, (), (override));
    MOCK_METHOD(void, fuse_loop_cfg_set_max_threads,
        (struct fuse_loop_config * config, unsigned int max_threads), (override));
    MOCK_METHOD(void, fuse_loop_cfg_destroy, (struct fuse_loop_config * config), (override));
    MOCK_METHOD(void, fuse_destroy, (struct fuse * f), (override));
    MOCK_METHOD(void, fuse_exit, (struct fuse * f), (override));
    MOCK_METHOD(struct fuse_context *, fuse_get_context, (), (override));
    MOCK_METHOD(int, fuse_opt_add_arg, (struct fuse_args * args, const char *arg), (override));
    MOCK_METHOD(void, fuse_opt_free_args, (struct fuse_args * args), (override));
    static FuseApiMock *GetMock();

private:
    static inline FuseApiMock *mock_ = nullptr;
    struct fuse_context mockContext_{};
};
// NOLINTEND
#endif // FUSE_API_MOCK_H
