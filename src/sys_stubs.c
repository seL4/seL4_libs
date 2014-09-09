/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/* Dummy implementations of any syscalls we do not implement */

#ifdef ARCH_IA32

long sys_restart_syscall(va_list ap)
{
    assert(!"sys_restart_syscall not implemented");
    return 0;
}
/*long sys_exit(va_list ap) {
	assert(!"sys_exit not implemented");
	return 0;
}*/
long sys_fork(va_list ap)
{
    assert(!"sys_fork not implemented");
    return 0;
}
/*long sys_read(va_list ap)
{
    assert(!"sys_read not implemented");
    return 0;
}*/
long sys_write(va_list ap)
{
    assert(!"sys_write not implemented");
    return 0;
}
long sys_waitpid(va_list ap)
{
    assert(!"sys_waitpid not implemented");
    return 0;
}
long sys_creat(va_list ap)
{
    assert(!"sys_creat not implemented");
    return 0;
}
long sys_link(va_list ap)
{
    assert(!"sys_link not implemented");
    return 0;
}
long sys_unlink(va_list ap)
{
    assert(!"sys_unlink not implemented");
    return 0;
}
long sys_execve(va_list ap)
{
    assert(!"sys_execve not implemented");
    return 0;
}
long sys_chdir(va_list ap)
{
    assert(!"sys_chdir not implemented");
    return 0;
}
long sys_time(va_list ap)
{
    assert(!"sys_time not implemented");
    return 0;
}
long sys_mknod(va_list ap)
{
    assert(!"sys_mknod not implemented");
    return 0;
}
long sys_chmod(va_list ap)
{
    assert(!"sys_chmod not implemented");
    return 0;
}
long sys_lchown(va_list ap)
{
    assert(!"sys_lchown not implemented");
    return 0;
}
long sys_break(va_list ap)
{
    assert(!"sys_break not implemented");
    return 0;
}
long sys_oldstat(va_list ap)
{
    assert(!"sys_oldstat not implemented");
    return 0;
}
/*long sys_getpid(va_list ap)
{
    assert(!"sys_getpid not implemented");
    return 0;
}*/
long sys_mount(va_list ap)
{
    assert(!"sys_mount not implemented");
    return 0;
}
long sys_umount(va_list ap)
{
    assert(!"sys_umount not implemented");
    return 0;
}
long sys_setuid(va_list ap)
{
    assert(!"sys_setuid not implemented");
    return 0;
}
long sys_getuid(va_list ap)
{
    assert(!"sys_getuid not implemented");
    return 0;
}
long sys_stime(va_list ap)
{
    assert(!"sys_stime not implemented");
    return 0;
}
long sys_ptrace(va_list ap)
{
    assert(!"sys_ptrace not implemented");
    return 0;
}
long sys_alarm(va_list ap)
{
    assert(!"sys_alarm not implemented");
    return 0;
}
long sys_oldfstat(va_list ap)
{
    assert(!"sys_oldfstat not implemented");
    return 0;
}
long sys_pause(va_list ap)
{
    assert(!"sys_pause not implemented");
    return 0;
}
long sys_utime(va_list ap)
{
    assert(!"sys_utime not implemented");
    return 0;
}
long sys_stty(va_list ap)
{
    assert(!"sys_stty not implemented");
    return 0;
}
long sys_gtty(va_list ap)
{
    assert(!"sys_gtty not implemented");
    return 0;
}
/*long sys_access(va_list ap)
{
    assert(!"sys_access not implemented");
    return 0;
}*/
long sys_nice(va_list ap)
{
    assert(!"sys_nice not implemented");
    return 0;
}
long sys_ftime(va_list ap)
{
    assert(!"sys_ftime not implemented");
    return 0;
}
long sys_sync(va_list ap)
{
    assert(!"sys_sync not implemented");
    return 0;
}
long sys_kill(va_list ap)
{
    assert(!"sys_kill not implemented");
    return 0;
}
long sys_rename(va_list ap)
{
    assert(!"sys_rename not implemented");
    return 0;
}
long sys_mkdir(va_list ap)
{
    assert(!"sys_mkdir not implemented");
    return 0;
}
long sys_rmdir(va_list ap)
{
    assert(!"sys_rmdir not implemented");
    return 0;
}
long sys_dup(va_list ap)
{
    assert(!"sys_dup not implemented");
    return 0;
}
long sys_pipe(va_list ap)
{
    assert(!"sys_pipe not implemented");
    return 0;
}
long sys_times(va_list ap)
{
    assert(!"sys_times not implemented");
    return 0;
}
long sys_prof(va_list ap)
{
    assert(!"sys_prof not implemented");
    return 0;
}
/*long sys_brk(va_list ap) {
	assert(!"sys_brk not implemented");
	return 0;
}*/
long sys_setgid(va_list ap)
{
    assert(!"sys_setgid not implemented");
    return 0;
}
long sys_getgid(va_list ap)
{
    assert(!"sys_getgid not implemented");
    return 0;
}
long sys_signal(va_list ap)
{
    assert(!"sys_signal not implemented");
    return 0;
}
long sys_geteuid(va_list ap)
{
    assert(!"sys_geteuid not implemented");
    return 0;
}
long sys_getegid(va_list ap)
{
    assert(!"sys_getegid not implemented");
    return 0;
}
long sys_acct(va_list ap)
{
    assert(!"sys_acct not implemented");
    return 0;
}
long sys_umount2(va_list ap)
{
    assert(!"sys_umount2 not implemented");
    return 0;
}
long sys_lock(va_list ap)
{
    assert(!"sys_lock not implemented");
    return 0;
}
/*long sys_ioctl(va_list ap) {
	assert(!"sys_ioctl not implemented");
	return 0;
}*/
long sys_fcntl(va_list ap)
{
    assert(!"sys_fcntl not implemented");
    return 0;
}
long sys_mpx(va_list ap)
{
    assert(!"sys_mpx not implemented");
    return 0;
}
long sys_setpgid(va_list ap)
{
    assert(!"sys_setpgid not implemented");
    return 0;
}
long sys_ulimit(va_list ap)
{
    assert(!"sys_ulimit not implemented");
    return 0;
}
long sys_oldolduname(va_list ap)
{
    assert(!"sys_oldolduname not implemented");
    return 0;
}
long sys_umask(va_list ap)
{
    assert(!"sys_umask not implemented");
    return 0;
}
long sys_chroot(va_list ap)
{
    assert(!"sys_chroot not implemented");
    return 0;
}
long sys_ustat(va_list ap)
{
    assert(!"sys_ustat not implemented");
    return 0;
}
long sys_dup2(va_list ap)
{
    assert(!"sys_dup2 not implemented");
    return 0;
}
long sys_getppid(va_list ap)
{
    assert(!"sys_getppid not implemented");
    return 0;
}
long sys_getpgrp(va_list ap)
{
    assert(!"sys_getpgrp not implemented");
    return 0;
}
long sys_setsid(va_list ap)
{
    assert(!"sys_setsid not implemented");
    return 0;
}
long sys_sigaction(va_list ap)
{
    assert(!"sys_sigaction not implemented");
    return 0;
}
long sys_sgetmask(va_list ap)
{
    assert(!"sys_sgetmask not implemented");
    return 0;
}
long sys_ssetmask(va_list ap)
{
    assert(!"sys_ssetmask not implemented");
    return 0;
}
long sys_setreuid(va_list ap)
{
    assert(!"sys_setreuid not implemented");
    return 0;
}
long sys_setregid(va_list ap)
{
    assert(!"sys_setregid not implemented");
    return 0;
}
long sys_sigsuspend(va_list ap)
{
    assert(!"sys_sigsuspend not implemented");
    return 0;
}
long sys_sigpending(va_list ap)
{
    assert(!"sys_sigpending not implemented");
    return 0;
}
long sys_sethostname(va_list ap)
{
    assert(!"sys_sethostname not implemented");
    return 0;
}
long sys_setrlimit(va_list ap)
{
    assert(!"sys_setrlimit not implemented");
    return 0;
}
long sys_getrlimit(va_list ap)
{
    assert(!"sys_getrlimit not implemented");
    return 0;
}
long sys_getrusage(va_list ap)
{
    assert(!"sys_getrusage not implemented");
    return 0;
}
long sys_gettimeofday(va_list ap)
{
    assert(!"sys_gettimeofday not implemented");
    return 0;
}
long sys_settimeofday(va_list ap)
{
    assert(!"sys_settimeofday not implemented");
    return 0;
}
long sys_getgroups(va_list ap)
{
    assert(!"sys_getgroups not implemented");
    return 0;
}
long sys_setgroups(va_list ap)
{
    assert(!"sys_setgroups not implemented");
    return 0;
}
long sys_select(va_list ap)
{
    assert(!"sys_select not implemented");
    return 0;
}
long sys_symlink(va_list ap)
{
    assert(!"sys_symlink not implemented");
    return 0;
}
long sys_oldlstat(va_list ap)
{
    assert(!"sys_oldlstat not implemented");
    return 0;
}
long sys_readlink(va_list ap)
{
    assert(!"sys_readlink not implemented");
    return 0;
}
long sys_uselib(va_list ap)
{
    assert(!"sys_uselib not implemented");
    return 0;
}
long sys_swapon(va_list ap)
{
    assert(!"sys_swapon not implemented");
    return 0;
}
long sys_reboot(va_list ap)
{
    assert(!"sys_reboot not implemented");
    return 0;
}
long sys_readdir(va_list ap)
{
    assert(!"sys_readdir not implemented");
    return 0;
}
long sys_mmap(va_list ap)
{
    assert(!"sys_mmap not implemented");
    return 0;
}
long sys_munmap(va_list ap)
{
    assert(!"sys_munmap not implemented");
    return 0;
}
long sys_truncate(va_list ap)
{
    assert(!"sys_truncate not implemented");
    return 0;
}
long sys_ftruncate(va_list ap)
{
    assert(!"sys_ftruncate not implemented");
    return 0;
}
long sys_fchmod(va_list ap)
{
    assert(!"sys_fchmod not implemented");
    return 0;
}
long sys_fchown(va_list ap)
{
    assert(!"sys_fchown not implemented");
    return 0;
}
long sys_getpriority(va_list ap)
{
    assert(!"sys_getpriority not implemented");
    return 0;
}
long sys_setpriority(va_list ap)
{
    assert(!"sys_setpriority not implemented");
    return 0;
}
long sys_profil(va_list ap)
{
    assert(!"sys_profil not implemented");
    return 0;
}
long sys_statfs(va_list ap)
{
    assert(!"sys_statfs not implemented");
    return 0;
}
long sys_fstatfs(va_list ap)
{
    assert(!"sys_fstatfs not implemented");
    return 0;
}
long sys_ioperm(va_list ap)
{
    assert(!"sys_ioperm not implemented");
    return 0;
}
long sys_socketcall(va_list ap)
{
    assert(!"sys_socketcall not implemented");
    return 0;
}
long sys_syslog(va_list ap)
{
    assert(!"sys_syslog not implemented");
    return 0;
}
long sys_setitimer(va_list ap)
{
    assert(!"sys_setitimer not implemented");
    return 0;
}
long sys_getitimer(va_list ap)
{
    assert(!"sys_getitimer not implemented");
    return 0;
}
long sys_stat(va_list ap)
{
    assert(!"sys_stat not implemented");
    return 0;
}
long sys_lstat(va_list ap)
{
    assert(!"sys_lstat not implemented");
    return 0;
}
long sys_fstat(va_list ap)
{
    assert(!"sys_fstat not implemented");
    return 0;
}
long sys_olduname(va_list ap)
{
    assert(!"sys_olduname not implemented");
    return 0;
}
long sys_iopl(va_list ap)
{
    assert(!"sys_iopl not implemented");
    return 0;
}
long sys_vhangup(va_list ap)
{
    assert(!"sys_vhangup not implemented");
    return 0;
}
long sys_idle(va_list ap)
{
    assert(!"sys_idle not implemented");
    return 0;
}
long sys_vm86old(va_list ap)
{
    assert(!"sys_vm86old not implemented");
    return 0;
}
long sys_wait4(va_list ap)
{
    assert(!"sys_wait4 not implemented");
    return 0;
}
long sys_swapoff(va_list ap)
{
    assert(!"sys_swapoff not implemented");
    return 0;
}
long sys_sysinfo(va_list ap)
{
    assert(!"sys_sysinfo not implemented");
    return 0;
}
long sys_ipc(va_list ap)
{
    assert(!"sys_ipc not implemented");
    return 0;
}
long sys_fsync(va_list ap)
{
    assert(!"sys_fsync not implemented");
    return 0;
}
long sys_sigreturn(va_list ap)
{
    assert(!"sys_sigreturn not implemented");
    return 0;
}
long sys_clone(va_list ap)
{
    assert(!"sys_clone not implemented");
    return 0;
}
long sys_setdomainname(va_list ap)
{
    assert(!"sys_setdomainname not implemented");
    return 0;
}
long sys_uname(va_list ap)
{
    assert(!"sys_uname not implemented");
    return 0;
}
long sys_modify_ldt(va_list ap)
{
    assert(!"sys_modify_ldt not implemented");
    return 0;
}
long sys_adjtimex(va_list ap)
{
    assert(!"sys_adjtimex not implemented");
    return 0;
}
long sys_mprotect(va_list ap)
{
    assert(!"sys_mprotect not implemented");
    return 0;
}
long sys_sigprocmask(va_list ap)
{
    assert(!"sys_sigprocmask not implemented");
    return 0;
}
long sys_create_module(va_list ap)
{
    assert(!"sys_create_module not implemented");
    return 0;
}
long sys_init_module(va_list ap)
{
    assert(!"sys_init_module not implemented");
    return 0;
}
long sys_delete_module(va_list ap)
{
    assert(!"sys_delete_module not implemented");
    return 0;
}
long sys_get_kernel_syms(va_list ap)
{
    assert(!"sys_get_kernel_syms not implemented");
    return 0;
}
long sys_quotactl(va_list ap)
{
    assert(!"sys_quotactl not implemented");
    return 0;
}
long sys_getpgid(va_list ap)
{
    assert(!"sys_getpgid not implemented");
    return 0;
}
long sys_fchdir(va_list ap)
{
    assert(!"sys_fchdir not implemented");
    return 0;
}
long sys_bdflush(va_list ap)
{
    assert(!"sys_bdflush not implemented");
    return 0;
}
long sys_sysfs(va_list ap)
{
    assert(!"sys_sysfs not implemented");
    return 0;
}
long sys_personality(va_list ap)
{
    assert(!"sys_personality not implemented");
    return 0;
}
long sys_afs_syscall(va_list ap)
{
    assert(!"sys_afs_syscall not implemented");
    return 0;
}
long sys_setfsuid(va_list ap)
{
    assert(!"sys_setfsuid not implemented");
    return 0;
}
long sys_setfsgid(va_list ap)
{
    assert(!"sys_setfsgid not implemented");
    return 0;
}
/*long sys__llseek(va_list ap)
{
    assert(!"sys__llseek not implemented");
    return 0;
}*/
long sys_getdents(va_list ap)
{
    assert(!"sys_getdents not implemented");
    return 0;
}
long sys__newselect(va_list ap)
{
    assert(!"sys__newselect not implemented");
    return 0;
}
long sys_flock(va_list ap)
{
    assert(!"sys_flock not implemented");
    return 0;
}
long sys_msync(va_list ap)
{
    assert(!"sys_msync not implemented");
    return 0;
}
/*long sys_readv(va_list ap) {
	assert(!"sys_readv not implemented");
	return 0;
}*/
/*long sys_writev(va_list ap) {
	assert(!"sys_writev not implemented");
	return 0;
}*/
long sys_getsid(va_list ap)
{
    assert(!"sys_getsid not implemented");
    return 0;
}
long sys_fdatasync(va_list ap)
{
    assert(!"sys_fdatasync not implemented");
    return 0;
}
long sys__sysctl(va_list ap)
{
    assert(!"sys__sysctl not implemented");
    return 0;
}
long sys_mlock(va_list ap)
{
    assert(!"sys_mlock not implemented");
    return 0;
}
long sys_munlock(va_list ap)
{
    assert(!"sys_munlock not implemented");
    return 0;
}
long sys_mlockall(va_list ap)
{
    assert(!"sys_mlockall not implemented");
    return 0;
}
long sys_munlockall(va_list ap)
{
    assert(!"sys_munlockall not implemented");
    return 0;
}
long sys_sched_setparam(va_list ap)
{
    assert(!"sys_sched_setparam not implemented");
    return 0;
}
long sys_sched_getparam(va_list ap)
{
    assert(!"sys_sched_getparam not implemented");
    return 0;
}
long sys_sched_setscheduler(va_list ap)
{
    assert(!"sys_sched_setscheduler not implemented");
    return 0;
}
long sys_sched_getscheduler(va_list ap)
{
    assert(!"sys_sched_getscheduler not implemented");
    return 0;
}
long sys_sched_get_priority_max(va_list ap)
{
    assert(!"sys_sched_get_priority_max not implemented");
    return 0;
}
long sys_sched_get_priority_min(va_list ap)
{
    assert(!"sys_sched_get_priority_min not implemented");
    return 0;
}
long sys_sched_rr_get_interval(va_list ap)
{
    assert(!"sys_sched_rr_get_interval not implemented");
    return 0;
}
long sys_nanosleep(va_list ap)
{
    assert(!"sys_nanosleep not implemented");
    return 0;
}
long sys_setresuid(va_list ap)
{
    assert(!"sys_setresuid not implemented");
    return 0;
}
long sys_getresuid(va_list ap)
{
    assert(!"sys_getresuid not implemented");
    return 0;
}
long sys_vm86(va_list ap)
{
    assert(!"sys_vm86 not implemented");
    return 0;
}
long sys_query_module(va_list ap)
{
    assert(!"sys_query_module not implemented");
    return 0;
}
long sys_poll(va_list ap)
{
    assert(!"sys_poll not implemented");
    return 0;
}
long sys_nfsservctl(va_list ap)
{
    assert(!"sys_nfsservctl not implemented");
    return 0;
}
long sys_setresgid(va_list ap)
{
    assert(!"sys_setresgid not implemented");
    return 0;
}
long sys_getresgid(va_list ap)
{
    assert(!"sys_getresgid not implemented");
    return 0;
}
long sys_prctl(va_list ap)
{
    assert(!"sys_prctl not implemented");
    return 0;
}
long sys_rt_sigreturn(va_list ap)
{
    assert(!"sys_rt_sigreturn not implemented");
    return 0;
}
long sys_rt_sigaction(va_list ap)
{
    assert(!"sys_rt_sigaction not implemented");
    return 0;
}
/*long sys_rt_sigprocmask(va_list ap)
{
    assert(!"sys_rt_sigprocmask not implemented");
    return 0;
}*/
long sys_rt_sigpending(va_list ap)
{
    assert(!"sys_rt_sigpending not implemented");
    return 0;
}
long sys_rt_sigtimedwait(va_list ap)
{
    assert(!"sys_rt_sigtimedwait not implemented");
    return 0;
}
long sys_rt_sigqueueinfo(va_list ap)
{
    assert(!"sys_rt_sigqueueinfo not implemented");
    return 0;
}
long sys_rt_sigsuspend(va_list ap)
{
    assert(!"sys_rt_sigsuspend not implemented");
    return 0;
}
long sys_pread64(va_list ap)
{
    assert(!"sys_pread64 not implemented");
    return 0;
}
long sys_pwrite64(va_list ap)
{
    assert(!"sys_pwrite64 not implemented");
    return 0;
}
long sys_chown(va_list ap)
{
    assert(!"sys_chown not implemented");
    return 0;
}
long sys_getcwd(va_list ap)
{
    assert(!"sys_getcwd not implemented");
    return 0;
}
long sys_capget(va_list ap)
{
    assert(!"sys_capget not implemented");
    return 0;
}
long sys_capset(va_list ap)
{
    assert(!"sys_capset not implemented");
    return 0;
}
long sys_sigaltstack(va_list ap)
{
    assert(!"sys_sigaltstack not implemented");
    return 0;
}
long sys_sendfile(va_list ap)
{
    assert(!"sys_sendfile not implemented");
    return 0;
}
long sys_getpmsg(va_list ap)
{
    assert(!"sys_getpmsg not implemented");
    return 0;
}
long sys_putpmsg(va_list ap)
{
    assert(!"sys_putpmsg not implemented");
    return 0;
}
long sys_vfork(va_list ap)
{
    assert(!"sys_vfork not implemented");
    return 0;
}
long sys_ugetrlimit(va_list ap)
{
    assert(!"sys_ugetrlimit not implemented");
    return 0;
}
/*long sys_mmap2(va_list ap) {
	assert(!"sys_mmap2 not implemented");
	return 0;
}*/
long sys_truncate64(va_list ap)
{
    assert(!"sys_truncate64 not implemented");
    return 0;
}
long sys_ftruncate64(va_list ap)
{
    assert(!"sys_ftruncate64 not implemented");
    return 0;
}
long sys_stat64(va_list ap)
{
    assert(!"sys_stat64 not implemented");
    return 0;
}
long sys_lstat64(va_list ap)
{
    assert(!"sys_lstat64 not implemented");
    return 0;
}
long sys_fstat64(va_list ap)
{
    assert(!"sys_fstat64 not implemented");
    return 0;
}
long sys_lchown32(va_list ap)
{
    assert(!"sys_lchown32 not implemented");
    return 0;
}
long sys_getuid32(va_list ap)
{
    assert(!"sys_getuid32 not implemented");
    return 0;
}
long sys_getgid32(va_list ap)
{
    assert(!"sys_getgid32 not implemented");
    return 0;
}
long sys_geteuid32(va_list ap)
{
    assert(!"sys_geteuid32 not implemented");
    return 0;
}
long sys_getegid32(va_list ap)
{
    assert(!"sys_getegid32 not implemented");
    return 0;
}
long sys_setreuid32(va_list ap)
{
    assert(!"sys_setreuid32 not implemented");
    return 0;
}
long sys_setregid32(va_list ap)
{
    assert(!"sys_setregid32 not implemented");
    return 0;
}
long sys_getgroups32(va_list ap)
{
    assert(!"sys_getgroups32 not implemented");
    return 0;
}
long sys_setgroups32(va_list ap)
{
    assert(!"sys_setgroups32 not implemented");
    return 0;
}
long sys_fchown32(va_list ap)
{
    assert(!"sys_fchown32 not implemented");
    return 0;
}
long sys_setresuid32(va_list ap)
{
    assert(!"sys_setresuid32 not implemented");
    return 0;
}
long sys_getresuid32(va_list ap)
{
    assert(!"sys_getresuid32 not implemented");
    return 0;
}
long sys_setresgid32(va_list ap)
{
    assert(!"sys_setresgid32 not implemented");
    return 0;
}
long sys_getresgid32(va_list ap)
{
    assert(!"sys_getresgid32 not implemented");
    return 0;
}
long sys_chown32(va_list ap)
{
    assert(!"sys_chown32 not implemented");
    return 0;
}
long sys_setuid32(va_list ap)
{
    assert(!"sys_setuid32 not implemented");
    return 0;
}
long sys_setgid32(va_list ap)
{
    assert(!"sys_setgid32 not implemented");
    return 0;
}
long sys_setfsuid32(va_list ap)
{
    assert(!"sys_setfsuid32 not implemented");
    return 0;
}
long sys_setfsgid32(va_list ap)
{
    assert(!"sys_setfsgid32 not implemented");
    return 0;
}
long sys_pivot_root(va_list ap)
{
    assert(!"sys_pivot_root not implemented");
    return 0;
}
long sys_mincore(va_list ap)
{
    assert(!"sys_mincore not implemented");
    return 0;
}
long sys_madvise(va_list ap)
{
    assert(!"sys_madvise not implemented");
    return 0;
}
long sys_madvise1(va_list ap)
{
    assert(!"sys_madvise1 not implemented");
    return 0;
}
long sys_getdents64(va_list ap)
{
    assert(!"sys_getdents64 not implemented");
    return 0;
}
long sys_fcntl64(va_list ap)
{
    assert(!"sys_fcntl64 not implemented");
    return 0;
}
/*long sys_gettid(va_list ap)
{
    assert(!"sys_gettid not implemented");
    return 0;
}*/
long sys_readahead(va_list ap)
{
    assert(!"sys_readahead not implemented");
    return 0;
}
long sys_setxattr(va_list ap)
{
    assert(!"sys_setxattr not implemented");
    return 0;
}
long sys_lsetxattr(va_list ap)
{
    assert(!"sys_lsetxattr not implemented");
    return 0;
}
long sys_fsetxattr(va_list ap)
{
    assert(!"sys_fsetxattr not implemented");
    return 0;
}
long sys_getxattr(va_list ap)
{
    assert(!"sys_getxattr not implemented");
    return 0;
}
long sys_lgetxattr(va_list ap)
{
    assert(!"sys_lgetxattr not implemented");
    return 0;
}
long sys_fgetxattr(va_list ap)
{
    assert(!"sys_fgetxattr not implemented");
    return 0;
}
long sys_listxattr(va_list ap)
{
    assert(!"sys_listxattr not implemented");
    return 0;
}
long sys_llistxattr(va_list ap)
{
    assert(!"sys_llistxattr not implemented");
    return 0;
}
long sys_flistxattr(va_list ap)
{
    assert(!"sys_flistxattr not implemented");
    return 0;
}
long sys_removexattr(va_list ap)
{
    assert(!"sys_removexattr not implemented");
    return 0;
}
long sys_lremovexattr(va_list ap)
{
    assert(!"sys_lremovexattr not implemented");
    return 0;
}
long sys_fremovexattr(va_list ap)
{
    assert(!"sys_fremovexattr not implemented");
    return 0;
}
long sys_tkill(va_list ap)
{
    assert(!"sys_tkill not implemented");
    return 0;
}
long sys_sendfile64(va_list ap)
{
    assert(!"sys_sendfile64 not implemented");
    return 0;
}
long sys_futex(va_list ap)
{
    assert(!"sys_futex not implemented");
    return 0;
}
long sys_sched_setaffinity(va_list ap)
{
    assert(!"sys_sched_setaffinity not implemented");
    return 0;
}
long sys_sched_getaffinity(va_list ap)
{
    assert(!"sys_sched_getaffinity not implemented");
    return 0;
}
long sys_get_thread_area(va_list ap)
{
    assert(!"sys_get_thread_area not implemented");
    return 0;
}
long sys_io_setup(va_list ap)
{
    assert(!"sys_io_setup not implemented");
    return 0;
}
long sys_io_destroy(va_list ap)
{
    assert(!"sys_io_destroy not implemented");
    return 0;
}
long sys_io_getevents(va_list ap)
{
    assert(!"sys_io_getevents not implemented");
    return 0;
}
long sys_io_submit(va_list ap)
{
    assert(!"sys_io_submit not implemented");
    return 0;
}
long sys_io_cancel(va_list ap)
{
    assert(!"sys_io_cancel not implemented");
    return 0;
}
long sys_fadvise64(va_list ap)
{
    assert(!"sys_fadvise64 not implemented");
    return 0;
}
/*long sys_exit_group(va_list ap)
{
    assert(!"sys_exit_group not implemented");
    return 0;
}*/
long sys_lookup_dcookie(va_list ap)
{
    assert(!"sys_lookup_dcookie not implemented");
    return 0;
}
long sys_epoll_create(va_list ap)
{
    assert(!"sys_epoll_create not implemented");
    return 0;
}
long sys_epoll_ctl(va_list ap)
{
    assert(!"sys_epoll_ctl not implemented");
    return 0;
}
long sys_epoll_wait(va_list ap)
{
    assert(!"sys_epoll_wait not implemented");
    return 0;
}
long sys_remap_file_pages(va_list ap)
{
    assert(!"sys_remap_file_pages not implemented");
    return 0;
}
long sys_timer_create(va_list ap)
{
    assert(!"sys_timer_create not implemented");
    return 0;
}
long sys_timer_settime(va_list ap)
{
    assert(!"sys_timer_settime not implemented");
    return 0;
}
long sys_timer_gettime(va_list ap)
{
    assert(!"sys_timer_gettime not implemented");
    return 0;
}
long sys_timer_getoverrun(va_list ap)
{
    assert(!"sys_timer_getoverrun not implemented");
    return 0;
}
long sys_timer_delete(va_list ap)
{
    assert(!"sys_timer_delete not implemented");
    return 0;
}
long sys_clock_settime(va_list ap)
{
    assert(!"sys_clock_settime not implemented");
    return 0;
}
long sys_clock_gettime(va_list ap)
{
    assert(!"sys_clock_gettime not implemented");
    return 0;
}
long sys_clock_getres(va_list ap)
{
    assert(!"sys_clock_getres not implemented");
    return 0;
}
long sys_clock_nanosleep(va_list ap)
{
    assert(!"sys_clock_nanosleep not implemented");
    return 0;
}
long sys_statfs64(va_list ap)
{
    assert(!"sys_statfs64 not implemented");
    return 0;
}
long sys_fstatfs64(va_list ap)
{
    assert(!"sys_fstatfs64 not implemented");
    return 0;
}
/*long sys_tgkill(va_list ap)
{
    assert(!"sys_tgkill not implemented");
    return 0;
}*/
long sys_utimes(va_list ap)
{
    assert(!"sys_utimes not implemented");
    return 0;
}
long sys_fadvise64_64(va_list ap)
{
    assert(!"sys_fadvise64_64 not implemented");
    return 0;
}
long sys_vserver(va_list ap)
{
    assert(!"sys_vserver not implemented");
    return 0;
}
long sys_mbind(va_list ap)
{
    assert(!"sys_mbind not implemented");
    return 0;
}
long sys_get_mempolicy(va_list ap)
{
    assert(!"sys_get_mempolicy not implemented");
    return 0;
}
long sys_set_mempolicy(va_list ap)
{
    assert(!"sys_set_mempolicy not implemented");
    return 0;
}
long sys_mq_open(va_list ap)
{
    assert(!"sys_mq_open not implemented");
    return 0;
}
long sys_mq_unlink(va_list ap)
{
    assert(!"sys_mq_unlink not implemented");
    return 0;
}
long sys_mq_timedsend(va_list ap)
{
    assert(!"sys_mq_timedsend not implemented");
    return 0;
}
long sys_mq_timedreceive(va_list ap)
{
    assert(!"sys_mq_timedreceive not implemented");
    return 0;
}
long sys_mq_notify(va_list ap)
{
    assert(!"sys_mq_notify not implemented");
    return 0;
}
long sys_mq_getsetattr(va_list ap)
{
    assert(!"sys_mq_getsetattr not implemented");
    return 0;
}
long sys_kexec_load(va_list ap)
{
    assert(!"sys_kexec_load not implemented");
    return 0;
}
long sys_waitid(va_list ap)
{
    assert(!"sys_waitid not implemented");
    return 0;
}
long sys_add_key(va_list ap)
{
    assert(!"sys_add_key not implemented");
    return 0;
}
long sys_request_key(va_list ap)
{
    assert(!"sys_request_key not implemented");
    return 0;
}
long sys_keyctl(va_list ap)
{
    assert(!"sys_keyctl not implemented");
    return 0;
}
long sys_ioprio_set(va_list ap)
{
    assert(!"sys_ioprio_set not implemented");
    return 0;
}
long sys_ioprio_get(va_list ap)
{
    assert(!"sys_ioprio_get not implemented");
    return 0;
}
long sys_inotify_init(va_list ap)
{
    assert(!"sys_inotify_init not implemented");
    return 0;
}
long sys_inotify_add_watch(va_list ap)
{
    assert(!"sys_inotify_add_watch not implemented");
    return 0;
}
long sys_inotify_rm_watch(va_list ap)
{
    assert(!"sys_inotify_rm_watch not implemented");
    return 0;
}
long sys_migrate_pages(va_list ap)
{
    assert(!"sys_migrate_pages not implemented");
    return 0;
}
long sys_openat(va_list ap)
{
    assert(!"sys_openat not implemented");
    return 0;
}
long sys_mkdirat(va_list ap)
{
    assert(!"sys_mkdirat not implemented");
    return 0;
}
long sys_mknodat(va_list ap)
{
    assert(!"sys_mknodat not implemented");
    return 0;
}
long sys_fchownat(va_list ap)
{
    assert(!"sys_fchownat not implemented");
    return 0;
}
long sys_futimesat(va_list ap)
{
    assert(!"sys_futimesat not implemented");
    return 0;
}
long sys_fstatat64(va_list ap)
{
    assert(!"sys_fstatat64 not implemented");
    return 0;
}
long sys_unlinkat(va_list ap)
{
    assert(!"sys_unlinkat not implemented");
    return 0;
}
long sys_renameat(va_list ap)
{
    assert(!"sys_renameat not implemented");
    return 0;
}
long sys_linkat(va_list ap)
{
    assert(!"sys_linkat not implemented");
    return 0;
}
long sys_symlinkat(va_list ap)
{
    assert(!"sys_symlinkat not implemented");
    return 0;
}
long sys_readlinkat(va_list ap)
{
    assert(!"sys_readlinkat not implemented");
    return 0;
}
long sys_fchmodat(va_list ap)
{
    assert(!"sys_fchmodat not implemented");
    return 0;
}
long sys_faccessat(va_list ap)
{
    assert(!"sys_faccessat not implemented");
    return 0;
}
long sys_pselect6(va_list ap)
{
    assert(!"sys_pselect6 not implemented");
    return 0;
}
long sys_ppoll(va_list ap)
{
    assert(!"sys_ppoll not implemented");
    return 0;
}
long sys_unshare(va_list ap)
{
    assert(!"sys_unshare not implemented");
    return 0;
}
long sys_set_robust_list(va_list ap)
{
    assert(!"sys_set_robust_list not implemented");
    return 0;
}
long sys_get_robust_list(va_list ap)
{
    assert(!"sys_get_robust_list not implemented");
    return 0;
}
long sys_splice(va_list ap)
{
    assert(!"sys_splice not implemented");
    return 0;
}
long sys_sync_file_range(va_list ap)
{
    assert(!"sys_sync_file_range not implemented");
    return 0;
}
long sys_tee(va_list ap)
{
    assert(!"sys_tee not implemented");
    return 0;
}
long sys_vmsplice(va_list ap)
{
    assert(!"sys_vmsplice not implemented");
    return 0;
}
long sys_move_pages(va_list ap)
{
    assert(!"sys_move_pages not implemented");
    return 0;
}
long sys_getcpu(va_list ap)
{
    assert(!"sys_getcpu not implemented");
    return 0;
}
long sys_epoll_pwait(va_list ap)
{
    assert(!"sys_epoll_pwait not implemented");
    return 0;
}
long sys_utimensat(va_list ap)
{
    assert(!"sys_utimensat not implemented");
    return 0;
}
long sys_signalfd(va_list ap)
{
    assert(!"sys_signalfd not implemented");
    return 0;
}
long sys_timerfd_create(va_list ap)
{
    assert(!"sys_timerfd_create not implemented");
    return 0;
}
long sys_eventfd(va_list ap)
{
    assert(!"sys_eventfd not implemented");
    return 0;
}
long sys_fallocate(va_list ap)
{
    assert(!"sys_fallocate not implemented");
    return 0;
}
long sys_timerfd_settime(va_list ap)
{
    assert(!"sys_timerfd_settime not implemented");
    return 0;
}
long sys_timerfd_gettime(va_list ap)
{
    assert(!"sys_timerfd_gettime not implemented");
    return 0;
}
long sys_signalfd4(va_list ap)
{
    assert(!"sys_signalfd4 not implemented");
    return 0;
}
long sys_eventfd2(va_list ap)
{
    assert(!"sys_eventfd2 not implemented");
    return 0;
}
long sys_epoll_create1(va_list ap)
{
    assert(!"sys_epoll_create1 not implemented");
    return 0;
}
long sys_dup3(va_list ap)
{
    assert(!"sys_dup3 not implemented");
    return 0;
}
long sys_pipe2(va_list ap)
{
    assert(!"sys_pipe2 not implemented");
    return 0;
}
long sys_inotify_init1(va_list ap)
{
    assert(!"sys_inotify_init1 not implemented");
    return 0;
}
long sys_preadv(va_list ap)
{
    assert(!"sys_preadv not implemented");
    return 0;
}
long sys_pwritev(va_list ap)
{
    assert(!"sys_pwritev not implemented");
    return 0;
}
long sys_name_to_handle_at(va_list ap)
{
    assert(!"sys_name_to_handle_at not implemented");
    return 0;
}
long sys_open_by_handle_at(va_list ap)
{
    assert(!"sys_open_by_handle_at not implemented");
    return 0;
}
long sys_clock_adjtime(va_list ap)
{
    assert(!"sys_clock_adjtime not implemented");
    return 0;
}
long sys_syncfs(va_list ap)
{
    assert(!"sys_syncfs not implemented");
    return 0;
}
long sys_sendmmsg(va_list ap)
{
    assert(!"sys_sendmmsg not implemented");
    return 0;
}
long sys_setns(va_list ap)
{
    assert(!"sys_setns not implemented");
    return 0;
}
long sys_process_vm_readv(va_list ap)
{
    assert(!"sys_process_vm_readv not implemented");
    return 0;
}
long sys_process_vm_writev(va_list ap)
{
    assert(!"sys_process_vm_writev not implemented");
    return 0;
}
long sys_fstatat(va_list ap)
{
    assert(!"sys_fstatat not implemented");
    return 0;
}
long sys_pread(va_list ap)
{
    assert(!"sys_pread not implemented");
    return 0;
}
long sys_pwrite(va_list ap)
{
    assert(!"sys_pwrite not implemented");
    return 0;
}
long sys_fadvise(va_list ap)
{
    assert(!"sys_fadvise not implemented");
    return 0;
}

