/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* Booting and setup functions used for guest OS threads only.
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 *         Xi (Ma) Chen
 *
 *     Fri 22 Nov 2013 05:13:50 EST
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <autoconf.h>
#include <cpio/cpio.h>
#include "vmm/config.h"
#include "vmm/platform/boot.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/platform/guest_devices.h"
#include "vmm/platform/phys_alloc.h"
#include "vmm/platform/ram_alloc.h"
#include <sel4utils/mapping.h>

#include <simple/simple.h>
#include <simple-stable/simple-stable.h>

#ifdef CONFIG_VMM_VESA_FRAMEBUFFER
#include <sel4/arch/bootinfo.h>
#endif

#define VMM_PLAT_ELFNAME_MAXLEN 256
#define VADDR_KERNEL_RESERVED 0xE0000000 /* seL4 kernel restriction. */
extern char _cpio_archive[];

extern simple_t vmm_plat_simple;

/* ---------------------------------------------------------------------------------------------- */

static void vmm_plat_touch_guest_frame(thread_rec_t *resource, uint32_t addr, uint32_t size,
                                       void (*touch_frame_ptr)(thread_rec_t *, void *));

/* Copy the vcpu cap of the guest thread into host thread. */
static void vmm_plat_copy_vcpu(thread_rec_t *host_thread, thread_rec_t *guest_thread) {
    vmm_plat_copy_cap(LIB_VMM_VCPU_CAP, guest_thread->vcpu, host_thread);
}

static void vmm_plat_copy_guest_tcb(thread_rec_t *host_thread, thread_rec_t *guest_thread) {
    vmm_plat_copy_cap(LIB_VMM_GUEST_TCB_CAP, guest_thread->tcb, host_thread);
}

static void vmm_plat_guest_elf_relocate_address(thread_rec_t *resource, void* vaddr) {
    assert(resource && vaddr);
    *((uint32_t*)vaddr) += resource->guest_relocation_offset;
}

void vmm_plat_guest_elf_relocate(thread_rec_t *resource) {
    int delta = resource->guest_relocation_offset;
    if (delta == 0) {
        /* No relocation needed. */
        return;
    }

    dprintf(1, "plat: relocating guest kernel from 0x%x --> 0x%x\n", resource->guest_kernel_addr,
            resource->guest_kernel_addr + delta);

#ifndef CONFIG_VMM_VMLINUZ_COMPRESSED
    /* Work out the relocation data file by adding .relocs to the end of the ELF name. */
    char *elf_name = resource->elf_name;
    static char elf_relocs_name[VMM_PLAT_ELFNAME_MAXLEN];
    snprintf(elf_relocs_name, VMM_PLAT_ELFNAME_MAXLEN, "%s.relocs", elf_name);
#else
    /* Assume the kernel binary has the .relocs file concaternated to the end. */
    char *elf_relocs_name = resource->elf_name;
#endif

    /* Open the relocs file. */
    dprintf(2, "plat: opening relocs file %s\n", elf_relocs_name);
    long unsigned int relocs_size = 0;
    char* relocs_file = cpio_get_file(_cpio_archive, elf_relocs_name, &relocs_size);
    if (!relocs_file) {
        printf(COLOUR_Y "ERROR: Guest OS kernel relocation is required, but corresponding"
               "%s was not found in CPIO archive. This is most likely due to a Makefile"
               "error, or configuration error.\n", elf_relocs_name);
        panic("Relocation required but relocation data file not found.");
        return;
    }

    /* The relocs file is the same relocs file format used by the Linux kernel decompressor to
     * relocate the Linux kernel:
     *
     *     0 - zero terminator for 64 bit relocations
     *     64 bit relocation repeated
     *     0 - zero terminator for 32 bit relocations
     *     32 bit relocation repeated
     *     <EOF>
     *
     * So we work backwards from the end of the file, and modify the guest kernel OS binary.
     * We only support 32-bit relocations, and ignore the 64-bit data.
     *
     * src: Linux kernel 3.5.3 arch/x86/boot/compressed/misc.c
     */
    uint32_t last_relocated_vaddr = 0xFFFFFFFF;
    uint32_t num_relocations = relocs_size / sizeof(uint32_t) - 1;
    for (int i = 0; ; i++) {
        /* Get the next relocation from the relocs file. */
        uint32_t vaddr = *( ((uint32_t*)(relocs_file + relocs_size)) - (i+1) );
        if (!vaddr) {
            #ifndef CONFIG_VMM_VMLINUZ_COMPRESSED
                assert(num_relocations == i);
            #endif
            break;
        }
        assert(i * sizeof(uint32_t) < relocs_size);
        last_relocated_vaddr = vaddr;

        /* Calculate the corresponding guest-physical address at which we have already
           allocated and mapped the ELF contents into. */
        assert(vaddr >= resource->guest_kernel_vaddr);
        uint32_t guest_paddr = vaddr - resource->guest_kernel_vaddr +
            (resource->guest_kernel_addr + delta);
        assert(vmm_guest_mem_check_elf_segment(resource, guest_paddr, guest_paddr + 4));

        /* Perform the relocation. */
        dprintf(5, "   reloc vaddr 0x%x guest_addr 0x%x\n", vaddr, guest_paddr);
        vmm_plat_touch_guest_frame(resource, guest_paddr, sizeof(int),
                vmm_plat_guest_elf_relocate_address);

        if (i && i % 50000 == 0) {
            dprintf(2, "    %u relocs done.\n", i);
        }
    }
    dprintf(3, "plat: last relocated addr was %d\n", last_relocated_vaddr);
    dprintf(2, "plat: %d kernel relocations completed.\n", num_relocations);
    (void) last_relocated_vaddr;

    if (num_relocations == 0) {
        panic("Relocation required, but Kernel has not been build with CONFIG_RELOCATABLE.");
    }
}

