/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Internal interfaces for booting the system. */
/* This library uses twinkle as the allocator. */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <autoconf.h>
#include <simple/simple.h>
#include <simple-stable/simple-stable.h>
#include <allocman/allocman.h>
#include <sel4platsupport/platsupport.h>
#include "vmm/platform/boot.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/platform/ram_alloc.h"
#include <simple/simple_helpers.h>

#ifndef CONFIG_VMM_INIT_MEMPOOL_SIZE
    #define CONFIG_VMM_INIT_MEMPOOL_SIZE (8 * 1024 * 1024)
#endif

/* The CPIO archive. */
extern char _cpio_archive[];

/* The virtual address space and allocator of initial thread. */
vka_t vmm_plat_vka;
vspace_t vmm_plat_vspace;
sel4utils_alloc_data_t vmm_plat_vspace_data;
simple_t vmm_plat_simple;

/* Allocator. */
static allocman_t* vka_allocator_allocman;
static char vka_allocator_mempool[CONFIG_VMM_INIT_MEMPOOL_SIZE];

/* Resources for platform init. */
plat_net_t vmm_plat_net;  

/* Host internal address used for mapping, used by root init thread only. */
void *vmm_plat_map_frame_vaddr = (void*)0xD0000000;
void *vmm_plat_map_guest_frame_vaddr = (void *)0xD1000000;

/* Reservation area used by init thread, these areas used as temporary vaddress for ELF loading. */
reservation_t *reserve_area_map_frame;
reservation_t *reserve_area_map_guest_frame;

/* ---------------------------------------------------------------------------------------------- */