#endif

#ifdef ARCH_ARM

long sys_restart_syscall(va_list ap)
{
    assert(!"sys_restart_syscall not implemented");
    return 0;
}
/*long sys_exit(va_list ap) {
    assert(!"sys_exit not implemented");
    return 0;
}*/
long sys_fork(va_list ap)
{
    assert(!"sys_fork not implemented");
    return 0;
}
/*long sys_read(va_list ap)
{
    assert(!"sys_read not implemented");
    return 0;
}*/
long sys_write(va_list ap)
{
    assert(!"sys_write not implemented");
    return 0;
}
long sys_creat(va_list ap)
{
    assert(!"sys_creat not implemented");
    return 0;
}
long sys_link(va_list ap)
{
    assert(!"sys_link not implemented");
    return 0;
}
long sys_unlink(va_list ap)
{
    assert(!"sys_unlink not implemented");
    return 0;
}
long sys_execve(va_list ap)
{
    assert(!"sys_execve not implemented");
    return 0;
}
long sys_chdir(va_list ap)
{
    assert(!"sys_chdir not implemented");
    return 0;
}
long sys_mknod(va_list ap)
{
    assert(!"sys_mknod not implemented");
    return 0;
}
long sys_chmod(va_list ap)
{
    assert(!"sys_chmod not implemented");
    return 0;
}
long sys_lchown(va_list ap)
{
    assert(!"sys_lchown not implemented");
    return 0;
}
/*long sys_getpid(va_list ap)
{
    assert(!"sys_getpid not implemented");
    return 0;
}*/
long sys_mount(va_list ap)
{
    assert(!"sys_mount not implemented");
    return 0;
}
long sys_setuid(va_list ap)
{
    assert(!"sys_setuid not implemented");
    return 0;
}
long sys_getuid(va_list ap)
{
    assert(!"sys_getuid not implemented");
    return 0;
}
long sys_ptrace(va_list ap)
{
    assert(!"sys_ptrace not implemented");
    return 0;
}
long sys_pause(va_list ap)
{
    assert(!"sys_pause not implemented");
    return 0;
}
/*long sys_access(va_list ap)
{
    assert(!"sys_access not implemented");
    return 0;
}*/
long sys_nice(va_list ap)
{
    assert(!"sys_nice not implemented");
    return 0;
}
long sys_sync(va_list ap)
{
    assert(!"sys_sync not implemented");
    return 0;
}
long sys_kill(va_list ap)
{
    assert(!"sys_kill not implemented");
    return 0;
}
long sys_rename(va_list ap)
{
    assert(!"sys_rename not implemented");
    return 0;
}
long sys_mkdir(va_list ap)
{
    assert(!"sys_mkdir not implemented");
    return 0;
}
long sys_rmdir(va_list ap)
{
    assert(!"sys_rmdir not implemented");
    return 0;
}
long sys_dup(va_list ap)
{
    assert(!"sys_dup not implemented");
    return 0;
}
long sys_pipe(va_list ap)
{
    assert(!"sys_pipe not implemented");
    return 0;
}
long sys_times(va_list ap)
{
    assert(!"sys_times not implemented");
    return 0;
}
/*long sys_brk(va_list ap) {
    assert(!"sys_brk not implemented");
    return 0;
}*/
long sys_setgid(va_list ap)
{
    assert(!"sys_setgid not implemented");
    return 0;
}
long sys_getgid(va_list ap)
{
    assert(!"sys_getgid not implemented");
    return 0;
}
long sys_geteuid(va_list ap)
{
    assert(!"sys_geteuid not implemented");
    return 0;
}
long sys_getegid(va_list ap)
{
    assert(!"sys_getegid not implemented");
    return 0;
}
long sys_acct(va_list ap)
{
    assert(!"sys_acct not implemented");
    return 0;
}
long sys_umount2(va_list ap)
{
    assert(!"sys_umount2 not implemented");
    return 0;
}
/*long sys_ioctl(va_list ap) {
    assert(!"sys_ioctl not implemented");
    return 0;
}*/
long sys_fcntl(va_list ap)
{
    assert(!"sys_fcntl not implemented");
    return 0;
}
long sys_setpgid(va_list ap)
{
    assert(!"sys_setpgid not implemented");
    return 0;
}
long sys_umask(va_list ap)
{
    assert(!"sys_umask not implemented");
    return 0;
}
long sys_chroot(va_list ap)
{
    assert(!"sys_chroot not implemented");
    return 0;
}
long sys_ustat(va_list ap)
{
    assert(!"sys_ustat not implemented");
    return 0;
}
long sys_dup2(va_list ap)
{
    assert(!"sys_dup2 not implemented");
    return 0;
}
long sys_getppid(va_list ap)
{
    assert(!"sys_getppid not implemented");
    return 0;
}
long sys_getpgrp(va_list ap)
{
    assert(!"sys_getpgrp not implemented");
    return 0;
}
long sys_setsid(va_list ap)
{
    assert(!"sys_setsid not implemented");
    return 0;
}
long sys_sigaction(va_list ap)
{
    assert(!"sys_sigaction not implemented");
    return 0;
}
long sys_setreuid(va_list ap)
{
    assert(!"sys_setreuid not implemented");
    return 0;
}
long sys_setregid(va_list ap)
{
    assert(!"sys_setregid not implemented");
    return 0;
}
long sys_sigsuspend(va_list ap)
{
    assert(!"sys_sigsuspend not implemented");
    return 0;
}
long sys_sigpending(va_list ap)
{
    assert(!"sys_sigpending not implemented");
    return 0;
}
long sys_sethostname(va_list ap)
{
    assert(!"sys_sethostname not implemented");
    return 0;
}
long sys_setrlimit(va_list ap)
{
    assert(!"sys_setrlimit not implemented");
    return 0;
}
long sys_getrusage(va_list ap)
{
    assert(!"sys_getrusage not implemented");
    return 0;
}
long sys_gettimeofday(va_list ap)
{
    assert(!"sys_gettimeofday not implemented");
    return 0;
}
long sys_settimeofday(va_list ap)
{
    assert(!"sys_settimeofday not implemented");
    return 0;
}
long sys_getgroups(va_list ap)
{
    assert(!"sys_getgroups not implemented");
    return 0;
}
long sys_setgroups(va_list ap)
{
    assert(!"sys_setgroups not implemented");
    return 0;
}
long sys_symlink(va_list ap)
{
    assert(!"sys_symlink not implemented");
    return 0;
}
long sys_readlink(va_list ap)
{
    assert(!"sys_readlink not implemented");
    return 0;
}
long sys_uselib(va_list ap)
{
    assert(!"sys_uselib not implemented");
    return 0;
}
long sys_swapon(va_list ap)
{
    assert(!"sys_swapon not implemented");
    return 0;
}
long sys_reboot(va_list ap)
{
    assert(!"sys_reboot not implemented");
    return 0;
}
long sys_munmap(va_list ap)
{
    assert(!"sys_munmap not implemented");
    return 0;
}
long sys_truncate(va_list ap)
{
    assert(!"sys_truncate not implemented");
    return 0;
}
long sys_ftruncate(va_list ap)
{
    assert(!"sys_ftruncate not implemented");
    return 0;
}
long sys_fchmod(va_list ap)
{
    assert(!"sys_fchmod not implemented");
    return 0;
}
long sys_fchown(va_list ap)
{
    assert(!"sys_fchown not implemented");
    return 0;
}
long sys_getpriority(va_list ap)
{
    assert(!"sys_getpriority not implemented");
    return 0;
}
long sys_setpriority(va_list ap)
{
    assert(!"sys_setpriority not implemented");
    return 0;
}
long sys_statfs(va_list ap)
{
    assert(!"sys_statfs not implemented");
    return 0;
}
long sys_fstatfs(va_list ap)
{
    assert(!"sys_fstatfs not implemented");
    return 0;
}
long sys_syslog(va_list ap)
{
    assert(!"sys_syslog not implemented");
    return 0;
}
long sys_setitimer(va_list ap)
{
    assert(!"sys_setitimer not implemented");
    return 0;
}
long sys_getitimer(va_list ap)
{
    assert(!"sys_getitimer not implemented");
    return 0;
}
long sys_stat(va_list ap)
{
    assert(!"sys_stat not implemented");
    return 0;
}
long sys_lstat(va_list ap)
{
    assert(!"sys_lstat not implemented");
    return 0;
}
long sys_fstat(va_list ap)
{
    assert(!"sys_fstat not implemented");
    return 0;
}
long sys_vhangup(va_list ap)
{
    assert(!"sys_vhangup not implemented");
    return 0;
}
long sys_wait4(va_list ap)
{
    assert(!"sys_wait4 not implemented");
    return 0;
}
long sys_swapoff(va_list ap)
{
    assert(!"sys_swapoff not implemented");
    return 0;
}
long sys_sysinfo(va_list ap)
{
    assert(!"sys_sysinfo not implemented");
    return 0;
}
long sys_fsync(va_list ap)
{
    assert(!"sys_fsync not implemented");
    return 0;
}
long sys_sigreturn(va_list ap)
{
    assert(!"sys_sigreturn not implemented");
    return 0;
}
long sys_clone(va_list ap)
{
    assert(!"sys_clone not implemented");
    return 0;
}
long sys_setdomainname(va_list ap)
{
    assert(!"sys_setdomainname not implemented");
    return 0;
}
long sys_uname(va_list ap)
{
    assert(!"sys_uname not implemented");
    return 0;
}
long sys_adjtimex(va_list ap)
{
    assert(!"sys_adjtimex not implemented");
    return 0;
}
long sys_mprotect(va_list ap)
{
    assert(!"sys_mprotect not implemented");
    return 0;
}
long sys_sigprocmask(va_list ap)
{
    assert(!"sys_sigprocmask not implemented");
    return 0;
}
long sys_init_module(va_list ap)
{
    assert(!"sys_init_module not implemented");
    return 0;
}
long sys_delete_module(va_list ap)
{
    assert(!"sys_delete_module not implemented");
    return 0;
}
long sys_quotactl(va_list ap)
{
    assert(!"sys_quotactl not implemented");
    return 0;
}
long sys_getpgid(va_list ap)
{
    assert(!"sys_getpgid not implemented");
    return 0;
}
long sys_fchdir(va_list ap)
{
    assert(!"sys_fchdir not implemented");
    return 0;
}
long sys_bdflush(va_list ap)
{
    assert(!"sys_bdflush not implemented");
    return 0;
}
long sys_sysfs(va_list ap)
{
    assert(!"sys_sysfs not implemented");
    return 0;
}
long sys_personality(va_list ap)
{
    assert(!"sys_personality not implemented");
    return 0;
}
long sys_setfsuid(va_list ap)
{
    assert(!"sys_setfsuid not implemented");
    return 0;
}
long sys_setfsgid(va_list ap)
{
    assert(!"sys_setfsgid not implemented");
    return 0;
}
long sys_getdents(va_list ap)
{
    assert(!"sys_getdents not implemented");
    return 0;
}
long sys__newselect(va_list ap)
{
    assert(!"sys__newselect not implemented");
    return 0;
}
long sys_flock(va_list ap)
{
    assert(!"sys_flock not implemented");
    return 0;
}
long sys_msync(va_list ap)
{
    assert(!"sys_msync not implemented");
    return 0;
}
/*long sys_readv(va_list ap) {
    assert(!"sys_readv not implemented");
    return 0;
}*/
/*long sys_writev(va_list ap) {
    assert(!"sys_writev not implemented");
    return 0;
}*/
long sys_getsid(va_list ap)
{
    assert(!"sys_getsid not implemented");
    return 0;
}
long sys_fdatasync(va_list ap)
{
    assert(!"sys_fdatasync not implemented");
    return 0;
}
long sys__sysctl(va_list ap)
{
    assert(!"sys__sysctl not implemented");
    return 0;
}
long sys_mlock(va_list ap)
{
    assert(!"sys_mlock not implemented");
    return 0;
}
long sys_munlock(va_list ap)
{
    assert(!"sys_munlock not implemented");
    return 0;
}
long sys_mlockall(va_list ap)
{
    assert(!"sys_mlockall not implemented");
    return 0;
}
long sys_munlockall(va_list ap)
{
    assert(!"sys_munlockall not implemented");
    return 0;
}
long sys_sched_setparam(va_list ap)
{
    assert(!"sys_sched_setparam not implemented");
    return 0;
}
long sys_sched_getparam(va_list ap)
{
    assert(!"sys_sched_getparam not implemented");
    return 0;
}
long sys_sched_setscheduler(va_list ap)
{
    assert(!"sys_sched_setscheduler not implemented");
    return 0;
}
long sys_sched_getscheduler(va_list ap)
{
    assert(!"sys_sched_getscheduler not implemented");
    return 0;
}
long sys_sched_get_priority_max(va_list ap)
{
    assert(!"sys_sched_get_priority_max not implemented");
    return 0;
}
long sys_sched_get_priority_min(va_list ap)
{
    assert(!"sys_sched_get_priority_min not implemented");
    return 0;
}
long sys_sched_rr_get_interval(va_list ap)
{
    assert(!"sys_sched_rr_get_interval not implemented");
    return 0;
}
long sys_nanosleep(va_list ap)
{
    assert(!"sys_nanosleep not implemented");
    return 0;
}

