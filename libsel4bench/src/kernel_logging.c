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

#include <sel4bench/kernel_logging.h>

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
unsigned int
kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n)
{
    return 0;
}
#endif