/* Bootstrap the VMM system from the seL4 root thread. This function should be called first. */
void vmm_plat_init(simple_t *simple, uint32_t num_guest_os, char *guest_kernel, char *guest_initrd, void **existing_frames) {
    int i;
    int error;
    int page_size;
    seL4_CPtr free_start;
    /* During early bootstrapping we may steal untypeds from simple.
     * They get recorded here so we do not pass them to the allocator */
    bool stole_untyped[simple_get_untyped_count(simple)];

    for (i = 0; i < simple_get_untyped_count(simple); i++) {
        stole_untyped[i] = false;
    }

#ifdef CONFIG_VMM_USE_4M_FRAMES
    page_size = seL4_4MBits;
#else
    page_size = seL4_PageBits;
#endif

    vmm_plat_simple = *simple;

    /* Calculate the first free cptr */
    free_start = simple_last_valid_cap(simple) + 1;

    /* Quickly steal any frame information. We are still bootstrapping so printing not allowed! */
    #if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)
        /* Initialise 1:1 physical memory allocator. */
        uint32_t kernel_initrd_sz = vmm_guest_mem_determine_kernel_initrd_size(
                num_guest_os, guest_kernel, guest_initrd);
        assert(kernel_initrd_sz);
        pframe_alloc_init(simple, LIB_VMM_GUEST_PHYSADDR_MIN, kernel_initrd_sz, page_size, stole_untyped, &free_start);
    #else
        #error "No DMA solution chosen. Comment this error out to boot VM without DMA."
    #endif

    /* Construct the allocator, manually giving it untypeds. */
    vka_allocator_allocman = bootstrap_use_current_1level(
            simple_get_cnode(simple),
            simple_get_cnode_size_bits(simple),
            free_start, BIT(simple_get_cnode_size_bits(simple)),
            CONFIG_VMM_INIT_MEMPOOL_SIZE, vka_allocator_mempool
    );
    assert(vka_allocator_allocman);
    for (i = 0; i < simple_get_untyped_count(simple); i++) {
        if (!stole_untyped[i]) {
            uint32_t size_bits;
            uintptr_t paddr;
            seL4_CPtr cap = simple_get_nth_untyped(simple, i, &size_bits, (uint32_t*)&paddr);
            cspacepath_t utslot = allocman_cspace_make_path(vka_allocator_allocman, cap);
            error = allocman_utspace_add_uts(vka_allocator_allocman, 1,
                    &utslot, &size_bits, (uint32_t*)&paddr);
            assert(!error);
        }
    }

    /* Get a VKA interface for the allocator */
    allocman_make_vka(&vmm_plat_vka, vka_allocator_allocman);

    /* Initialize the vspace */
    error = sel4utils_bootstrap_vspace(&vmm_plat_vspace, &vmm_plat_vspace_data,
            simple_get_init_cap(&vmm_plat_simple, seL4_CapInitThreadPD), &vmm_plat_vka, NULL, NULL, existing_frames);
    assert(!error);

    /* Now we can finally initialise the plat support library and start printing */
    platsupport_serial_setup_simple(&vmm_plat_vspace, &vmm_plat_simple, &vmm_plat_vka);

    dprintf(1, "plat: Initialising VMM ...\n");

    /* Print out the platform information. */
    simple_print(&vmm_plat_simple);

    /* Create reservations for temporary mapping locations in the host addr space. */
    reserve_area_map_frame = vspace_reserve_range_at(&vmm_plat_vspace,
            vmm_plat_map_frame_vaddr,
            (3 << seL4_PageBits), seL4_AllRights, 1);

    uint32_t initrd_size = vmm_guest_get_initrd_size(guest_initrd);
    reserve_area_map_guest_frame = vspace_reserve_range_at(&vmm_plat_vspace,
            vmm_plat_map_guest_frame_vaddr,
            initrd_size + (2 << page_size), seL4_AllRights, 1);
    if (!reserve_area_map_frame || !reserve_area_map_guest_frame) {
        panic("Could not create vspace reservations on host.");
    }


    /* Create platform communication endpoints. */
    vmm_plat_net.serial_ep = vka_alloc_endpoint_leaky(&vmm_plat_vka);
    assert(vmm_plat_net.serial_ep);
    dprintf(4, "plat: serial ep 0x%x\n", vmm_plat_net.serial_ep);

    vmm_plat_net.pci_ep = vka_alloc_endpoint_leaky(&vmm_plat_vka);
    assert(vmm_plat_net.pci_ep);
    dprintf(4, "plat: pci ep 0x%x\n", vmm_plat_net.pci_ep);

    vmm_plat_net.interrupt_ep = vka_alloc_endpoint_leaky(&vmm_plat_vka);
    assert(vmm_plat_net.interrupt_ep);
    dprintf(4, "plat: interrupt ep 0x%x\n", vmm_plat_net.interrupt_ep);

    vmm_plat_net.fault_ep = vka_alloc_endpoint_leaky(&vmm_plat_vka);
    assert(vmm_plat_net.fault_ep);
    dprintf(4, "plat: fault ep 0x%x\n", vmm_plat_net.fault_ep);

    if (!vmm_plat_net.serial_ep || !vmm_plat_net.pci_ep ||
        !vmm_plat_net.interrupt_ep || !vmm_plat_net.fault_ep) {
        panic("Failed to allocate platform endpoints.");
    }

    /* Initialise the guest OS physical RAM allocator. */
    guest_ram_alloc_init(vka_allocator_allocman);
}

/* Get frame cap that recorded in the cap array. */
void vmm_plat_get_frame_cap(thread_rec_t *thread, seL4_Word addr,
        unsigned int size_bits, seL4_CPtr *frame) {
    /* Calculate the cap no. according to mem start. */
    seL4_Word mem_start = thread->mem_areas[0].start;
    unsigned int cap_no = (addr - mem_start) / (1 << size_bits);
    assert(cap_no < LIB_VMM_FRAME_CAPS);
    *frame = thread->frame_caps[cap_no];
}


/* Record frame cap in the cap array. */
void vmm_plat_put_frame_cap(thread_rec_t *thread, seL4_Word addr,
        unsigned int size_bits, seL4_CPtr frame) {
    /* Calculate the cap no. according to mem start. */
    seL4_Word mem_start = thread->mem_areas[0].start;
    unsigned int cap_no = (addr - mem_start) / (1 << size_bits);
    assert(cap_no < LIB_VMM_FRAME_CAPS);
    thread->frame_caps[cap_no] = frame;
}

