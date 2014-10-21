/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sel4/sel4.h>

#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/uio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <bits/syscall.h>

#include <cpio/cpio.h>

#include <sel4utils/util.h>

#include "arch_stdio.h"

#define STDOUT_FD 1
#define STDERR_FD 2
#define FIRST_USER_FD 3

#define FILE_TYPE_CPIO 0

#define FD_TABLE_SIZE(x) (sizeof(muslcsys_fd_t) * (x))
/* this implementation does not allow users to close STDOUT or STDERR, so they can't be freed */
#define FREE_FD_TABLE_SIZE(x) (sizeof(int) * ((x) - FIRST_USER_FD))

/* We need to wrap this in the config to prevent linker errors */
#ifdef CONFIG_LIB_SEL4_MUSLC_SYS_CPIO_FS
extern char _cpio_archive[];
#endif

typedef struct cpio_file_data {
    char *start;
    uint32_t size;
    off_t current;
} cpio_file_data_t;

typedef struct muslcsys_fd {
    int filetype;
    void *data;
} muslcsys_fd_t;

/* file table, indexed by file descriptor */
static muslcsys_fd_t *fd_table = NULL;
/* stack of free file descriptors */
static int *free_fd_table = NULL;
/* head of the stack */
static int free_fd_table_index;
/* total number of fds */
static int num_fds = 256;


static void 
add_free_fd(int fd)
{
    free_fd_table_index++;
    assert(free_fd_table_index < num_fds);
    free_fd_table[free_fd_table_index] = fd;
}

static int
get_free_fd(void)
{
    if (free_fd_table_index == -1) {
        return -EMFILE;
    }

    free_fd_table_index--;
    return free_fd_table[free_fd_table_index + 1];
}

static int 
valid_fd(int fd) 
{
    return fd < num_fds && fd >= FIRST_USER_FD;
}


static int 
allocate_file_table(void)
{
    fd_table = malloc(FD_TABLE_SIZE(num_fds));
    if (fd_table == NULL) {
        return -ENOMEM;
    }

    free_fd_table = malloc(FREE_FD_TABLE_SIZE(num_fds));
    if (free_fd_table == NULL) {
        free(fd_table);
        return -ENOMEM;
    }

    free_fd_table_index = -1;
    
    /* populate free list */
    for (int i = FIRST_USER_FD; i < num_fds; i++) {
        add_free_fd(i);
    }

    return 0;
}

static int
grow_fds(int how_much)
{
    int new_num_fds = num_fds + how_much;

    /* Ensure file table exists */
    if (fd_table == NULL) {
        if (allocate_file_table() == -ENOMEM) {
            return -ENOMEM;
        }
    }

    /* allocate new arrays */
    muslcsys_fd_t *new_fd_table = malloc(FD_TABLE_SIZE(new_num_fds));
    if (!new_fd_table) {
        LOG_ERROR("Failed to allocate new_vfds\n");
        return -ENOMEM;
    }

    int *new_free_fd_table = malloc(FREE_FD_TABLE_SIZE(new_num_fds));
    if (!new_free_fd_table) {
        free(new_fd_table);
        LOG_ERROR("Failed to allocate free fd table\n");
        return -ENOMEM;
    }

    /* copy old contents */
    memcpy(new_free_fd_table, free_fd_table, FREE_FD_TABLE_SIZE(num_fds));
    memcpy(new_fd_table, fd_table, FD_TABLE_SIZE(num_fds));

    /* free old tables */
    free(fd_table);
    free(free_fd_table);

    /* update global pointers */
    fd_table = new_fd_table;
    free_fd_table = new_free_fd_table;

    /* Update the size */
    num_fds = new_num_fds;

    /* add all of the new available fds to the free list */
    for (int i = num_fds; i < new_num_fds; i++) {
        add_free_fd(i);
    }
    return 0;
}

static int 
allocate_fd()
{
    if (fd_table == NULL) {
        if (allocate_file_table() == -ENOMEM) {
            return -ENOMEM;
        }
    }

    return get_free_fd();
}

static muslcsys_fd_t *
get_fd_struct(int fd)
{
    assert(fd < num_fds && fd >= FIRST_USER_FD);
    return &fd_table[fd - FIRST_USER_FD];
}

static size_t
sys_platform_write(void *data, size_t count)
{
    size_t i;
    char *realdata = data;
    for (i = 0; i < count; i++) {
        __arch_putchar(realdata[i]);
    }
    return count;
}

