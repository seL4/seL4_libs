/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4muslcsys/gen_config.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <bits/syscall.h>
#include <unistd.h>

#define FIRST_USER_FD (STDERR_FILENO + 1)

#define FILE_TYPE_FREE -1
#define FILE_TYPE_CPIO 0

typedef struct cpio_file_data {
    char const *start;
    uint32_t size;
    off_t current;
} cpio_file_data_t;

typedef struct muslcsys_fd {
    int filetype;
    void *data;
} muslcsys_fd_t;

void
add_free_fd(int fd);

int
get_free_fd(void);

int
valid_fd(int fd);

int
grow_fds(int how_much);

int
allocate_fd();

muslcsys_fd_t *
get_fd_struct(int fd);

/* install a cpio interface to use with open */
typedef void const *(*muslcsys_cpio_get_file_fn_t)(void const *cpio_symbol,
                                                   unsigned long len,
                                                   char const *name,
                                                   unsigned long *size);
void muslcsys_install_cpio_interface(void const *cpio_symbol,
                                     unsigned long cpio_len,
                                     muslcsys_cpio_get_file_fn_t fn);
