/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <autoconf.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <bits/syscall.h>
#include <unistd.h>

#define FIRST_USER_FD (STDERR_FILENO + 1)

#define FILE_TYPE_FREE -1
#define FILE_TYPE_CPIO 0

typedef struct cpio_file_data {
    char *start;
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
typedef void *(*muslcsys_cpio_get_file_fn_t)(void *cpio_symbol, const char *name, unsigned long *size);
void muslcsys_install_cpio_interface(void *cpio_symbol, muslcsys_cpio_get_file_fn_t fn);
