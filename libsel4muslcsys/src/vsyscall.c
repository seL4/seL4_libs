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
#include <stdio.h>
#include <sel4/sel4.h>
#include <utils/util.h>
#include <bits/syscall.h>
#include <bits/errno.h>
#include "syscalls.h"

/* construct a lookup table of all the syscalls */
static long (*syscall_table[])(va_list) = {
#ifdef __NR_set_thread_area
    [__NR_set_thread_area] = sys_set_thread_area,
#endif
    [__NR_set_tid_address] = sys_set_tid_address,
    [__NR_writev] = sys_writev,
    [__NR_sched_yield] = sys_sched_yield,
    [__NR_exit] = sys_exit,
    [__NR_rt_sigprocmask] = sys_rt_sigprocmask,
    [__NR_gettid] = sys_gettid,
    [__NR_getpid] = sys_getpid,
    [__NR_tgkill] = sys_tgkill,
    [__NR_exit_group] = sys_exit_group,
    [__NR_open] = sys_open,
    [__NR_close] = sys_close,
    [__NR_readv] = sys_readv,
    [__NR_read] = sys_read,
    [__NR_ioctl] = sys_ioctl,
    [__NR_prlimit64] = sys_prlimit64,
    [__NR_lseek] = sys_lseek,
    [__NR__llseek] = sys__llseek,
    [__NR_access] = sys_access,
    [__NR_brk] = sys_brk,
    [__NR_mmap2] = sys_mmap2,
    [__NR_mremap] = sys_mremap,
};

#ifdef CONFIG_DEBUG_BUILD
static void debug_error(int sysnum) {
    char buf[100];
    int i;
    sprintf(buf, "libsel4muslcsys: Error attempting syscall %d\n", sysnum);
    for (i = 0; buf[i]; i++) {
        seL4_DebugPutChar(buf[i]);
    }
}
#else
static void debug_error(int sysnum) {
}
#endif

long sel4_vsyscall(long sysnum, ...) {
    va_list al;
    va_start(al, sysnum);
    if (sysnum < 0 || sysnum >= ARRAY_SIZE(syscall_table)) {
        debug_error(sysnum);
        return -ENOSYS;
    }
    /* Check a syscall is implemented there */
    if (!syscall_table[sysnum]) {
        debug_error(sysnum);
        return -ENOSYS;
    }
    /* Call it */
    long ret = syscall_table[sysnum](al);
    va_end(al);
    return ret;
}

/* Put a pointer to sel4_vsyscall in a special section so anyone loading us
 * knows how to configure our syscall table */
uintptr_t __attribute__((externally_visible)) __attribute__((section("__vsyscall"))) __vsyscall_ptr = (uintptr_t) sel4_vsyscall;
