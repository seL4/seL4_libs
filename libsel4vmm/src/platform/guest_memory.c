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

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sel4/sel4.h>
#include <utils/util.h>
#include <vka/capops.h>

#include "vmm/vmm.h"
#include "vmm/debug.h"
#include "vmm/platform/guest_memory.h"

static void push_guest_ram_region(guest_memory_t *guest_memory, uintptr_t start, size_t size, int allocated) {
    int last_region = guest_memory->num_ram_regions;
    if (size == 0) return;
    guest_memory->ram_regions = realloc(guest_memory->ram_regions, sizeof(guest_ram_region_t) * (last_region + 1));
    assert(guest_memory->ram_regions);

    guest_memory->ram_regions[last_region].start = start;
    guest_memory->ram_regions[last_region].size = size;
    guest_memory->ram_regions[last_region].allocated = allocated;
    guest_memory->num_ram_regions++;
}

static int ram_region_cmp(const void *a, const void *b) {
    const guest_ram_region_t *aa = a;
    const guest_ram_region_t *bb = b;
    return aa->start - bb->start;
}

static void sort_guest_ram_regions(guest_memory_t *guest_memory) {
    qsort(guest_memory->ram_regions, guest_memory->num_ram_regions, sizeof(guest_ram_region_t), ram_region_cmp);
}

static void guest_ram_remove_region(guest_memory_t *guest_memory, int region) {
    assert(region < guest_memory->num_ram_regions);
    guest_memory->num_ram_regions--;
    memcpy(&guest_memory->ram_regions[region], &guest_memory->ram_regions[region + 1], sizeof(guest_ram_region_t) * (guest_memory->num_ram_regions - region));
    /* realloc it smaller */
    guest_memory->ram_regions = realloc(guest_memory->ram_regions, sizeof(guest_ram_region_t) * guest_memory->num_ram_regions);
    assert(guest_memory->ram_regions);
}

static void collapse_guest_ram_regions(guest_memory_t *guest_memory) {
    int i;
    for (i = 1; i < guest_memory->num_ram_regions;) {
        /* Only collapse regions with the same allocation flag that are contiguous */
        if (guest_memory->ram_regions[i - 1].allocated == guest_memory->ram_regions[i]. allocated &&
            guest_memory->ram_regions[i - 1].start + guest_memory->ram_regions[i - 1].size == guest_memory->ram_regions[i].start) {

            guest_memory->ram_regions[i - 1].size += guest_memory->ram_regions[i].size;
            guest_ram_remove_region(guest_memory, i);
        } else {
            /* We are satisified that this entry cannot be merged. So now we
             * move onto the next one */
            i++;
        }
    }
}

static int expand_guest_ram_region(guest_memory_t *guest_memory, uintptr_t start, size_t bytes) {
    /* blindly put a new region at the end */
    push_guest_ram_region(guest_memory, start, bytes, 0);
    /* sort the region we just added */
    sort_guest_ram_regions(guest_memory);
    /* collapse any contiguous regions */
    collapse_guest_ram_regions(guest_memory);
    return 0;
}

uintptr_t guest_ram_largest_free_region_start(guest_memory_t *guest_memory) {
    int largest = -1;
    int i;
    /* find a first region */
    for (i = 0; i < guest_memory->num_ram_regions && largest == -1; i++) {
        if (!guest_memory->ram_regions[i].allocated) {
            largest = i;
        }
    }
    assert(largest != -1);
    for (i++; i < guest_memory->num_ram_regions; i++) {
        if (!guest_memory->ram_regions[i].allocated &&
            guest_memory->ram_regions[i].size > guest_memory->ram_regions[largest].size) {
            largest = i;
        }
    }
    return guest_memory->ram_regions[largest].start;
}

void print_guest_ram_regions(guest_memory_t *guest_memory) {
    int i;
    for (i = 0; i < guest_memory->num_ram_regions; i++) {
        char size_char = 'B';
        size_t size = guest_memory->ram_regions[i].size;
        if (size >= 1024) {
            size >>= 10;
            size_char = 'K';
        }
        if (size >= 1024) {
            size >>= 10;
            size_char = 'M';
        }
        printf("\t0x%x-0x%x (%zu%c) %s\n",
               (unsigned int)guest_memory->ram_regions[i].start,
               (unsigned int)(guest_memory->ram_regions[i].start + guest_memory->ram_regions[i].size),
               size, size_char,
               guest_memory->ram_regions[i].allocated ? "allocated" : "free");
    }
}

