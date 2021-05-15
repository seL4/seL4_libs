/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4muslcsys/gen_config.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <stdlib.h>
#include <stdarg.h>
#include <utils/util.h>

static void sel4_abort(void)
{
#if defined(CONFIG_DEBUG_BUILD) && defined(CONFIG_LIB_SEL4_MUSLC_SYS_DEBUG_HALT)
    printf("seL4 root server abort()ed\n");
    seL4_DebugHalt();
#endif
    while (1); /* We don't return after this */
}

long sys_exit(va_list ap)
{
    abort();
    return 0;
}

long sys_rt_sigprocmask(va_list ap)
{
    ZF_LOGV("Ignoring call to %s", __FUNCTION__);
    return 0;
}

long sys_gettid(va_list ap)
{
    ZF_LOGV("Ignoring call to %s", __FUNCTION__);
    return 0;
}

long sys_getpid(va_list ap)
{
    ZF_LOGV("Ignoring call to %s", __FUNCTION__);
    return 0;
}

long sys_tgkill(va_list ap)
{
    ZF_LOGV("%s assuming self kill", __FUNCTION__);
    sel4_abort();
    return 0;
}

long sys_tkill(va_list ap)
{
    ZF_LOGV("%s assuming self kill", __FUNCTION__);
    sel4_abort();
    return 0;
}

long sys_exit_group(va_list ap)
{
    ZF_LOGV("Ignoring call to %s", __FUNCTION__);
    return 0;
}
