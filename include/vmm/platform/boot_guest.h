/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_PLATFORM_BOOT_GUEST_H_
#define _LIB_VMM_PLATFORM_BOOT_GUEST_H_

#include <stdint.h>
#include "vmm/vmm.h"

void vmm_plat_guest_elf_relocate(vmm_t *vmm, const char *relocs_filename);
int vmm_guest_load_boot_module(vmm_t *vmm, const char *name);
void vmm_plat_init_guest_boot_structure(vmm_t *vmm, const char *cmdline);
void vmm_init_guest_thread_state(vmm_t *vmm);
int vmm_load_guest_elf(vmm_t *vmm, const char *elfname, size_t alignment);

#endif /* _LIB_VMM_PLATFORM_BOOT_GUEST_H_ */