void guest_ram_mark_allocated(guest_memory_t *guest_memory, uintptr_t start, size_t bytes) {
    /* Find the region */
    int i;
    int region = -1;
    for (i = 0; i < guest_memory->num_ram_regions; i++) {
        if (guest_memory->ram_regions[i].start <= start &&
            guest_memory->ram_regions[i].start + guest_memory->ram_regions[i].size >= start + bytes) {
            region = i;
            break;
        }
    }
    assert(region != -1);
    assert(!guest_memory->ram_regions[region].allocated);
    /* Remove the region */
    guest_ram_region_t r = guest_memory->ram_regions[region];
    guest_ram_remove_region(guest_memory, region);
    /* Split the region into three pieces and add them */
    push_guest_ram_region(guest_memory, r.start, start - r.start, 0);
    push_guest_ram_region(guest_memory, start, bytes, 1);
    push_guest_ram_region(guest_memory, start + bytes, r.size - bytes - (start - r.start), 0);
    /* sort and collapse */
    sort_guest_ram_regions(guest_memory);
    collapse_guest_ram_regions(guest_memory);
}

uintptr_t guest_ram_allocate(guest_memory_t *guest_memory, size_t bytes) {
    /* Do the obvious dumb search through the regions */
    for (int i = 0; i < guest_memory->num_ram_regions; i++) {
        if (!guest_memory->ram_regions[i].allocated && guest_memory->ram_regions[i].size >= bytes) {
            uintptr_t addr = guest_memory->ram_regions[i].start;
            guest_ram_mark_allocated(guest_memory, addr, bytes);
            return addr;
        }
    }
    ZF_LOGE("Failed to allocate %zu bytes of guest RAM", bytes);
    return 0;
}

static int vmm_alloc_guest_ram_one_to_one(vmm_t *vmm, size_t bytes) {
    int ret;
    int i;
    int page_size = vmm->page_size;
    int num_pages = ROUND_UP(bytes, BIT(page_size)) >> page_size;
    guest_memory_t *guest_memory = &vmm->guest_mem;

    DPRINTF(0, "Allocating %d frames of size %d for guest RAM\n", num_pages, page_size);
    /* Allocate all the frames first */
    vka_object_t *objects = malloc(sizeof(vka_object_t) * num_pages);
    assert(objects);
    for (i = 0; i < num_pages; i++) {
        ret = vka_alloc_frame(&vmm->vka, page_size, &objects[i]);
        if (ret) {
            ZF_LOGE("Failed to allocate frame %d/%d", i, num_pages);
            return -1;
        }
    }

    for (i = 0; i < num_pages; i++) {
        /* Get its physical address */
        uintptr_t paddr = vka_object_paddr(&vmm->vka, &objects[i]);
        if (paddr == VKA_NO_PADDR) {
            ZF_LOGE("Allocated frame has no physical address");
            return -1;
        }
        reservation_t reservation = vspace_reserve_range_at(&guest_memory->vspace, (void*)paddr, BIT(page_size), seL4_AllRights, 1);
        if (!reservation.res) {
            ZF_LOGI("Failed to reserve address 0x%x in guest vspace. Skipping frame and leaking memory!", (unsigned int)paddr);
            continue;
        }
        /* Map in */
        ret = vspace_map_pages_at_vaddr(&guest_memory->vspace, &objects[i].cptr, NULL, (void*)paddr, 1, page_size, reservation);
        if (ret) {
            ZF_LOGE("Failed to map page %d/%d into guest vspace at 0x%x\n", i, num_pages, (unsigned int)paddr);
            return -1;
        }
        /* Free the reservation */
        vspace_free_reservation(&guest_memory->vspace, reservation);

        /* Add the frame to our list */
        ret = expand_guest_ram_region(guest_memory, paddr, BIT(page_size));
        if (ret) {
            return ret;
        }
    }
    printf("Guest RAM regions after allocation:\n");
    print_guest_ram_regions(guest_memory);
    /* free the objects */
    ZF_LOGI("sys_munmap not currently implemented. Leaking memory!");
    //free(objects);
    return 0;
}

