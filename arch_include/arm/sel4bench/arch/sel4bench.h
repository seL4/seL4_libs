/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __ARCH_SEL4BENCH_H__
#define __ARCH_SEL4BENCH_H__

#ifdef ARMV7_A
#include "sel4bench_armv7a.h"
#endif //ARMV7_A

#if defined(ARM1136J_S) || defined(ARM1136JF_S)
#include "sel4bench_arm1136.h"
#endif //ARM1136JF_S

#endif /* __ARCH_SEL4BENCH_H__ */