/* Callback function which writes the initrd from the CPIO archive into the guest EPT pages. */
static void vmm_plat_guest_write_initrd(thread_rec_t *resource, void* vaddr) {
#ifdef LIB_VMM_INITRD_SUPPORT
    assert(resource && vaddr);
    assert(resource->guest_initrd_filename);

    /* First find the file in CPIO archive. */
    dprintf(2, "plat: finding initrd file [%s] in CPIO archive..\n",
            resource->guest_initrd_filename);
    unsigned long initrd_size = 0; 
    char *initrd = cpio_get_file(_cpio_archive, resource->guest_initrd_filename, &initrd_size);
    if (!initrd) {
        printf("ERROR: initrd file %s not found.\n", resource->guest_initrd_filename);
        panic("initrd file not found in cpio archive.");
        return;
    }
    if (!initrd_size) {
        printf("WARNING: initrd has zero size. This is probably not what you want.\n");
        return;
    }

    /* Finally memcpy it into the frame. */
    dprintf(2, "plat: loading initrd CPIO into guest memory, size 0x%x.....\n",
            (uint32_t) initrd_size);
    memcpy(vaddr, initrd, initrd_size);
    resource->guest_initrd_size = initrd_size;

#else
    printf("WARNING: vmm_plat_guest_write_initrd() called for no reason.\n");
    return;
#endif
}


#ifdef CONFIG_VMM_VESA_FRAMEBUFFER
static inline uint32_t vmm_plat_vesa_fbuffer_size(seL4_IA32_BootInfo *bi) {
    assert(bi);
    return ALIGN_UP(bi->vbeModeInfoBlock.bytesPerScanLine * bi->vbeModeInfoBlock.yRes, 65536);
}
#endif

static int vmm_plat_copy_map_frame_iospace(thread_rec_t *resource, uintptr_t vaddr, int page_size, seL4_CPtr frame) {
#ifdef LIB_VMM_GUEST_DMA_IOMMU
    int error;
    /* Map into all the IOspaces */
    for (int j = 0; j < resource->num_pci_iospace; j++) {
        cspacepath_t src, dest;
        /* dupliacate the frame cap */
        error = vka_cspace_alloc_path(&vmm_plat_vka, &dest);
        assert(!error);
        vka_cspace_make_path(&vmm_plat_vka, frame, &src);
        error = vka_cnode_copy(&dest, &src, seL4_AllRights);
        assert(!error);
        /* map into the iospace */
        error = sel4utils_map_iospace_page(&vmm_plat_vka, resource->pci_iospace_caps[j], dest.capPtr, vaddr, seL4_AllRights, 1, page_size, NULL, NULL);
        assert(!error);
    }
#endif
    return 0;
}

/* Map a device frame into the guest's EPT. */
void vmm_plat_guest_map_device_frame(thread_rec_t *resource, void *vaddr,
                                     uint32_t paddr, uint32_t size, bool zone_dma) {
    assert(resource);
    assert(size);

    /* Try and work out what size frames we should be using */
    int size_bits = ((size & MASK(seL4_4MBits)) == 0) ? seL4_4MBits : seL4_PageBits;
#ifndef CONFIG_VMM_USE_4M_FRAMES
    size_bits = seL4_PageBits;
#endif
    dprintf(2, "plat:    mapping 1:1 device region paddr 0x%x size 0x%x, frame size %d.\n",
            paddr, size, size_bits);

//    assert(size % BIT(size_bits) == 0);

    /* Map this region into Guest OS EPT. */
    cspacepath_t frame;
    uint32_t cpaddr;
    for (cpaddr = paddr; cpaddr < paddr + size; cpaddr += BIT(size_bits)) {
        /* Allocate a cap slot for the frame copy. */
        int ret = vka_cspace_alloc_path(&vmm_plat_vka, &frame);
        assert(ret == seL4_NoError && frame.capPtr);

        ret = simple_get_frame_cap(&vmm_plat_simple, (void*)cpaddr, size_bits, &frame);
        if (ret != seL4_NoError) {

            /* There may be a device the kernel didn't detect. We print a verbose error message
             * here for easy debugging. */
            printf(COLOUR_Y "VMM WARNING: " COLOUR_RESET
                "PCI device region 0x%x not found to be exported by the seL4 kernel.\n",
                paddr);
            printf("This most likely means the Guest OS kernel will EPT fault and die.\n");
            continue;
        }

        /* Calculate the vaddr to map at by offseting the vaddr along with paddr. */
        void *cvaddr = (void*) ( ((uint32_t) vaddr) + (cpaddr - paddr) );

        /* Quick sanity checks on the addr, in hopes of avoiding insane hard to find bugs
         * later on. */
        if (!zone_dma && (uint32_t) cvaddr <=
                resource->mem_areas[resource->n_mem_areas - 1].end) {
            printf("ERROR: vaddr 0x%x overlaps guest OS physical mem region.\n",
                   (uint32_t) cvaddr);
            printf("       This is most likely due to a misconfiguration of map_offset.\n");
            panic("VMM guest_device misconfiguration.");
            continue;
        }

        if ((uint32_t) cvaddr >= VADDR_KERNEL_RESERVED) {
            printf("ERROR: vaddr 0x%x is in seL4 kernel reserved region.\n",
                    (uint32_t) cvaddr);
            printf("       This is most likely due to a misconfiguration of map_offset.\n");
            panic("VMM guest_device misconfiguration.");
            continue;
        }

        /* Map IO device frame into guest. This can be mapped in cached as
         * the guest paging structures can override the caching information
         * this way the guest can take advantage of write combining if possible */
        dprintf(4, "plat:    mapping frame cptr=0x%x paddr=0x%x into guest @ va 0x%x.\n",
                frame.capPtr, cpaddr, (uint32_t) cvaddr);
        ret = sel4util_map_guest_physical_pages_at_vaddr(
            &resource->vspace, &frame.capPtr, cvaddr,
            seL4_AllRights, 1, size_bits, 0);
        assert(ret == seL4_NoError);
        ret = vmm_plat_copy_map_frame_iospace(resource, (uintptr_t)cvaddr, size_bits, frame.capPtr);
        assert(!ret);
    }
}

