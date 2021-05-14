/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <sel4utils/gen_config.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vspace/vspace.h>
#include <sel4runtime.h>
#include <sel4utils/api.h>
#include <sel4utils/mapping.h>
#include <sel4utils/thread.h>
#include <sel4utils/util.h>
#include <sel4utils/arch/util.h>
#include <sel4utils/helpers.h>
#include <utils/stack.h>


int sel4utils_configure_thread(vka_t *vka, vspace_t *parent, vspace_t *alloc, seL4_CPtr fault_endpoint,
                               seL4_CNode cspace, seL4_Word cspace_root_data, sel4utils_thread_t *res)
{

    sel4utils_thread_config_t config = {0};
    config = thread_config_fault_endpoint(config, fault_endpoint);
    config = thread_config_cspace(config, cspace, cspace_root_data);
    config = thread_config_create_reply(config);
    return sel4utils_configure_thread_config(vka, parent, alloc, config, res);
}

int sel4utils_configure_thread_config(vka_t *vka, vspace_t *parent, vspace_t *alloc,
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
    }

    if (config_set(CONFIG_KERNEL_MCS) && config.create_reply) {
        if (vka_alloc_reply(vka, &res->reply)) {
            ZF_LOGE("Failed to allocate reply");
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }
        res->own_reply = true;
    } else {
        res->reply.cptr = config.reply;
    }

    if (config.sched_params.create_sc) {
        if (!config_set(CONFIG_KERNEL_MCS)) {
            ZF_LOGE("Cannot create a scheduling context on a non-RT kernel");
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }

        /* allocate a scheduling context */
        if (vka_alloc_sched_context(vka, &res->sched_context)) {
            ZF_LOGE("Failed to allocate sched context");
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }

        /* configure the scheduling context */
        if (config_set(CONFIG_KERNEL_MCS)) {
            error = api_sched_ctrl_configure(config.sched_params.sched_ctrl, res->sched_context.cptr,
                                             config.sched_params.budget, config.sched_params.period,
                                             config.sched_params.extra_refills, config.sched_params.badge);
        }
        if (error != seL4_NoError) {
            ZF_LOGE("Failed to configure sched context");
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }
        res->own_sc = true;
    } else {
        res->sched_context.cptr = config.sched_params.sched_context;
    }
    seL4_Word null_cap_data = seL4_NilData;
    error = api_tcb_configure(res->tcb.cptr, config.fault_endpoint,
                              seL4_CapNull,
                              res->sched_context.cptr,
                              config.cspace,
                              config.cspace_root_data, vspace_get_root(alloc),
                              null_cap_data, res->ipc_buffer_addr, res->ipc_buffer);

    if (error != seL4_NoError) {
        ZF_LOGE("TCB configure failed with seL4 error code %d", error);
        sel4utils_clean_up_thread(vka, alloc, res);
        return -1;
    }

    /* only set the prio fields if the value is > 0. As we just allocated the
     * TCB above, the prio and mcp are already 0. */
    if (config.sched_params.mcp) {
        error = seL4_TCB_SetMCPriority(res->tcb.cptr, config.sched_params.auth,
                                       config.sched_params.mcp);
        if (error) {
            ZF_LOGE("Failed to set mcpriority, %d", error);
            return -1;
        }
    }

    if (config.sched_params.priority) {
        error = seL4_TCB_SetPriority(res->tcb.cptr, config.sched_params.auth,
                                     config.sched_params.priority);
        if (error) {
            ZF_LOGE("Failed to set priority, %d", error);
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }
    }

    if (config.custom_stack_size) {
        res->stack_size = config.stack_size;
    } else {
        res->stack_size = BYTES_TO_4K_PAGES(CONFIG_SEL4UTILS_STACK_SIZE);
    }

    if (res->stack_size > 0) {
        res->stack_top = vspace_new_sized_stack(alloc, res->stack_size);

        if (res->stack_top == NULL) {
            ZF_LOGE("Stack allocation failed!");
            sel4utils_clean_up_thread(vka, alloc, res);
            return -1;
        }

        res->initial_stack_pointer = res->stack_top;
    }

    return 0;
}