/* Mint a cap from init host thread's cspace to a thread's cspace. */
void vmm_plat_mint_cap(seL4_CPtr dest, seL4_CPtr src, thread_rec_t *thread) {
    cspacepath_t cap_src, cap_dest;
    int ret;

    seL4_CapData_t badge = seL4_CapData_Badge_new(thread->badge_no);
    vka_cspace_make_path(&vmm_plat_vka, src, &cap_src);
    
    cap_dest.root = thread->cnode;
    cap_dest.capPtr = dest;
    cap_dest.capDepth = LIB_VMM_CNODE_SIZE;

    ret = vka_cnode_mint(&cap_dest, &cap_src, seL4_AllRights, badge);
    assert(ret == seL4_NoError);
    dprintf(4, "plat: mint cap src 0x%x dest 0x%x  to %s space\n", src, dest, thread->thread_name);

}

/* Copy a cap from init host thread's cspace to a thread's cspace. */
void vmm_plat_copy_cap(seL4_CPtr dest, seL4_CPtr src, thread_rec_t *thread) {
    cspacepath_t cap_src, cap_dest;
    int ret;

    vka_cspace_make_path(&vmm_plat_vka, src, &cap_src);
    
    cap_dest.root = thread->cnode;
    cap_dest.capPtr = dest;
    cap_dest.capDepth = LIB_VMM_CNODE_SIZE;

    ret = vka_cnode_copy(&cap_dest, &cap_src, seL4_AllRights);
    assert(ret == seL4_NoError);
    dprintf(4, "plat: copy cap src 0x%x dest 0x%x  to %s space\n", src, dest, thread->elf_name);
}

/* ---------------------------------------------------------------------------------------------- */

