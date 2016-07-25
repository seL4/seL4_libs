/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_PLATFORM_BOOT_H_
#define _LIB_VMM_PLATFORM_BOOT_H_

#include <simple/simple.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <allocman/allocman.h>
#include "vmm/vmm.h"

int vmm_init(vmm_t *vmm, allocman_t *allocman, simple_t simple, vka_t vka, vspace_t vspace, platform_callbacks_t callbacks);
int vmm_init_host(vmm_t *vmm);
int vmm_init_guest(vmm_t *vmm, int priority);
int vmm_init_guest_multi(vmm_t *vmm, int priority, int num_vcpus);

#endif /* _LIB_VMM_PLATFORM_BOOT_H_ */
