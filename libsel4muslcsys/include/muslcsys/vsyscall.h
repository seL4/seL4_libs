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
#include <stdbool.h>
#include <bits/syscall.h>
#include <utils/attribute.h>

typedef long (*muslcsys_syscall_t)(va_list);

/* Installs a new handler for the given syscall. Any previous handler is returned
 * and can be used to provide layers of syscall handling */
muslcsys_syscall_t muslcsys_install_syscall(int syscall, muslcsys_syscall_t new_syscall);

/* Some syscalls happen in the C library prior the init constructors being called,
 * and hence prior to syscall handlers being able to be installed. For such cases
 * we simply record such occurances and provide functions here for retrieving the
 * arguments allow a process to do anything necessary once they get to `main`.
 * These functions return true/false if the syscall was called on boot, and take
 * pointers for returning the arguments. Pointers must be non NULL
 */
bool muslcsys_get_boot_set_tid_address(int **arg) NONNULL(1);
#if defined(__NR_set_thread_area) || defined(__ARM_NR_set_tls)
bool muslcsys_get_boot_set_thread_area(void **arg) NONNULL(1);
#endif
