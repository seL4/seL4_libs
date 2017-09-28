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

#pragma once

#include <simple/simple.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <allocman/allocman.h>
#include "vmm/vmm.h"

int vmm_init(vmm_t *vmm, allocman_t *allocman, simple_t simple, vka_t vka, vspace_t vspace, platform_callbacks_t callbacks);
int vmm_init_host(vmm_t *vmm);
int vmm_init_guest(vmm_t *vmm, int priority);
int vmm_init_guest_multi(vmm_t *vmm, int priority, int num_vcpus);

