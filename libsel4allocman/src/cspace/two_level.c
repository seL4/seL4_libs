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

#include <allocman/cspace/two_level.h>
#include <allocman/util.h>
#include <allocman/allocman.h>
#include <sel4/sel4.h>
#include <string.h>
#include <utils/attribute.h>

cspacepath_t _cspace_two_level_make_path(void *_cspace, seL4_CPtr slot)
{
    cspacepath_t l1_path, l2_path;
    size_t l1slot, l2slot;
    cspace_two_level_t *cspace = (cspace_two_level_t*)_cspace;
    l1slot = slot >> cspace->config.level_two_bits;
    l2slot = slot & MASK(cspace->config.level_two_bits);
    /* This is an excessive way of constructing the path. But it's cool to do it in a way that
       makes no assumptions about the underlying cspace structures. In particular the second level
       could be a two level cspace and this would still work */
    if(!cspace->second_levels[l1slot]) {
        assert(!"ERROR: Tried make a path to a non existant slot\n");
        return (cspacepath_t) {0, 0, 0, 0, 0, 0, 0};
    }
    l1_path = _cspace_single_level_make_path(&cspace->first_level, l1slot);
    l2_path =_cspace_single_level_make_path(&cspace->second_levels[l1slot]->second_level, l2slot);
    return (cspacepath_t) {
        .root = l1_path.root,
        .capPtr = (l1_path.capPtr << l2_path.capDepth) | l2_path.capPtr,
        .capDepth = l1_path.capDepth + l2_path.capDepth,
        .dest = (l1_path.capPtr << l2_path.destDepth) | l2_path.dest,
        .destDepth = l1_path.capDepth + l2_path.destDepth,
        .offset = l2_path.offset
    };
    /* This is the simpler version */
    /* path->root = cspace->config.cnode;
    path->slot = slot;
    path->slot_depth = cspace->config.cnode_size_bits + cspace->config.cnode_guard_bits + cspace->config.level_two_bits;
    path->cnode = l1slot;
    path->cnode_depth = cspace->config.cnode_size_bits + cspace->config.cnode_guard_bits;
    path->cnode_offset = l2slot;*/
}

static int _create_second_level(allocman_t *alloc, cspace_two_level_t *cspace, size_t index, int alloc_node)
{
    int error;
    struct cspace_single_level_config single_config = {
        /* There is no cap at depth 32 to reference the cnode of this cspace, fortunately
           though we don't actually need it */
        .cnode = 0,
        .cnode_size_bits = cspace->config.level_two_bits,
        .cnode_guard_bits = 0,
        .first_slot = 0,
        .end_slot = BIT(cspace->config.level_two_bits)
    };
    cspace->second_levels[index] = (struct cspace_two_level_node*) allocman_mspace_alloc(alloc, sizeof(struct cspace_two_level_node), &error);
    if (error) {
        return error;
    }
    if (alloc_node) {
        cspacepath_t path = _cspace_single_level_make_path(&cspace->first_level, index);
        cspace->second_levels[index]->cookie = allocman_utspace_alloc(alloc, cspace->config.level_two_bits + seL4_SlotBits, seL4_CapTableObject, &path, false, &error);
        cspace->second_levels[index]->cookie_valid = 1;
    } else {
        cspace->second_levels[index]->cookie_valid = 0;
    }
    if (error) {
        allocman_mspace_free(alloc, cspace->second_levels[index], sizeof(struct cspace_two_level_node));
        cspace->second_levels[index] = NULL;
        return error;
    }
    error = cspace_single_level_create(alloc, &cspace->second_levels[index]->second_level, single_config);
    if (error) {
        allocman_utspace_free(alloc, cspace->second_levels[index]->cookie, cspace->config.level_two_bits + seL4_SlotBits);
        allocman_mspace_free(alloc, cspace->second_levels[index], sizeof(struct cspace_two_level_node));
        cspace->second_levels[index] = NULL;
        return error;
    }
    cspace->second_levels[index]->count = 0;
    return 0;
}

int cspace_two_level_create(allocman_t *alloc, cspace_two_level_t *cspace, struct cspace_two_level_config config)
{
    int error;
    size_t i;
    struct cspace_single_level_config single_config = {
        .cnode = config.cnode,
        .cnode_size_bits = config.cnode_size_bits,
        .cnode_guard_bits = config.cnode_guard_bits,
        .first_slot = config.first_slot,
        .end_slot = config.end_slot
    };
    cspace->config = config;
    cspace->second_levels = (struct cspace_two_level_node **)allocman_mspace_alloc(alloc, sizeof(struct cspace_two_level_node*) * BIT(config.cnode_size_bits), &error);
    if (error) {
        return error;
    }
    error = cspace_single_level_create(alloc, &cspace->first_level, single_config);
    if (error) {
        allocman_mspace_free(alloc, cspace->second_levels, sizeof(struct cspace_two_level_node*) * BIT(config.cnode_size_bits));
        return error;
    }
    for (i = 0; i < BIT(config.cnode_size_bits); i++) {
        cspace->second_levels[i] = NULL;
    }
    cspace->last_second_level = 0;
    for (i = config.start_existing_index; i < config.end_existing_index; i++) {
        error = _cspace_single_level_alloc_at(alloc, &cspace->first_level, (seL4_CPtr) i);
        if (error) {
            cspace_two_level_destroy(alloc, cspace);
            return error;
        }
        error = _create_second_level(alloc, cspace, i, 0);
        if (error) {
            cspace_two_level_destroy(alloc, cspace);
            return error;
        }
    }
    /* now fill in any slots that may already be allocated */
    for (i = config.start_existing_slot; i < config.end_existing_slot; i++) {
        error = _cspace_two_level_alloc_at(alloc, cspace, (seL4_CPtr) i);
        if (error) {
            cspace_two_level_destroy(alloc, cspace);
            return error;
        }
    }
    return 0;
}