/* Map a 4k non-DMA capable frame into the guest's EPT. */
void vmm_plat_guest_map_nondma_frame(thread_rec_t *resource, void *vaddr) {
    seL4_CPtr frame = vmm_plat_allocate_frame_cap(resource, 0, seL4_PageBits, DMA_IGNORE);
    int ret = sel4util_map_guest_physical_pages_at_vaddr(&resource->vspace, &frame, vaddr,
            seL4_AllRights, 1, seL4_PageBits, 1);
    assert(ret == seL4_NoError);
    ret = vmm_plat_copy_map_frame_iospace(resource, (uintptr_t)vaddr, seL4_PageBits, frame);
    assert(!ret);
}

/* Comparison function for guest paddrs. */
static int vmm_plat_qsort_paddr_intcmp(const void* a, const void* b) {
    return *(uint32_t*)a - *(uint32_t*)b;
}

/* Give the guest blocks of mostly-contiguous RAM to use. The majority of guest OS RAM is
 * mapped using this function, with the exceptions of bootinfo structures, devices, kernel ELF
 * region and initrd image. This is because kernel ELF and initrd must reside in a single
 * contiguous block of mejmory, while the RAM we give the guest to use may be segmented to some
 * extent, as allowed by E820.
 */
static void vmm_plat_guest_setup_guest_memory(thread_rec_t *resource) {
    assert(resource);

    /* Allocate a paddr list which we will use to merge frame addresses into E820 segments. */
    int page_size;
#ifdef CONFIG_VMM_USE_4M_FRAMES
    page_size = seL4_4MBits;
#else
    page_size = seL4_PageBits;
#endif
    uint32_t npages = MBYTE(CONFIG_VMM_GUEST_RAM) >> page_size;
    uint32_t* paddr_list = (uint32_t*) malloc(npages * sizeof(uint32_t));
    assert(paddr_list);

    /* Allocate & map frames into guest OS. */
    seL4_CPtr frame = 0;
    uint32_t frame_paddr = 0;
    for (int i = 0; i < npages; i++) {
    #if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)
        /* Allocate frame. */
        int error = guest_ram_frame_alloc(&vmm_plat_vka, &frame, &frame_paddr);
        if (error) {
            panic("Out of memory, could not allocate guest RAM frame.");
            return;
        }
        assert(frame != 0);
        paddr_list[i] = frame_paddr;
        /* Map frame into guest EPT. */
        dprintf(3, "    ram_alloc mapping vaddr 0x%x using paddr 0x%x\n", frame_paddr, frame_paddr);
        error = sel4util_map_guest_physical_pages_at_vaddr(
            &resource->vspace, &frame, (void *)frame_paddr,
            seL4_AllRights, 1, page_size, 1
        );
        assert(error == seL4_NoError);
        error = vmm_plat_copy_map_frame_iospace(resource, frame_paddr, page_size, frame);
        assert(!error);
    #else
        #error "Guest RAM allocation and mapping strategy not implemented."
    #endif
    }

    /* Merge the paddr list into segments. */
    qsort(paddr_list, npages, sizeof(uint32_t), vmm_plat_qsort_paddr_intcmp);
    for (int i = 0; i < npages; i++) {
        if (i > 0 && paddr_list[i-1] + (1 << page_size) == paddr_list[i]) {
            /* Contiguous from last frame. */
            resource->guest_bootinfo_mem_areas[resource->n_guest_bootinfo_mem_areas - 1].end = 
                paddr_list[i] + (1 << page_size);
        } else {
            /* Discontinuity. Start a new E820 segment in guest OS. */
            assert(resource->n_guest_bootinfo_mem_areas < E820_MAX_REGIONS);
            guest_mem_area_t *area = &resource->guest_bootinfo_mem_areas
                [resource->n_guest_bootinfo_mem_areas];
            area->start = paddr_list[i];
            area->end = paddr_list[i] + (1 << page_size);
            area->type = E820_RAM;
            area->perm = seL4_AllRights;
            resource->n_guest_bootinfo_mem_areas++;
        }
    }
#ifdef CONFIG_VMM_USE_4M_FRAMES
    free(paddr_list);
#else
    printf("WARNING: Leaking memory because free is broken\n");
#endif

    /* Print the information for debugging. */
    for (int i = 0; i < resource->n_guest_bootinfo_mem_areas; i++) {
        guest_mem_area_t *area = &resource->guest_bootinfo_mem_areas[i];
        dprintf(2, "plat:     E820 region 0x%x --> 0x%x, type %d\n",
                area->start, area->end, area->type);
    }
}

