/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4bench/kernel_logging.h>

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
unsigned int
kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n)
{
    return 0;
}
#endif
