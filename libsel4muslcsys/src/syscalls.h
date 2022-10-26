/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdarg.h>

/* prototype all the syscalls we implement */
long sys_set_thread_area(va_list ap);
long sys_set_tid_address(va_list ap);
long sys_sched_yield(va_list ap);
long sys_exit(va_list ap);
long sys_rt_sigprocmask(va_list ap);
long sys_gettid(va_list ap);
long sys_getpid(va_list ap);
long sys_tgkill(va_list ap);
long sys_tkill(va_list ap);
long sys_exit_group(va_list ap);
long sys_open(va_list ap);
long sys_openat(va_list ap);
long sys_close(va_list ap);
long sys_readv(va_list ap);
long sys_read(va_list ap);
long sys_ioctl(va_list ap);
long sys_prlimit64(va_list ap);
long sys_lseek(va_list ap);
long sys__llseek(va_list ap);
long sys_access(va_list ap);
long sys_brk(va_list ap);
long sys_mmap2(va_list ap);
long sys_mmap(va_list ap);
long sys_munmap(va_list ap);
long sys_mremap(va_list ap);
long sys_write(va_list ap);
long sys_writev(va_list ap);
long sys_madvise(va_list ap);
