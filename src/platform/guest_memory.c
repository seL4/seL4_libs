/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpio/cpio.h>
#include <elf/elf.h>
#include <autoconf.h>
#include "vmm/config.h"
#include "vmm/platform/guest_memory.h"
#include "vmm/platform/sys.h"

#define MEMAREA_INVALID 0xFFFFDEAD
extern char _cpio_archive[];

/* Pre-defined mem info for zone DMA, ELF mapping, initrd mapping, and boot info structures.
 * e820 regions should be contiguous. */

uint32_t vmm_plat_guest_n_mem_boot _UNUSED_ = 5;
guest_mem_area_t vmm_plat_guest_mem_areas_boot[] _UNUSED_ = {
    /* First 4MB page. Reserved to be used for zone DMA. */
    {.start = 0x0,     .end = 0x1F000,  .type = E820_RAM, .perm = seL4_AllRights},
    {.start = 0x1F000, .end = 0x400000, .type = E820_RESERVED, .perm = seL4_AllRights},

    /* The next few pages are used for bootinfo. */
    {.start = 0x400000, .end = 0x404000,
     .type = E820_RESERVED, .perm = seL4_AllRights},

    /* Connector to ELF/initrd region (the .end will be relocated). No RAM exists here. */
    {.start = 0x404000, .end = MEMAREA_INVALID,
     .type = E820_RESERVED, .perm = seL4_AllRights},
 
    /* The kernel ELF & initrd region (This section is dynamic and will be relocated).
     * This section has to be the last section. */
    {
     .start = MEMAREA_INVALID,
     .end = MEMAREA_INVALID,
     .type = E820_RAM,
     .perm = seL4_AllRights
    }

    /* Actual guest usable RAM sections will be dynamically allocated via the guest_ram_alloc
     * allocator and appended here. */
};

uint32_t vmm_plat_guest_n_mem_init _UNUSED_ = 4;
guest_mem_area_t vmm_plat_guest_mem_areas_init[] _UNUSED_ = {
    /* First page of memory - reserved to be used for zone DMA. */
    {
     .start = 0x0,
     .end = 0x400000,
     .type = DMA_ZONE,
     .perm = seL4_AllRights
    },

    /* The next few pages are used for bootinfo. */
    {
     .start = 0x400000,
     .end = 0x408000,
     .type = DMA_IGNORE,
     .perm = seL4_AllRights
    },

    /* Connector to ELF/initrd region (the .end will be relocated). No RAM exists here. */
    {.start = 0x408000, .end = MEMAREA_INVALID,
     .type = E820_RESERVED, .perm = seL4_AllRights},

    /* The kernel ELF & initrd region (This section is dynamic and will be relocated).
     * This section has to be the last section. */
    {
     .start = MEMAREA_INVALID,
     .end = MEMAREA_INVALID,
     .type = E820_RAM,
     .perm = seL4_AllRights
    }

    /* Actual guest usable RAM sections will be dynamically allocated via the guest_ram_alloc
     * allocator and appended here. */
};

static void vmm_guest_mem_clone_readjust(uint32_t base_gpaddr, uint32_t size_bytes,
        uint32_t src_n, guest_mem_area_t *src_areas,
        uint32_t *dest_n, guest_mem_area_t **dest_areas,
        int n_memareas_allocate) {

    assert(dest_n && dest_areas);
    assert(src_n > 0 && src_areas);
    assert(src_n <= n_memareas_allocate && n_memareas_allocate <= E820_MAX_REGIONS);
    assert(base_gpaddr >= LIB_VMM_GUEST_PHYSADDR_MIN);
    assert(base_gpaddr % LIB_VMM_GUEST_PHYSADDR_ALIGN == 0);
    assert(size_bytes > 0);

    /* Create a copy of the pre-defined mem regions. */
    guest_mem_area_t *mem = (guest_mem_area_t*) malloc(n_memareas_allocate *
            sizeof(guest_mem_area_t));
    assert(mem);
    memset(mem, 0, sizeof(n_memareas_allocate * sizeof(guest_mem_area_t)));
    memcpy(mem, src_areas, src_n * sizeof(guest_mem_area_t));

    /* Readjust the memory regions above to match what the allocator can actually allocate.
     * We assume here that the LAST memory area is the kernel ELF + initrd region, to be
     * dynamically relocated. */
 
    uint32_t rindex = src_n - 1;
    assert(mem[rindex].type == E820_RAM);
    mem[rindex].start = base_gpaddr;
    mem[rindex].end = base_gpaddr + size_bytes;

    /* Readjust the second last index's .end to match the new base addr. */
    if (rindex - 1 > 0) {
        mem[rindex - 1].end = base_gpaddr;
        assert(mem[rindex - 1].start <= mem[rindex - 1].end);
    }

    /* Write output. */
    (*dest_n) = src_n;
    (*dest_areas) = mem;
}

void vmm_guest_mem_create(uint32_t base_gpaddr, uint32_t size_bytes,
        uint32_t *n_mem_areas, guest_mem_area_t **mem_areas,
        uint32_t *n_bootinfo_mem_areas, guest_mem_area_t **bootinfo_mem_areas) {

    assert(n_mem_areas && mem_areas);
    assert(n_bootinfo_mem_areas && bootinfo_mem_areas);

    /* Clone and adjust the guest init mem area. */
    vmm_guest_mem_clone_readjust(base_gpaddr, size_bytes,
            vmm_plat_guest_n_mem_init, vmm_plat_guest_mem_areas_init,
            n_mem_areas, mem_areas,
            vmm_plat_guest_n_mem_init);

    /* Clone and adjust the guest bootinfo mem area that we pass through the e820. */
    vmm_guest_mem_clone_readjust(base_gpaddr, size_bytes,
            vmm_plat_guest_n_mem_boot, vmm_plat_guest_mem_areas_boot,
            n_bootinfo_mem_areas, bootinfo_mem_areas,
            E820_MAX_REGIONS);

}