int vmm_alloc_guest_device_at(vmm_t *vmm, uintptr_t start, size_t bytes) {
    int page_size = vmm->page_size;
    int num_pages = ROUND_UP(bytes, BIT(page_size)) >> page_size;
    int ret;
    int i;
    guest_memory_t *guest_memory = &vmm->guest_mem;
    uintptr_t page_start = ROUND_DOWN(start, BIT(page_size));
    printf("Add guest memory region 0x%x-0x%x\n", (unsigned int)start, (unsigned int)(start + bytes));
    printf("Will be allocating region 0x%x-0x%x after page alignment\n", (unsigned int)page_start, (unsigned int)(page_start + num_pages * BIT(page_size)));
    for (i = 0; i < num_pages; i++) {
        reservation_t reservation = vspace_reserve_range_at(&guest_memory->vspace, (void*)(page_start + i * BIT(page_size)), 1, seL4_AllRights, 1);
        if (!reservation.res) {
            ZF_LOGI("Failed to create reservation for guest memory page 0x%x size %d, assuming already allocated", (unsigned int)(page_start + i * BIT(page_size)), page_size);
            continue;
        }
        ret = vspace_new_pages_at_vaddr(&guest_memory->vspace, (void*)(page_start + i * BIT(page_size)), 1, page_size, reservation);
        if (ret) {
            ZF_LOGE("Failed to create page 0x%x size %d in guest memory region", (unsigned int)(page_start + i * BIT(page_size)), page_size);
            return -1;
        }
        vspace_free_reservation(&guest_memory->vspace, reservation);
    }
    return 0;
}

static int vmm_map_guest_device_reservation(vmm_t *vmm, uintptr_t paddr, size_t bytes, reservation_t reservation, uintptr_t map_base) {
    int page_size = vmm->page_size;
    int error;
    guest_memory_t *guest_memory = &vmm->guest_mem;
    ZF_LOGI("Mapping passthrough device region 0x%x-0x%x to 0x%x", (unsigned int)paddr, (unsigned int)(paddr + bytes), (unsigned int)map_base);
    /* Go through and try and map all the frames */
    uintptr_t current_paddr;
    uintptr_t current_map;
    for (current_paddr = paddr, current_map = map_base; current_paddr < paddr + bytes; current_paddr += BIT(page_size), current_map += BIT(page_size)) {
        cspacepath_t path;
        error = vka_cspace_alloc_path(&vmm->vka, &path);
        if (error) {
            ZF_LOGE("Failed to allocate cslot");
            return error;
        }
        error = simple_get_frame_cap(&vmm->host_simple, (void*)current_paddr, page_size, &path);
        uintptr_t cookie;
        bool allocated = false;
        if (error) {
            /* attempt to allocate */
            allocated = true;
            error = vka_utspace_alloc_at(&vmm->vka, &path, kobject_get_type(KOBJECT_FRAME, page_size), page_size, current_paddr, &cookie);
        }
        if (error) {
            ZF_LOGE("Failed to find device frame 0x%x size 0x%x for region 0x%x 0x%x", (unsigned int)current_paddr, (unsigned int)BIT(page_size), (unsigned int)paddr, (unsigned int)bytes);
            vka_cnode_delete(&path);
            vka_cspace_free(&vmm->vka, path.capPtr);
            return error;
        }
        error = vspace_map_pages_at_vaddr(&guest_memory->vspace, &path.capPtr, NULL, (void*)current_map, 1, page_size, reservation);
        if (error) {
            ZF_LOGE("Failed to map device page 0x%x at 0x%x from region 0x%x 0x%x\n", (unsigned int)current_paddr, (unsigned int)current_map, (unsigned int)paddr, (unsigned int)bytes);
            if (allocated) {
                vka_utspace_free(&vmm->vka, kobject_get_type(KOBJECT_FRAME, page_size), page_size, cookie);
            }
            vka_cnode_delete(&path);
            vka_cspace_free(&vmm->vka, path.capPtr);
            return error;
        }
    }
    return 0;
}

