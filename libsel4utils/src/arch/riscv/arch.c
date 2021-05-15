/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>
#include <sel4utils/gen_config.h>
#include <sel4/types.h>
#include <sel4utils/thread.h>
#include <stdbool.h>

int sel4utils_arch_init_context(void *entry_point, void *stack_top, seL4_UserContext *context)
{
    context->pc = (seL4_Word) entry_point;
    context->sp = (seL4_Word) stack_top;
    if ((uintptr_t) stack_top % (sizeof(seL4_Word) * 2) != 0) {
        ZF_LOGE("Stack %p not aligned on double word boundary", stack_top);
        return -1;
    }
    return 0;
}

int sel4utils_arch_init_context_with_args(void *entry_point, void *arg0, void *arg1, void *arg2,
                                          bool local_stack, void *stack_top, seL4_UserContext *context,
                                          vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace)
{
    extern char __global_pointer$[];

    context->a0 = (seL4_Word) arg0;
    context->a1 = (seL4_Word) arg1;
    context->a2 = (seL4_Word) arg2;
    context->gp = (seL4_Word) __global_pointer$;

    return sel4utils_arch_init_context(entry_point, stack_top, context);
}
