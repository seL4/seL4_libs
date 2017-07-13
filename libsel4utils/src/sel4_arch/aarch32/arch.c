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
#include <sel4/types.h>
#include <sel4utils/thread.h>
#include <sel4utils/helpers.h>
#include <utils/zf_log.h>
#include <utils/stack.h>
#include <stdbool.h>

int
sel4utils_arch_init_context(void *entry_point, void *stack_top, seL4_UserContext *context)
{
    context->pc = (seL4_Word) entry_point;
    context->sp = (seL4_Word) stack_top;
    if (!IS_ALIGNED((uintptr_t)stack_top, STACK_CALL_ALIGNMENT_BITS)) {
        ZF_LOGE("Initial stack pointer must be %d byte aligned", STACK_CALL_ALIGNMENT);
        return -1;
    }
    return 0;
}

int
sel4utils_arch_init_context_with_args(sel4utils_thread_entry_fn entry_point,
                                      void *arg0, void *arg1, void *arg2,
                                      bool local_stack, void *stack_top,
                                      seL4_UserContext *context,
                                      vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace)
{

    context->r0 = (seL4_Word) arg0;
    context->r1 = (seL4_Word) arg1;
    context->r2 = (seL4_Word) arg2;

    return sel4utils_arch_init_context(entry_point, stack_top, context);
}
