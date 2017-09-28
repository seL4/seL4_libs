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

/* This file contains macros for CPUID emulation in x86.
 * Most of the code in this file is from arch/x86/kvm/cpuid.h Linux 3.8.8
 * This file should used only by cpuid.c
 *
 *     Authors:
 *         Qian Ge
 */

#pragma once

#include <utils/util.h>

#define F(x) BIT( (X86_FEATURE_##x) & 31)

/* Basic information for the processor P4 hyperthread. */
#define VMM_CPUID_EAX_P4_HT    0x5
#define VMM_CPUID_P4_FAMILY    (6 << 8)
#define VMM_CPUID_P4_MODLE     (7 << 4)
#define VMM_CPUID_P4_STEP      (1)

/* Extended function information for p4 ht. */
#define VMM_CPUID_P4_MAX_EXTFUNCTION 0x80000008

/* 32 bit for both phy & vir address space*/
#define VMM_CPUID_P4_PHYADDR_SIZE    0x20
#define VMM_CPUID_P4_VIRADDR_SIZE    0x20

/* This CPUID returns the signature 'KVMKVMKVM' in ebx, ecx, and edx if running under KVM. */
#define VMM_CPUID_KVM_SIGNATURE     0x40000000
/* This CPUID returns a feature bitmap in eax */
#define VMM_CPUID_KVM_FEATURES      0x40000001

/* CPUID instruction return value. */
struct cpuid_val {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
};

