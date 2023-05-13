/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4muslcsys/gen_config.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <utils/util.h>
#include <bits/syscall.h>
#include <bits/errno.h>
#include <sys/uio.h>
#include <muslcsys/vsyscall.h>
#include "syscalls.h"
#ifdef CONFIG_LIB_SEL4_MUSLC_SYS_CPIO_FS
#include <cpio/cpio.h>
#endif

#define MUSLC_NUM_SYSCALLS (MUSLC_HIGHEST_SYSCALL + 1)

/* For each TLS syscalls we record up to one occurance here that happens
 * on startup before the syscall table is initialized. In the case of more
 * than one occurance we will panic */
static bool boot_set_tid_address_happened;
static int *boot_set_tid_address_arg;

#if defined(__NR_set_thread_area) || defined(__ARM_NR_set_tls)
static bool boot_set_thread_area_happened;
static void *boot_set_thread_area_arg;

static long boot_set_thread_area(va_list ap)
{
    void *tp = va_arg(ap, void *);
    if (boot_set_thread_area_happened) {
        ZF_LOGE("Boot version of set_thread_area somehow got called twice");
        return -ESRCH;
    }

    /* For platforms (such as aarch64) that have an architecturally defined thread pointer
     * that is user accessible set_thread_area will happen internally in the C library.
     * Unfortunately in x86-64 the method to *set* the thread pointer is not defined completely
     * by the architecture, but the kernel *may* be using a strategy that allows us to
     * write to it directly, but the C library will not know this. In this case we set the
     * thread pointer here and then return, otherwise we have to resort to using the TCB invocation */
#if defined(CONFIG_FSGSBASE_INST)
    asm volatile("wrfsbase %0" :: "r"(tp));
#else
    char *tcb_string = getenv("boot_tcb_cptr");
    if (tcb_string) {
        seL4_CPtr tcb;
        if (sscanf(tcb_string, "%p", (void **)&tcb) == 1) {
            seL4_TCB_SetTLSBase(tcb, (seL4_Word)tp);
        }
    }
#endif
    boot_set_thread_area_happened = true;
    boot_set_thread_area_arg = tp;
    return 0;
}

bool muslcsys_get_boot_set_thread_area(void **arg)
{
    *arg = boot_set_thread_area_arg;
    return boot_set_thread_area_happened;
}
#endif

static long boot_set_tid_address(va_list ap)
{
    int *tid = va_arg(ap, int *);
    if (boot_set_tid_address_happened) {
        ZF_LOGE("Boot version of set_tid_address somehow got called twice");
        return 1;
    }
    boot_set_tid_address_happened = true;
    boot_set_tid_address_arg = tid;
    return 1;
}

bool muslcsys_get_boot_set_tid_address(int **arg)
{
    *arg = boot_set_tid_address_arg;
    return boot_set_tid_address_happened;
}

/* Basic sys_writev for use during booting that will only use seL4_DebugPutChar */
long boot_sys_writev(va_list ap)
{
    int UNUSED fildes = va_arg(ap, int);
    struct iovec *iov = va_arg(ap, struct iovec *);
    int iovcnt = va_arg(ap, int);

    ssize_t ret = 0;

    for (int i = 0; i < iovcnt; i++) {
        char *UNUSED base = (char *)iov[i].iov_base;
        for (int j = 0; j < iov[i].iov_len; j++) {
#ifdef CONFIG_PRINTING
            seL4_DebugPutChar(base[j]);
#endif
            ret++;
        }
    }

    return ret;
}

