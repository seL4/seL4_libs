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
                            vka_t *vka, vspace_t *local_vspace, vspace_t *remote_vspace);
{
    
    context->rdi = (seL4_Word) arg0;
    context->rsi = (seL4_Word) arg1;
    context->rdx = (seL4_Word) arg2;
    context->esp = (seL4_Word) stack_top;
    context->gs = IPCBUF_GDT_SELECTOR;
    context->eip = (seL4_Word) entry_point;

    return 0;
}