uintptr_t vmm_map_guest_device(vmm_t *vmm, uintptr_t paddr, size_t bytes, size_t align) {
    int error;
    /* Reserve a region that is guaranteed to have an aligned region in it */
    guest_memory_t *guest_memory = &vmm->guest_mem;
    uintptr_t reservation_base;
    reservation_t reservation = vspace_reserve_range(&guest_memory->vspace, bytes + align, seL4_AllRights, 1, (void**)&reservation_base);
    if (!reservation.res) {
        ZF_LOGE("Failed to allocate reservation of size %zu when mapping memory from %p", bytes + align, (void*)paddr);
        return 0;
    }
    /* Round up reservation so it is aligned. Hopefully it also ends up page aligned with the
     * sun moon and stars and we can actually find a frame cap and map it */
    uintptr_t map_base = ROUND_UP(reservation_base, align);
    error = vmm_map_guest_device_reservation(vmm, paddr, bytes, reservation, map_base);
    vspace_free_reservation(&guest_memory->vspace, reservation);
    if (error) {
        return 0;
    }
    return map_base;
}

int vmm_map_guest_device_at(vmm_t *vmm, uintptr_t vaddr, uintptr_t paddr, size_t bytes) {
    int error;
    /* Reserve a region that is guaranteed to have an aligned region in it */
    guest_memory_t *guest_memory = &vmm->guest_mem;
    reservation_t reservation = vspace_reserve_range_at(&guest_memory->vspace, (void*)vaddr, bytes, seL4_AllRights, 1);
    if (!reservation.res) {
        ZF_LOGE("Failed to allocate reservation of size %zu when mapping device from %p at %p", bytes, (void*)paddr, (void*)vaddr);
        return -1;
    }
    error = vmm_map_guest_device_reservation(vmm, paddr, bytes, reservation, vaddr);
    vspace_free_reservation(&guest_memory->vspace, reservation);
    return error;
}

int vmm_alloc_guest_ram_at(vmm_t *vmm, uintptr_t start, size_t bytes) {
    int ret;
    ret = vmm_alloc_guest_device_at(vmm, start, bytes);
    if (ret) {
        return ret;
    }
    ret = expand_guest_ram_region(&vmm->guest_mem, start, bytes);
    if (ret) {
        return ret;
    }
    printf("Guest RAM regions after allocating range 0x%x-0x%x:\n", (unsigned int)start, (unsigned int)(start + bytes));
    print_guest_ram_regions(&vmm->guest_mem);
    return 0;
}

int vmm_alloc_guest_ram(vmm_t *vmm, size_t bytes, int onetoone) {
    if (onetoone) {
        return vmm_alloc_guest_ram_one_to_one(vmm, bytes);
    }
    /* Allocate ram anywhere */
    guest_memory_t *guest_memory = &vmm->guest_mem;
    int page_size = vmm->page_size;
    uintptr_t base;
    int error;
    int num_pages = ROUND_UP(bytes, BIT(page_size)) >> page_size;
    reservation_t reservation = vspace_reserve_range(&guest_memory->vspace, num_pages * BIT(page_size), seL4_AllRights, 1, (void**)&base);
    if (!reservation.res) {
        ZF_LOGE("Failed to create reservation for %zu guest ram bytes", bytes);
        return -1;
    }
    /* Create pages */
    for (int i = 0 ; i < num_pages; i++) {
        seL4_Word cookie;
        cspacepath_t path;
        error = vka_cspace_alloc_path(&vmm->vka, &path);
        if (error) {
            ZF_LOGE("Failed to allocate path");
            return -1;
        }
        cookie = allocman_utspace_alloc(vmm->allocman, page_size, kobject_get_type(KOBJECT_FRAME, page_size), &path, true, &error);
        if (error) {
            ZF_LOGE("Failed to allocate page");
            return -1;
        }
        error = vspace_map_pages_at_vaddr(&guest_memory->vspace, &path.capPtr, &cookie, (void*)(base + i * BIT(page_size)), 1, page_size, reservation);
        if (error) {
            ZF_LOGE("Failed to map page");
            return -1;
        }
    }
    error = expand_guest_ram_region(&vmm->guest_mem, base, bytes);
    if (error) {
        return error;
    }
    vspace_free_reservation(&guest_memory->vspace, reservation);
    printf("Guest RAM regions after allocating range 0x%x-0x%x:\n", (unsigned int)base, (unsigned int)(base + bytes));
    print_guest_ram_regions(&vmm->guest_mem);
    return 0;
}
