/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Common booting and startup functions. The interface here should be private, and this header file
 * should NOT be included by other parts of VMM which aren't part of the boot and startup. Currently
 * the only users of this interface should be
 *     vmm/platform/boot.c
 *     vmm/platform/boot_host.c
 *     vmm/platform/boot_guest.c
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 *         Xi (Ma) Chen
 *
 *     Thu 07 Nov 2013 20:14:12 EST 
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sel4/sel4.h>

#include <vka/vka.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <allocman/bootstrap.h>
#include <allocman/allocman.h>
#include <allocman/vka.h>
#include <pci/virtual_pci.h>
#include <sel4utils/vspace.h>
#include <elf/elf.h>
#include <cpio/cpio.h>
#include <simple/simple.h>

#include "vmm/config.h"
#include "vmm/vmm.h"
#include "vmm/vmcs.h"
#include "vmm/io.h"
#include "vmm/platform/sys.h"
#include "vmm/platform/bootinfo.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/platform/guest_devices.h"
#include "vmm/platform/phys_alloc.h"
#include "vmm/driver/pci.h"

/* The virtual address space and allocator of initial thread. */
extern simple_t vmm_plat_simple;
extern vka_t vmm_plat_vka;
extern struct allocator *vmm_plat_alloc;
extern vspace_t vmm_plat_vspace;
extern sel4utils_alloc_data_t vmm_plat_vspace_data;

/* Resources for platform init. */
extern plat_net_t vmm_plat_net;

/* Host internal address used for mapping, used by root init thread only. */
extern void *vmm_plat_map_frame_vaddr;
extern void *vmm_plat_map_guest_frame_vaddr;

/* Reservation area used by init thread, these areas used as temporary vaddress for ELF loading. */
extern reservation_t *reserve_area_map_frame;
extern reservation_t *reserve_area_map_guest_frame;


/* ---------------------------------------------------------------------------------------------- */

/* Get frame cap that recorded in the cap array. */
void vmm_plat_get_frame_cap(thread_rec_t *thread, seL4_Word addr, unsigned int size_bits,
                            seL4_CPtr *frame);

/* Record frame cap in the cap array. */
void vmm_plat_put_frame_cap(thread_rec_t *thread, seL4_Word addr, unsigned int size_bits,
                            seL4_CPtr frame);

/* Mint a cap from init host thread's cspace to a thread's cspace. */
void vmm_plat_mint_cap(seL4_CPtr dest, seL4_CPtr src, thread_rec_t *thread);

/* Copy a cap from init host thread's cspace to a thread's cspace. */
void vmm_plat_copy_cap(seL4_CPtr dest, seL4_CPtr src, thread_rec_t *thread);

/* Create a kernel thread, along with the most basic resources it needs to function.

  @param caps the recource structure of the thread
  @param vmm_ep the endpoint of the host thread (for guest OS only)
  @param flag  software flags defined in sys.h
  @param elf_name string containing name of elf file

*/
void vmm_plat_create_thread(thread_rec_t *caps, seL4_CPtr vmm_ep, unsigned int flag,
        char *elf_name, seL4_Word badge_no, char *thread_name);

/* Allocate a single frame for host / guest therad. */
seL4_CPtr vmm_plat_allocate_frame_cap(thread_rec_t *resource, uint32_t vaddr,
        uint32_t size_bits, uint16_t type);

/* Pre-allocate the frames for a host or guest thread. */
void vmm_plat_init_thread_mem(thread_rec_t *resource);

/* Load an ELF file segment into thread address space. Used for both host and guest threads. The
   frame allocation is based on 4M page size.
 
   @param dest_addr  executing address of the elf image
 
 */
void vmm_plat_load_segment(thread_rec_t *resource, seL4_Word source_addr,
        seL4_Word dest_addr, unsigned int segment_size, unsigned int file_size);

/* Convert ELF permissions into seL4 permissions. */
seL4_Word vmm_get_seL4_rights_from_elf(unsigned long permissions);

/* Parse the ELF file, then allocate and fill out the mem area information.
   This function is used for creating host thread's vspace.
 */
void vmm_plat_elf_parse (char *elf_name, unsigned int *n_areas, guest_mem_area_t **areas);

/* Load the actual ELF file contents into pre-allocated frames.
   Used for both host and guest threads.

 @param resource    the thread structure for resource 
 @param elf_name    the name of elf file 
 
*/
void vmm_plat_elf_load(thread_rec_t *resource);

/* Set up the entire vspace layout of host / guest threads from its assigned ELF file.
   Used to set up vspace for host threads, and to set up EPT guest-physical-space for guest OS
   threads. First pre-allocated and maps all the areas, and then fills it with ELF file
   contents.
   Used for both host and guest threads.
 */
void vmm_plat_create_thread_vspace(thread_rec_t *resource,
                                   unsigned int n_mem_area,
                                   guest_mem_area_t *mem_areas);

/* ---------------------------------------------------------------------------------------------- */

/* Set up guest-specific vspace mappings such as devices. Only used for guest OS threads.
   This function assumes that the PCI driver host thread has already been started.
   If it is not then this function will block indefinitely.
 */
void vmm_plat_guest_setup_vspace(thread_rec_t *resource);

/* Set up seL4-specific things such as IPC buffer. Used for host threads only.
   Assumes the vspace has already been set up, and the ELF file has been loaded.
 */
void vmm_plat_host_setup_vspace(thread_rec_t *resource);

