/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

/* This file contains definitions related with features in x86 platform
 *     Authors:
 *         Qian Ge
 */

#pragma once

/* Exception vector. */

#define DE_VECTOR 0
#define DB_VECTOR 1
#define BP_VECTOR 3
#define OF_VECTOR 4
#define BR_VECTOR 5
#define UD_VECTOR 6
#define NM_VECTOR 7
#define DF_VECTOR 8
#define TS_VECTOR 10
#define NP_VECTOR 11
#define SS_VECTOR 12
#define GP_VECTOR 13
#define PF_VECTOR 14
#define MF_VECTOR 16
#define MC_VECTOR 18

/* Processor flags. */

/*
 * EFLAGS bits
 */
#define X86_EFLAGS_CF   0x00000001 /* Carry Flag */
#define X86_EFLAGS_BIT1 0x00000002 /* Bit 1 - always on */
#define X86_EFLAGS_PF   0x00000004 /* Parity Flag */
#define X86_EFLAGS_AF   0x00000010 /* Auxiliary carry Flag */
#define X86_EFLAGS_ZF   0x00000040 /* Zero Flag */
#define X86_EFLAGS_SF   0x00000080 /* Sign Flag */
#define X86_EFLAGS_TF   0x00000100 /* Trap Flag */
#define X86_EFLAGS_IF   0x00000200 /* Interrupt Flag */
#define X86_EFLAGS_DF   0x00000400 /* Direction Flag */
#define X86_EFLAGS_OF   0x00000800 /* Overflow Flag */
#define X86_EFLAGS_IOPL 0x00003000 /* IOPL mask */
#define X86_EFLAGS_NT   0x00004000 /* Nested Task */
#define X86_EFLAGS_RF   0x00010000 /* Resume Flag */
#define X86_EFLAGS_VM   0x00020000 /* Virtual Mode */
#define X86_EFLAGS_AC   0x00040000 /* Alignment Check */
#define X86_EFLAGS_VIF  0x00080000 /* Virtual Interrupt Flag */
#define X86_EFLAGS_VIP  0x00100000 /* Virtual Interrupt Pending */
#define X86_EFLAGS_ID   0x00200000 /* CPUID detection flag */

/*
 * Basic CPU control in CR0
 */
#define X86_CR0_PE  0x00000001 /* Protection Enable */
#define X86_CR0_MP  0x00000002 /* Monitor Coprocessor */
#define X86_CR0_EM  0x00000004 /* Emulation */
#define X86_CR0_TS  0x00000008 /* Task Switched */
#define X86_CR0_ET  0x00000010 /* Extension Type */
#define X86_CR0_NE  0x00000020 /* Numeric Error */
#define X86_CR0_WP  0x00010000 /* Write Protect */
#define X86_CR0_AM  0x00040000 /* Alignment Mask */
#define X86_CR0_NW  0x20000000 /* Not Write-through */
#define X86_CR0_CD  0x40000000 /* Cache Disable */
#define X86_CR0_PG  0x80000000 /* Paging */

/*
 * Paging options in CR3
 */
#define X86_CR3_PWT 0x00000008 /* Page Write Through */
#define X86_CR3_PCD 0x00000010 /* Page Cache Disable */
#define X86_CR3_PCID_MASK 0x00000fff /* PCID Mask */

/*
 * Intel CPU features in CR4
 */