/* Set up guest-specific vspace mappings such as devices. Only used for guest OS threads.
   This function assumes that the PCI driver host thread has already been started.
   If it is not then this function will block indefinitely.
 */
void vmm_plat_guest_setup_vspace(thread_rec_t *resource) {
    libpci_device_t dev;
    assert(resource);
    assert(vmm_plat_net.pci_ep); 

    if (resource->flag != LIB_VMM_GUEST_OS_THREAD) {
        panic("vmm_plat_guest_setup_vspace should only be called for guest threads");
        return;
    }

#ifdef CONFIG_VMM_GUEST_DMA_IOMMU
    /* First go through all the devices and create IO space capabilities */
    for (int i = 0; ; i++) {
        vmm_guest_device_cfg_t *dv = &vmm_plat_guest_allowed_devices[i];

        /* 0xFFFF vendor marks end of the allowed devices list. */
        if (dv->ven == 0xFFFF) {
            break;
        }

        for (int j = 0;; j++) {
            memset(&dev, 0, sizeof(libpci_device_t));
            bool found = vmm_driver_pci_find_device(vmm_plat_net.pci_ep,
                                                    dv->ven, dv->dev, &dev, j);
            if (!found) {
                break;
            }
            /* Construct new iospace cap */
            assert(resource->num_pci_iospace < LIB_VMM_MAX_PCI_DEVICES);
            seL4_CPtr cptr;
            int error = vka_cspace_alloc(&vmm_plat_vka, &cptr);
            assert(!error);
            cspacepath_t path;
            vka_cspace_make_path(&vmm_plat_vka, cptr, &path);
            cspacepath_t src;
            vka_cspace_make_path(&vmm_plat_vka, 
                simple_get_init_cap(&vmm_plat_simple, seL4_CapIOSpace), &src);
            /* Make a badge manually, as no way of specifying it properly. We use
             * a single domain ID for all devices, as we are going to provide
             * identical mappings for all of them */
            int domain_id = 0xf;
            int pci_dev = (dev.bus << 8) | (dev.dev << 3) | dev.fun;
            seL4_CapData_t data = (seL4_CapData_t) { .words = {(domain_id << 16) | pci_dev}};
            error = vka_cnode_mint(&path, &src, seL4_AllRights, data);
            assert(!error);
            resource->pci_iospace_caps[resource->num_pci_iospace] = cptr;
            resource->num_pci_iospace++;
        }
    }
    /* We mapped some memory initially in vmm_plat_init_thread_mem
     * we now need to add that to the iospace mappings.
     * This is directly duplicating a bunch of logic and should
     * be fixed */
    for (int i = 0; i < resource->n_mem_areas; i++) {
        seL4_Word start = resource->mem_areas[i].start;
        seL4_Word end = resource->mem_areas[i].end;
        seL4_Word type = resource->mem_areas[i].type;
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
            seL4_CPtr frame;
            vmm_plat_get_frame_cap(resource, start, seL4_PageBits, &frame);
            assert(frame);
            int error = vmm_plat_copy_map_frame_iospace(resource, start, seL4_PageBits, frame);
            assert(!error);
            start += BIT(seL4_PageBits);
        }
    }
