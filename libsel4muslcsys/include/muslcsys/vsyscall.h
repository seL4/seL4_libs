/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include <sel4muslcsys/gen_config.h>
#include <stdarg.h>
#include <stdbool.h>
#include <bits/syscall.h>
#include <utils/attribute.h>

#define MUSLCSYS_WITH_VSYSCALL_PRIORITY (CONSTRUCTOR_MIN_PRIORITY + 10)

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
