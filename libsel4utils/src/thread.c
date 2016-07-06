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
    seL4_IPCBuffer *buffer = mapping;
    buffer->userData = buf_loc;
    sel4utils_unmap_dup(vka, vspace, mapping, seL4_PageBits);
    return 0;
}

int sel4utils_configure_thread(simple_t *simple, vka_t *vka, vspace_t *parent, vspace_t *alloc, 
                               seL4_CPtr fault_endpoint,
                               uint8_t priority, seL4_CNode cspace, 
                               seL4_CapData_t cspace_root_data, sel4utils_thread_t *res)
{

    sel4utils_thread_config_t config = {
        .fault_endpoint = fault_endpoint,
        .priority = priority,
        .mcp = priority,
        .criticality = seL4_MinCrit,
        .mcc = seL4_MinCrit,
        .cspace = cspace,
        .cspace_root_data = cspace_root_data,
        .create_sc = true,
    };

    return sel4utils_configure_thread_config(simple, vka, parent, alloc, config, res);
}


int
sel4utils_configure_thread_config(simple_t *simple, vka_t *vka, vspace_t *parent, vspace_t *alloc,
                                  sel4utils_thread_config_t config, sel4utils_thread_t *res)
{
    memset(res, 0, sizeof(sel4utils_thread_t));

    int error = vka_alloc_tcb(vka, &res->tcb);
    if (error == -1) {
        ZF_LOGE("vka_alloc tcb failed");
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    if (!config.no_ipc_buffer) {
        res->ipc_buffer_addr = (seL4_Word) vspace_new_ipc_buffer(alloc, &res->ipc_buffer);

        if (res->ipc_buffer_addr == 0) {
            ZF_LOGE("ipc buffer allocation failed");
            return -1;
        }

        if (write_ipc_buffer_user_data(vka, parent, res->ipc_buffer, res->ipc_buffer_addr)) {
            ZF_LOGE("failed to set user data word in IPC buffer");
            return -1;
        }
    }

    if (config.create_sc) {
        error = vka_alloc_sched_context(vka, &res->sched_context);
        if (error) {
            ZF_LOGE("Failed to allocate sched context");
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }

        seL4_Time budget = CONFIG_SEL4UTILS_TIMESLICE_US;
        seL4_Time period = CONFIG_SEL4UTILS_TIMESLICE_US;
        if (config.custom_sched_params) {
            budget = config.custom_budget;
            period = config.custom_period;
        }

        error = seL4_SchedControl_Configure(simple_get_sched_ctrl(simple), 
                                            res->sched_context.cptr, budget, period, 0);
        if (error) {
            ZF_LOGW("Failed to configure sched context");
            vka_free_object(vka, &res->sched_context);
            res->sched_context.cptr = seL4_CapNull;
            res->own_sc = false;
        } else {
            res->own_sc = true;
        }
    } else {
        res->sched_context.cptr = config.sched_context;
        res->own_sc = false;
    }

    seL4_CapData_t null_cap_data = {{0}};
    seL4_Prio_t prio = seL4_Prio_new(config.priority, config.mcp, config.criticality, config.mcc);
    error = seL4_TCB_Configure(res->tcb.cptr, config.fault_endpoint, config.temporal_fault_endpoint,
                               prio, res->sched_context.cptr, config.cspace, config.cspace_root_data, vspace_get_root(alloc), null_cap_data, res->ipc_buffer_addr, res->ipc_buffer);

    if (error != seL4_NoError) {
        ZF_LOGE("TCB configure failed with seL4 error code %d", error);
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    if (config.custom_stack_size) {
        res->stack_size = config.stack_size;
    } else {
        res->stack_size = BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE);
    }

    res->stack_top = vspace_new_sized_stack(alloc, res->stack_size);

    if (res->stack_top == NULL) {
        ZF_LOGE("Stack allocation failed!");
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    return 0;
}

int
sel4utils_start_thread(sel4utils_thread_t *thread, void *entry_point, void *arg0, void *arg1,
                       int resume)
{
    seL4_UserContext context = {0};
    size_t context_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    int error = sel4utils_arch_init_local_context(entry_point, arg0, arg1,
                                              (void *) thread->ipc_buffer_addr,
                                              thread->stack_top, &context);
    if (error) {
        return error;
    }

    return seL4_TCB_WriteRegisters(thread->tcb.cptr, resume, 0, context_size, &context);
}

void
sel4utils_clean_up_thread(vka_t *vka, vspace_t *alloc, sel4utils_thread_t *thread)
{
    if (thread->tcb.cptr != 0) {
        vka_free_object(vka, &thread->tcb);
    }

    if (thread->sched_context.cptr != 0 && thread->own_sc) {
        vka_free_object(vka, &thread->sched_context);
    }

    if (thread->ipc_buffer_addr != 0) {
        vspace_free_ipc_buffer(alloc, (seL4_Word *) thread->ipc_buffer_addr);
    }

    if (thread->stack_top != 0) {
        vspace_free_sized_stack(alloc, thread->stack_top, thread->stack_size);
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
    seL4_MessageInfo_t info = seL4_Recv(endpoint, &badge);

    while (1) {
        sel4utils_print_fault_message(info, name);

        /* go back to sleep so other things can run */
        seL4_Recv(endpoint, &badge);
    }

    return 0;
}

int
sel4utils_start_fault_handler(seL4_CPtr fault_endpoint, simple_t *simple, vka_t *vka, vspace_t *vspace,
                              uint8_t prio, seL4_CPtr cspace, seL4_CapData_t cap_data, char *name,
                              sel4utils_thread_t *res)
{
    int error = sel4utils_configure_thread(simple, vka, vspace, vspace, 0, prio, cspace,
                                           cap_data, res);

    if (error) {
        ZF_LOGE("Failed to configure fault handling thread\n");
        return -1;
    }

    return sel4utils_start_thread(res, fault_handler, name,
                                  (void *) fault_endpoint, 1);
}

int
sel4utils_checkpoint_thread(sel4utils_thread_t *thread, sel4utils_checkpoint_t *checkpoint, bool suspend) 
{
    assert(checkpoint != NULL);

    int error = seL4_TCB_ReadRegisters(thread->tcb.cptr, suspend, 0, sizeof(seL4_UserContext) / sizeof(seL4_Word), 
            &checkpoint->regs);
    if (error) {
        ZF_LOGE("Failed to read registers of tcb while checkpointing\n");
        return error;
    }
    
    size_t stack_size = (uintptr_t) thread->stack_top - (uintptr_t) sel4utils_get_sp(checkpoint->regs);
    
    checkpoint->stack = malloc(stack_size);
    if (checkpoint->stack == NULL) {
        ZF_LOGE("Failed to malloc stack of size %zu\n", stack_size);
        return -1;
    }

    memcpy(checkpoint->stack, (void *) sel4utils_get_sp(checkpoint->regs), stack_size);
    checkpoint->thread = thread;

    return error;
}

int 
sel4utils_checkpoint_restore(sel4utils_checkpoint_t *checkpoint, bool free_memory, bool resume)
{
    assert(checkpoint != NULL);

    size_t stack_size = (uintptr_t) checkpoint->thread->stack_top - (uintptr_t) sel4utils_get_sp(checkpoint->regs);
    memcpy((void *) sel4utils_get_sp(checkpoint->regs), checkpoint->stack, stack_size);
    
    int error = seL4_TCB_WriteRegisters(checkpoint->thread->tcb.cptr, resume, 0,
            sizeof(seL4_UserContext) / sizeof (seL4_Word), 
            &checkpoint->regs);
    if (error) {
        ZF_LOGE("Failed to restore registers of tcb while restoring checkpoint\n");
        return error;
    }

    if (free_memory) {
       sel4utils_free_checkpoint(checkpoint);
    }

    return error;
}

void
sel4utils_free_checkpoint(sel4utils_checkpoint_t *checkpoint)
{
    free(checkpoint->stack);
}


#endif /* CONFIG_LIB_SEL4_VSPACE */
