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
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <vka/vka.h>
#include <vka/object.h>
#include <vka/capops.h>

#include <simple/simple.h>
#include <simple-stable/simple-stable.h>

#include "vmm/platform/phys_alloc.h"

extern simple_t vmm_plat_simple;

#if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)

/* Hopefully the seL4 kernel won't segment memory worse than this. */
#define VMM_MAX_BOOTINFO_CONTIGUOUS_SEGMENTS 64

/* Max. number of capabilities segment. */
#define VMM_MAX_RESERVATION_MAXCAPS 32

typedef struct phys_reservation {
    uint32_t start_paddr;
    uint32_t size;
    uint32_t page_size;
    uint32_t page_size_bits;

    seL4_CPtr untyped[VMM_MAX_RESERVATION_MAXCAPS];
    uint32_t untyped_size[VMM_MAX_RESERVATION_MAXCAPS];
    uint32_t num_untypeds;

    int cur;
    uint32_t cur_paddr;
    uint32_t cur_offset;
} phys_reservation_t;

static phys_reservation_t pframe_reservation;

/* This is called while bootstrapping and must *NOT* printout, except in error cases */
uint32_t pframe_alloc_init(simple_t *simple, uint32_t paddr_min,
                           uint32_t size, uint32_t page_size_bits, bool *stole_untyped_list, seL4_CPtr *free_start) {
    assert(simple);

    /* Create a list of contiguous memory regions, joining any contigious untyped memory. */
    /* Assumption made here is that the untypes in bootinfo are sorted by paddr ascending. */
    /* We are only searching for the largest one so just need a list of two */
    uint32_t cont_reg_start_index[2];
    uint32_t cont_reg_end_index[2];
    uint32_t cont_reg_size[2] = {0};
    uint32_t cont_reg = 0;
    uint32_t page_size = (1 << page_size_bits);
    uint32_t dummy; // used for passing to functions when we don't care
    for (int i = 0; i < simple_get_untyped_count(simple); i++) {
        uintptr_t prev_paddr;
        uint32_t prev_size;
        uintptr_t cur_paddr;
        uint32_t cur_size;
        if (i > 0) {
            simple_get_nth_untyped(simple, i - 1, &prev_size, (uint32_t*)&prev_paddr);
        }
        simple_get_nth_untyped(simple, i, &cur_size, (uint32_t*)&cur_paddr);
        if (    /* Always new region for first untyped. */
                i > 0 &&
                /* May only continue if contiguous from last untyped. */
                (prev_paddr + (1 << prev_size)) == cur_paddr &&
                /* Size has to be multiple of the supported page size, or else seamless continuation
                 * between the two untypeds is impossible. */
                ((1 << prev_size) % page_size) == 0 &&
                cur_paddr > paddr_min
            ) {
            /* Add to previous region. */
            cont_reg_end_index[cont_reg] = i;
            cont_reg_size[cont_reg] += (1 << cur_size);
        } else {
            /* Start a new region. on whichever region is currently smaller. unless
             * we have found something of the right size, then replace the large one */
            if (cont_reg_size[0] > size && cont_reg_size[1] > size) {
                cont_reg = cont_reg_size[0] < cont_reg_size[1] ? 1 : 0;
            } else {
                cont_reg = cont_reg_size[0] < cont_reg_size[1] ? 0 : 1;
            }
            cont_reg_start_index[cont_reg] = i;
            cont_reg_end_index[cont_reg] = i;
            cont_reg_size[cont_reg] = (1 << cur_size);
        }
    }

    /* Pick the largest of the regions */
    uint32_t largest_region = 0, allocated_region = 0;
    for (uint32_t i = 0; i < 2; i++) {
        if (cont_reg_size[i] > largest_region) {
            largest_region = cont_reg_size[i];
            allocated_region = i;
        }
    }

    /* Reserve this region and save its information, then hide the reserved region from the global
     * seL4_BootInfo structure, so that future allocators do not know about these untypeds and thus
     * do not allocate from them. */

    simple_get_nth_untyped(simple, cont_reg_start_index[allocated_region], &dummy, &pframe_reservation.start_paddr);
    pframe_reservation.size = 0;
    pframe_reservation.num_untypeds = 0;
    pframe_reservation.page_size = page_size;
    pframe_reservation.page_size_bits = page_size_bits;
    pframe_reservation.cur = 0;
    pframe_reservation.cur_paddr = pframe_reservation.start_paddr;
    pframe_reservation.cur_offset = 0;
    uint32_t last_reserved_cap = 0xFFFFFFF0;

    for (int i = cont_reg_start_index[allocated_region];
         i <= cont_reg_end_index[allocated_region]; i++) {
        uintptr_t cur_paddr;
        uint32_t cur_size;
        seL4_CPtr cur_ut = simple_get_nth_untyped(simple, i, &cur_size, (uint32_t*)&cur_paddr);

        if (paddr_min && cur_paddr < paddr_min) {
            /* We can't use this untyped because its paddr is below the minimum allowed. */
            continue;
        }

        assert(i >= 0 && i < simple_get_untyped_count(simple));
        if (pframe_reservation.size >= size){
            /* We got enough memory. Don't steal everything. */
            break;
        }

        /* Move this cap into its new location in the empty caps space. */
        last_reserved_cap = i;
        int error = seL4_CNode_Move(
            simple_get_cnode(simple),
                *free_start,
            seL4_WordBits,
            simple_get_cnode(simple),
                cur_ut,
            seL4_WordBits);
        assert(error == seL4_NoError);

        pframe_reservation.untyped[pframe_reservation.num_untypeds] = *free_start;
        pframe_reservation.untyped_size[pframe_reservation.num_untypeds] =
            (1 << cur_size);
        pframe_reservation.size += pframe_reservation.untyped_size[pframe_reservation.num_untypeds];
        pframe_reservation.num_untypeds++;

        /* Steal this empty cap away from future allocators. */
        (*free_start)++;

        stole_untyped_list[i] = true;
    }
    if (last_reserved_cap == 0xFFFFFFF0) {
        panic("Could not reserved any untypeds.");
        return 0;
    }
    if (pframe_reservation.size < size) {
        printf(COLOUR_R "ERROR: pframe allocator could not allocate 0x%x bytes. It could only"
               "allocate 0x%x bytes.\n" COLOUR_RESET, size, pframe_reservation.size);
        panic("Could not allocate enough contiguous memory.");
        return 0;
    }

    assert(pframe_reservation.size >= size);
    assert(last_reserved_cap + 1 == cont_reg_start_index[allocated_region] +
           pframe_reservation.num_untypeds);

    assert(pframe_reservation.cur_paddr >= paddr_min);

    /* Return the size of anything remaining */
    return pframe_reservation.size -
        (pframe_reservation.cur_paddr - pframe_reservation.start_paddr);
}