#endif

    /* Map RAM into the guest's vspace. */
    vmm_plat_guest_setup_guest_memory(resource);

    /* Reset the guest's IO port mappings. */
    uint32_t guest_ioport_n = 0;
    memset(resource->guest_ioport_map, 0, LIB_VMM_GUEST_MAX_IOPORT_RANGES * sizeof(ioport_range_t));
    for (int i = 0; i < LIB_VMM_GUEST_MAX_IOPORT_RANGES; i++) {
        resource->guest_ioport_map[i].port_start = X86_IO_INVALID;
        resource->guest_ioport_map[i].port_end = X86_IO_INVALID;
        resource->guest_ioport_map[i].cap_no = LIB_VMM_CAP_INVALID;
    }

    /* Handle devices mappings into guest OS. */
    for (int i = 0; ; i++) {
        vmm_guest_device_cfg_t *dv = &vmm_plat_guest_allowed_devices[i];

        /* 0xFFFF vendor marks end of the allowed devices list. */
        if (dv->ven == 0xFFFF) {
            break;
        }

        for (int j = 0;; j++) {
            memset(&dev, 0, sizeof(libpci_device_t));
            bool found = vmm_driver_pci_find_device(vmm_plat_net.pci_ep,
                                                    dv->ven, dv->dev, &dev, j);
            if (!found) {
                break;
            }
            assert(dev.vendor_id == dv->ven && dev.device_id == dv->dev);
            /* libpci_device_iocfg_debug_print(&dev.cfg, false); */

            /* Map this device region into guest OS EPT space. */
            for (int j = 0; j < 6; j++) {
                if (dev.cfg.base_addr[j] == 0 || dev.cfg.base_addr_64H[j]) continue;

                /* Get the physical address of the device mem region. */
                uint32_t paddr = libpci_device_iocfg_get_baseaddr32(&dev.cfg, j);
                uint32_t size = dev.cfg.base_addr_size[j];

                /* Handle IO base addresses - we do not memory map them, but rather tell the IO
                * port module to allow this IO port from now on. */
                if (dev.cfg.base_addr_space[j] == PCI_BASE_ADDRESS_SPACE_IO) {
                    if (!dv->io_map) {
                        continue;
                    }
                    ioport_range_t *portr = &resource->guest_ioport_map[guest_ioport_n];
                    portr->port_start = paddr;
                    portr->port_end = paddr + dev.cfg.base_addr_size[j];
                    portr->cap_no = LIB_VMM_CAP_IGNORE;
                    portr->passthrough_mode = X86_IO_PASSTHROUGH;
                    portr->desc = "Guest-specific device.";
                    guest_ioport_n++;
                    if (guest_ioport_n >= LIB_VMM_GUEST_MAX_IOPORT_RANGES) {
                        panic("Out of guest ioport ranges. Try increasing "
                            "LIB_VMM_GUEST_MAX_IOPORT_RANGES");
                    }
                    dprintf(2, "plat:    allowed guest device IO port 0x%x - 0x%x.\n",
                            portr->port_start, portr->port_end);
                    continue;
                }

                if (!dv->mem_map) {
                    continue;
                }

                /* Handle memory mapped base addresses - Map this device somewhere the seL4 kernel
                * can actually map in. */
                void* vaddr = (void*) ((uint32_t) (paddr + dv->map_offset));
                vmm_plat_guest_map_device_frame(resource, vaddr, paddr, size, false);
            }
            if (j == 0) {
                printf(COLOUR_C "VMM ALERT: " COLOUR_RESET
                    "PCI Device vendor 0x%x device 0x%x not found on this system.\n",
                    dv->ven, dv->dev);
            }
        }
    }

    /* Map the VESA frame buffer into guest OS. */
    #ifdef LIB_VMM_VESA_FRAMEBUFFER
    dprintf(2, "plat: Attempting to read seL4_IA32_BootInfo for VESA fb info. If this crashes\n"
               "then that most likely means your kernel isn't patched with x86 VESA support.\n");
    seL4_IA32_BootInfo *bi = seL4_IA32_GetBootInfo(seL4_GetBootInfo());
    uint32_t fb_paddr = bi->vbeModeInfoBlock.physBasePtr;
    uint32_t fb_vaddr = fb_paddr + VMM_PCI_DEFAULT_MAP_OFFSET;
    uint32_t fb_sz = vmm_plat_vesa_fbuffer_size(bi);
    dprintf(2, "plat: mapping VESA frame buffer paddr 0x%x vaddr 0x%x size 0x%x\n",
            fb_paddr, fb_vaddr, fb_sz);
    vmm_plat_guest_map_device_frame(resource, (void*) fb_vaddr, fb_paddr, fb_sz, false);
    #endif /* LIB_VMM_VESA_FRAMEBUFFER */

    /* Map a copy of the initrd image into guest OS. */
    #ifdef LIB_VMM_INITRD_SUPPORT
    assert(resource->guest_initrd_filename);
    uint32_t initrd_index = vmm_guest_get_kernel_initrd_memarea_index();
    uint32_t initrd_sz = vmm_guest_get_initrd_size(resource->guest_initrd_filename);
    resource->guest_initrd_addr = (char*)(resource->mem_areas[initrd_index].end - initrd_sz);
    dprintf(2, "plat: mapping initrd image into guest OS at 0x%x\n",
            (uint32_t) resource->guest_initrd_addr);
    assert(resource->guest_initrd_addr);
    vmm_plat_touch_guest_frame(resource, (uint32_t) resource->guest_initrd_addr,
            initrd_sz, vmm_plat_guest_write_initrd);
    #endif /* LIB_VMM_INITRD_SUPPORT */

    /* Workaround for zone DMA enabled kernels. */
    #ifdef CONFIG_VMM_ZONE_DMA_WORKAROUND
    assert(resource->mem_areas[0].type == DMA_ZONE);
    assert(resource->mem_areas[0].end >= resource->guest_bootinfo_mem_areas[0].end);
    dprintf(2, "plat: mapping zone DMA region (workaround) 0x%x ---> 0x%x.\n",
            resource->mem_areas[0].start, resource->mem_areas[0].end);
    uint32_t zregion_sz = (resource->mem_areas[0].end -
                           resource->mem_areas[0].start);
    uint32_t zdma_paddr = resource->guest_bootinfo_mem_areas[0].end;
    assert(zregion_sz % (1 << seL4_PageBits) == 0);
 
    /* The early memory region is exported by kernel as a device. Thus we also effectively treat it
     * as a device here. */
    for (uint32_t zdma_vaddr = 0; zdma_vaddr < zregion_sz; zdma_vaddr += (1 << seL4_PageBits)) {
        uint32_t zpaddr = resource->guest_bootinfo_mem_areas[0].start + zdma_vaddr;
        if (zpaddr > zdma_paddr) {
            /* Map 1st frame and non-zone-DMA frames using ordinary 4k pages. */
            vmm_plat_guest_map_nondma_frame(resource, (void*) zpaddr);
            continue;
        }
        vmm_plat_guest_map_device_frame(resource, (void*) zpaddr, zpaddr,
                (1 << seL4_PageBits), true);
    }

    #endif /* CONFIG_ZONE_DMA_WORKAROUND */
}

/* Find the frame cap in guest-physical address space, map it into init thread address space, and
 * passes the virtual address to ven callback function touch_frame_ptr.
 * Used for guest OS threads with 4M pages only.
 *     @param: addr guest physical address
 */