int sel4utils_start_thread(sel4utils_thread_t *thread, sel4utils_thread_entry_fn entry_point,
                           void *arg0, void *arg1, int resume)
{
    seL4_UserContext context = {0};
    size_t context_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    size_t tls_size = sel4runtime_get_tls_size();
    /* make sure we're not going to use too much of the stack */
    if (tls_size > thread->stack_size * PAGE_SIZE_4K / 8) {
        ZF_LOGE("TLS would use more than 1/8th of the application stack %zu/%zu", tls_size, thread->stack_size);
        return -1;
    }
    uintptr_t tls_base = (uintptr_t)thread->initial_stack_pointer - tls_size;
    uintptr_t tp = (uintptr_t)sel4runtime_write_tls_image((void *)tls_base);
    seL4_IPCBuffer *ipc_buffer_addr = (void *)thread->ipc_buffer_addr;
    sel4runtime_set_tls_variable(tp, __sel4_ipc_buffer, ipc_buffer_addr);

    uintptr_t aligned_stack_pointer = ALIGN_DOWN(tls_base, STACK_CALL_ALIGNMENT);

    int error = sel4utils_arch_init_local_context(entry_point, arg0, arg1,
                                                  (void *) thread->ipc_buffer_addr,
                                                  (void *) aligned_stack_pointer,
                                                  &context);
    if (error) {
        return error;
    }

    error = seL4_TCB_WriteRegisters(thread->tcb.cptr, false, 0, context_size, &context);
    if (error) {
        return error;
    }

    error = seL4_TCB_SetTLSBase(thread->tcb.cptr, tp);
    if (error) {
        return error;
    }

    if (resume) {
        return seL4_TCB_Resume(thread->tcb.cptr);
    }
    return 0;
}

void sel4utils_clean_up_thread(vka_t *vka, vspace_t *alloc, sel4utils_thread_t *thread)
{
    if (thread->tcb.cptr != 0) {
        vka_free_object(vka, &thread->tcb);
    }

    if (thread->ipc_buffer_addr != 0) {
        vspace_free_ipc_buffer(alloc, (seL4_Word *) thread->ipc_buffer_addr);
    }

    if (thread->stack_top != 0) {
        vspace_free_sized_stack(alloc, thread->stack_top, thread->stack_size);
    }

    if (thread->own_sc && thread->sched_context.cptr != 0) {
        vka_free_object(vka, &thread->sched_context);
    }

    if (thread->own_reply && thread->reply.cptr != 0) {
        vka_free_object(vka, &thread->reply);
    }

    memset(thread, 0, sizeof(sel4utils_thread_t));
}

void sel4utils_print_fault_message(seL4_MessageInfo_t tag, const char *thread_name)
{
    seL4_Fault_t fault = seL4_getFault(tag);

    switch (seL4_Fault_get_seL4_FaultType(fault)) {
    case seL4_Fault_VMFault:
        assert(seL4_MessageInfo_get_length(tag) == seL4_VMFault_Length);
        printf("%sPagefault from [%s]: %s %s at PC: %p vaddr: %p, FSR %p%s\n",
               COLOR_ERROR,
               thread_name,
               sel4utils_is_read_fault() ? "read" : "write",
               seL4_Fault_VMFault_get_PrefetchFault(fault) ? "prefetch fault" : "fault",
               (void *)seL4_Fault_VMFault_get_IP(fault),
               (void *)seL4_Fault_VMFault_get_Addr(fault),
               (void *)seL4_Fault_VMFault_get_FSR(fault),
               COLOR_NORMAL);
        break;

    case seL4_Fault_UnknownSyscall:
        assert(seL4_MessageInfo_get_length(tag) == seL4_UnknownSyscall_Length);
        printf("%sBad syscall from [%s]: scno %"PRIuPTR" at PC: %p%s\n",
               COLOR_ERROR,
               thread_name,
               seL4_Fault_UnknownSyscall_get_Syscall(fault),
               (void *) seL4_Fault_UnknownSyscall_get_FaultIP(fault),
               COLOR_NORMAL
              );

        break;

    case seL4_Fault_UserException:
        assert(seL4_MessageInfo_get_length(tag) == seL4_UserException_Length);
        printf("%sInvalid instruction from [%s] at PC: %p%s\n",
               COLOR_ERROR,
               thread_name,
               (void *)seL4_Fault_UserException_get_FaultIP(fault),
               COLOR_NORMAL);
        break;

    case seL4_Fault_CapFault:
        printf("%sCap fault from [%s] in phase %s\nPC = %p\nCPtr = %p%s\n",
               COLOR_ERROR, thread_name,
               seL4_Fault_CapFault_get_InRecvPhase(fault) ? "receive" : "send",
               (void *) seL4_Fault_CapFault_get_IP(fault),
               (void *) seL4_Fault_CapFault_get_Addr(fault),
               COLOR_NORMAL);
        break;
#ifdef CONFIG_KERNEL_MCS
    case seL4_Fault_Timeout:
        printf("Timeout fault from %s\n", thread_name);
        break;
#endif

    default:
        /* What? Why are we here? What just happened? */
        printf("Unknown fault from [%s]: %"PRIuPTR" (length = %"PRIuPTR")\n", thread_name, seL4_MessageInfo_get_label(tag), seL4_MessageInfo_get_length(tag));
        break;
    }
}

