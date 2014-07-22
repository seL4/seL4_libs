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
#include <allocman/allocman.h>

#include <vka/vka.h>
#include <vka/object.h>
#include <vka/capops.h>

#include <simple/simple.h>
#include <simple-stable/simple-stable.h>

#include "vmm/platform/ram_alloc.h"

extern simple_t vmm_plat_simple;

#if defined(LIB_VMM_GUEST_DMA_ONE_TO_ONE) || defined(LIB_VMM_GUEST_DMA_IOMMU)

#define RAM_ALLOC_MAX_UNTYPED_SIZE 27

typedef struct guest_ram_alloc_state {
    seL4_CPtr untyped;
    uint32_t untyped_size;
    uint32_t untyped_offset;
    uint32_t untyped_paddr;
    vka_object_t untyped_vka_object;

    uint32_t page_size_bits;
    allocman_t* allocman;
    uint32_t max_untyped_size;
} guest_ram_alloc_state_t;

static bool guest_ram_alloc_initialised = false;
static guest_ram_alloc_state_t guest_ram_alloc_state;

void guest_ram_alloc_init(allocman_t* allocman) {
    guest_ram_alloc_state_t* state = &guest_ram_alloc_state;
    assert(allocman);
    assert(!guest_ram_alloc_initialised);
    state->untyped = 0;
    state->untyped_size = 0;
    state->untyped_offset = 0;
#ifdef CONFIG_VMM_USE_4M_FRAMES
    state->page_size_bits = seL4_4MBits;
#else
    state->page_size_bits = seL4_PageBits;
#endif
    state->allocman = allocman;
    state->max_untyped_size = RAM_ALLOC_MAX_UNTYPED_SIZE;
    guest_ram_alloc_initialised = true;
}

static void guest_ram_frame_allocate_untyped(vka_t* vka, guest_ram_alloc_state_t* state) {
    assert(vka && state && state->allocman);
 
    if (state->untyped_size >= (1 << state->page_size_bits)) {
        /* No need to reallocate untyped, current one has enough space left. */
        return;
    }

    state->untyped = 0;
    state->untyped_size = 0;
    state->untyped_offset = 0;

    /* We allocate the largest untyped object currently available, in order to minimise contiguous
     * memory segmentation. */
    for (uint32_t untyped_bits = state->max_untyped_size;
            untyped_bits >= state->page_size_bits; untyped_bits--) {
        dprintf(3, "guest_ram_frame_allocate_untyped trying 2^%d\n", untyped_bits);
        int err = vka_alloc_untyped(vka, untyped_bits, &state->untyped_vka_object);
        if (err == seL4_NoError) {
            /* Found new untyped. */
            state->untyped = state->untyped_vka_object.cptr;
            state->untyped_size = (1 << untyped_bits);
            state->untyped_paddr = allocman_utspace_paddr(state->allocman,
                    state->untyped_vka_object.ut, untyped_bits);
            break;
        }
        state->max_untyped_size = untyped_bits - 1;
    }
    if (state->untyped == 0 || state->untyped_size == 0) {
        panic("Guest RAM allocator could not find any untypeds. Out of memory.");
        return;
    }
}

int guest_ram_frame_alloc(vka_t* vka, seL4_CPtr* out_cap, uint32_t* out_paddr) {
    assert(vka && out_cap && out_paddr);
    guest_ram_alloc_state_t* state = &guest_ram_alloc_state;

    if (!guest_ram_alloc_initialised) {
        assert(!"Guest RAM allocator used before initialised.");
        return -1;
    }

    /* Allocate a new untyped if the current untyped doesn't have enough space left. */
    if (state->untyped_size < (1 << state->page_size_bits)) {
        guest_ram_frame_allocate_untyped(vka, state);
    }

    /* Create a cspace slot for the frame to sit in. */
    cspacepath_t frame;
    int error = vka_cspace_alloc_path(vka, &frame);
    assert(error == seL4_NoError);

    /* Create the actual frame. */
#ifdef CONFIG_KERNEL_STABLE
    error = seL4_Untyped_RetypeAtOffset(
        state->untyped,
        kobject_get_type(KOBJECT_FRAME, state->page_size_bits),
        state->untyped_offset,
        kobject_get_size(KOBJECT_FRAME, state->page_size_bits),
        simple_get_cnode(&vmm_plat_simple), frame.root, frame.capDepth, frame.capPtr,
        1);
#else
    error = seL4_Untyped_Retype(
        state->untyped,
        kobject_get_type(KOBJECT_FRAME, state->page_size_bits),
        kobject_get_size(KOBJECT_FRAME, state->page_size_bits),
        simple_get_cnode(&vmm_plat_simple), frame.root, frame.capDepth, frame.capPtr,
        1);
#endif
    if (error != seL4_NoError) {
        /* If it reached here, it is most likely due to an allocator / kernel bug. */
        assert(!"Guest RAM allocator could not allocate frame from untyped.");
        return -1;
    }

    /* Output allocation. */
    *out_cap = frame.capPtr;
    *out_paddr = state->untyped_paddr;

    /* Update book keeping. */
    state->untyped_paddr += (1 << state->page_size_bits);
    state->untyped_offset += (1 << state->page_size_bits);
    state->untyped_size -= (1 << state->page_size_bits);

    return 0;
}

#endif /* LIB_VMM_GUEST_DMA_ONE_TO_ONE */

