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

#include <autoconf.h>
#include <sel4debug/debug.h>
#include <sel4/sel4.h>
#include <stdio.h>

void
debug_cap_identify(seL4_CPtr cap)
{
#ifdef CONFIG_DEBUG_BUILD
    int type = seL4_DebugCapIdentify(cap);

    printf("Cap type number is %d\n", type);
#endif /* CONFIG_DEBUG_BUILD */
}
