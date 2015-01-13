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

#ifdef CONFIG_LIB_SEL4_VSPACE

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/thread.h>
#include <sel4utils/util.h>
#include <sel4utils/arch/util.h>

#include "helpers.h"

static int
write_ipc_buffer_user_data(vka_t *vka, vspace_t *vspace, seL4_CPtr ipc_buf, uintptr_t buf_loc)
{
    void *mapping = sel4utils_dup_and_map(vka, vspace, ipc_buf, seL4_PageBits);
    if (!mapping) {
        return -1;
    }
    seL4_IPCBuffer *buffer = (seL4_IPCBuffer*)mapping;
    buffer->userData = buf_loc;
    sel4utils_unmap_dup(vka, vspace, mapping, seL4_PageBits);
    return 0;
}

int sel4utils_configure_thread(vka_t *vka, vspace_t *parent, vspace_t *alloc, seL4_CPtr fault_endpoint,
                               uint8_t priority, seL4_CNode cspace, seL4_CapData_t cspace_root_data, sel4utils_thread_t *res)
{

    sel4utils_thread_config_t config = {
        .fault_endpoint = fault_endpoint,
        .priority = priority,
        .cspace = cspace,
        .cspace_root_data = cspace_root_data,
    };

    return sel4utils_configure_thread_config(vka, parent, alloc, config, res);
}