/* Create a kernel thread, along with the most basic resources it needs to function.

  @param caps the resource structure of the thread
  @param vmm_ep the endpoint of the host thread (for guest OS only)
  @param flag  software flags defined in sys.h
  @param elf_name string containing name of elf file

*/
void vmm_plat_create_thread(thread_rec_t *caps, seL4_CPtr vmm_ep, unsigned int flag,
        char *elf_name, seL4_Word badge_no, char *thread_name) {

    int ret;
    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, 32 - LIB_VMM_CNODE_SIZE);

    assert(caps);
    memset(caps, 0, sizeof (thread_rec_t));
    caps->flag = flag;
    caps->elf_name = elf_name;
    caps->badge_no = badge_no;
    caps->thread_name = thread_name;

    dprintf(3, "plat: create thread %s flag %d badge_no %d\n", thread_name, flag, badge_no);

    /* Create Kernel thread objects - TCB, PD, Thread Root CNode*/

    /* Create TCB */
    caps->tcb = vka_alloc_tcb_leaky(&vmm_plat_vka); 
    assert(caps->tcb);

    /* Create Root CNode */
    caps->cnode = vka_alloc_cnode_object_leaky(&vmm_plat_vka, LIB_VMM_CNODE_SIZE);
    assert(caps->cnode);

    /* Create thread PD. */
    if (flag == LIB_VMM_GUEST_OS_THREAD) {
        /* For guest OS thread, create an EPT PD object to map into guest-physical. */
        caps->pd = vka_alloc_ept_page_directory_pointer_table_leaky(&vmm_plat_vka);
        assert(caps->pd);
    } else {
        /* For VMM host threads, create a normal PD to map into its vaddr. */
        caps->pd = vka_alloc_page_directory_leaky(&vmm_plat_vka);
        assert(caps->pd);
    } 

    /* Assign ASID*/
    #ifndef CONFIG_KERNEL_STABLE
    #ifdef CONFIG_ARCH_IA32
    ret = seL4_IA32_ASIDPool_Assign(seL4_CapInitThreadASIDPool, caps->pd);
    assert(ret == seL4_NoError);
    #else
        #error "Unsupported platform."
    #endif
    #endif

    dprintf(4, "cap: tcb 0x%x    cnode 0x%x  pd 0x%x\n", caps->tcb, caps->cnode, caps->pd);

    /* Create a vspace manager for the new thread. */
    sel4utils_get_vspace_leaky(&vmm_plat_vspace, &caps->vspace, &caps->vspace_data,
                               &vmm_plat_vka, caps->pd);
    
    /* Set up endpoints. */
    switch (flag) {
        case LIB_VMM_VMM_THREAD: {
            /* Create the main VM Exit endpoints, used by the host VMM thread to catch VM Exits of
               Guest OS. */
            int error;
            caps->vmm_ep = vka_alloc_endpoint_leaky(&vmm_plat_vka);
            assert(caps->vmm_ep);
            caps->async_vmm_ep = vka_alloc_async_endpoint_leaky(&vmm_plat_vka);
            assert(caps->async_vmm_ep);
            error = seL4_TCB_BindAEP(caps->tcb, caps->async_vmm_ep);
            assert(error == seL4_NoError);

            vmm_plat_mint_cap(LIB_VMM_GUEST_OS_FAULT_EP_CAP, caps->vmm_ep, caps);
            break;
        }
        case LIB_VMM_GUEST_OS_THREAD: {
            /* Mint the VM exit endpoint into the guest OS cspace. */
            assert(vmm_ep);
            vmm_plat_mint_cap(LIB_VMM_GUEST_OS_FAULT_EP_CAP, vmm_ep, caps);
            break;
        }
        default:
            break;
    }

    /* Link up endpoints and set up TCB. */
    switch (flag) {
        case LIB_VMM_VMM_THREAD: {
            ret = seL4_TCB_SetSpace(caps->tcb, LIB_VMM_FAULT_EP_CAP, caps->cnode,
                                cspace_root_data, caps->pd, seL4_NilData);
            assert(ret == seL4_NoError);

            ret = seL4_TCB_SetPriority(caps->tcb, LIB_VMM_VMM_PRIORITY);
            assert(ret == seL4_NoError);
            break;
        }
        case LIB_VMM_GUEST_OS_THREAD: {
            /* Set it as the guest OS fault endpoint. */
            ret = seL4_TCB_SetSpace(caps->tcb, LIB_VMM_GUEST_OS_FAULT_EP_CAP, caps->cnode,
                                cspace_root_data, caps->pd, seL4_NilData);
            assert(ret == seL4_NoError);

            /* Set guest OS priority. */
            ret = seL4_TCB_SetPriority(caps->tcb, LIB_VMM_GUEST_PRIORITY);
            assert(ret == seL4_NoError);

            /* Create a new VCPU object for new guest OS. */
            caps->vcpu = vka_alloc_vcpu_leaky(&vmm_plat_vka);
            assert(caps->vcpu);
            dprintf(4, "cap: vcpu 0x%x\n", caps->vcpu);

            /* Bind the guest OS thread TCB with its VCPU. */
            ret = seL4_IA32_VCPU_SetTCB(caps->vcpu, caps->tcb);
            assert(ret == seL4_NoError);

            /* Bind the IO port with VCPU. This will init the IO bitmap into all 1s. */
            ret = seL4_IA32_VCPU_SetIOPort(caps->vcpu, 
                simple_get_IOPort_cap(&vmm_plat_simple, 0, /* give it all 2^16 io ports */(1<<16)-1));
            assert(ret == seL4_NoError);
            break;
        }
        case LIB_VMM_DRIVER_THREAD: {
            /* Set up driver thread. */
            ret = seL4_TCB_SetSpace(caps->tcb, LIB_VMM_FAULT_EP_CAP, caps->cnode,
                                    cspace_root_data, caps->pd, seL4_NilData);
            assert(ret == seL4_NoError);

            ret = seL4_TCB_SetPriority(caps->tcb, LIB_VMM_DRIVER_PRIORITY);
            assert(ret == seL4_NoError);
            break;
        }
        default:
            panic("Unknown thread flag.");
            break;
    }
}

