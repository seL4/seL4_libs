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
#pragma once

#include <stdint.h>
#include <sel4/sel4.h>
#include <sel4/faults.h>

#define EXCEPT_IPC_SYS_MR_IP EXCEPT_IPC_SYS_MR_EIP

static inline void
sel4utils_set_instruction_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->eip = value;
}

static inline seL4_Word
sel4utils_get_instruction_pointer(seL4_UserContext regs)
{
    return regs.eip;
}

static inline seL4_Word
sel4utils_get_sp(seL4_UserContext regs)
{
    return regs.esp;
}

static inline void
sel4utils_set_stack_pointer(seL4_UserContext *regs, seL4_Word value)
{
    regs->esp = value;
}

/* Helper function for writing your OWN TLS_BASE register on x86.
 * This function assumes that the tcb capability your are modifying
 * is the currently executing thread and will not work otherwise.
 * This function is also hand written and changes to the IPC buffer layout,
 * number of message registers, or WriteRegisters invocation will need
 * to be manually reflected here. This is all neccessary as we need
 * to tread very carefully over our registers and stack to make this work */
static inline int sel4utils_ia32_tcb_set_tls_base(seL4_CPtr tcb_cap, seL4_Word tls_base)
{
    /* Declared as variables since input operands cannot overlap with clobbers */
    uint32_t edi = 1; /* first message argument. 1 means 'resume_target' everything else is 0 */
    uint32_t ecx = sizeof(seL4_UserContext) / sizeof(seL4_Word); /* second message argument. number of registers of thread to modify */
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(TCBWriteRegisters, 0, 0, ecx + 2); /* message length is user context + 2 arguments */
    asm volatile(
        /* eax is what we want for tls_base, so write this first so we can reuse eax */
        "movl %%eax, %%fs:24 \n"
        "movl %%eax, %%fs:52 \n"
        /* Get a copy of the stack pointer */
        "movl %%esp, %%eax \n"
        /* The value we want to be set is the current value - 4, as we will push to it in the invocation stub */
        "subl $4, %%eax \n"
        "movl %%eax, %%fs:16 \n"
        /* The kernel should not let us overwrite our own EIP, but set
         * something sensible anyway */
        "leal 2f, %%eax \n"
        "movl %%eax, %%fs:12 \n"
        /* Set fs and gs. We set GS to the correct tls selector, and leave fs unchanged */
        "movl %%fs, %%eax \n"
        "movl %%eax, %%fs:56 \n"
        "movl %[tls], %%eax \n"
        "movl %%eax, %%fs:60 \n"
        /* No point saving any other registers as they are going to get reset
         * as a result of the syscall anyway */
        /* Now the syscall invocation */
        "movl %[syscall], %%eax \n" /* we the syscall value */
        "pushl %%ebp \n" /* we added 4 bytes to esp earlier so after the write registers our stack will point to here */
        "movl %%ecx, %%ebp \n" /* stash ecx for after invocation */
        "movl %%esp, %%ecx \n" /* esp goes into ecx, per sysenter convention */
        "leal 1f, %%edx \n"    /* calculate return address */
        "1: \n"
        "sysenter \n"
        "2: \n"
        "movl %%ebp, %%ecx \n"
        "popl %%ebp \n"
        : "=S" (tag),
        "=D" (edi),
        "=c" (ecx),
        "=b" (tcb_cap)
        : "a" (tls_base),
        [syscall] "i" (seL4_SysCall),
        [tls] "i" (TLS_GDT_SELECTOR),
        "b" (tcb_cap),
        "S" (tag.words[0]),
        "D" (edi),
        "c" (ecx)
        : "cc", /* we possibly clobber the eflags register */
        "%edx", /* edx gets clobbered due to sysexit convention */
        "memory"
    );
    /* seL4 will not actually reload our tls_base until it switches to us.
     * we can force this to happen with a yield */
    seL4_Yield();
    return seL4_MessageInfo_get_label(tag);
}