static void vmm_plat_touch_guest_frame(thread_rec_t *resource, uint32_t addr, uint32_t size,
                                       void (*touch_frame_ptr)(thread_rec_t *, void *)) {
    assert(touch_frame_ptr && size && resource);
    assert(addr != 0);
    int num_frames;
    int i;
    int page_size;
#ifdef CONFIG_VMM_USE_4M_FRAMES
    page_size = seL4_4MBits;
#else
    page_size = seL4_PageBits;
#endif
    dprintf(5, "Trying to touch guest frame at 0x%x of size 0x%x\n", addr, size);

    int start_frame = addr >> page_size;
    int end_frame = (addr + size - 1) >> page_size;
    num_frames = (end_frame - start_frame) + 1;
    assert(num_frames <=
            (vmm_guest_get_initrd_size(resource->guest_initrd_filename) >> page_size) + 2);
    seL4_CPtr frames[num_frames];
    for (i = 0; i < num_frames; i++) {
        vmm_plat_get_frame_cap(resource, addr + (i << page_size), page_size, &frames[i]);
        assert(frames[i]);
    }

    cspacepath_t dest_frame_cap[num_frames];
    cspacepath_t src_frame_cap;
    for (i = 0; i < num_frames; i++) {
        /* Allocate a cap slot for the frame copy. */
        int ret = vka_cspace_alloc_path(&vmm_plat_vka, &dest_frame_cap[i]);
        assert(ret == seL4_NoError);
        assert(dest_frame_cap[i].capPtr);

        /* Copy the cap so we can map it into our own vspace. */
        vka_cspace_make_path(&vmm_plat_vka, frames[i], &src_frame_cap);
        ret = vka_cnode_copy(&dest_frame_cap[i], &src_frame_cap, seL4_AllRights);
        assert(ret == seL4_NoError);

        /* Now map the frame into our own (init thread's) vspace. */
        ret = vspace_map_pages_at_vaddr(&vmm_plat_vspace, &dest_frame_cap[i].capPtr,
                (void*) ( (uintptr_t) vmm_plat_map_guest_frame_vaddr + i * (1 << page_size)),
                1, page_size, reserve_area_map_guest_frame);
        assert(ret == seL4_NoError);
    }

    /* Call the callback function ptr, giving it the virtual address pointer mapped. */
    void* write_vaddr;
    write_vaddr = vmm_plat_map_guest_frame_vaddr;
    write_vaddr += addr % (1 << page_size);
    touch_frame_ptr(resource, write_vaddr);

    for (int i = num_frames - 1; i >= 0; i--) {
        /* Delete the mapping and copy of the cap. */
        int ret = vspace_unmap_reserved_pages(&vmm_plat_vspace,
                (void*) ( (uintptr_t) vmm_plat_map_guest_frame_vaddr + i * (1 << page_size)),
                1, page_size, reserve_area_map_guest_frame);
        assert(ret == 0);
        vka_cnode_delete(&dest_frame_cap[i]);
        vka_cspace_free(&vmm_plat_vka, dest_frame_cap[i].capPtr);
    }
}

static void vmm_plat_init_guest_page_dir(thread_rec_t *resource, void *vaddr) {
    dprintf(4, "plat: init guest page dir\n");
#ifdef CONFIG_VMM_ENABLE_PAE
    uint64_t *pdpt = vaddr;
    uint64_t pd_phys_base = (uintptr_t)resource->guest_page_dir + BIT(seL4_PageBits);
    for (int i = 0; i < 4; i++) {
//        pdpt[i] = (pd_phys_base + (i << seL4_PageBits)) | BIT(0);
        pdpt[i] = -1ll;
        printf("pdpt %d is %llu\n", i, pdpt[i]);
        uint64_t *pd = vaddr + (i << seL4_PageBits) + BIT(seL4_PageBits);
        for (int j = 0; j < 512; j++) {
            pd[j] = ((uint64_t)(i * 512 + j) << 21ull) | //Calculate the 2mb page
                    BIT(0) | //present
                    BIT(1) | //write
                    BIT(2) | //user
                    BIT(7); //page size
        }
    }
    for (int i = 0; i < 512; i++) {
        pdpt[i] = -1ll;
    }
#else
    /* Map in bootinfo guest page directory info. */
    /* Write into this frame as the init page directory: 4M pages, 1 to 1 mapping. */
    unsigned int *pd = vaddr;
    unsigned int n_4m_pages = vmm_guest_mem_get_max_e820_addr(resource) / (1 << seL4_4MBits);
    assert(n_4m_pages * sizeof(unsigned int) < (1 << seL4_PageBits));
    for (int i = 0; i < n_4m_pages; i++) {
        pd[i] = (i << 22) | LIB_VMM_GUEST_PAGE_DIR_ATTR;
    }
#endif
}

static void vmm_plat_init_guest_cmd_line(thread_rec_t *resource, void *vaddr) {
    /* Map in bootinfo for cmd line args. */
    dprintf(2, "plat: init guest cmd line [%s]\n",
            resource->elf_params ? resource->elf_params : "");
    if (!resource->elf_params) {
        ((char*) vaddr) [0] = '\0';
        return;
    }
    /* Copy the string to this area. */
    strcpy(vaddr, resource->elf_params);
}