/* Allocate a single frame for host / guest therad. */
seL4_CPtr vmm_plat_allocate_frame_cap(thread_rec_t *resource, uint32_t vaddr,
        uint32_t size_bits, uint16_t type) {

    seL4_CPtr frame = 0;
    assert(resource);
    assert(type != E820_RESERVED);
    (void) vaddr;
    
    if (type != DMA_IGNORE &&
        resource->flag == LIB_VMM_GUEST_OS_THREAD) {
         #if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)
            /* DMA support via 1:1 physical mapping. */
            uint32_t paddr;
            pframe_alloc(&vmm_plat_vka, &frame, &paddr);

//            dprintf(2, "    pframe_alloc mapping vaddr 0x%x using paddr 0x%x\n", vaddr, paddr);

            /* Check that the paddr lines up with the vaddr. */
            if (vaddr != paddr) {
                printf("ERROR: misaligned vaddr 0x%x <--> paddr 0x%x.\n", vaddr, paddr);
                printf("       This is possibly caused by guest memory misconfiguration in\n");
                printf("       guest_memory.h. Please make sure your kernel is compiled with\n");
                printf("       suitable start address, and that the first page is special\n");
                printf("       cased, and that the pframe_alloc allocator is initialising\n");
                printf("       correctly.\n");
                panic("misaligned vaddr <--> paddr for DMA 1:1 mapping.");
            }
         #else
             /* No DMA support.top
              * jAllocate frame as normal. */
             frame = vka_alloc_frame_leaky(&vmm_plat_vka, size_bits);
         #endif
     } else {
         /* Allocate frame as normal from the host allocator. */
         frame = vka_alloc_frame_leaky(&vmm_plat_vka, size_bits);
     }

     assert(frame);
     return frame;
}

/* Pre-allocate the frames for a host or guest thread. */
void vmm_plat_init_thread_mem(thread_rec_t *resource) {
    seL4_Word start, end, type;
    seL4_Word size_bits = seL4_PageBits;
    seL4_CapRights rights = seL4_AllRights;

    int ret;
    seL4_CPtr frame = 0;
    reservation_t * reserve_area = NULL;

    dprintf(4, "plat: init thread mem %s\n", resource->thread_name);
   
    /* GUEST OS uses 4M pages*/
    if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
#ifdef CONFIG_VMM_USE_4M_FRAMES
        size_bits = seL4_4MBits;
#endif
    }

    for (int i = 0; i < resource->n_mem_areas; i++) {
        start = resource->mem_areas[i].start;
        end = resource->mem_areas[i].end;
        type = resource->mem_areas[i].type;
        rights =  resource->mem_areas[i].perm;

        dprintf(4, "mem area %d start 0x%x end 0x%x\n", i, start, end);

        /* Only reserve for host thread, as EPT interface does not need it. */
        if (resource->flag != LIB_VMM_GUEST_OS_THREAD) {
            /* Reserve the memory region, check the return value. */
            reserve_area = vspace_reserve_range_at(&resource->vspace, (void *)start,
                    end - start, rights, 1);
            assert(reserve_area);
        }

        if (type == E820_RESERVED) {
            /* Don't bother mapping reserved regions. */
            continue;
        }
 
#ifdef CONFIG_VMM_ZONE_DMA_WORKAROUND
        if (type == DMA_ZONE) {
            /* We handle zone DMA later. */
            continue;
        }
#else
        /* Otherwise treat zone DMA region as ordinary RAM. */
        if (type == DMA_ZONE) {
            type = DMA_IGNORE;
        }
#endif

        while (start < end) {
            /* Allocate a frame. */
            frame = vmm_plat_allocate_frame_cap(resource, start, size_bits, type);
  
            /* Record the frame cap. */
            vmm_plat_put_frame_cap(resource, start, size_bits, frame);

            /* Make sure start is aligned to the current page size. */
            dprintf(5, "start 0x%x size bits %d frame cap 0x%x\n", start, size_bits, frame);
            assert(start % (1 << size_bits) == 0);

            if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
                /* Map the frame into guest physical address with EPT. */
                /* FIXME: Qian replace the vspace interface later */
                ret = sel4util_map_guest_physical_pages_at_vaddr(
                    &resource->vspace, &frame, (void *)start,
                    rights, 1, size_bits, 1
                );
            } else {
                /* Map the frame into vmm address space with PT. */
                ret = vspace_map_pages_at_vaddr(&resource->vspace, &frame,
                    (void *)start, 1, size_bits,
                    reserve_area
                ); 
            }
            assert(ret == seL4_NoError);

            start += 1 << size_bits;
        }
    }
}


/* Load an ELF file segment into thread address space. Used for both host and guest threads. The
   frame allocation is based on 4M page size.
 
   @param dest_addr  executing address of the elf image
 
 */
