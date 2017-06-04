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
    context->esp = (seL4_Word) stack_top;
    /* set edx to zero in case we are setting this when spawning a process as
     * edx is the atexit parameter, which we currently do not use */
    context->edx = 0;
    context->fs  = IPCBUF_GDT_SELECTOR;
    context->eip = (seL4_Word) entry_point;
    context->gs = TLS_GDT_SELECTOR;

    return 0;
}

int
sel4utils_arch_init_context_with_args(sel4utils_thread_entry_fn entry_point, void *arg0, void *arg1, void *arg2,
                            bool local_stack, void *stack_top, seL4_UserContext *context,
                            vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace)
{

    if (!IS_ALIGNED((uintptr_t)stack_top, STACK_CALL_ALIGNMENT_BITS)) {
        ZF_LOGE("Initial stack pointer must be %d byte aligned", STACK_CALL_ALIGNMENT);
        return -1;
    }

    /* arguments as they should appear on the stack */
    seL4_Word stack_args[] = {(seL4_Word) arg0, (seL4_Word) arg1, (seL4_Word) arg2};
    if (stack_top) {
        /* if we were to increase the stack pointer such that after the arguments are
         * pushed, the stack pointer would be correctly aligned, we would add this
         * value */
        size_t up_padding = sizeof(stack_args) % STACK_CALL_ALIGNMENT;

        /* but we can't add to the stack pointer as we might run off the end of our stack,
         * so we'll decrease it by this value */
        size_t down_padding = (STACK_CALL_ALIGNMENT - up_padding) % STACK_CALL_ALIGNMENT;

        stack_top = (void *)((uintptr_t) stack_top - down_padding);
    }

    if (local_stack && stack_top) {
        seL4_Word *stack_ptr = (seL4_Word *) stack_top;
        stack_ptr[-3] = (seL4_Word) arg0;
        stack_ptr[-2] = (seL4_Word) arg1;
        stack_ptr[-1] = (seL4_Word) arg2;
        stack_top = (void *) ((uintptr_t) stack_top - sizeof(stack_args));
    } else if (local_vspace && remote_vspace && vka) {
        int error = sel4utils_stack_write(local_vspace, remote_vspace, vka, stack_args, sizeof(stack_args),
                                          (uintptr_t *) &stack_top);
        if (error) {
            ZF_LOGE("Failed to copy arguments");
            return -1;
        }
    }

    /* we've pushed the arguments, so at this point our stack should be aligned
     * thanks to our padding */
    assert(IS_ALIGNED((uintptr_t)stack_top, STACK_CALL_ALIGNMENT_BITS));

    /* the entry point function was compiled under the assumption that it will
     * be called, and thus expects a return address to follow the arguments */
    stack_top = (void *)((uintptr_t) stack_top - sizeof(uintptr_t));

    return sel4utils_arch_init_context(entry_point, stack_top, context);
}
