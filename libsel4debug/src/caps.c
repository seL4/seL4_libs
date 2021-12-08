/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4debug/gen_config.h>
#include <sel4debug/debug.h>
#include <sel4/sel4.h>
#include <stdio.h>

void debug_cap_identify(seL4_CPtr cap)
{
#ifdef CONFIG_DEBUG_BUILD
    int type = seL4_DebugCapIdentify(cap);
    printf("Cap %"SEL4_PRIu_word" has type %d\n", cap, type);
#else
    printf("DEBUG_BUILD not set, can't get type of cap %"SEL4_PRIu_word"\n", cap);
#endif
}