#define X86_CR4_VME 0x00000001 /* enable vm86 extensions */
#define X86_CR4_PVI 0x00000002 /* virtual interrupts flag enable */
#define X86_CR4_TSD 0x00000004 /* disable time stamp at ipl 3 */
#define X86_CR4_DE  0x00000008 /* enable debugging extensions */
#define X86_CR4_PSE 0x00000010 /* enable page size extensions */
#define X86_CR4_PAE 0x00000020 /* enable physical address extensions */
#define X86_CR4_MCE 0x00000040 /* Machine check enable */
#define X86_CR4_PGE 0x00000080 /* enable global pages */
#define X86_CR4_PCE 0x00000100 /* enable performance counters at ipl 3 */
#define X86_CR4_OSFXSR  0x00000200 /* enable fast FPU save and restore */
#define X86_CR4_OSXMMEXCPT 0x00000400 /* enable unmasked SSE exceptions */
#define X86_CR4_VMXE    0x00002000 /* enable VMX virtualization */
#define X86_CR4_RDWRGSFS 0x00010000 /* enable RDWRGSFS support */
#define X86_CR4_PCIDE   0x00020000 /* enable PCID support */
#define X86_CR4_OSXSAVE 0x00040000 /* enable xsave and xrestore */
#define X86_CR4_SMEP    0x00100000 /* enable SMEP support */
#define X86_CR4_SMAP    0x00200000 /* enable SMAP support */

/*
 * x86-64 Task Priority Register, CR8
 */
#define X86_CR8_TPR 0x0000000F /* task priority register */

/* Reserved bits for CR registers. */
#define CR0_RESERVED_BITS                                               \
    (~(unsigned long)(X86_CR0_PE | X86_CR0_MP | X86_CR0_EM | X86_CR0_TS \
              | X86_CR0_ET | X86_CR0_NE | X86_CR0_WP | X86_CR0_AM \
              | X86_CR0_NW | X86_CR0_CD | X86_CR0_PG))

#define CR3_PAE_RESERVED_BITS ((X86_CR3_PWT | X86_CR3_PCD) - 1)
#define CR3_NONPAE_RESERVED_BITS ((PAGE_SIZE-1) & ~(X86_CR3_PWT | X86_CR3_PCD))
#define CR3_PCID_ENABLED_RESERVED_BITS 0xFFFFFF0000000000ULL
#define CR3_L_MODE_RESERVED_BITS (CR3_NONPAE_RESERVED_BITS |    \
                  0xFFFFFF0000000000ULL)
#define CR4_RESERVED_BITS                                               \
    (~(unsigned long)(X86_CR4_VME | X86_CR4_PVI | X86_CR4_TSD | X86_CR4_DE\
              | X86_CR4_PSE | X86_CR4_PAE | X86_CR4_MCE     \
              | X86_CR4_PGE | X86_CR4_PCE | X86_CR4_OSFXSR | X86_CR4_PCIDE \
              | X86_CR4_OSXSAVE | X86_CR4_SMEP | X86_CR4_RDWRGSFS \
              | X86_CR4_OSXMMEXCPT | X86_CR4_VMXE))

#define CR8_RESERVED_BITS (~(unsigned long)X86_CR8_TPR)

/*
 * Definitions of Primary Processor-Based VM-Execution Controls.
 */
#define CPU_BASED_VIRTUAL_INTR_PENDING          0x00000004
#define CPU_BASED_USE_TSC_OFFSETING             0x00000008
#define CPU_BASED_HLT_EXITING                   0x00000080
#define CPU_BASED_INVLPG_EXITING                0x00000200
#define CPU_BASED_MWAIT_EXITING                 0x00000400
#define CPU_BASED_RDPMC_EXITING                 0x00000800
#define CPU_BASED_RDTSC_EXITING                 0x00001000
#define CPU_BASED_CR3_LOAD_EXITING      0x00008000
#define CPU_BASED_CR3_STORE_EXITING     0x00010000
#define CPU_BASED_CR8_LOAD_EXITING              0x00080000
#define CPU_BASED_CR8_STORE_EXITING             0x00100000
#define CPU_BASED_TPR_SHADOW                    0x00200000
#define CPU_BASED_VIRTUAL_NMI_PENDING       0x00400000
#define CPU_BASED_MOV_DR_EXITING                0x00800000
#define CPU_BASED_UNCOND_IO_EXITING             0x01000000
#define CPU_BASED_USE_IO_BITMAPS                0x02000000
#define CPU_BASED_USE_MSR_BITMAPS               0x10000000
#define CPU_BASED_MONITOR_EXITING               0x20000000
#define CPU_BASED_PAUSE_EXITING                 0x40000000
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS   0x80000000
/*
 * Definitions of Secondary Processor-Based VM-Execution Controls.
 */