static muslcsys_syscall_t syscall_table[MUSLC_NUM_SYSCALLS] = {
#ifdef __NR_set_thread_area
    [__NR_set_thread_area] = boot_set_thread_area,
#endif
    [__NR_set_tid_address] = boot_set_tid_address,
    [__NR_writev] = boot_sys_writev,
    /* We don't need a boot_sys_write variant as this implementation wraps
     * whatever __NR_writev is set to. */
    [__NR_write] = sys_write,
    [__NR_sched_yield] = sys_sched_yield,
    [__NR_exit] = sys_exit,
    [__NR_rt_sigprocmask] = sys_rt_sigprocmask,
    [__NR_gettid] = sys_gettid,
    [__NR_getpid] = sys_getpid,
    [__NR_tgkill] = sys_tgkill,
    [__NR_tkill] = sys_tkill,
    [__NR_exit_group] = sys_exit_group,
#ifdef __NR_open
    [__NR_open] = sys_open,
#endif
#ifdef __NR_openat
    [__NR_openat] = sys_openat,
#endif
    [__NR_close] = sys_close,
    [__NR_readv] = sys_readv,
    [__NR_read] = sys_read,
    [__NR_ioctl] = sys_ioctl,
    [__NR_prlimit64] = sys_prlimit64,
    [__NR_lseek] = sys_lseek,
#ifdef __NR__llseek
    [__NR__llseek] = sys__llseek,
#endif
#ifdef __NR_access
    [__NR_access] = sys_access,
#endif
    [__NR_brk] = sys_brk,
#ifdef __NR_mmap2
    [__NR_mmap2] = sys_mmap2,
#endif
#ifdef __NR_mmap
    [__NR_mmap] = sys_mmap,
#endif
#ifdef __NR_munmap
    [__NR_munmap] = sys_munmap,
#endif
    [__NR_mremap] = sys_mremap,
    [__NR_madvise] = sys_madvise,
};

/* Additional syscall lookup table for handling spare syscalls or syscalls that have large
 * numbers. Currently The number of these is very small and so it's an unordered list that
 * must be searched to find a syscall */
typedef struct sparse_syscall {
    int sysnum;
    muslcsys_syscall_t syscall;
} sparse_syscall_t;

static sparse_syscall_t sparse_syscall_table[] = {
#ifdef __ARM_NR_breakpoint
    {__ARM_NR_breakpoint, NULL},
#endif
#ifdef __ARM_NR_cacheflush
    {__ARM_NR_cacheflush, NULL},
#endif
#ifdef __ARM_NR_usr26
    {__ARM_NR_usr26, NULL},
#endif
#ifdef __ARM_NR_usr32
    {__ARM_NR_usr32, NULL},
#endif
#ifdef __ARM_NR_set_tls
    {__ARM_NR_set_tls, boot_set_thread_area},
#endif
};

static int find_sparse_syscall(int syscall)
{
    for (int i = 0; i < ARRAY_SIZE(sparse_syscall_table); i++) {
        if (sparse_syscall_table[i].sysnum == syscall) {
            return i;
        }
    }
    return -1;
}

muslcsys_syscall_t muslcsys_install_syscall(int syscall, muslcsys_syscall_t new_syscall)
{
    muslcsys_syscall_t ret;
    if (syscall >= ARRAY_SIZE(syscall_table)) {
        int index = find_sparse_syscall(syscall);
        if (index < 0) {
            ZF_LOGF("Syscall %d exceeds syscall table size of %zu and not found in sparse table", syscall,
                    ARRAY_SIZE(syscall_table));
        }
        ret = sparse_syscall_table[index].syscall;
        sparse_syscall_table[index].syscall = ret;
    } else {
        ret = syscall_table[syscall];
        syscall_table[syscall] = new_syscall;
    }
    return ret;
}

/* Switch the thread syscalls from their boot variant to their regular
 * default implementation. We do this at the lowest priority so that
 * it can be overriden. We are able to have this constructor
 * in this file since we know it will get looked at by the linker due
 * to __vsyscall_ptr being here */
