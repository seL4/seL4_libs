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

 *     Authors:
 *         Qian Ge
 */

#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>

#include "vmm/debug.h"

#include "vmm/vmm.h"

#include "vmm/processor/cpuid.h"
#include "vmm/processor/cpufeature.h"

static inline void native_cpuid(unsigned int *eax, unsigned int *ebx,
                unsigned int *ecx, unsigned int *edx) {
    /* ecx is often an input as well as an output. */
    asm volatile("cpuid"
        : "=a" (*eax),
          "=b" (*ebx),
          "=c" (*ecx),
          "=d" (*edx)
        : "0" (*eax), "2" (*ecx)
        : "memory");
}

static int vmm_cpuid_virt(unsigned int function, unsigned int index, struct cpuid_val *val, vmm_vcpu_t *vcpu) {
    unsigned int eax, ebx, ecx, edx;

    eax = function;
    ecx = index;

    native_cpuid(&eax, &ebx, &ecx, &edx);

    /* cpuid 1.edx */
    const unsigned int kvm_supported_word0_x86_features =
        F(FPU) | 0 /*F(VME)*/ | 0 /*F(DE)*/ | 0/*F(PSE)*/ |
        F(TSC) | 0/*F(MSR)*/ | 0 /*F(PAE)*/ | 0/*F(MCE)*/ |
        0 /*F(CX8)*/ | F(APIC) | 0 /* Reserved */ | F(SEP) |
        /*F(MTRR)*/ 0 | F(PGE) | 0/*F(MCA)*/ | F(CMOV) |
        0 /*F(PAT)*/ | 0 /* F(PSE36)*/ | 0 /* PSN */ | 0/*F(CLFLSH)*/ |
        0 /* Reserved, DS, ACPI */ | F(MMX) |
        F(FXSR) | F(XMM) | F(XMM2) | 0/*F(SELFSNOOP)*/ |
        0 /* HTT, TM, Reserved, PBE */;

    /* cpuid 1.ecx */
    const unsigned int kvm_supported_word4_x86_features =
        F(XMM3) | 0 /*F(PCLMULQDQ)*/ | 0 /* DTES64, MONITOR */ |
        0 /* DS-CPL, VMX, SMX, EST */ |
        0 /* TM2 */ | F(SSSE3) | 0 /* CNXT-ID */ | 0 /* Reserved */ |
        0 /*F(FMA)*/ | 0 /*F(CX16)*/ | 0 /* xTPR Update, PDCM */ |
        0 /*F(PCID)*/ | 0 /* Reserved, DCA */ | F(XMM4_1) |
        F(XMM4_2) | 0 /*F(X2APIC)*/ | 0 /*F(MOVBE)*/ | 0 /*F(POPCNT)*/ |
        0 /* Reserved*/ | 0 /*F(AES)*/ | 0/*F(XSAVE)*/ | 0/*F(OSXSAVE)*/ | 0 /*F(AVX)*/ |
        0 /*F(F16C)*/ | 0 /*F(RDRAND)*/;

    /* cpuid 0x80000001.edx */
    const unsigned int kvm_supported_word1_x86_features =
        0 /*F(NX)*/ | 0/*F(RDTSCP)*/;  /*not support x86 64*/

    /* cpuid 0x80000001.ecx */
    const unsigned int kvm_supported_word6_x86_features = 0;

#if 0
    /* cpuid 0xC0000001.edx */
    const unsigned int kvm_supported_word5_x86_features =
        F(XSTORE) | F(XSTORE_EN) | F(XCRYPT) | F(XCRYPT_EN) |
        F(ACE2) | F(ACE2_EN) | F(PHE) | F(PHE_EN) |
        F(PMM) | F(PMM_EN);
#endif

    /* cpuid 7.0.ebx */
    const unsigned int kvm_supported_word9_x86_features =
        F(FSGSBASE) | F(BMI1) | F(HLE) | F(AVX2) | F(SMEP) |
        F(BMI2) | F(ERMS) | 0 /*F(INVPCID)*/| F(RTM);

    /* Virtualize the return value according to the function. */

    DPRINTF(4, "cpuid function 0x%x index 0x%x eax 0x%x ebx 0%x ecx 0x%x edx 0x%x\n", function, index, eax, ebx, ecx, edx);

    /* ref: http://www.sandpile.org/x86/cpuid.htm */

    switch (function) {
        case 0: /* Get highest function supported plus vendor ID */
	    if (eax > 0xb)
	        eax = 0xb;
	    break;

        case 1: /* Processor, info and feature. family, model, stepping */
            edx &= kvm_supported_word0_x86_features;
            ecx &= kvm_supported_word4_x86_features;
            break;

        case 2:
        case 4: /* Cache and TLB descriptor information */
            /* Simply pass through information from native CPUID. */
            break;

        case 7: /* Extended flags */
            ebx &= kvm_supported_word9_x86_features;
            break;

        case 0xa: /* disable performance monitoring */
            eax = ebx = ecx = edx = 0;
            break;

        case 0xb: /* Disable topology information */
            eax = ebx = ecx = edx = 0;
            break;

        case VMM_CPUID_KVM_SIGNATURE: /* Unsupported KVM features. We are not KVM. */
        case VMM_CPUID_KVM_FEATURES:
            eax = ebx = ecx = edx = 0;
            break;

        case 0x80000000: /* Get highest extended function supported */
            break;

        case 0x80000001: /* extended processor info and feature bits */
            ecx &= kvm_supported_word6_x86_features;
            edx &= kvm_supported_word1_x86_features;
            break;

        case 0x80000002: /* Get processor name string. */
        case 0x80000003:
        case 0x80000004:
        case 0x80000005:
        case 0x80000006: /* Cache information. */
            /* Pass through brand name from native CPUID. */
            break;

        case 0x80000008: /* Virtual and Physics address sizes */
            break;

        case 3: /* Processor serial number. */
            break;
        case 5: /* MONITOR / MWAIT */
        case 6: /* Thermal management */
        case 0x80000007: /* Advanced power management - unsupported. */
        case 0xC0000002:
        case 0xC0000003:
        case 0xC0000004:
            eax = ebx = ecx = edx = 0;
            break;

        default:
            /* TODO: Adding more CPUID functions whenever necessary */
            DPRINTF(1, "CPUID unimplemented function 0x%x\n", function);
            return -1;

    }

    val->eax = eax;
    val->ebx = ebx;
    val->ecx = ecx;
    val->edx = edx;

    DPRINTF(4, "cpuid virt value eax 0x%x ebx 0x%x ecx 0x%x edx 0x%x\n", eax, ebx, ecx, edx);

    return 0;

}

