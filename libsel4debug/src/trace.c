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
#include <sel4debug/instrumentation.h>

#ifdef CONFIG_LIBSEL4DEBUG_FUNCTION_INSTRUMENTATION_TRACE

/* Function entry and exit printing. Useful for low effort tracing of
 * execution.
 *
 * Enable this in your app by including:
 *  NK_CFLAGS += -finstrument-functions \
 *    -finstrument-functions-exclude-function-list=seL4_MessageInfo_get_length
 * If you don't exclude seL4_MessageInfo_get_length, you will most likely find
 * that __seL4_*WithMRs will not function correctly because calls to
 * seL4_MessageInfo_get_length will be instrumented and clobber necessary
 * registers for the following syscall.
 */
void __cyg_profile_func_enter(void *func, void *caller)
{
    debug_safe_printf("ENTER: %p called from %p\n", func, caller);
}

void __cyg_profile_func_exit(void *func, void *caller)
{
    debug_safe_printf("EXIT: %p returning to %p\n", func, caller);
}

#endif
