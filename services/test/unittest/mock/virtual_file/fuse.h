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

#ifndef MOCK_FUSE_H
#define MOCK_FUSE_H

// This header mocks the libfuse C API; snake_case names are required for ABI compatibility.
// NOLINTBEGIN
#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct fuse_args {
    int argc;
    char **argv;
    int allocated;
};

enum fuse_readdir_flags {
    FUSE_READDIR_DEFAULTS = 0,
    FUSE_READDIR_PLUS = 1 << 0,
};

enum fuse_fill_dir_flags {
    FUSE_FILL_DIR_DEFAULTS = 0,
    FUSE_FILL_DIR_PLUS = 1 << 0,
};

typedef int (*fuse_fill_dir_t)(void *buf, const char *name, const struct stat *stbuf,
    off_t off, enum fuse_fill_dir_flags flags);

struct fuse_file_info {
    int flags;
    unsigned int writepage : 1;
    unsigned int direct_io : 1;
    unsigned int keep_cache : 1;
    unsigned int flush : 1;
    unsigned int nonseekable : 1;
    unsigned int flock_release : 1;
    unsigned int padding : 27;
    uint64_t fh;
    uint64_t lock_owner;
    uint32_t poll_events;
    int padding2;
};

struct fuse_operations {
    int (*getattr)(const char *, struct stat *, struct fuse_file_info *);
    int (*readlink)(const char *, char *, size_t);
    int (*getlink)(const char *, struct stat *, struct fuse_file_info *);
    int (*mknod)(const char *, mode_t, dev_t);
    int (*mkdir)(const char *, mode_t);
    int (*unlink)(const char *);
    int (*rmdir)(const char *);
    int (*symlink)(const char *, const char *);
    int (*rename)(const char *, const char *, unsigned int);
    int (*link)(const char *, const char *);
    int (*chmod)(const char *, mode_t, struct fuse_file_info *);
    int (*chown)(const char *, uid_t, gid_t, struct fuse_file_info *);
    int (*truncate)(const char *, off_t, struct fuse_file_info *);
    int (*open)(const char *, struct fuse_file_info *);
    int (*read)(const char *, char *, size_t, off_t, struct fuse_file_info *);
    int (*write)(const char *, const char *, size_t, off_t, struct fuse_file_info *);
    int (*statfs)(const char *, struct statvfs *);
    int (*flush)(const char *, struct fuse_file_info *);
    int (*release)(const char *, struct fuse_file_info *);
    int (*fsync)(const char *, int, struct fuse_file_info *);
    int (*opendir)(const char *, struct fuse_file_info *);
    int (*readdir)(const char *, void *, fuse_fill_dir_t, off_t,
        struct fuse_file_info *, enum fuse_readdir_flags);
    int (*releasedir)(const char *, struct fuse_file_info *);
    int (*fsyncdir)(const char *, int, struct fuse_file_info *);
    void *(*init)(struct fuse_conn_info *, struct fuse_config *);
    void (*destroy)(void *);
    int (*access)(const char *, int);
    int (*create)(const char *, mode_t, struct fuse_file_info *);
    int (*lock)(const char *, struct fuse_file_info *, int, struct flock *);
    int (*bmap)(const char *, size_t, uint64_t *);
    ssize_t (*copy_file_range)(const char *, struct fuse_file_info *, off_t,
        const char *, struct fuse_file_info *, off_t, size_t, int);
    int (*fallocate)(const char *, int, off_t, off_t, struct fuse_file_info *);
    ssize_t (*copy_file_range2)(const char *, struct fuse_file_info *, off_t,
        const char *, struct fuse_file_info *, off_t, size_t, int);
};

struct fuse_context {
    struct fuse *fuse;
    uint64_t fuse_pt;
    pid_t pid;
    pid_t owner;
    void *private_data;
    mode_t umask;
};

struct fuse;
struct fuse_loop_config;
struct fuse_conn_info;
struct fuse_config;

#ifdef __cplusplus
extern "C" {
#endif

struct fuse *fuse_new(struct fuse_args *args, const struct fuse_operations *op,
    size_t op_size, void *user_data);
int fuse_mount(struct fuse *f, const char *mountpoint);
void fuse_unmount(struct fuse *f);
int fuse_loop_mt(struct fuse *f, struct fuse_loop_config *config);
struct fuse_loop_config *fuse_loop_cfg_create(void);
void fuse_loop_cfg_set_max_threads(struct fuse_loop_config *config, unsigned int max_threads);
void fuse_loop_cfg_destroy(struct fuse_loop_config *config);
void fuse_destroy(struct fuse *f);
void fuse_exit(struct fuse *f);
struct fuse_context *fuse_get_context(void);
int fuse_opt_add_arg(struct fuse_args *args, const char *arg);
void fuse_opt_free_args(struct fuse_args *args);

#ifdef __cplusplus
}
#endif

// NOLINTEND
#endif // MOCK_FUSE_H
