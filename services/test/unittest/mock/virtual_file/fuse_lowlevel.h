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
// that pasteboard_fuse_manager.cpp uses for pure lowlevel FUSE operation.
// NOLINTBEGIN

#include <cstdint>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>

// Forward declarations from fuse.h (mock version provides full definitions)
struct fuse_file_info;
struct fuse_loop_config;

// ===== Lowlevel types =====

struct fuse_req;
struct fuse_session;

struct fuse_ctx {
    uid_t uid;
    gid_t gid;
    pid_t pid;
};

struct fuse_conn_info {
    unsigned proto_major;
    unsigned proto_minor;
    unsigned max_write;
    unsigned max_read;
    unsigned max_readahead;
    unsigned capable;
    unsigned want;
    unsigned max_background;
    unsigned congestion_threshold;
    unsigned time_gran;
    unsigned reserved[22];
};

struct fuse_entry_param {
    uint64_t ino;
    uint64_t generation;
    struct stat attr;
    double attr_timeout;
    double entry_timeout;
};

// Lowlevel operations struct — mirrors libfuse's fuse_lowlevel_ops.
// Only the fields used by pasteboard_fuse_manager are defined.
struct fuse_lowlevel_ops {
    void (*init)(void *userdata, struct fuse_conn_info *conn);
    void (*destroy)(void *userdata);
    void (*lookup)(struct fuse_req *req, uint64_t parent, const char *name);
    void (*forget)(struct fuse_req *req, uint64_t ino, uint64_t nlookup);
    void (*getattr)(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    void (*setattr)(struct fuse_req *req, uint64_t ino, struct stat *attr,
        int to_set, struct fuse_file_info *fi);
    void (*mknod)(struct fuse_req *req, uint64_t parent, const char *name,
        mode_t mode, dev_t rdev);
    void (*mkdir)(struct fuse_req *req, uint64_t parent, const char *name, mode_t mode);
    void (*unlink)(struct fuse_req *req, uint64_t parent, const char *name);
    void (*rmdir)(struct fuse_req *req, uint64_t parent, const char *name);
    void (*open)(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    void (*read)(struct fuse_req *req, uint64_t ino, size_t size, off_t off,
        struct fuse_file_info *fi);
    void (*write)(struct fuse_req *req, uint64_t ino, const char *buf,
        size_t size, off_t off, struct fuse_file_info *fi);
    void (*release)(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    void (*opendir)(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
    void (*readdir)(struct fuse_req *req, uint64_t ino, size_t size, off_t off,
        struct fuse_file_info *fi);
    void (*releasedir)(struct fuse_req *req, uint64_t ino, struct fuse_file_info *fi);
};

struct fuse_custom_io {
    ssize_t (*read)(int fd, void *buf, size_t sz, void *userdata);
    ssize_t (*writev)(int fd, struct iovec *iov, int count, void *userdata);
};

// ===== Lowlevel free functions =====

#ifdef __cplusplus
extern "C" {
#endif

// Session lifecycle
struct fuse_session *fuse_session_new(struct fuse_args *args,
    const struct fuse_lowlevel_ops *ops, size_t op_size, void *userdata);
void fuse_session_destroy(struct fuse_session *se);
void fuse_session_exit(struct fuse_session *se);
int fuse_session_custom_io(struct fuse_session *se, void *io, size_t io_size, int fd);
int fuse_session_loop_mt(struct fuse_session *se, struct fuse_loop_config *config);

// Request context
void *fuse_req_userdata(struct fuse_req *req);
const struct fuse_ctx *fuse_req_ctx(struct fuse_req *req);

// Reply functions
void fuse_reply_entry(struct fuse_req *req, const struct fuse_entry_param *e);
void fuse_reply_err(struct fuse_req *req, int err);
void fuse_reply_attr(struct fuse_req *req, const struct stat *attr, double timeout);
void fuse_reply_open(struct fuse_req *req, const struct fuse_file_info *fi);
void fuse_reply_buf(struct fuse_req *req, const char *buf, size_t size);
void fuse_reply_none(struct fuse_req *req);

// Directory entry helper
size_t fuse_add_direntry(struct fuse_req *req, char *buf, size_t bufsize,
    const char *name, const struct stat *stbuf, off_t off);

// Loop config
void fuse_loop_cfg_set_idle_threads(struct fuse_loop_config *config, unsigned int idle_threads);
void fuse_loop_cfg_set_clone_fd(struct fuse_loop_config *config, int clone_fd);

#ifdef __cplusplus
}
#endif

// NOLINTEND
#endif // MOCK_FUSE_LOWLEVEL_H
