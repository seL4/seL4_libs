/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* The Guest OS memory configuration.
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 *         Xi (Ma) Chen
 *
 *     Tue 05 Nov 2013 14:42:17 EST 
 */

#ifndef __LIB_VMM_GUEST_MEMORY_H_
#define __LIB_VMM_GUEST_MEMORY_H_

#include <autoconf.h>
#include "vmm/config.h"
#include "vmm/platform/sys.h"

/* Guest kernel start address alignment requirements. */
#define LIB_VMM_GUEST_PHYSADDR_ALIGN 0x400000
#define LIB_VMM_GUEST_PHYSADDR_MIN   0x2000000

/* Where the bootinfo will be mapped. Must be a valid E820_RAM region.
 * and must be addressable in real mode. */
#define LIB_VMM_GUEST_BOOTINFO_ADDR 0x400000

/* e820 memory region types. */
#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5
#define DMA_NONE        0
#define DMA_IGNORE      0xff
#define DMA_ZONE        0xfe

#define E820_MAX_REGIONS 128 /* E820 supports up to 128. */

/* Allocate and set up guest memory areas, readjusting its kernel ELF regions to appear at the given
 * base addr. */
void vmm_guest_mem_create(uint32_t base_gpaddr, uint32_t size_bytes,
        uint32_t *n_mem_areas, guest_mem_area_t **mem_areas,
        uint32_t *n_bootinfo_mem_areas, guest_mem_area_t **bootinfo_mem_areas);

/* Determine the combines size of the contiguous region in which the kernel + initrd will be. */
uint32_t vmm_guest_mem_determine_kernel_initrd_size(uint32_t num_guest_os,
        char *guest_kernel, char *guest_initrd);

/* Determine size of the initrd file in the CPIO archive. */
uint32_t vmm_guest_get_initrd_size(char *guest_initrd);

/* Get the mem area index of the kernel & initrd mapping region. */
uint32_t vmm_guest_get_kernel_initrd_memarea_index(void);

/* Check if given segment is within the kernel & initrd mapping region. */
bool vmm_guest_mem_check_elf_segment(thread_rec_t *resource,
        uint32_t addr_start, uint32_t addr_end);

/* Get the very bottom address of any mapped RAM. */
uint32_t vmm_guest_mem_get_max_e820_addr(thread_rec_t *resource);

#endif /* __LIB_VMM_GUEST_MEMORY_H_ */