#if 0
            /* function 2 entries are STATEFUL. That is, repeated cpuid commands
             * may return different values. This forces us to get_cpu() before
             * issuing the first command, and also to emulate this annoying behavior
             * in kvm_emulate_cpuid() using KVM_CPUID_FLAG_STATE_READ_NEXT */
        case 2: {
                    int t, times = entry->eax & 0xff;

                    entry->flags |= KVM_CPUID_FLAG_STATEFUL_FUNC;
                    entry->flags |= KVM_CPUID_FLAG_STATE_READ_NEXT;
                    for (t = 1; t < times; ++t) {
                        if (*nent >= maxnent)
                            goto out;

                        do_cpuid_1_ent(&entry[t], function, 0);
                        entry[t].flags |= KVM_CPUID_FLAG_STATEFUL_FUNC;
                        ++*nent;
                    }
                    break;
                }
                /* function 4 has additional index. */
        case 4: {
                    int i, cache_type;

                    entry->flags |= KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                    /* read more entries until cache_type is zero */
                    for (i = 1; ; ++i) {
                        if (*nent >= maxnent)
                            goto out;

                        cache_type = entry[i - 1].eax & 0x1f;
                        if (!cache_type)
                            break;
                        do_cpuid_1_ent(&entry[i], function, i);
                        entry[i].flags |=
                            KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                        ++*nent;
                    }
                    break;
                }
        case 7: {
                    entry->flags |= KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                    /* Mask ebx against host capability word 9 */
                    if (index == 0) {
                        entry->ebx &= kvm_supported_word9_x86_features;
                        cpuid_mask(&entry->ebx, 9);
                        // TSC_ADJUST is emulated
                        entry->ebx |= F(TSC_ADJUST);
                    } else
                        entry->ebx = 0;
                    entry->eax = 0;
                    entry->ecx = 0;
                    entry->edx = 0;
                    break;
                }
        case 9:
                break;
        case 0xa: { /* Architectural Performance Monitoring */
                      struct x86_pmu_capability cap;
                      union cpuid10_eax eax;
                      union cpuid10_edx edx;

                      perf_get_x86_pmu_capability(&cap);

                      /*
                       * Only support guest architectural pmu on a host
                       * with architectural pmu.
                       */
                      if (!cap.version)
                          memset(&cap, 0, sizeof(cap));

                      eax.split.version_id = min(cap.version, 2);
                      eax.split.num_counters = cap.num_counters_gp;
                      eax.split.bit_width = cap.bit_width_gp;
                      eax.split.mask_length = cap.events_mask_len;

                      edx.split.num_counters_fixed = cap.num_counters_fixed;
                      edx.split.bit_width_fixed = cap.bit_width_fixed;
                      edx.split.reserved = 0;

                      entry->eax = eax.full;
                      entry->ebx = cap.events_mask;
                      entry->ecx = 0;
                      entry->edx = edx.full;
                      break;
                  }
                  /* function 0xb has additional index. */
        case 0xb: {
                      int i, level_type;

                      entry->flags |= KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                      /* read more entries until level_type is zero */
                      for (i = 1; ; ++i) {
                          if (*nent >= maxnent)
                              goto out;

                          level_type = entry[i - 1].ecx & 0xff00;
                          if (!level_type)
                              break;
                          do_cpuid_1_ent(&entry[i], function, i);
                          entry[i].flags |=
                              KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                          ++*nent;
                      }
                      break;
                  }
        case 0xd: {
                      int idx, i;

                      entry->flags |= KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                      for (idx = 1, i = 1; idx < 64; ++idx) {
                          if (*nent >= maxnent)
                              goto out;

                          do_cpuid_1_ent(&entry[i], function, idx);
                          if (entry[i].eax == 0 || !supported_xcr0_bit(idx))
                              continue;
                          entry[i].flags |=
                              KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                          ++*nent;
                          ++i;
                      }
                      break;
                  }
        case KVM_CPUID_SIGNATURE: {
                                      static const char signature[12] = "KVMKVMKVM\0\0";
                                      const unsigned int *sigptr = (const unsigned int *)signature;
                                      entry->eax = KVM_CPUID_FEATURES;
                                      entry->ebx = sigptr[0];
                                      entry->ecx = sigptr[1];
                                      entry->edx = sigptr[2];
                                      break;
                                  }
        case KVM_CPUID_FEATURES:
                                  entry->eax = (BIT(KVM_FEATURE_CLOCKSOURCE)) |
                                      (BIT(KVM_FEATURE_NOP_IO_DELAY)) |
                                      (BIT(KVM_FEATURE_CLOCKSOURCE2)) |
                                      (BIT(KVM_FEATURE_ASYNC_PF)) |
                                      (BIT(KVM_FEATURE_PV_EOI)) |
                                      (BIT(KVM_FEATURE_CLOCKSOURCE_STABLE_BIT));

                                  if (sched_info_on())
                                      entry->eax |= (BIT(KVM_FEATURE_STEAL_TIME));

                                  entry->ebx = 0;
                                  entry->ecx = 0;
                                  entry->edx = 0;
                                  break;
       case 0x80000019:
                         entry->ecx = entry->edx = 0;
                         break;
        case 0x8000001a:
                         break;
        case 0x8000001d:
                         break;
                         /*Add support for Centaur's CPUID instruction*/
        case 0xC0000000:
                         /*Just support up to 0xC0000004 now*/
                         entry->eax = min(entry->eax, 0xC0000004);
                         break;
        case 0xC0000001:
                         entry->edx &= kvm_supported_word5_x86_features;
                         cpuid_mask(&entry->edx, 5);
                         break;
        case 3: /* Processor serial number */
        case 5: /* MONITOR/MWAIT */
        case 6: /* Thermal management */
        case 0x80000007: /* Advanced power management */
        case 0xC0000002:
        case 0xC0000003:
        case 0xC0000004:
        default:
                         entry->eax = entry->ebx = entry->ecx = entry->edx = 0;
                         break;
    }
#endif

/* VM exit handler: for the CPUID instruction. */
int vmm_cpuid_handler(vmm_vcpu_t *vcpu) {

    int ret;
    struct cpuid_val val;

    /* Read parameter information. */
    unsigned int function = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_EAX);
    unsigned int index = vmm_read_user_context(&vcpu->guest_state, USER_CONTEXT_ECX);

    /* Virtualise the CPUID instruction. */
    ret = vmm_cpuid_virt(function, index, &val, vcpu);
    if (ret)
        return ret;

    /* Set the return values in guest context. */
    vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EAX, val.eax);
    vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EBX, val.ebx);
    vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_ECX, val.ecx);
    vmm_set_user_context(&vcpu->guest_state, USER_CONTEXT_EDX, val.edx);

    vmm_guest_exit_next_instruction(&vcpu->guest_state, vcpu->guest_vcpu);

    /* Return success. */
    return 0;
}