void vmm_plat_load_segment(thread_rec_t *resource, seL4_Word source_addr,
        seL4_Word dest_addr, unsigned int segment_size, unsigned int file_size) {

    int ret;
    seL4_CPtr slot = 0, frame = 0;
    cspacepath_t dest_frame_cap, src_frame_cap;
    unsigned int copy_len, offset, current = 0, remain = file_size;
    void *copy_vaddr = NULL, *map_vaddr = vmm_plat_map_frame_vaddr; 
    unsigned int page_size = seL4_PageBits; 
    reservation_t *reserve_area = reserve_area_map_frame;

    /* Memory managment schemes used by guest OS. */
    if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
        map_vaddr = vmm_plat_map_guest_frame_vaddr;
        reserve_area = reserve_area_map_guest_frame;
#ifdef CONFIG_VMM_USE_4M_FRAMES
        page_size = seL4_4MBits;
#endif
    }

    assert(file_size <= segment_size);

    /* Allocate a cap slot to copy the dest frame, so we can map it into our own vspace. */
    vka_cspace_alloc(&vmm_plat_vka, &slot);
    assert(slot);
    vka_cspace_make_path(&vmm_plat_vka, slot, &dest_frame_cap);

    while (current < file_size) {
        /* Retrieve the pre-allocated frame cap. */
        vmm_plat_get_frame_cap(resource, dest_addr, page_size, &frame); 
        vka_cspace_make_path(&vmm_plat_vka, frame, &src_frame_cap);

        /* Copy the cap so we can map it too. */
        ret = vka_cnode_copy(&dest_frame_cap, &src_frame_cap, seL4_AllRights); 
        assert(ret == 0); 

        /* Map the frame temporarily into our vspace in order to write to it. */
        ret = vspace_map_pages_at_vaddr(&vmm_plat_vspace, &dest_frame_cap.capPtr, map_vaddr,
                                        1, page_size, reserve_area);
        assert(ret == seL4_NoError);

        /* Copy the contents of page from ELF into the mapped frame. */

        offset = dest_addr & ((1 << page_size) - 1); 
        copy_vaddr = map_vaddr + offset;
        copy_len = (1 << page_size) - offset;
        
        if (copy_len > remain) {
            /* Don't copy past end of data. */
            copy_len = remain;
        }

        dprintf(5, "load page src 0x%x dest 0x%x remain 0x%x offset 0x%x copy vaddr %p "
                "copy len 0x%x\n", source_addr, dest_addr, remain, offset, copy_vaddr, copy_len);

        memcpy(copy_vaddr, (void*)source_addr, copy_len);
        source_addr += copy_len;
        dest_addr += copy_len;

        current += copy_len;
        remain -= copy_len;

        /* Unmap the frame from current vspace after we're finished writing to it. */
        ret = vspace_unmap_reserved_pages(&vmm_plat_vspace, map_vaddr, 1, page_size, reserve_area);
        assert(ret == 0);
    
        /* Delete the temporary copied cap to use for the next frame. */
        vka_cnode_delete(&dest_frame_cap);
    }
}
        

/* Convert ELF permissions into seL4 permissions. */
seL4_Word vmm_get_seL4_rights_from_elf(unsigned long permissions) {
    seL4_Word result = 0;
    if (permissions & PF_R) {
        result |= seL4_CanRead;
    }
    if (permissions & PF_X) {
        result |= seL4_CanRead;
    }
    if (permissions & PF_W) {
        result |= seL4_CanWrite;
    }
    return result;
}

/* Parse the ELF file, then allocate and fill out the mem area information.
   This function is used for creating host thread's vspace.
 */
