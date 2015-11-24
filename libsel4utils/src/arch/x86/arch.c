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
#include <stdbool.h>

#include "../../helpers.h"

int 
sel4utils_arch_init_context(void *entry_point, void *arg0, void *arg1, void *arg2,
                            bool local_stack, void *stack_top, seL4_UserContext *context, 
                            vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace)
{
    
    if (local_stack) {
        seL4_Word *stack_ptr = (seL4_Word *) stack_top;
        stack_ptr[-5] = (seL4_Word) arg0;
        stack_ptr[-4] = (seL4_Word) arg1;
        stack_ptr[-3] = (seL4_Word) arg2;
        stack_top = (void *) ((uintptr_t) stack_top - 24u);
    } else if (local_vspace && remote_vspace && vka) {
        seL4_Word stack_args[6] = {0, (seL4_Word) arg0, (seL4_Word) arg1, (seL4_Word) arg2, 0, 0};
        int error = sel4utils_stack_write(local_vspace, remote_vspace, vka, stack_args, sizeof(stack_args), 
                                          (uintptr_t *) &stack_top);
        if (error) {
            return -1;
        }
    }
    
    context->esp = (seL4_Word) stack_top;
    context->edx = 0;
    context->gs  = IPCBUF_GDT_SELECTOR;
    context->eip = (seL4_Word) entry_point;

    return 0;
}


