/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>
#include <allocman/utspace/twinkle.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <string.h>

static inline size_t _round_up(size_t v, size_t bits)
{
    size_t mask = MASK(bits);
    if ( (v & mask) != 0) {
        v = (v & ~mask) + BIT(bits);
    }
    return v;
}

void utspace_twinkle_create(utspace_twinkle_t *twinkle)
{
    twinkle->num_uts = 0;
    twinkle->uts = NULL;
}

int _utspace_twinkle_add_uts(allocman_t *alloc, void *_twinkle, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, int utType) {
    utspace_twinkle_t *twinkle = (utspace_twinkle_t*) _twinkle;
    struct utspace_twinkle_ut *new_uts;
    int error;
    size_t i;
    if (utType != ALLOCMAN_UT_KERNEL) {
        ZF_LOGE("Twinkle does not support device untypeds");
        return -1;
    }
    new_uts = allocman_mspace_alloc(alloc, sizeof(struct utspace_twinkle_ut) * (num + twinkle->num_uts), &error);
    if (error) {
        return error;
    }
    if (twinkle->uts) {
        memcpy(new_uts, twinkle->uts, sizeof(struct utspace_twinkle_ut) * twinkle->num_uts);
        allocman_mspace_free(alloc, twinkle->uts, sizeof(struct utspace_twinkle_ut) * twinkle->num_uts);
    }
    for (i = 0; i < num; i++,twinkle->num_uts++) {
        new_uts[twinkle->num_uts] = (struct utspace_twinkle_ut) {uts[i], 0, size_bits[i]};
    }
    twinkle->uts = new_uts;
    return 0;
}

seL4_Word _utspace_twinkle_alloc(allocman_t *alloc, void *_twinkle, size_t size_bits, seL4_Word type, const cspacepath_t *slot, uintptr_t paddr, bool canBeDev, int *error)
{
    utspace_twinkle_t *twinkle = (utspace_twinkle_t*)_twinkle;
    size_t sel4_size_bits;
    int sel4_error;
    size_t i, j;
    if (paddr != ALLOCMAN_NO_PADDR) {
        ZF_LOGE("Twinkle does not support allocating explicit physical addresses");
        return -1;
    }
    /* get size of untyped call */
    sel4_size_bits = get_sel4_object_size(type, size_bits);
    if (size_bits != vka_get_object_size(type, sel4_size_bits) || size_bits == 0) {
        SET_ERROR(error, 1);
        return 0;
    }
    /* Search for the first thing that will support us */
    for (i = 0; i < twinkle->num_uts; i++) {
        if (_round_up(twinkle->uts[i].offset, size_bits) + BIT(size_bits) <= BIT(twinkle->uts[i].size_bits)) {
            break;
        }
    }
    if (i == twinkle->num_uts) {
        SET_ERROR(error, 1);
        return 0;
    }
    /* Check all untypeds of this size, and pick the one that will waste the least space */
    for (j = i; j < twinkle->num_uts && twinkle->uts[i].size_bits == twinkle->uts[j].size_bits; j++) {
        if ((_round_up(twinkle->uts[j].offset, size_bits) + BIT(size_bits) <= BIT(twinkle->uts[j].size_bits)) &&
                (_round_up(twinkle->uts[j].offset, size_bits) - twinkle->uts[j].offset <
                 _round_up(twinkle->uts[i].offset, size_bits) - twinkle->uts[i].offset)) {
            i = j;
        }
    }
    /* if using inc retype then our offset calculation is effectively emulating the kernels calculations. This
     * means we track the free space of the untyped correctly, and since we are not going to try and free then
     * allocate again, this allocator can be used with either allocation scheme */
    sel4_error = seL4_Untyped_Retype(twinkle->uts[i].path.capPtr, type, sel4_size_bits, slot->root, slot->dest, slot->destDepth, slot->offset, 1);
    if (sel4_error != seL4_NoError) {
        /* Well this shouldn't happen */
        SET_ERROR(error, 1);
        return 0;
    }

    /* Update allocation information */
    twinkle->uts[i].offset = _round_up(twinkle->uts[i].offset, size_bits) + BIT(size_bits);

    /* We do not support free so we return an empty cookie */
    SET_ERROR(error, 0);
    return 0;
}

void _utspace_twinkle_free(allocman_t *alloc, void *_twinkle, seL4_Word cookie, size_t size_bits)
{
    /* Do nothing */
}