long sys_setresuid(va_list ap)
{
    assert(!"sys_setresuid not implemented");
    return 0;
}
long sys_getresuid(va_list ap)
{
    assert(!"sys_getresuid not implemented");
    return 0;
}
long sys_poll(va_list ap)
{
    assert(!"sys_poll not implemented");
    return 0;
}
long sys_nfsservctl(va_list ap)
{
    assert(!"sys_nfsservctl not implemented");
    return 0;
}
long sys_setresgid(va_list ap)
{
    assert(!"sys_setresgid not implemented");
    return 0;
}
long sys_getresgid(va_list ap)
{
    assert(!"sys_getresgid not implemented");
    return 0;
}
long sys_prctl(va_list ap)
{
    assert(!"sys_prctl not implemented");
    return 0;
}
long sys_rt_sigreturn(va_list ap)
{
    assert(!"sys_rt_sigreturn not implemented");
    return 0;
}
long sys_rt_sigaction(va_list ap)
{
    assert(!"sys_rt_sigaction not implemented");
    return 0;
}
/*long sys_rt_sigprocmask(va_list ap)
{
    assert(!"sys_rt_sigprocmask not implemented");
    return 0;
}*/
long sys_rt_sigpending(va_list ap)
{
    assert(!"sys_rt_sigpending not implemented");
    return 0;
}
long sys_rt_sigtimedwait(va_list ap)
{
    assert(!"sys_rt_sigtimedwait not implemented");
    return 0;
}
long sys_rt_sigqueueinfo(va_list ap)
{
    assert(!"sys_rt_sigqueueinfo not implemented");
    return 0;
}
long sys_rt_sigsuspend(va_list ap)
{
    assert(!"sys_rt_sigsuspend not implemented");
    return 0;
}
long sys_pread64(va_list ap)
{
    assert(!"sys_pread64 not implemented");
    return 0;
}
long sys_pwrite64(va_list ap)
{
    assert(!"sys_pwrite64 not implemented");
    return 0;
}
long sys_chown(va_list ap)
{
    assert(!"sys_chown not implemented");
    return 0;
}
long sys_getcwd(va_list ap)
{
    assert(!"sys_getcwd not implemented");
    return 0;
}
long sys_capget(va_list ap)
{
    assert(!"sys_capget not implemented");
    return 0;
}
long sys_capset(va_list ap)
{
    assert(!"sys_capset not implemented");
    return 0;
}
long sys_sigaltstack(va_list ap)
{
    assert(!"sys_sigaltstack not implemented");
    return 0;
}
long sys_sendfile(va_list ap)
{
    assert(!"sys_sendfile not implemented");
    return 0;
}
long sys_vfork(va_list ap)
{
    assert(!"sys_vfork not implemented");
    return 0;
}
long sys_ugetrlimit(va_list ap)
{
    assert(!"sys_ugetrlimit not implemented");
    return 0;
}
/*long sys_mmap2(va_list ap) {
    assert(!"sys_mmap2 not implemented");
    return 0;
}*/
long sys_truncate64(va_list ap)
{
    assert(!"sys_truncate64 not implemented");
    return 0;
}
long sys_ftruncate64(va_list ap)
{
    assert(!"sys_ftruncate64 not implemented");
    return 0;
}
long sys_stat64(va_list ap)
{
    assert(!"sys_stat64 not implemented");
    return 0;
}
long sys_lstat64(va_list ap)
{
    assert(!"sys_lstat64 not implemented");
    return 0;
}
long sys_fstat64(va_list ap)
{
    assert(!"sys_fstat64 not implemented");
    return 0;
}
long sys_lchown32(va_list ap)
{
    assert(!"sys_lchown32 not implemented");
    return 0;
}
long sys_getuid32(va_list ap)
{
    assert(!"sys_getuid32 not implemented");
    return 0;
}
long sys_getgid32(va_list ap)
{
    assert(!"sys_getgid32 not implemented");
    return 0;
}
long sys_geteuid32(va_list ap)
{
    assert(!"sys_geteuid32 not implemented");
    return 0;
}
long sys_getegid32(va_list ap)
{
    assert(!"sys_getegid32 not implemented");
    return 0;
}
long sys_setreuid32(va_list ap)
{
    assert(!"sys_setreuid32 not implemented");
    return 0;
}
long sys_setregid32(va_list ap)
{
    assert(!"sys_setregid32 not implemented");
    return 0;
}
long sys_getgroups32(va_list ap)
{
    assert(!"sys_getgroups32 not implemented");
    return 0;
}
long sys_setgroups32(va_list ap)
{
    assert(!"sys_setgroups32 not implemented");
    return 0;
}
long sys_fchown32(va_list ap)
{
    assert(!"sys_fchown32 not implemented");
    return 0;
}
long sys_setresuid32(va_list ap)
{
    assert(!"sys_setresuid32 not implemented");
    return 0;
}
long sys_getresuid32(va_list ap)
{
    assert(!"sys_getresuid32 not implemented");
    return 0;
}
long sys_setresgid32(va_list ap)
{
    assert(!"sys_setresgid32 not implemented");
    return 0;
}
long sys_getresgid32(va_list ap)
{
    assert(!"sys_getresgid32 not implemented");
    return 0;
}
long sys_chown32(va_list ap)
{
    assert(!"sys_chown32 not implemented");
    return 0;
}
long sys_setuid32(va_list ap)
{
    assert(!"sys_setuid32 not implemented");
    return 0;
}
long sys_setgid32(va_list ap)
{
    assert(!"sys_setgid32 not implemented");
    return 0;
}
long sys_setfsuid32(va_list ap)
{
    assert(!"sys_setfsuid32 not implemented");
    return 0;
}
long sys_setfsgid32(va_list ap)
{
    assert(!"sys_setfsgid32 not implemented");
    return 0;
}
long sys_getdents64(va_list ap)
{
    assert(!"sys_getdents64 not implemented");
    return 0;
}
long sys_pivot_root(va_list ap)
{
    assert(!"sys_pivot_root not implemented");
    return 0;
}
long sys_mincore(va_list ap)
{
    assert(!"sys_mincore not implemented");
    return 0;
}
long sys_madvise(va_list ap)
{
    assert(!"sys_madvise not implemented");
    return 0;
}
long sys_fcntl64(va_list ap)
{
    assert(!"sys_fcntl64 not implemented");
    return 0;
}
/*long sys_gettid(va_list ap)
{
    assert(!"sys_gettid not implemented");
    return 0;
}*/
long sys_readahead(va_list ap)
{
    assert(!"sys_readahead not implemented");
    return 0;
}
long sys_setxattr(va_list ap)
{
    assert(!"sys_setxattr not implemented");
    return 0;
}
long sys_lsetxattr(va_list ap)
{
    assert(!"sys_lsetxattr not implemented");
    return 0;
}
long sys_fsetxattr(va_list ap)
{
    assert(!"sys_fsetxattr not implemented");
    return 0;
}
long sys_getxattr(va_list ap)
{
    assert(!"sys_getxattr not implemented");
    return 0;
}
long sys_lgetxattr(va_list ap)
{
    assert(!"sys_lgetxattr not implemented");
    return 0;
}
long sys_fgetxattr(va_list ap)
{
    assert(!"sys_fgetxattr not implemented");
    return 0;
}
long sys_listxattr(va_list ap)
{
    assert(!"sys_listxattr not implemented");
    return 0;
}
long sys_llistxattr(va_list ap)
{
    assert(!"sys_llistxattr not implemented");
    return 0;
}
long sys_flistxattr(va_list ap)
{
    assert(!"sys_flistxattr not implemented");
    return 0;
}
long sys_removexattr(va_list ap)
{
    assert(!"sys_removexattr not implemented");
    return 0;
}
long sys_lremovexattr(va_list ap)
{
    assert(!"sys_lremovexattr not implemented");
    return 0;
}
long sys_fremovexattr(va_list ap)
{
    assert(!"sys_fremovexattr not implemented");
    return 0;
}
long sys_tkill(va_list ap)
{
    assert(!"sys_tkill not implemented");
    return 0;
}
long sys_sendfile64(va_list ap)
{
    assert(!"sys_sendfile64 not implemented");
    return 0;
}
long sys_futex(va_list ap)
{
    assert(!"sys_futex not implemented");
    return 0;
}
long sys_sched_setaffinity(va_list ap)
{
    assert(!"sys_sched_setaffinity not implemented");
    return 0;
}
long sys_sched_getaffinity(va_list ap)
{
    assert(!"sys_sched_getaffinity not implemented");
    return 0;
}
long sys_io_setup(va_list ap)
{
    assert(!"sys_io_setup not implemented");
    return 0;
}
long sys_io_destroy(va_list ap)
{
    assert(!"sys_io_destroy not implemented");
    return 0;
}
long sys_io_getevents(va_list ap)
{
    assert(!"sys_io_getevents not implemented");
    return 0;
}
long sys_io_submit(va_list ap)
{
    assert(!"sys_io_submit not implemented");
    return 0;
}
long sys_io_cancel(va_list ap)
{
    assert(!"sys_io_cancel not implemented");
    return 0;
}
/*long sys_exit_group(va_list ap)
{
    assert(!"sys_exit_group not implemented");
    return 0;
}*/
long sys_lookup_dcookie(va_list ap)
{
    assert(!"sys_lookup_dcookie not implemented");
    return 0;
}
long sys_epoll_create(va_list ap)
{
    assert(!"sys_epoll_create not implemented");
    return 0;
}
long sys_epoll_ctl(va_list ap)
{
    assert(!"sys_epoll_ctl not implemented");
    return 0;
}
long sys_epoll_wait(va_list ap)
{
    assert(!"sys_epoll_wait not implemented");
    return 0;
}
long sys_remap_file_pages(va_list ap)
{
    assert(!"sys_remap_file_pages not implemented");
    return 0;
}
long sys_timer_create(va_list ap)
{
    assert(!"sys_timer_create not implemented");
    return 0;
}
long sys_timer_settime(va_list ap)
{
    assert(!"sys_timer_settime not implemented");
    return 0;
}
long sys_timer_gettime(va_list ap)
{
    assert(!"sys_timer_gettime not implemented");
    return 0;
}
long sys_timer_getoverrun(va_list ap)
{
    assert(!"sys_timer_getoverrun not implemented");
    return 0;
}
long sys_timer_delete(va_list ap)
{
    assert(!"sys_timer_delete not implemented");
    return 0;
}
long sys_clock_settime(va_list ap)
{
    assert(!"sys_clock_settime not implemented");
    return 0;
}
long sys_clock_gettime(va_list ap)
{
    assert(!"sys_clock_gettime not implemented");
    return 0;
}
long sys_clock_getres(va_list ap)
{
    assert(!"sys_clock_getres not implemented");
    return 0;
}
long sys_clock_nanosleep(va_list ap)
{
    assert(!"sys_clock_nanosleep not implemented");
    return 0;
}
long sys_statfs64(va_list ap)
{
    assert(!"sys_statfs64 not implemented");
    return 0;
}
long sys_fstatfs64(va_list ap)
{
    assert(!"sys_fstatfs64 not implemented");
    return 0;
}
/*long sys_tgkill(va_list ap)
{
    assert(!"sys_tgkill not implemented");
    return 0;
}*/
long sys_utimes(va_list ap)
{
    assert(!"sys_utimes not implemented");
    return 0;
}
long sys_fadvise64_64(va_list ap)
{
    assert(!"sys_fadvise64_64 not implemented");
    return 0;
}
long sys_pciconfig_iobase(va_list ap)
{
    assert(!"sys_pciconfig_iobase not implemented");
    return 0;
}
long sys_pciconfig_read(va_list ap)
{
    assert(!"sys_pciconfig_read not implemented");
    return 0;
}
long sys_pciconfig_write(va_list ap)
{
    assert(!"sys_pciconfig_write not implemented");
    return 0;
}
long sys_mq_open(va_list ap)
{
    assert(!"sys_mq_open not implemented");
    return 0;
}
long sys_mq_unlink(va_list ap)
{
    assert(!"sys_mq_unlink not implemented");
    return 0;
}
long sys_mq_timedsend(va_list ap)
{
    assert(!"sys_mq_timedsend not implemented");
    return 0;
}
long sys_mq_timedreceive(va_list ap)
{
    assert(!"sys_mq_timedreceive not implemented");
    return 0;
}
long sys_mq_notify(va_list ap)
{
    assert(!"sys_mq_notify not implemented");
    return 0;
}
long sys_mq_getsetattr(va_list ap)
{
    assert(!"sys_mq_getsetattr not implemented");
    return 0;
}
long sys_waitid(va_list ap)
{
    assert(!"sys_waitid not implemented");
    return 0;
}
long sys_socket(va_list ap)
{
    assert(!"sys_socket not implemented");
    return 0;
}
long sys_bind(va_list ap)
{
    assert(!"sys_bind not implemented");
    return 0;
}
long sys_connect(va_list ap)
{
    assert(!"sys_connect not implemented");
    return 0;
}
long sys_listen(va_list ap)
{
    assert(!"sys_listen not implemented");
    return 0;
}
long sys_accept(va_list ap)
{
    assert(!"sys_accept not implemented");
    return 0;
}
long sys_getsockname(va_list ap)
{
    assert(!"sys_getsockname not implemented");
    return 0;
}
long sys_getpeername(va_list ap)
{
    assert(!"sys_getpeername not implemented");
    return 0;
}
long sys_socketpair(va_list ap)
{
    assert(!"sys_socketpair not implemented");
    return 0;
}
long sys_send(va_list ap)
{
    assert(!"sys_send not implemented");
    return 0;
}
long sys_sendto(va_list ap)
{
    assert(!"sys_sendto not implemented");
    return 0;
}
long sys_recv(va_list ap)
{
    assert(!"sys_recv not implemented");
    return 0;
}
long sys_recvfrom(va_list ap)
{
    assert(!"sys_recvfrom not implemented");
    return 0;
}
long sys_shutdown(va_list ap)
{
    assert(!"sys_shutdown not implemented");
    return 0;
}
long sys_setsockopt(va_list ap)
{
    assert(!"sys_setsockopt not implemented");
    return 0;
}
long sys_getsockopt(va_list ap)
{
    assert(!"sys_getsockopt not implemented");
    return 0;
}
long sys_sendmsg(va_list ap)
{
    assert(!"sys_sendmsg not implemented");
    return 0;
}
long sys_recvmsg(va_list ap)
{
    assert(!"sys_recvmsg not implemented");
    return 0;
}
long sys_semop(va_list ap)
{
    assert(!"sys_semop not implemented");
    return 0;
}
long sys_semget(va_list ap)
{
    assert(!"sys_semget not implemented");
    return 0;
}
long sys_semctl(va_list ap)
{
    assert(!"sys_semctl not implemented");
    return 0;
}
long sys_msgsnd(va_list ap)
{
    assert(!"sys_msgsnd not implemented");
    return 0;
}
long sys_msgrcv(va_list ap)
{
    assert(!"sys_msgrcv not implemented");
    return 0;
}
long sys_msgget(va_list ap)
{
    assert(!"sys_msgget not implemented");
    return 0;
}
long sys_msgctl(va_list ap)
{
    assert(!"sys_msgctl not implemented");
    return 0;
}
long sys_shmat(va_list ap)
{
    assert(!"sys_shmat not implemented");
    return 0;
}
long sys_shmdt(va_list ap)
{
    assert(!"sys_shmdt not implemented");
    return 0;
}
long sys_shmget(va_list ap)
{
    assert(!"sys_shmget not implemented");
    return 0;
}
long sys_shmctl(va_list ap)
{
    assert(!"sys_shmctl not implemented");
    return 0;
}
long sys_add_key(va_list ap)
{
    assert(!"sys_add_key not implemented");
    return 0;
}
long sys_request_key(va_list ap)
{
    assert(!"sys_request_key not implemented");
    return 0;
}
long sys_keyctl(va_list ap)
{
    assert(!"sys_keyctl not implemented");
    return 0;
}
long sys_semtimedop(va_list ap)
{
    assert(!"sys_semtimedop not implemented");
    return 0;
}
long sys_vserver(va_list ap)
{
    assert(!"sys_vserver not implemented");
    return 0;
}
long sys_ioprio_set(va_list ap)
{
    assert(!"sys_ioprio_set not implemented");
    return 0;
}
long sys_ioprio_get(va_list ap)
{
    assert(!"sys_ioprio_get not implemented");
    return 0;
}
long sys_inotify_init(va_list ap)
{
    assert(!"sys_inotify_init not implemented");
    return 0;
}
long sys_inotify_add_watch(va_list ap)
{
    assert(!"sys_inotify_add_watch not implemented");
    return 0;
}
long sys_inotify_rm_watch(va_list ap)
{
    assert(!"sys_inotify_rm_watch not implemented");
    return 0;
}
long sys_mbind(va_list ap)
{
    assert(!"sys_mbind not implemented");
    return 0;
}
long sys_get_mempolicy(va_list ap)
{
    assert(!"sys_get_mempolicy not implemented");
    return 0;
}
long sys_set_mempolicy(va_list ap)
{
    assert(!"sys_set_mempolicy not implemented");
    return 0;
}
long sys_openat(va_list ap)
{
    assert(!"sys_openat not implemented");
    return 0;
}
long sys_mkdirat(va_list ap)
{
    assert(!"sys_mkdirat not implemented");
    return 0;
}
long sys_mknodat(va_list ap)
{
    assert(!"sys_mknodat not implemented");
    return 0;
}
long sys_fchownat(va_list ap)
{
    assert(!"sys_fchownat not implemented");
    return 0;
}
long sys_futimesat(va_list ap)
{
    assert(!"sys_futimesat not implemented");
    return 0;
}
long sys_fstatat64(va_list ap)
{
    assert(!"sys_fstatat64 not implemented");
    return 0;
}
long sys_unlinkat(va_list ap)
{
    assert(!"sys_unlinkat not implemented");
    return 0;
}
long sys_renameat(va_list ap)
{
    assert(!"sys_renameat not implemented");
    return 0;
}
long sys_linkat(va_list ap)
{
    assert(!"sys_linkat not implemented");
    return 0;
}
long sys_symlinkat(va_list ap)
{
    assert(!"sys_symlinkat not implemented");
    return 0;
}
long sys_readlinkat(va_list ap)
{
    assert(!"sys_readlinkat not implemented");
    return 0;
}
long sys_fchmodat(va_list ap)
{
    assert(!"sys_fchmodat not implemented");
    return 0;
}
long sys_faccessat(va_list ap)
{
    assert(!"sys_faccessat not implemented");
    return 0;
}
long sys_pselect6(va_list ap)
{
    assert(!"sys_pselect6 not implemented");
    return 0;
}
long sys_ppoll(va_list ap)
{
    assert(!"sys_ppoll not implemented");
    return 0;
}
long sys_unshare(va_list ap)
{
    assert(!"sys_unshare not implemented");
    return 0;
}
long sys_set_robust_list(va_list ap)
{
    assert(!"sys_set_robust_list not implemented");
    return 0;
}
long sys_get_robust_list(va_list ap)
{
    assert(!"sys_get_robust_list not implemented");
    return 0;
}
long sys_splice(va_list ap)
{
    assert(!"sys_splice not implemented");
    return 0;
}
long sys_sync_file_range2(va_list ap)
{
    assert(!"sys_sync_file_range2 not implemented");
    return 0;
}
long sys_tee(va_list ap)
{
    assert(!"sys_tee not implemented");
    return 0;
}
long sys_vmsplice(va_list ap)
{
    assert(!"sys_vmsplice not implemented");
    return 0;
}
long sys_move_pages(va_list ap)
{
    assert(!"sys_move_pages not implemented");
    return 0;
}
long sys_getcpu(va_list ap)
{
    assert(!"sys_getcpu not implemented");
    return 0;
}
long sys_epoll_pwait(va_list ap)
{
    assert(!"sys_epoll_pwait not implemented");
    return 0;
}
long sys_kexec_load(va_list ap)
{
    assert(!"sys_kexec_load not implemented");
    return 0;
}
long sys_utimensat(va_list ap)
{
    assert(!"sys_utimensat not implemented");
    return 0;
}
long sys_signalfd(va_list ap)
{
    assert(!"sys_signalfd not implemented");
    return 0;
}
long sys_timerfd_create(va_list ap)
{
    assert(!"sys_timerfd_create not implemented");
    return 0;
}
long sys_eventfd(va_list ap)
{
    assert(!"sys_eventfd not implemented");
    return 0;
}
long sys_fallocate(va_list ap)
{
    assert(!"sys_fallocate not implemented");
    return 0;
}
long sys_timerfd_settime(va_list ap)
{
    assert(!"sys_timerfd_settime not implemented");
    return 0;
}
long sys_timerfd_gettime(va_list ap)
{
    assert(!"sys_timerfd_gettime not implemented");
    return 0;
}
long sys_signalfd4(va_list ap)
{
    assert(!"sys_signalfd4 not implemented");
    return 0;
}
long sys_eventfd2(va_list ap)
{
    assert(!"sys_eventfd2 not implemented");
    return 0;
}
long sys_epoll_create1(va_list ap)
{
    assert(!"sys_epoll_create1 not implemented");
    return 0;
}
long sys_dup3(va_list ap)
{
    assert(!"sys_dup3 not implemented");
    return 0;
}
long sys_pipe2(va_list ap)
{
    assert(!"sys_pipe2 not implemented");
    return 0;
}
long sys_inotify_init1(va_list ap)
{
    assert(!"sys_inotify_init1 not implemented");
    return 0;
}
long sys_preadv(va_list ap)
{
    assert(!"sys_preadv not implemented");
    return 0;
}
long sys_pwritev(va_list ap)
{
    assert(!"sys_pwritev not implemented");
    return 0;
}
long sys_rt_tgsigqueueinfo(va_list ap)
{
    assert(!"sys_rt_tgsigqueueinfo not implemented");
    return 0;
}
long sys_perf_event_open(va_list ap)
{
    assert(!"sys_perf_event_open not implemented");
    return 0;
}
long sys_recvmmsg(va_list ap)
{
    assert(!"sys_recvmmsg not implemented");
    return 0;
}
long sys_accept4(va_list ap)
{
    assert(!"sys_accept4 not implemented");
    return 0;
}
long sys_fanotify_init(va_list ap)
{
    assert(!"sys_fanotify_init not implemented");
    return 0;
}
long sys_fanotify_mark(va_list ap)
{
    assert(!"sys_fanotify_mark not implemented");
    return 0;
}
long sys_name_to_handle_at(va_list ap)
{
    assert(!"sys_name_to_handle_at not implemented");
    return 0;
}
long sys_open_by_handle_at(va_list ap)
{
    assert(!"sys_open_by_handle_at not implemented");
    return 0;
}
long sys_clock_adjtime(va_list ap)
{
    assert(!"sys_clock_adjtime not implemented");
    return 0;
}
long sys_syncfs(va_list ap)
{
    assert(!"sys_syncfs not implemented");
    return 0;
}
long sys_sendmmsg(va_list ap)
{
    assert(!"sys_sendmmsg not implemented");
    return 0;
}
long sys_setns(va_list ap)
{
    assert(!"sys_setns not implemented");
    return 0;
}
long sys_process_vm_readv(va_list ap)
{
    assert(!"sys_process_vm_readv not implemented");
    return 0;
}
long sys_process_vm_writev(va_list ap)
{
    assert(!"sys_process_vm_writev not implemented");
    return 0;
}

#endif