int _cspace_two_level_alloc_at(allocman_t *alloc, void *_cspace, seL4_CPtr slot) {
    cspace_two_level_t *cspace = (cspace_two_level_t*)_cspace;
    size_t l1slot;
    size_t l2slot;
    int error;
    l1slot = slot >> cspace->config.level_two_bits;
    l2slot = slot & MASK(cspace->config.level_two_bits);
    /* see if the first level exists */
    if (!cspace->second_levels[l1slot]) {
        error = _cspace_single_level_alloc_at(alloc, &cspace->first_level, l1slot);
        if(error) {
            return error;
        }
        error = _create_second_level(alloc, cspace, l1slot, 1);
        if (error) {
            return error;
        }
    }
    /* now try and allocate from the second level */
    error = _cspace_single_level_alloc_at(alloc, &cspace->second_levels[l1slot]->second_level, (seL4_CPtr) l2slot);
    if (error) {
        return error;
    }
    cspace->second_levels[l1slot]->count++;
    return 0;
}

int _cspace_two_level_alloc(allocman_t *alloc, void *_cspace, cspacepath_t *slot)
{
    cspace_two_level_t *cspace = (cspace_two_level_t*)_cspace;
    size_t i;
    int found;
    int first;
    int error;
    cspacepath_t level2_slot;
    /* Hunt for a slot */
    i = cspace->last_second_level;
    found = 0;
    first = 1;
    while (!found && (first || i != cspace->last_second_level)) {
        first = 0;
        if (cspace->second_levels[i] && cspace->second_levels[i]->count < MASK(cspace->config.level_two_bits)) {
            found = 1;
        } else {
            i = (i + 1) % BIT(cspace->config.cnode_size_bits);
        }
    }
    if (!found) {
        /* ask the first level node for an empty slot */
        cspacepath_t l1slot;
        error = _cspace_single_level_alloc(alloc, &cspace->first_level, &l1slot);
        if (error) {
            /* our cspace is just full */
            return error;
        }
        /* use this index */
        error = _create_second_level(alloc, cspace, l1slot.offset, 1);
        if (error) {
            return error;
        }
    }
    cspace->last_second_level = i;
    error = _cspace_single_level_alloc(alloc, &cspace->second_levels[i]->second_level, &level2_slot);
    if (error) {
        /* This just shouldn't be possible */
        assert(!"cspace_single_level not behaving as expected");
        return error;
    }
    cspace->second_levels[i]->count++;
    *slot = _cspace_two_level_make_path(cspace, (i << cspace->config.level_two_bits) | level2_slot.capPtr);
    return 0;
}

static void _destroy_second_level(allocman_t *alloc, cspace_two_level_t *cspace, size_t index)
{
    cspacepath_t path;
    cspace_single_level_destroy(alloc, &cspace->second_levels[index]->second_level);
    if (cspace->second_levels[index]->cookie_valid) {
        int error UNUSED;
        error = seL4_CNode_Delete(cspace->config.cnode, index, 32 - cspace->config.level_two_bits);
        assert(error == seL4_NoError);
        allocman_utspace_free(alloc, cspace->second_levels[index]->cookie, cspace->config.level_two_bits + seL4_SlotBits);
    }
    allocman_mspace_free(alloc, cspace->second_levels[index], sizeof(struct cspace_two_level_node));
    path = _cspace_single_level_make_path(&cspace->first_level, index);
    _cspace_single_level_free(alloc, &cspace->first_level, &path);
}

void _cspace_two_level_free(struct allocman *alloc, void *_cspace, const cspacepath_t *slot)
{
    size_t l1slot;
    size_t l2slot;
    seL4_CPtr cptr = slot->capPtr;
    cspacepath_t path;
    cspace_two_level_t *cspace = (cspace_two_level_t*)_cspace;
    l1slot = cptr >> cspace->config.level_two_bits;
    l2slot = cptr & MASK(cspace->config.level_two_bits);
    path = _cspace_single_level_make_path(&cspace->second_levels[l1slot]->second_level, l2slot);
    _cspace_single_level_free(alloc, &cspace->second_levels[l1slot]->second_level, &path);
    cspace->second_levels[l1slot]->count--;
    if (cspace->second_levels[l1slot]->count == 0) {
        _destroy_second_level(alloc, cspace, l1slot);
        cspace->second_levels[l1slot] = NULL;
    }
}

void cspace_two_level_destroy(struct allocman *alloc, cspace_two_level_t *cspace)
{
    size_t i;
    for (i = 0; i < BIT(cspace->config.cnode_size_bits); i++) {
        if (cspace->second_levels[i]) {
            _destroy_second_level(alloc, cspace, i);
        }
    }
    allocman_mspace_free(alloc, cspace->second_levels, sizeof(struct cspace_two_level_node*) * BIT(cspace->config.cnode_size_bits));
    cspace_single_level_destroy(alloc, &cspace->first_level);
}