void vmm_plat_elf_parse (char *elf_name, unsigned int *n_areas, guest_mem_area_t **areas) {
    seL4_Word dest_addr, seg_end;
    unsigned int segment_size, n_headers, actual_headers = 0; 
    seL4_Word last_page_start = 0;

    /* Lookup the ELF file. */
    long unsigned int elf_size = 0;
    char *elf_file = cpio_get_file(_cpio_archive, elf_name, &elf_size);
    if (!elf_file || !elf_size) {
        printf("ERROR: ELF file %s not found in CPIO archive. Are you sure your Makefile\n"
               "is set up correctly to compile the above executable into the CPIO archive of this\n"
               "executable's binary?\n", elf_name);
        panic("ELF file not found in CPIO archive.");
        return;
    }

    /* Check the ELF file. */ 
    if (elf_checkFile(elf_file)) {
        panic("ELF file check failed.");
        return;
    }

    n_headers = elf_getNumProgramHeaders(elf_file);
    assert(n_headers);

    /* Create the mem area structure. */
    guest_mem_area_t *mem_areas = malloc(n_headers * sizeof (guest_mem_area_t));
    assert(mem_areas);
    memset(mem_areas, 0, n_headers * sizeof (guest_mem_area_t));

    for (int i = 0; i < n_headers; i++) {
        segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
        dest_addr = (seL4_Word)elf_getProgramHeaderVaddr(elf_file, i);
        seg_end = dest_addr + segment_size;

        /* If the segment is already covered by previous segment, we simply ignore it. */
        if (seg_end <= last_page_start + LIB_VMM_HOST_PAGE_SIZE) {
            continue;
        }

        /* Host thread pages are 4K, so segments must be 4K aligned. */
        dest_addr &= LIB_VMM_HOST_PAGE_MASK;
        if (seg_end % LIB_VMM_HOST_PAGE_SIZE) {
            seg_end = (seg_end & LIB_VMM_HOST_PAGE_MASK) + LIB_VMM_HOST_PAGE_SIZE;
        }

        /* Move the start of this segment if there is a collison. */
        if (dest_addr == last_page_start) {
            dest_addr += LIB_VMM_HOST_PAGE_SIZE;
        }

        /* Write to new mem segment. */
        mem_areas[i].start = dest_addr;
        mem_areas[i].end = seg_end;
        mem_areas[i].type = E820_RAM;
        mem_areas[i].perm = seL4_AllRights;

        last_page_start = seg_end - LIB_VMM_HOST_PAGE_SIZE;
        actual_headers++;
    }

    /* Output regions array. */
    *n_areas = actual_headers;
    *areas = mem_areas;
}

