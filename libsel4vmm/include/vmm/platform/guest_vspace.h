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

#include <autoconf.h>
#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <vka/vka.h>

/* Constructs a vspace that will duplicate mappings between an ept
 * and several IO spaces as well as translation to mappings in the VMM vspace */
int vmm_get_guest_vspace(vspace_t *loader, vspace_t *vmm_vspace, vspace_t *new_vspace, vka_t *vka, seL4_CPtr page_directory);

/* Helpers for use with touch below */
int vmm_guest_get_phys_data_help(uintptr_t addr, void *vaddr, size_t size,
        size_t offset, void *cookie);

int vmm_guest_set_phys_data_help(uintptr_t addr, void *vaddr, size_t size,
        size_t offset, void *cookie);

/* callback used for each portion of vmm_guest_vspace_touch.
 * if the return value is non zero this signals the parent
 * loop to stop and return the given value */
typedef int (*vmm_guest_vspace_touch_callback)(uintptr_t guest_phys, void *vmm_vaddr, size_t size, size_t offset, void *cookie);

/* 'touch' a series of guest physical addresses by invoking the callback function
 * each equivalent range of addresses in the vmm vspace */
int vmm_guest_vspace_touch(vspace_t *guest_vspace, uintptr_t addr, size_t size, vmm_guest_vspace_touch_callback callback, void *cookie);

#ifdef CONFIG_IOMMU
/* Attach an additional IO space to the vspace */
int vmm_guest_vspace_add_iospace(vspace_t *loader, vspace_t *vspace, seL4_CPtr iospace);
#endif

