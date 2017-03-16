/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <autoconf.h>
#include <sel4/types.h>
#include <sel4utils/thread.h>
#include <sel4utils/helpers.h>
#include <stdbool.h>

int
sel4utils_arch_init_context(void *entry_point, void *stack_top, seL4_UserContext *context)
{
    context->rsp = (seL4_Word) stack_top;
    /* set edx to zero in case we are setting this when spawning a process as
     * edx is the atexit parameter, which we currently do not use */
    context->rdx = 0;
    context->rip = (seL4_Word) entry_point;
    return 0;
}

int
sel4utils_arch_init_context_with_args(sel4utils_thread_entry_fn entry_point, void *arg0, void *arg1, void *arg2,
                            bool local_stack, void *stack_top, seL4_UserContext *context,
                            vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace)
{
    sel4utils_arch_init_context(entry_point, stack_top, context);
    context->rdi = (seL4_Word) arg0;
    context->rsi = (seL4_Word) arg1;
    /* if we are setting args then we must not be spawning a process, therefore
     * even though rdx was set to zero by sel4utils_arch_init_context we can
     * safely put the arg in here */
    context->rdx = (seL4_Word) arg2;
    return 0;
}

