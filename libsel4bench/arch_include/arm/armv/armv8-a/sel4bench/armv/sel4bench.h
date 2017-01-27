/*
 *  Copyright 2016, Data61
 *  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 *  ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */
#ifndef __ARCH_ARMV_SEL4BENCH_H
#define __ARCH_ARMV_SEL4BENCH_H

#define SEL4BENCH_READ_CCNT(var) do { \
    asm volatile("mrc p15, 0, %0, c9, c13, 0\n" \
        : "=r"(var) \
    ); \
} while(0)

#endif /* __ARCH_ARMV_SEL4BENCH_H */
