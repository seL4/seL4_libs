/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4bench/sel4bench.h>
#include <utils/util.h>

#include "../event_counters.h"

const char*
sel4bench_arch_get_counter_description(seL4_Word counter)
{
    ZF_LOGE("Not implemented");
    return NULL;
}