void vmm_plat_parse_elf_name_params(char *kernel_cmdline, char *elf_name, char *elf_params);

uint32_t vmm_guest_mem_determine_kernel_initrd_size(uint32_t num_guest_os,
        char *guest_kernel, char *guest_initrd) {
    int page_size;
#ifdef CONFIG_VMM_USE_4M_FRAMES
    page_size = seL4_4MBits;
#else
    page_size = seL4_PageBits;
#endif
    /* Can't printf here, called during boot. */
    assert(num_guest_os < LIB_VMM_GUEST_NUMBER);
    assert(guest_kernel);

    /* Determine size of kernel. */
    long unsigned int kernel_size = 0;
    static char elf_name[LIB_VMM_MAX_BUFFER_LEN];
    static char elf_params[LIB_VMM_MAX_BUFFER_LEN];
    vmm_plat_parse_elf_name_params(guest_kernel, elf_name, elf_params);

    char* kernel_file = cpio_get_file(_cpio_archive, elf_name, &kernel_size);
    if (!kernel_file) {
        printf("Kernel file not found: [%s]\n", guest_kernel);
        panic("vmm_guest_mem_determine_kernel_initrd_size: Kernel file not found!");
        return 0;
    }
    assert(kernel_size > 0);
    if (elf_checkFile(kernel_file)) {
        panic("Kernel ELF file check failed.");
        return 0;
    }
    uint32_t kernel_elf_start = 0xFFFFFFFF;
    uint32_t kernel_elf_end = 0;
    int n_headers = elf_getNumProgramHeaders(kernel_file);
    assert(n_headers);
    for (int i = 0; i < n_headers; i++) {
        uint32_t segment_size = elf_getProgramHeaderMemorySize(kernel_file, i);
        uint32_t dest_addr = (uint32_t) elf_getProgramHeaderPaddr(kernel_file, i);
        uint32_t segment_end = dest_addr + segment_size;
        if (segment_end > kernel_elf_end) {
            kernel_elf_end = segment_end;
        }
        if (dest_addr < kernel_elf_start) {
            kernel_elf_start = dest_addr;
        }
    }
    kernel_elf_end = ((kernel_elf_end - kernel_elf_start) >> page_size) << page_size;
    kernel_elf_end += (1 << page_size);

    /* Determine size of initrd. */
    long unsigned int initrd_size = 0;
#ifdef LIB_VMM_INITRD_SUPPORT
    char* initrd_file = cpio_get_file(_cpio_archive, guest_initrd, &initrd_size);
    if (!initrd_file) {
        panic("vmm_guest_mem_determine_kernel_initrd_size: Initrd file not found!");
        return 0;
    }
    assert(initrd_size > 0);
#endif

    uint32_t zone_dma_xtra = CONFIG_VMM_ZONE_DMA_RAM * (1024 * 1024);
#ifndef CONFIG_VMM_ZONE_DMA_WORKAROUND
    zone_dma_xtra = 0;
#endif
    uint32_t total_size = kernel_elf_end + initrd_size + zone_dma_xtra;
    assert(total_size);

    return total_size;
}

uint32_t vmm_guest_get_initrd_size(char *guest_initrd) {
    /* Can't printf here, called during boot. */
    #ifdef LIB_VMM_INITRD_SUPPORT
    long unsigned int initrd_size = 0;
    char* initrd_file = cpio_get_file(_cpio_archive, guest_initrd, &initrd_size);
    if (!initrd_file) {
        panic("vmm_guest_mem_determine_kernel_initrd_size: Initrd file not found!");
        return 0;
    }
    assert(initrd_size > 0);
    return initrd_size;
#else
    (void) guest_initrd;
    return 0;
#endif
}

uint32_t vmm_guest_get_kernel_initrd_memarea_index(void) {
    /* The last static pre-defined mem area is assumed to be the one used to map
     * the kernel and initrd. */
    return vmm_plat_guest_n_mem_init - 1;
}

bool vmm_guest_mem_check_elf_segment(thread_rec_t *resource,
        uint32_t addr_start, uint32_t addr_end) {
    assert(resource);
    assert(addr_end >= addr_start);
    uint32_t ix = vmm_guest_get_kernel_initrd_memarea_index();
    if (addr_start < resource->guest_mem_start) {
        goto checkfail;
    }
    if (addr_start < resource->mem_areas[ix].start) {
        goto checkfail;
    }
    if (addr_end > resource->mem_areas[ix].end) {
        goto checkfail;
    }
    return true;

checkfail:
    printf("ERROR: Kernel has loadable segment outside valid address range.\n");
    printf("       Attempted to create segment 0x%x - 0x%x\n", addr_start, addr_end);
    printf("       All segments must lie between 0x%x - 0x%x\n",
        resource->mem_areas[ix].start,
        resource->mem_areas[ix].end);
    return false;
}

uint32_t vmm_guest_mem_get_max_e820_addr(thread_rec_t *resource) {
    assert(resource);
    return resource->guest_bootinfo_mem_areas[resource->n_guest_bootinfo_mem_areas - 1].end;
}

