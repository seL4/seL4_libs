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
#include <sel4/types.h>

#define PMU_WRITE(reg, v)                      \
    do {                                       \
        seL4_Word _v = v;                         \
        asm volatile("msr  " reg ", %0" :: "r" (_v)); \
    }while(0)

#define PMU_READ(reg, v) asm volatile("mrs %0, " reg :  "=r"(v))

#define PMUSERENR   "PMUSERENR_EL0"
#define PMINTENCLR  "PMINTENCLR_EL0"
#define PMINTENSET  "PMINTENSET_EL1"
#define PMCR        "PMCR_EL0"
#define PMCNTENCLR  "PMCNTENCLR_EL0"
#define PMCNTENSET  "PMCNTENSET_EL0"
#define PMXEVCNTR   "PMXEVCNTR_EL0"
#define PMSELR      "PMSELR_EL0"
#define PMXEVTYPER  "PMXEVTYPER_EL0"
#define PMCCNTR     "PMCCNTR_EL0"

#define CCNT_FORMAT "%"PRIu64
typedef uint64_t ccnt_t;