static void vmm_plat_init_guest_boot_info(thread_rec_t *resource, void *vaddr) {
    dprintf(2, "plat: init guest boot info\n");

    /* Map in BIOS boot info structure. */
    struct boot_params *boot_info = vaddr;
    memset(boot_info, 0, sizeof (struct boot_params));

    /* Initialise basic bootinfo structure. Src: Linux kernel Documentation/x86/boot.txt */    
    boot_info->hdr.header = 0x53726448; /* Magic number 'HdrS' */
    boot_info->hdr.boot_flag = 0xAA55; /* Magic number for Linux. */
#ifdef LIB_VMM_INITRD_SUPPORT
    boot_info->hdr.version = 0x0204; /* Report version 2.04 in order to report ramdisk_image. */
#else
    boot_info->hdr.version = 0x0202;
#endif
    boot_info->hdr.type_of_loader = 0xFF; /* Undefined loeader type. */
    boot_info->hdr.code32_start = resource->guest_mem_start;
    boot_info->hdr.kernel_alignment = LIB_VMM_GUEST_PHYSADDR_ALIGN;
    boot_info->hdr.relocatable_kernel = true;

    /* Set up screen information. */
    /* Tell Guest OS about VESA mode. */
    struct screen_info *info = &boot_info->screen_info;

    /* VESA information */
#ifdef CONFIG_VMM_VESA_FRAMEBUFFER
    seL4_IA32_BootInfo *bi = seL4_IA32_GetBootInfo(seL4_GetBootInfo());
    info->orig_video_isVGA = 0x23; // Tell Linux it's a VESA mode
    info->lfb_width = bi->vbeModeInfoBlock.xRes;
    info->lfb_height = bi->vbeModeInfoBlock.yRes;
    info->lfb_depth = bi->vbeModeInfoBlock.bitsPerPixel;
    info->lfb_base = bi->vbeModeInfoBlock.physBasePtr + VMM_PCI_DEFAULT_MAP_OFFSET;
    info->lfb_size = vmm_plat_vesa_fbuffer_size(bi) >> 16;
    info->lfb_linelength = bi->vbeModeInfoBlock.bytesPerScanLine;

    info->red_size = bi->vbeModeInfoBlock.redLen;
    info->red_pos = bi->vbeModeInfoBlock.redOff;
    info->green_size = bi->vbeModeInfoBlock.greenLen;
    info->green_pos = bi->vbeModeInfoBlock.greenOff;
    info->blue_size = bi->vbeModeInfoBlock.blueLen;
    info->blue_pos = bi->vbeModeInfoBlock.blueOff;
    info->rsvd_size = bi->vbeModeInfoBlock.rsvdLen;
    info->rsvd_pos = bi->vbeModeInfoBlock.rsvdOff;
    info->vesapm_seg = bi->vbeInterfaceSeg;
    info->vesapm_off = bi->vbeInterfaceOff;
#else
    memset(info, 0, sizeof(*info));
#endif

    /* Initialize memory area according to the machine configuration. */
    assert(resource->n_guest_bootinfo_mem_areas);
    boot_info->e820_entries = resource->n_guest_bootinfo_mem_areas;

    for (int i = 0; i < resource->n_guest_bootinfo_mem_areas; i++) {
        boot_info->e820_map[i].addr = resource->guest_bootinfo_mem_areas[i].start;
        boot_info->e820_map[i].size = resource->guest_bootinfo_mem_areas[i].end -
                                      resource->guest_bootinfo_mem_areas[i].start;
        boot_info->e820_map[i].type = resource->guest_bootinfo_mem_areas[i].type;
    }

    /* Pass in the command line string. */
    boot_info->hdr.cmd_line_ptr = resource->guest_cmd_line;
    boot_info->hdr.cmdline_size = resource->elf_params ? strlen(resource->elf_params) : 0;

    /* These are not needed to be precise, because Linux uses these values
     * only to raise an error when the decompression code cannot find good
     * space. ref: GRUB2 source code loader/i386/linux.c */
    boot_info->alt_mem_k = ((32 * 0x100000) >> 10);

    /* Pass in initramfs. */
    #ifdef LIB_VMM_INITRD_SUPPORT
    if (resource->guest_initrd_addr) {
        assert(resource->guest_initrd_size > 0);
        boot_info->hdr.ramdisk_image = (uint32_t) resource->guest_initrd_addr;
        boot_info->hdr.ramdisk_size = resource->guest_initrd_size;
        boot_info->hdr.root_dev = 0x0100;
    }
    #endif
}

/* Init the guest page directory, cmd line args and boot info structures. */
static void vmm_plat_init_guest_boot_structure(thread_rec_t *resource) {
    /* Init the Guest OS boot info. The boot info is located at the first 7 pages of the second 4M
     * page. */
    seL4_Word vaddr = LIB_VMM_GUEST_BOOTINFO_ADDR;

    /* The boot info memory layout looks like:
     * Diagram is wrong, page dir is 5 pages, not 1
     *
     *   ---------- 0x400000       ^ lower mem
     *   4K                        |
     *   ---------- page dir       | 0x401000
     *   4K                        |
     *   ---------- cmd line       | 0x402000
     *   4K                        |
     *   ---------- boot info      | 0x403000 
     *                             |
     *                             v higher mem
     */
    resource->guest_page_dir = vaddr + (1 << seL4_PageBits);
    resource->guest_cmd_line = vaddr + (6 << seL4_PageBits);
    resource->guest_boot_info = vaddr + (7 << seL4_PageBits);

    dprintf(2, "plat: boot info vaddr 0x%x cmd line ptr 0x%x page dir 0x%x\n",
        resource->guest_boot_info, resource->guest_cmd_line, resource->guest_page_dir);

    /* The start of the 4M page, map it in and write to it in the respective callback functions. */
    vmm_plat_touch_guest_frame(resource, resource->guest_page_dir, (5 << seL4_PageBits),
            vmm_plat_init_guest_page_dir);
    vmm_plat_touch_guest_frame(resource, resource->guest_boot_info, (1 << seL4_PageBits),
            vmm_plat_init_guest_boot_info);
    vmm_plat_touch_guest_frame(resource, resource->guest_cmd_line, (1 << seL4_PageBits),
            vmm_plat_init_guest_cmd_line);
}