#define SECONDARY_EXEC_VIRTUALIZE_APIC_ACCESSES 0x00000001
#define SECONDARY_EXEC_ENABLE_EPT               0x00000002
#define SECONDARY_EXEC_RDTSCP           0x00000008
#define SECONDARY_EXEC_ENABLE_VPID              0x00000020
#define SECONDARY_EXEC_WBINVD_EXITING       0x00000040
#define SECONDARY_EXEC_UNRESTRICTED_GUEST   0x00000080
#define SECONDARY_EXEC_PAUSE_LOOP_EXITING   0x00000400
#define SECONDARY_EXEC_ENABLE_INVPCID       0x00001000

#define PIN_BASED_EXT_INTR_MASK                 0x00000001
#define PIN_BASED_NMI_EXITING                   0x00000008
#define PIN_BASED_VIRTUAL_NMIS                  0x00000020

#define VM_EXIT_SAVE_DEBUG_CONTROLS             0x00000002
#define VM_EXIT_HOST_ADDR_SPACE_SIZE            0x00000200
#define VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL      0x00001000
#define VM_EXIT_ACK_INTR_ON_EXIT                0x00008000
#define VM_EXIT_SAVE_IA32_PAT           0x00040000
#define VM_EXIT_LOAD_IA32_PAT           0x00080000
#define VM_EXIT_SAVE_IA32_EFER                  0x00100000
#define VM_EXIT_LOAD_IA32_EFER                  0x00200000
#define VM_EXIT_SAVE_VMX_PREEMPTION_TIMER       0x00400000

#define VM_ENTRY_LOAD_DEBUG_CONTROLS            0x00000002
#define VM_ENTRY_IA32E_MODE                     0x00000200
#define VM_ENTRY_SMM                            0x00000400
#define VM_ENTRY_DEACT_DUAL_MONITOR             0x00000800
#define VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL     0x00002000
#define VM_ENTRY_LOAD_IA32_PAT          0x00004000
#define VM_ENTRY_LOAD_IA32_EFER                 0x00008000

/* Interruption-information format. */

#define INTR_INFO_VECTOR_MASK           0xff            /* 7:0 */
#define INTR_INFO_INTR_TYPE_MASK        0x700           /* 10:8 */
#define INTR_INFO_DELIVER_CODE_MASK     0x800           /* 11 */
#define INTR_INFO_UNBLOCK_NMI       0x1000      /* 12 */
#define INTR_INFO_VALID_MASK            0x80000000      /* 31 */
#define INTR_INFO_RESVD_BITS_MASK       0x7ffff000

#define VECTORING_INFO_VECTOR_MASK              INTR_INFO_VECTOR_MASK
#define VECTORING_INFO_TYPE_MASK            INTR_INFO_INTR_TYPE_MASK
#define VECTORING_INFO_DELIVER_CODE_MASK        INTR_INFO_DELIVER_CODE_MASK
#define VECTORING_INFO_VALID_MASK           INTR_INFO_VALID_MASK

#define INTR_TYPE_EXT_INTR              (0 << 8) /* external interrupt */
#define INTR_TYPE_NMI_INTR      (2 << 8) /* NMI */
#define INTR_TYPE_HARD_EXCEPTION    (3 << 8) /* processor exception */
#define INTR_TYPE_SOFT_INTR             (4 << 8) /* software interrupt */
#define INTR_TYPE_SOFT_EXCEPTION    (6 << 8) /* software exception */

/* GUEST_INTERRUPTIBILITY_INFO flags. */
#define GUEST_INTR_STATE_STI        0x00000001
#define GUEST_INTR_STATE_MOV_SS     0x00000002
#define GUEST_INTR_STATE_SMI        0x00000004
#define GUEST_INTR_STATE_NMI        0x00000008

