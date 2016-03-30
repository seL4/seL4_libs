/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef __ARCH_ARMV_SEL4BENCH_H
#define __ARCH_ARMV_SEL4BENCH_H

#define SEL4BENCH_READ_CCNT(var) do { \
    asm volatile("b 2f\n\
                1:mrc p15, 0, %[counter], c15, c12," SEL4BENCH_ARM1136_COUNTER_CCNT "\n\
                  bx lr\n\
                2:sub r8, pc, #16\n\
                  .word 0xe7f000f0" \
        : [counter] "=r"(var) \
        : \
        : "r8", "lr"); \
} while(0)

#endif /* __ARCH_ARMV_SEL4BENCH_H */