/* Launch a guest OS thread. */
void vmm_plat_run_guest_thread(thread_rec_t *resource) {

    /* Init the thread context. */
    seL4_UserContext context;
    seL4_TCB_ReadRegisters(resource->tcb, true, 0, 13, &context);

    context.eax = 0;
    context.ebx = 0;
    context.ecx = 0;
    context.edx = 0;

    /* Entry point. */
    context.eip = resource->elf_entry;
    /* Top of stack. */
    context.esp = resource->stack_top;
    /* The boot_param structure. */
    context.esi = resource->guest_boot_info;

    vmm_plat_print_thread_context(resource->elf_name, &context);

    /* Init guest OS mem. */
    vmm_vmcs_init_guest(resource->vcpu, resource->guest_page_dir);

    seL4_TCB_WriteRegisters(resource->tcb, true, 0, 13, &context);
}

void vmm_plat_parse_elf_name_params(char *kernel_cmdline, char *elf_name, char *elf_params) {
    assert(kernel_cmdline && elf_name && elf_params);
    int len = strlen(kernel_cmdline);
    if (len > LIB_VMM_MAX_BUFFER_LEN) len = LIB_VMM_MAX_BUFFER_LEN;
    /* Find the first space. */
    char *space = strchr(kernel_cmdline, ' ');
    if (!space) {
        /* No params, just a single kernel ELF name. */
        strncpy(elf_name, kernel_cmdline, LIB_VMM_MAX_BUFFER_LEN);
        elf_name[LIB_VMM_MAX_BUFFER_LEN - 1] = '\0';
        elf_params[0] = '\0';
        return;
    }
    int index = space - kernel_cmdline + 1;
    assert(index > 0 && index < len);
    if (index > LIB_VMM_MAX_BUFFER_LEN - 1) index = (LIB_VMM_MAX_BUFFER_LEN - 1);
    strncpy(elf_name, kernel_cmdline, index);
    elf_name[LIB_VMM_MAX_BUFFER_LEN - 1] = '\0';
    elf_name[index - 1] = '\0';
    strncpy(elf_params, space + 1, LIB_VMM_MAX_BUFFER_LEN);
    elf_params[LIB_VMM_MAX_BUFFER_LEN - 1] = '\0';
    dprintf(4, "elf_name: [%s]\n", elf_name);
    dprintf(4, "elf_params: [%s]\n", elf_params);
}

/* Setup and boot a Guest OS thread. */
void vmm_plat_init_guest_thread(thread_rec_t *guest_thread, char *kernel_executable_name, 
                                char *initrd_name, thread_rec_t *host_thread, seL4_Word badge_no) {
    assert(guest_thread);
    assert(host_thread);
    assert(kernel_executable_name);
 
    dprintf(3, "plat: init guest kernel = %s, initrd = %s\n", kernel_executable_name, initrd_name);

    /* Parse kernel executable line into ELF name + parameters. */
    char *elf_name = malloc(LIB_VMM_MAX_BUFFER_LEN * sizeof(char));
    char *elf_params =  malloc(LIB_VMM_MAX_BUFFER_LEN * sizeof(char));
    vmm_plat_parse_elf_name_params(kernel_executable_name, elf_name, elf_params);

    /* Create the basic kernel objects needed for thread to run. */
    vmm_plat_create_thread(guest_thread, host_thread->vmm_ep, LIB_VMM_GUEST_OS_THREAD,
                           elf_name, badge_no, "guest_os");

    /* save thread info in its structure. */
    guest_thread->guest_host_thread = host_thread;
    guest_thread->elf_name = elf_name;
    guest_thread->elf_params = elf_params;
    guest_thread->guest_initrd_filename = initrd_name;
    
    /* Copy the VCPU cap into host thread's cnode. */
    vmm_plat_copy_vcpu(host_thread, guest_thread);
    vmm_plat_copy_guest_tcb(host_thread, guest_thread);

    #if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)
    /* Skip the physical frame allocator to the next available address that conforms to the
     * kernel relocation alignment requirements. */
    guest_thread->guest_mem_start = pframe_skip_to_aligned(LIB_VMM_GUEST_PHYSADDR_ALIGN, 0);
    dprintf(2, "plat: guest OS will be mapped 1:1 at paddr = gpaddr = 0x%x.\n",
            guest_thread->guest_mem_start);
    #else
        guest_thread->guest_mem_start = LIB_VMM_GUEST_PHYSADDR_MIN;
    #endif

    /* Create the guest-physical RAM mem regions that we map in, and the corresponding
     * regions we actually pass info the guest in the bootinfo. */
    uint32_t guest_kernel_initrd_sz = vmm_guest_mem_determine_kernel_initrd_size(1,
            guest_thread->elf_name, guest_thread->guest_initrd_filename);
    assert(guest_kernel_initrd_sz);
    vmm_guest_mem_create(guest_thread->guest_mem_start, guest_kernel_initrd_sz,
            &guest_thread->n_mem_areas, &guest_thread->mem_areas,
            &guest_thread->n_guest_bootinfo_mem_areas, &guest_thread->guest_bootinfo_mem_areas);

    /* Create mem mapping and load ELF file. */
    vmm_plat_create_thread_vspace(guest_thread,
            guest_thread->n_mem_areas, guest_thread->mem_areas);

    /* Relocate the guest OS kernel ELF file. */
    vmm_plat_guest_elf_relocate(guest_thread);

    /* Initialise boot info, cmd line args and pd info. */
    vmm_plat_init_guest_boot_structure(guest_thread);

    /* Initialise guest IO. */
    vmm_io_init_guest(&vmm_plat_simple, guest_thread, guest_thread->vcpu, guest_thread->badge_no);
}

