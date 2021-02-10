/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <sel4/types.h>

#define PMU_WRITE(cpreg, v)                      \
    do {                                         \
        seL4_Word _v = v;                           \
        asm volatile("mcr  " cpreg :: "r" (_v)); \
    }while(0)
#define PMU_READ(cpreg, v) asm volatile("mrc  " cpreg :  "=r"(v))

#define PMUSERENR  "p15, 0, %0, c9, c14, 0"
#define PMINTENCLR "p15, 0, %0, c9, c14, 2"
#define PMINTENSET "p15, 0, %0, c9, c14, 1"
#define PMCR       "p15, 0, %0, c9, c12, 0"
#define PMCNTENCLR "p15, 0, %0, c9, c12, 2"
#define PMCNTENSET "p15, 0, %0, c9, c12, 1"
#define PMXEVCNTR  "p15, 0, %0, c9, c13, 2"
#define PMSELR     "p15, 0, %0, c9, c12, 5"
#define PMXEVTYPER "p15, 0, %0, c9, c13, 1"
#define PMCCNTR    "p15, 0, %0, c9, c13, 0"

#define CCNT_FORMAT "%u"
typedef uint32_t ccnt_t;
