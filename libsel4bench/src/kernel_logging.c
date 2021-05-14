/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4bench/kernel_logging.h>

#if CONFIG_MAX_NUM_TRACE_POINTS > 0
unsigned int
kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n)
{
    return 0;
}
#endif