long
sys_open(va_list ap)
{
    const char *pathname __attribute__((unused)) = va_arg(ap, const char *);
    int flags = va_arg(ap, int);
    mode_t mode = va_arg(ap, mode_t);
    (void) mode;
    /* mask out flags we can support */
    flags &= ~O_LARGEFILE;
    /* only support reading in basic modes */
    if (flags != O_RDONLY) {
        LOG_ERROR("Open only supports O_RDONLY, not 0x%x on %s\n", flags, pathname);
        assert(flags == O_RDONLY);
        return -EINVAL;
    }
    /* as we do not support create, ignore the mode */
    long unsigned int size;
    /* wrapped in a config because the _cpio_archive definition is wrapped in a config */
    char *file = NULL;
#ifdef CONFIG_LIB_SEL4_MUSLC_SYS_CPIO_FS
    file = cpio_get_file(_cpio_archive, pathname, &size);
    if (!file && strncmp(pathname, "./", 2) == 0) {
        file = cpio_get_file(_cpio_archive, pathname + 2, &size);
    }
#else
    LOG_ERROR("Warning: attempted to use fopen with no file system (CONFIG_LIB_SEL4_MUSLC_SYS_CPIO_FS not set)\n");
    return -ENOENT;
#endif
    if (!file) {
        LOG_ERROR("Failed to open file %s\n", pathname);
        return -ENOENT;
    }
    int fd = allocate_fd();
    if (fd == -EMFILE) {
        LOG_ERROR("Out of fds!\n");
        return -EMFILE;
    }

    muslcsys_fd_t *fds = get_fd_struct(fd);
    fds->filetype = FILE_TYPE_CPIO;
    fds->data = malloc(sizeof(cpio_file_data_t));
    if (!fds->data) {
        LOG_ERROR("Malloc failed\n");
        add_free_fd(fd);
        return -ENOMEM;
    }
    cpio_file_data_t *fd_data = (cpio_file_data_t*)fds->data;
    fd_data->start = file;
    fd_data->size = size;
    fd_data->current = 0;
    return fd;
}

long
sys_close(va_list ap)
{
    int fd = va_arg(ap, int);
    if (fd < FIRST_USER_FD) {
        assert(!"not implemented");
        return -EBADF;
    }

    if (!valid_fd(fd)) {
        return -EBADF;
    }

    muslcsys_fd_t *fds = get_fd_struct(fd);
    
    if (fds->filetype == FILE_TYPE_CPIO) {
        free(fds->data);
    } else {
        assert(!"not implemented");
    }
    add_free_fd(fd);
    return 0;
}

/* Writev syscall implementation for muslc. Only implemented for stdin and stdout. */
long
sys_writev(va_list ap)
{
    int fildes = va_arg(ap, int);
    struct iovec *iov = va_arg(ap, struct iovec *);
    int iovcnt = va_arg(ap, int);

    long long sum = 0;
    ssize_t ret = 0;

    /* The iovcnt argument is valid if greater than 0 and less than or equal to IOV_MAX. */
    if (iovcnt <= 0 || iovcnt > IOV_MAX) {
        return -EINVAL;
    }

    /* The sum of iov_len is valid if less than or equal to SSIZE_MAX i.e. cannot overflow
       a ssize_t. */
    for (int i = 0; i < iovcnt; i++) {
        sum += (long long)iov[i].iov_len;
        if (sum > SSIZE_MAX) {
            return -EINVAL;
        }
    }

    /* If all the iov_len members in the array are 0, return 0. */
    if (!sum) {
        return 0;
    }

    /* Write the buffer to console if the fd is for stdout or stderr. */
    if (fildes == STDOUT_FD || fildes == STDERR_FD) {
        for (int i = 0; i < iovcnt; i++) {
            ret += sys_platform_write(iov[i].iov_base, iov[i].iov_len);
        }
    } else {
        assert(!"Not implemented");
        return -EBADF;
    }

    return ret;
}

long sys_readv(va_list ap)
{
    int fd = va_arg(ap, int);
    struct iovec *iov = va_arg(ap, struct iovec*);
    int iovcnt = va_arg(ap, int);
    int i;
    long read;
    if (fd < FIRST_USER_FD) {
        assert(!"not implemented");
        return -EBADF;
    }

    if (!valid_fd(fd)) {
        return -EBADF;
    }

    /* files can only be opened for reading so no need to check any permissions.
     * just get straight into it
     */
    muslcsys_fd_t *muslc_fd = get_fd_struct(fd);
    if (muslc_fd->filetype != FILE_TYPE_CPIO) {
        assert(!"not implemented");
        return -EINVAL;
    }
    cpio_file_data_t *cpio_fd = muslc_fd->data;
    read = 0;
    for (i = 0; i < iovcnt && cpio_fd->current < cpio_fd->size; i++) {
        long max = cpio_fd->size - cpio_fd->current;
        long len = max < iov[i].iov_len ? max : iov[i].iov_len;
        memcpy(iov[i].iov_base, cpio_fd->start + cpio_fd->current, len);
        cpio_fd->current += len;
        read += len;
    }
    return read;
}