int pframe_alloc(vka_t* vka, seL4_CPtr* out_cap, uint32_t* out_paddr) {
    assert(vka && out_cap && out_paddr);
    assert(pframe_reservation.size > 0);

    /* Create a cspace slot for the frame to sit in. */
    cspacepath_t frame;
    int error = vka_cspace_alloc_path(vka, &frame);
    assert(error == seL4_NoError);

    /* Do we have space left on the current untyped? */
    while (pframe_reservation.untyped_size[pframe_reservation.cur] < pframe_reservation.page_size) {
        /* Untyped sizes should be divisible by pagesize, the pframe_alloc_init() function should
         * have made sure of that already. */
        assert(pframe_reservation.untyped_size[pframe_reservation.cur] == 0);

        pframe_reservation.cur++;
        pframe_reservation.cur_offset = 0;
        if (pframe_reservation.cur >= pframe_reservation.num_untypeds) {
            panic("Out of memory for guest OS.");
            return -1;
        }
    }

    /* Create the actual frame. */
#ifdef CONFIG_KERNEL_STABLE
    error = seL4_Untyped_RetypeAtOffset(
        pframe_reservation.untyped[pframe_reservation.cur],
        kobject_get_type(KOBJECT_FRAME, pframe_reservation.page_size_bits),
        pframe_reservation.cur_offset,
        kobject_get_size(KOBJECT_FRAME, pframe_reservation.page_size_bits),
        simple_get_cnode(&vmm_plat_simple), frame.root, frame.capDepth, frame.capPtr,
        1);
#else
    error = seL4_Untyped_Retype(
        pframe_reservation.untyped[pframe_reservation.cur],
        kobject_get_type(KOBJECT_FRAME, pframe_reservation.page_size_bits),
        kobject_get_size(KOBJECT_FRAME, pframe_reservation.page_size_bits),
        simple_get_cnode(&vmm_plat_simple), frame.root, frame.capDepth, frame.capPtr,
        1);
#endif
    assert(error == seL4_NoError);
    if (error != seL4_NoError) {
        return -1;
    }

    /* Output allocation. */
    *out_cap = frame.capPtr;
    *out_paddr = pframe_reservation.cur_paddr;

    /* Update book keeping. */
    pframe_reservation.cur_paddr += pframe_reservation.page_size;
    pframe_reservation.cur_offset += pframe_reservation.page_size;
    pframe_reservation.untyped_size[pframe_reservation.cur] -= pframe_reservation.page_size;

    return 0;
}

uint32_t pframe_skip_to_aligned(uint32_t align, uint32_t size_left_min_check) {

    /* Skip the allocator to the next available address which meets the alignment. */
    while(pframe_reservation.cur_paddr % align != 0) {
        /* Skip to the next untyped and hope the alignment requirements are now met. */
        pframe_reservation.cur_paddr += pframe_reservation.untyped_size[pframe_reservation.cur];
        pframe_reservation.cur++;
        if (pframe_reservation.cur >= pframe_reservation.num_untypeds) {
            panic("Could not find suitable aligned address.");
            return 0;
        }
    }

    /* Check that there's still enough memory left. */
    if (size_left_min_check) {
        uint32_t size_left = 0;
        for (int i = pframe_reservation.cur; i < pframe_reservation.num_untypeds; i++) {
            size_left += pframe_reservation.untyped_size[i];
        }
        if (size_left < size_left_min_check) {
            printf(COLOUR_R "pframe_skip_to_aligned: failed size_left minimum check!!!\n"
                   COLOUR_RESET);
            panic("Not enough space left after alignment requirement.");
            return 0;
        }
    }

    return pframe_reservation.cur_paddr;
}

#endif /* LIB_VMM_GUEST_DMA_ONE_TO_ONE */
