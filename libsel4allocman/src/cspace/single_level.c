/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <allocman/cspace/single_level.h>
#include <allocman/util.h>
#include <allocman/allocman.h>
#include <sel4/sel4.h>
#include <string.h>

int cspace_single_level_create(struct allocman *alloc, cspace_single_level_t *cspace, struct cspace_single_level_config config)
{
    uint32_t num_slots;
    uint32_t num_entries;
    int error;
    cspace->config = config;
    /* Allocate bitmap */
    num_slots = cspace->config.end_slot - cspace->config.first_slot;
    num_entries = num_slots / 32;
    cspace->bitmap_length = num_entries;
    if (num_slots % 32 != 0) {
        num_entries++;
    }
    cspace->bitmap = (uint32_t*)allocman_mspace_alloc(alloc, num_entries * sizeof(uint32_t), &error);
    if (error) {
        return error;
    }
    /* Make everything 1's */
    memset(cspace->bitmap, -1, num_entries * sizeof(uint32_t));
    if (num_slots % 32 != 0) {
        /* Mark the padding slots as allocated */
        uint32_t excess = num_slots % 32;
        uint32_t i;
        for (i = excess; i < 32; i++) {
            cspace->bitmap[num_entries - 1] ^= BIT(i);
        }
    }
    cspace->last_entry = 0;
    return 0;
}

void cspace_single_level_destroy(struct allocman *alloc, cspace_single_level_t *cspace)
{
    allocman_mspace_free(alloc, cspace->bitmap, cspace->bitmap_length * sizeof(uint32_t));
}

int _cspace_single_level_alloc(allocman_t *alloc, void *_cspace, cspacepath_t *slot)
{
    uint32_t i;
    uint32_t index;
    cspace_single_level_t *cspace = (cspace_single_level_t*)_cspace;
    i = cspace->last_entry;
    if (cspace->bitmap[i] == 0) {
        assert(cspace->bitmap_length != 0);
        assert(cspace->last_entry < cspace->bitmap_length);
        do {
            i = (i + 1) % cspace->bitmap_length;
        } while (cspace->bitmap[i] == 0 && i != cspace->last_entry);
        if (i == cspace->last_entry) {
            return 1;
        }
        cspace->last_entry = i;
    }
    index = 31 - CLZ(cspace->bitmap[i]);
    cspace->bitmap[i] &= ~BIT(index);
    *slot = _cspace_single_level_make_path(cspace, cspace->config.first_slot + (i * 32 + index));
    return 0;
}

int _cspace_single_level_alloc_at(allocman_t *alloc, void *_cspace, seL4_CPtr slot) {
    cspace_single_level_t *cspace = (cspace_single_level_t*)_cspace;
    uint32_t index = slot - cspace->config.first_slot;
    /* make sure index is in range */
    if (index / 32 >= cspace->bitmap_length) {
        return 1;
    }
    /* make sure not already allocated */
    if ( (cspace->bitmap[index / 32] & BIT(index % 32)) == 0) {
        return 1;
    }
    /* mark it as allocated */
    cspace->bitmap[index / 32] &= ~BIT(index % 32);
    return 0;
}

void _cspace_single_level_free(allocman_t *alloc, void *_cspace, const cspacepath_t *slot)
{
    cspace_single_level_t *cspace = (cspace_single_level_t*)_cspace;
    uint32_t index = slot->capPtr - cspace->config.first_slot;
    assert((cspace->bitmap[index / 32] & BIT(index % 32)) == 0);
    cspace->bitmap[index / 32] |= BIT(index % 32);
}