static void CONSTRUCTOR(CONSTRUCTOR_MIN_PRIORITY) init_syscall_table(void)
{
    muslcsys_syscall_t ret UNUSED;
    ret = muslcsys_install_syscall(__NR_set_tid_address, sys_set_tid_address);
    assert(ret == boot_set_tid_address);
#ifdef __NR_set_thread_area
    ret = muslcsys_install_syscall(__NR_set_thread_area, sys_set_thread_area);
    assert(ret == boot_set_thread_area);
#endif
#ifdef __ARM_NR_set_tls
    ret = muslcsys_install_syscall(__ARM_NR_set_tls, NULL);
    assert(ret == boot_set_thread_area);
#endif
    ret = muslcsys_install_syscall(__NR_writev, sys_writev);
    assert(ret == boot_sys_writev);
}

/* If we have a default CPIO file interface defined in the config then install it here */
#ifdef CONFIG_LIB_SEL4_MUSLC_SYS_CPIO_FS
extern char _cpio_archive[];
extern char _cpio_archive_end[];
static void CONSTRUCTOR(CONSTRUCTOR_MIN_PRIORITY) install_default_cpio(void)
{
    unsigned long cpio_len = _cpio_archive_end - _cpio_archive;
    muslcsys_install_cpio_interface(_cpio_archive, cpio_len, cpio_get_file);
}
#endif

#ifdef CONFIG_PRINTING
static void debug_error(int sysnum)
{
    char buf[100];
    int i;
    sprintf(buf, "libsel4muslcsys: Error attempting syscall %d\n", sysnum);
    for (i = 0; buf[i]; i++) {
        seL4_DebugPutChar(buf[i]);
    }
}
#else
static void debug_error(int sysnum)
{
}
#endif

long sel4_vsyscall(long sysnum, ...)
{
    va_list al;
    va_start(al, sysnum);
    muslcsys_syscall_t syscall;
    if (sysnum < 0 || sysnum >= ARRAY_SIZE(syscall_table)) {
        int index = find_sparse_syscall(sysnum);
        if (index < 0) {
            debug_error(sysnum);
            return -ENOSYS;
        }
        syscall = sparse_syscall_table[index].syscall;
    } else {
        syscall = syscall_table[sysnum];
    }
    /* Check a syscall is implemented there */
    if (!syscall) {
        debug_error(sysnum);
        return -ENOSYS;
    }
    /* Call it */
    long ret = syscall(al);
    va_end(al);
    return ret;
}

extern void *__sysinfo;

/* Set the virtual syscall handler so that a portion of muslc will
 * function.
 *
 * This is required for apps using a dynamic heap, which need to make
 * use of malloc in order to provide an implementation of brk and mmap
 * that are used during the initialisation of muslc.
 */
static void CONSTRUCTOR(MUSLCSYS_WITH_VSYSCALL_PRIORITY - 1) init_vsyscall(void)
{
    __sysinfo = sel4_vsyscall;
}

/* Put a pointer to sel4_vsyscall in a special section so anyone loading us
 * knows how to configure our syscall table */
uintptr_t VISIBLE SECTION("__vsyscall") __vsyscall_ptr = (uintptr_t) sel4_vsyscall;

/* muslc provides a function used to initialise the C standard library
 * environment. */
extern void __init_libc(char const *const *envp, char const *pn);

/* This is needed to force GCC to re-read the TLS base address on some
 * platforms when setting the IPC buffer address after it has changed.
 *
 * At higher optimisation levels on aarch64, GCC will read the location
 * for `__sel4_ipc_buffer` only once in the same function, even across
 * function calls, and thus will not update any newly created TLS region
 * with the IPC buffer address.
 */
static void NO_INLINE update_ipc_buffer(seL4_IPCBuffer *tmp)
{
    __sel4_ipc_buffer = tmp;
}

/* Initialise muslc environment */
void CONSTRUCTOR(CONFIG_LIB_SEL4_MUSLC_SYS_CONSTRUCTOR_PRIORITY) muslcsys_init_muslc(void)
{
    seL4_IPCBuffer *tmp = __sel4_ipc_buffer;
    __init_libc(sel4runtime_envp(), sel4runtime_argv()[0]);
    update_ipc_buffer(tmp);
}