/* GUEST_ACTIVITY_STATE flags */
#define GUEST_ACTIVITY_ACTIVE       0
#define GUEST_ACTIVITY_HLT      1
#define GUEST_ACTIVITY_SHUTDOWN     2
#define GUEST_ACTIVITY_WAIT_SIPI    3

/* Exit Qualifications for MOV for Control Register Access. */
#define CONTROL_REG_ACCESS_NUM          0x7     /* 2:0, number of control reg.*/
#define CONTROL_REG_ACCESS_TYPE         0x30    /* 5:4, access type */
#define CONTROL_REG_ACCESS_REG          0xf00   /* 10:8, general purpose reg. */
#define LMSW_SOURCE_DATA_SHIFT 16
#define LMSW_SOURCE_DATA  (0xFFFF << LMSW_SOURCE_DATA_SHIFT) /* 16:31 lmsw source */
#define REG_EAX                         (0 << 8)
#define REG_ECX                         (1 << 8)
#define REG_EDX                         (2 << 8)
#define REG_EBX                         (3 << 8)
#define REG_ESP                         (4 << 8)
#define REG_EBP                         (5 << 8)
#define REG_ESI                         (6 << 8)
#define REG_EDI                         (7 << 8)
#define REG_R8                         (8 << 8)
#define REG_R9                         (9 << 8)
#define REG_R10                        (10 << 8)
#define REG_R11                        (11 << 8)
#define REG_R12                        (12 << 8)
#define REG_R13                        (13 << 8)
#define REG_R14                        (14 << 8)
#define REG_R15                        (15 << 8)

/* VM-instruction error numbers. */
enum vm_instruction_error_number {
    VMXERR_VMCALL_IN_VMX_ROOT_OPERATION = 1,
    VMXERR_VMCLEAR_INVALID_ADDRESS = 2,
    VMXERR_VMCLEAR_VMXON_POINTER = 3,
    VMXERR_VMLAUNCH_NONCLEAR_VMCS = 4,
    VMXERR_VMRESUME_NONLAUNCHED_VMCS = 5,
    VMXERR_VMRESUME_AFTER_VMXOFF = 6,
    VMXERR_ENTRY_INVALID_CONTROL_FIELD = 7,
    VMXERR_ENTRY_INVALID_HOST_STATE_FIELD = 8,
    VMXERR_VMPTRLD_INVALID_ADDRESS = 9,
    VMXERR_VMPTRLD_VMXON_POINTER = 10,
    VMXERR_VMPTRLD_INCORRECT_VMCS_REVISION_ID = 11,
    VMXERR_UNSUPPORTED_VMCS_COMPONENT = 12,
    VMXERR_VMWRITE_READ_ONLY_VMCS_COMPONENT = 13,
    VMXERR_VMXON_IN_VMX_ROOT_OPERATION = 15,
    VMXERR_ENTRY_INVALID_EXECUTIVE_VMCS_POINTER = 16,
    VMXERR_ENTRY_NONLAUNCHED_EXECUTIVE_VMCS = 17,
    VMXERR_ENTRY_EXECUTIVE_VMCS_POINTER_NOT_VMXON_POINTER = 18,
    VMXERR_VMCALL_NONCLEAR_VMCS = 19,
    VMXERR_VMCALL_INVALID_VM_EXIT_CONTROL_FIELDS = 20,
    VMXERR_VMCALL_INCORRECT_MSEG_REVISION_ID = 22,
    VMXERR_VMXOFF_UNDER_DUAL_MONITOR_TREATMENT_OF_SMIS_AND_SMM = 23,
    VMXERR_VMCALL_INVALID_SMM_MONITOR_FEATURES = 24,
    VMXERR_ENTRY_INVALID_VM_EXECUTION_CONTROL_FIELDS_IN_EXECUTIVE_VMCS = 25,
    VMXERR_ENTRY_EVENTS_BLOCKED_BY_MOV_SS = 26,
    VMXERR_INVALID_OPERAND_TO_INVEPT_INVVPID = 28,
};