/* Load the actual ELF file contents into pre-allocated frames.
   Used for both host and guest threads.

 @param resource    the thread structure for resource 
 @param elf_name    the name of elf file 
 
*/
void vmm_plat_elf_load(thread_rec_t *resource) {
    unsigned int n_headers;
    seL4_Word stack_top = 0;

    dprintf(4, "plat: ELF load %s \n", resource->thread_name);

    /* Lookup the ELF file. */
    long unsigned int elf_size = 0;
    char *elf_name = resource->elf_name;
    char *elf_file = cpio_get_file(_cpio_archive, elf_name, &elf_size);
    if (!elf_file || !elf_size) {
        printf("ERROR: ELF file %s not found in CPIO archive. Are you sure your Makefile\n"
               "is set up correctly to compile the above executable into the CPIO archive of this\n"
               "executable's binary?\n", elf_name);
        panic("ELF file not found in CPIO archive.");
        return;
    }

    /* Check the ELF file. */ 
    if (elf_checkFile(elf_file)) {
        panic("ELF file check failed.");
        return;
    }

    n_headers = elf_getNumProgramHeaders(elf_file);

    resource->guest_kernel_addr = 0;
    resource->guest_kernel_vaddr = 0;
    resource->guest_relocation_offset = 0;

    #if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)
    if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
        /* Calculate relocation offset. */
        resource->guest_kernel_addr = 0xFFFFFFFF;
        resource->guest_kernel_vaddr = 0xFFFFFFFF;
        for (int i = 0; i < n_headers; i++) {
            if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD) {
                continue;
            }
            uint32_t addr = elf_getProgramHeaderPaddr(elf_file, i);
            if (addr < resource->guest_kernel_addr) {
                resource->guest_kernel_addr = addr;
            }
            uint32_t vaddr = elf_getProgramHeaderVaddr(elf_file, i);
            if (vaddr < resource->guest_kernel_vaddr) {
                resource->guest_kernel_vaddr = vaddr;
            }
        }
        dprintf(2, "plat: guest kernel is compiled to be located at paddr 0x%x vaddr 0x%x\n",
                resource->guest_kernel_addr, resource->guest_kernel_vaddr);
        assert(resource->guest_mem_start);
        dprintf(2, "plat: guest kernel allocated 1:1 start is at paddr = 0x%x\n",
                resource->guest_mem_start);
        resource->guest_relocation_offset = (int)(
            (int64_t)resource->guest_mem_start - (int64_t)resource->guest_kernel_addr
        );
        dprintf(2, "plat: therefore relocation offset is %d (%s0x%x)\n",
                resource->guest_relocation_offset,
                resource->guest_relocation_offset < 0 ? "-" : "",
                abs(resource->guest_relocation_offset));
    }
    #endif /* LIB_VMM_GUEST_DMA_ONE_TO_ONE */

    for (int i = 0; i < n_headers; i++) {
        seL4_Word source_addr, dest_addr;
        unsigned int file_size, segment_size;

        /* Skip unloadable program headers. */
        if (elf_getProgramHeaderType(elf_file, i) != PT_LOAD) {
            continue;
        }

        /* Fetch information about this segment. */
        source_addr = (seL4_Word)elf_file + elf_getProgramHeaderOffset(elf_file, i);
        file_size = elf_getProgramHeaderFileSize(elf_file, i);
        segment_size = elf_getProgramHeaderMemorySize(elf_file, i);

        dest_addr = (seL4_Word)elf_getProgramHeaderPaddr(elf_file, i);
        dest_addr += resource->guest_relocation_offset;

        if (!segment_size) {
            /* Zero sized segment, ignore. */
            continue;
        }

        if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
            /* Quick sanity check here to make sure kernel ELF segments are within bounds. */
            if (!vmm_guest_mem_check_elf_segment(resource, dest_addr, dest_addr + segment_size)) {
                panic("Invalid configuration detected.");
            }
        }

        /* Load this ELf segment. */
        vmm_plat_load_segment(resource, source_addr, dest_addr, segment_size, file_size);

        /* Save last segment location to check for collision with stack_top. */
        if (dest_addr + segment_size > stack_top) {
            stack_top = dest_addr + segment_size;
        }
    }

    /* Record the entry point. */
    resource->elf_entry = elf_getEntryPoint(elf_file);
    resource->elf_entry += resource->guest_relocation_offset;

    /* Assign some arbitrary really high address for stack, and check that it's actually above the
     * last segment of the ELF file.
     * The actual stack frames are mapped later.
     */
    seL4_Word _stack_top_check = (stack_top & LIB_VMM_STACK_PAGE_MASK) +
        (1 + LIB_VMM_STACK_PAGES) * (1 << seL4_PageBits);
    resource->stack_top = 0x40000000;
    assert(_stack_top_check < resource->stack_top);
    dprintf(5, " top of elf 0x%x stack top 0x%x\n", stack_top, resource->stack_top);
}

/* Set up the entire vspace layout of host / guest threads from its assigned ELF file.
   Used to set up vspace for host threads, and to set up EPT guest-physical-space for guest OS
   threads. First pre-allocated and maps all the areas, and then fills it with ELF file
   contents.
   Used for both host and guest threads.
 */
void vmm_plat_create_thread_vspace(thread_rec_t *resource,
                                   unsigned int n_mem_area,
                                   guest_mem_area_t *mem_areas) {
    assert(mem_areas);

    resource->n_mem_areas = n_mem_area;
    resource->mem_areas = mem_areas; 

    /* Update the start of mem area, used for calculating frame array. */
    resource->mem_start = resource->mem_areas[0].start;

    /* Map the mem areas into thread's address space. */
    vmm_plat_init_thread_mem(resource);

    /* Load the elf file into thread's address space. */
    vmm_plat_elf_load(resource);

    if (resource->flag == LIB_VMM_GUEST_OS_THREAD) {
        /* Set up things like device drivers. */
        vmm_plat_guest_setup_vspace(resource);
    } else {
        /* Set up things like IPC buffer & stack. */
        vmm_plat_host_setup_vspace(resource);
    }
}