static int
fault_handler(char *name, seL4_CPtr endpoint)
{
    seL4_MessageInfo_t info;
    while (1) {
        /* sleep so other things can run */
        info = api_wait(endpoint, NULL);
        sel4utils_print_fault_message(info, name);
    }
    return 0;
}

int
sel4utils_start_fault_handler(seL4_CPtr fault_endpoint, vka_t *vka, vspace_t *vspace,
                              seL4_CPtr cspace, seL4_Word cap_data, char *name,
                              sel4utils_thread_t *res)
{
    int error = sel4utils_configure_thread(vka, vspace, vspace, 0, cspace,
                                           cap_data, res);

    if (error) {
        ZF_LOGE("Failed to configure fault handling thread\n");
        return -1;
    }

    return sel4utils_start_thread(res, (sel4utils_thread_entry_fn)fault_handler, name,
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

    checkpoint->sp = sel4utils_get_sp(checkpoint->regs);
#ifdef CONFIG_ARCH_X86_64
    if (config_set(CONFIG_SYSENTER)) {
        /* on x64, using sysenter, the kernel ABI expects rcx to be set to rsp,
         * and rdx to be set to the fault instruction. Simulate this behaviour here
         * before resuming. Note that this will only work for threads checkpointed
         * at sysenter, e.g. while in a system call (eg seL4_Recv). */
        checkpoint->regs.rcx = checkpoint->regs.rsp;
        checkpoint->regs.rdx = checkpoint->regs.rip;
    } else if (config_set(CONFIG_SYSCALL)) {
        /* on x64, using syscall, when a thread is in the kernel,
         * sp is stored in rbx. So use rbx for stack calculations */
         checkpoint->sp = checkpoint->regs.rbx;
    }
#endif /* CONFIG_ARCH_X86_64 */

    size_t stack_size = (uintptr_t) thread->stack_top - checkpoint->sp;
    checkpoint->stack = malloc(stack_size);
    if (checkpoint->stack == NULL) {
        ZF_LOGE("Failed to malloc stack of size %zu\n", stack_size);
        return -1;
    }

    memcpy(checkpoint->stack, (void *) checkpoint->sp, stack_size);
    checkpoint->thread = thread;

    return error;
}

int
sel4utils_checkpoint_restore(sel4utils_checkpoint_t *checkpoint, bool free_memory, bool resume)
{
    assert(checkpoint != NULL);

    size_t stack_size = (uintptr_t) checkpoint->thread->stack_top - checkpoint->sp;
    memcpy((void *) checkpoint->sp, checkpoint->stack, stack_size);

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

int sel4utils_set_sched_affinity(sel4utils_thread_t *thread, sched_params_t params) {
#if CONFIG_MAX_NUM_NODES > 1
#ifdef CONFIG_KERNEL_MCS
    return api_sched_ctrl_configure(params.sched_ctrl, thread->sched_context.cptr, params.budget, params.period,
                                    params.extra_refills, params.badge);

#else
    return seL4_TCB_SetAffinity(thread->tcb.cptr, params.core);
#endif
#else
    return -ENOSYS;
#endif
}
