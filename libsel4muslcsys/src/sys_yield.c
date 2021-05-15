/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sel4/sel4.h>

long sys_sched_yield(va_list ap)
{
    seL4_Yield();
    return 0;
}