long sys_read(va_list ap)
{
    int fd = va_arg(ap, int);
    void *buf = va_arg(ap, void*);
    size_t count = va_arg(ap, size_t);
    /* construct an iovec and call readv */
    struct iovec iov = {.iov_base = buf, .iov_len = count };
    return readv(fd, &iov, 1);
}

long
sys_ioctl(va_list ap)
{
    int fd = va_arg(ap, int);
    int request = va_arg(ap, int);
    (void)request;
    /* muslc does some ioctls to stdout, so just allow these to silently
       go through */
    if (fd == STDOUT_FD) {
        return 0;
    }
    assert(!"not implemented");
    return 0;
}


long 
sys_prlimit64(va_list ap)
{
    pid_t pid = va_arg(ap, pid_t);
    int resource = va_arg(ap, int);
    const struct rlimit *new_limit = va_arg(ap, const struct rlimit *);
    struct rlimit *old_limit = va_arg(ap, struct rlimit *);
    int result = 0;

    /* we have no concept of pids, so ignore this for now */
    (void) pid;

    if (resource == RLIMIT_NOFILE) {
        if (old_limit) {
            old_limit->rlim_cur = num_fds;
            /* pick some arbitrarily big number for max. In practice we are only constrained
             * by how large an array we can malloc */
            old_limit->rlim_max = 65536;
        }

        if (new_limit) {
            if (new_limit->rlim_cur < num_fds) {
                printf("Trying to reduce open file limit. Operation not supported, ignoring\n");
            } else {
                result = grow_fds(new_limit->rlim_cur - num_fds);
            }
        }
    } else {
        assert(!"not implemented");
    }

    return result;
}

static int
safe_addition(int a, int b) {
    return !(a >= 0 && b > INT_MAX - a) &&
           !(a < 0 && b < INT_MAX - a);
}

long
sys_lseek(va_list ap)
{
    int fd = va_arg(ap, int);
    off_t offset = va_arg(ap, off_t);
    int whence = va_arg(ap, int);

    if (!valid_fd(fd)) {
        return -EBADF;
    }

    muslcsys_fd_t *muslc_fd = get_fd_struct(fd);
    if (muslc_fd == NULL) {
        return -EBADF;
    }

    if (muslc_fd->filetype != FILE_TYPE_CPIO) {
        assert(!"Not implemented\n");
        return -EBADF;
    }

    /* if its a valid fd it must be a cpio file, we
     * don't support anything else */
    cpio_file_data_t *cpio_fd = muslc_fd->data;

    int new_offset = 0;
    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            if (!safe_addition(cpio_fd->current, offset)) {
                return -EOVERFLOW;
            }
            new_offset = cpio_fd->current + offset;
            break;
        case SEEK_END:
            if (offset > 0) {
                /* can't seek beyond the end of the cpio file */
                return -EINVAL;
            }
            new_offset = cpio_fd->size + offset;
            break;
        default:
            return -EINVAL;
    }

    if (new_offset < 0) {
        return -EINVAL;
    /* can't seek past the end of the cpio file */
    } else if (new_offset > cpio_fd->size) {
        return -EINVAL;
    }

    cpio_fd->current = new_offset;

    return new_offset;
}

long syscall(long n, ...);

long sys__llseek(va_list ap)
{
    int fd = va_arg(ap, int);
    uint32_t offset_high = va_arg(ap, uint32_t);
    uint32_t offset_low = va_arg(ap, uint32_t);
    off_t *result = va_arg(ap, off_t*);
    int whence = va_arg(ap, int);
    /* need to directly call syscall to prevent circular call to this function. the llseek function
     * is used when off_t is a 64bit type (see the lseek definition in muslc), Underneath the
     * hood all syscall arguments get cast to a 32bit long before the actual syscall function
     * gets called. This makes calling the old lseek syscall awkward as it will attempt to pull
     * a 64bit off_t off its syscall args, but we had all our arguments forced down to 32bits
     * before they got passed over. Therefore we can actually just pass the high and low
     * and everything will work. Assumptions on endianess */
    long ret = syscall(SYS_lseek, fd, (uint32_t)offset_low, (uint32_t)offset_high, whence);
    if (ret == -1) {
        /* propogate error up. see __syscall_ret to understand */
        return -errno;
    }
    if (result) {
        *result = (off_t)ret;
    }
    return 0;
}

long sys_access(va_list ap) {
    const char *pathname = va_arg(ap, const char *);
    int mode = va_arg(ap, int);
    /* just try and open. currently we only support reading with the CPIO file system */
    if (mode == F_OK || mode == R_OK) {
        int fd = open(pathname, O_RDONLY, 0);
        if(fd < 0) {
            return -EACCES;
        }
        close(fd);
        return 0;
    }
    LOG_ERROR("Must pass F_OK or R_OK to %s\n", __FUNCTION__);
    return -EACCES;
}