int
sel4utils_configure_thread_config(vka_t *vka, vspace_t *parent, vspace_t *alloc,
                                  sel4utils_thread_config_t config, sel4utils_thread_t *res)
{
    memset(res, 0, sizeof(sel4utils_thread_t));

    int error = vka_alloc_tcb(vka, &res->tcb);
    if (error == -1) {
        LOG_ERROR("vka_alloc tcb failed");
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    res->ipc_buffer_addr = (seL4_Word) vspace_new_ipc_buffer(alloc, &res->ipc_buffer);

    if (res->ipc_buffer_addr == 0) {
        LOG_ERROR("ipc buffer allocation failed");
        return -1;
    }

    if (write_ipc_buffer_user_data(vka, parent, res->ipc_buffer, res->ipc_buffer_addr)) {
        LOG_ERROR("failed to set user data word in IPC buffer");
        return -1;
    }

    seL4_CapData_t null_cap_data = {{0}};
    error = seL4_TCB_Configure(res->tcb.cptr, config.fault_endpoint, config.priority, config.cspace,
                               config.cspace_root_data, vspace_get_root(alloc), null_cap_data, res->ipc_buffer_addr, res->ipc_buffer);

    if (error != seL4_NoError) {
        LOG_ERROR("TCB configure failed with seL4 error code %d", error);
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    res->stack_top = vspace_new_stack(alloc);

    if (res->stack_top == NULL) {
        LOG_ERROR("Stack allocation failed!");
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    return 0;
}

int
sel4utils_internal_start_thread(sel4utils_thread_t *thread, void *entry_point, void *arg0,
                                void *arg1, int resume, void *local_stack_top, void *dest_stack_top)
{
    seL4_UserContext context = {0};
    size_t context_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    int error;
    /* one of these must be set */
    assert(local_stack_top || dest_stack_top);
    /* but not both of them */
    assert(!(local_stack_top && dest_stack_top));

#ifdef ARCH_IA32

#ifdef CONFIG_X86_64
    context.rdi = (seL4_Word)arg0;
    context.rsi = (seL4_Word)arg1;
    context.rdx = (seL4_Word)thread->ipc_buffer_addr;
    if (dest_stack_top) {
        context.rsp = (seL4_Word) dest_stack_top;
    } else {
        context.rsp = (seL4_Word) thread->stack_top;
    }
    context.gs = IPCBUF_GDT_SELECTOR;
    context.rip = (seL4_Word)entry_point;
#else
    if (local_stack_top) {
        seL4_Word *stack_ptr = (seL4_Word *) local_stack_top;
        stack_ptr[-5] = (seL4_Word) arg0;
        stack_ptr[-4] = (seL4_Word) arg1;
        stack_ptr[-3] = (seL4_Word) thread->ipc_buffer_addr;
        context.esp = (seL4_Word) (thread->stack_top - 24);
    } else {
        context.esp = (seL4_Word) dest_stack_top;
    }
    context.gs = IPCBUF_GDT_SELECTOR;
    context.eip = (seL4_Word)entry_point;
#endif

    /* check that stack is aligned correctly */
    assert((seL4_Word) thread->stack_top % (sizeof(seL4_Word) * 2) == 0);
    error = seL4_TCB_WriteRegisters(thread->tcb.cptr, resume, 0, context_size, &context);
#elif ARCH_ARM /* ARCH_IA32 */
    context.pc = (seL4_Word) entry_point;
    if (dest_stack_top) {
        context.sp = (seL4_Word) dest_stack_top;
    } else {
        context.sp = (seL4_Word) thread->stack_top;
    }
    context.r0 = (seL4_Word) arg0;
    context.r1 = (seL4_Word) arg1;
    context.r2 = (seL4_Word) thread->ipc_buffer_addr;

    /* check that stack is aligned correctly */
    assert((uint32_t) thread->stack_top % (sizeof(seL4_Word) * 2) == 0);
    error = seL4_TCB_WriteRegisters(thread->tcb.cptr, resume, 0, context_size, &context);
#endif /* ARCH_ARM */

    if (error) {
        LOG_ERROR("seL4_TCB_WriteRegisters failed with error %d", error);
        return -1;
    }

    return 0;
}


int
sel4utils_start_thread(sel4utils_thread_t *thread, void *entry_point, void *arg0, void *arg1,
                       int resume)
{
    return sel4utils_internal_start_thread(thread, entry_point, arg0, arg1, resume, thread->stack_top, NULL);
}

void
sel4utils_clean_up_thread(vka_t *vka, vspace_t *alloc, sel4utils_thread_t *thread)
{
    if (thread->tcb.cptr != 0) {
        vka_free_object(vka, &thread->tcb);
    }

    if (thread->ipc_buffer_addr != 0) {
        vspace_free_ipc_buffer(alloc, (seL4_Word *) thread->ipc_buffer_addr);
    }

    if (thread->stack_top != 0) {
        vspace_free_stack(alloc, thread->stack_top);
    }

    memset(thread, 0, sizeof(sel4utils_thread_t));
}

void
sel4utils_print_fault_message(seL4_MessageInfo_t tag, const char *thread_name)
{
    switch (seL4_MessageInfo_get_label(tag)) {
    case SEL4_PFIPC_LABEL:
        assert(seL4_MessageInfo_get_length(tag) == SEL4_PFIPC_LENGTH);
        printf("%sPagefault from [%s]: %s %s at PC: %p vaddr: %p%s\n",
               COLOR_ERROR,
               thread_name,
               sel4utils_is_read_fault() ? "read" : "write",
               seL4_GetMR(SEL4_PFIPC_PREFETCH_FAULT) ? "prefetch fault" : "fault",
               (void*)seL4_GetMR(SEL4_PFIPC_FAULT_IP),
               (void*)seL4_GetMR(SEL4_PFIPC_FAULT_ADDR),
               COLOR_NORMAL);
        break;

    case SEL4_EXCEPT_IPC_LABEL:
        assert(seL4_MessageInfo_get_length(tag) == SEL4_EXCEPT_IPC_LENGTH);
        printf("%sBad syscall from [%s]: scno %"PRIuPTR" at PC: %p%s\n",
               COLOR_ERROR,
               thread_name,
               seL4_GetMR(EXCEPT_IPC_SYS_MR_SYSCALL),
               (void*)seL4_GetMR(EXCEPT_IPC_SYS_MR_IP),
               COLOR_NORMAL
              );

        break;

    case SEL4_USER_EXCEPTION_LABEL:
        assert(seL4_MessageInfo_get_length(tag) == SEL4_USER_EXCEPTION_LENGTH);
        printf("%sInvalid instruction from [%s] at PC: %p%s\n",
               COLOR_ERROR,
               thread_name,
               (void*)seL4_GetMR(0),
               COLOR_NORMAL);
        break;

    default:
        /* What? Why are we here? What just happened? */
        printf("Unknown fault from [%s]: %"PRIuPTR" (length = %"PRIuPTR")\n", thread_name, seL4_MessageInfo_get_label(tag), seL4_MessageInfo_get_length(tag));
        break;
    }
}


static int
fault_handler(char *name, seL4_CPtr endpoint)
{
    seL4_Word badge;
    seL4_MessageInfo_t info = seL4_Wait(endpoint, &badge);

    sel4utils_print_fault_message(info, name);

    /* go back to sleep so other things can run */
    seL4_Wait(endpoint, &badge);

    return 0;
}

int
sel4utils_start_fault_handler(seL4_CPtr fault_endpoint, vka_t *vka, vspace_t *vspace,
                              uint8_t prio, seL4_CPtr cspace, seL4_CapData_t cap_data, char *name,
                              sel4utils_thread_t *res)
{
    int error = sel4utils_configure_thread(vka, vspace, vspace, 0, prio, cspace,
                                           cap_data, res);

    if (error) {
        LOG_ERROR("Failed to configure fault handling thread\n");
        return -1;
    }

    return sel4utils_start_thread(res, fault_handler, (void *) name,
                                  (void *) fault_endpoint, 1);
}

#endif /* CONFIG_LIB_SEL4_VSPACE */
