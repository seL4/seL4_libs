/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*This file contains the helper inline functions for vmm library*/
#ifndef _LIB_VMM_HELPER_H_
#define _LIB_VMM_HELPER_H_

#include "vmm/vmm.h"
#include <stdlib.h>
#include <string.h>
#include <utils/util.h>

/*converting badge to guest os id*/
#define VMM_GUEST_BADGE_TO_ID(guest_badge)   (guest_badge - LIB_VMM_GUEST_OS_BADGE_START)

/*converting badge to vmm id*/
#define VMM_VMM_BADGE_TO_ID(vmm_badge)       (vmm_badge - LIB_VMM_VMM_BADGE_START)

#ifndef MBYTE
    #define MBYTE(x) (x * 1024 * 1024)
#endif

/*copy in/out regs according for vm exit handlers*/
static inline void vm_exit_copyin_regs (gcb_t *guest) {

    /*arrange the registers into a correct sequence
      vm exit handler can use them according to the qualification*/
    guest->regs[VCPU_REGS_RAX] = guest->context.eax;
    guest->regs[VCPU_REGS_RCX] = guest->context.ecx;
    guest->regs[VCPU_REGS_RDX] = guest->context.edx;
    guest->regs[VCPU_REGS_RBX] = guest->context.ebx;
    guest->regs[VCPU_REGS_RSP] = guest->context.esp;
    guest->regs[VCPU_REGS_RBP] = guest->context.ebp;
    guest->regs[VCPU_REGS_RSI] = guest->context.esi;
    guest->regs[VCPU_REGS_RDI] = guest->context.edi;
    guest->regs[VCPU_REGS_RIP] = guest->context.eip;

}


static inline void vm_exit_copyout_regs (gcb_t *guest) {

    /*copy the registers into a correct sequence
     vm exit handler updated them according to the qualification*/
    guest->context.eax = guest->regs[VCPU_REGS_RAX]; 
    guest->context.ecx = guest->regs[VCPU_REGS_RCX]; 
    guest->context.edx = guest->regs[VCPU_REGS_RDX];
    guest->context.ebx = guest->regs[VCPU_REGS_RBX];
    guest->context.esp = guest->regs[VCPU_REGS_RSP];
    guest->context.ebp = guest->regs[VCPU_REGS_RBP];
    guest->context.esi = guest->regs[VCPU_REGS_RSI];
    guest->context.edi = guest->regs[VCPU_REGS_RDI];
}


/* put and get the IPC buffer */
static inline void vmm_get_msg (void *recv, unsigned int len) {
    for (int i = 0; i < len; i++, recv++)
        *(unsigned int *)recv = seL4_GetMR(i);
}

static inline void vmm_put_msg (void *send, unsigned int len) {
    for (int i = 0; i < len; i++, send++)
        seL4_SetMR(i, *(unsigned int *)send); 
}

static inline uint32_t vmm_get_buf(uint32_t cur_mr, char *str, uint32_t slen) {
    // Make sure str[] is big enough to hold!
    assert(str);
    if (slen == 0) return cur_mr;
    int i;
    for (i = 0; i < ROUND_DOWN(slen, 4); i+=4, str+=4) {
        *(seL4_Word*)str = seL4_GetMR(cur_mr++);
    }
    if (i != slen) {
        seL4_Word w = seL4_GetMR(cur_mr++);
        memcpy(str, &w, slen - i);
    }
    return cur_mr;
}

static inline uint32_t vmm_put_buf(uint32_t cur_mr, const char *str, uint32_t slen) {
    assert(str);
    if (slen == 0) return cur_mr;
    int i;
    for (i = 0; i < ROUND_DOWN(slen, 4); i+=4, str+=4) {
        seL4_SetMR(cur_mr++, *(seL4_Word*)str);
    }
    if (i != slen) {
        seL4_Word w = 0;
        memcpy(&w, str, slen - i);
        seL4_SetMR(cur_mr++, w);
    }
    return cur_mr;
}

/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))

#define ADDR				BITOP_ADDR(addr)


/**
 * __set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void __set_bit(int nr, volatile unsigned long *addr)
{
	asm volatile("bts %1,%0" : ADDR : "Ir" (nr) : "memory");
}


static inline void __clear_bit(int nr, volatile unsigned long *addr)
{
	asm volatile("btr %1,%0" : ADDR : "Ir" (nr));
}

static inline void vmm_write_dword_reg(uint64_t data, uint32_t *e1x, uint32_t *e2x) {
    *e1x = data & -1u;
    *e2x = (data >> 32) & -1u;
}

static inline void vmm_write_dwords_reg(uint32_t data1, uint32_t data2, uint32_t *e1x, uint32_t *e2x) {
    *e1x = data1;
    *e2x = data2;
}

#endif 

