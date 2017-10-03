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

#pragma once

#include <autoconf.h>
#include <sel4/types.h>
#include <allocman/cspace/cspace.h>
#include <allocman/cspace/single_level.h>

struct cspace_two_level_config {
    /* A cptr to the first level cnode that we are managing slots in */
    seL4_CPtr cnode;
    /* Size in bits of the cnode */
    size_t cnode_size_bits;
    /* Guard depth added to this cspace. */
    size_t cnode_guard_bits;
    /* First valid slot (as an index) */
    size_t first_slot;
    /* Last valid slot + 1 (as an index) */
    size_t end_slot;
    /* Size of the second level nodes
       cnode_size_bits + cnode_guard_bits + level_two_bits <= 32 */
    size_t level_two_bits;
    /* if any of the second level cnodes are already created we can describe them here.
     * up to the user to have put the cspace together in a way that is compatible with
     * this allocator. The slot range is usual 'valid slot' to 'valid slot +1', with
     * the cptr range describing a used cap range which can start and stop partially in
     * any of the level 2 cnodes */
    size_t start_existing_index;
    size_t end_existing_index;
    seL4_CPtr start_existing_slot;
    seL4_CPtr end_existing_slot;
};

struct cspace_two_level_node {
    /* We count how many things we have allocated in each level two cnode.
       This allows us to quickly know whether it is already full, or of it is
       empty and we can free it */
    size_t count;
    /* The cspace representing the second level */
    cspace_single_level_t second_level;
    /* Cookie representing the untyped object used to create the cnode for this
       second level. This is only valid if we allocated this node, it may have
       been given to us in bootstrapping */
    seL4_Word cookie;
    int cookie_valid;
};

typedef struct cspace_two_level {
    /* Remember the config */
    struct cspace_two_level_config config;
    /* First level of cspace */
    cspace_single_level_t first_level;
    /* Our second level cspaces */
    struct cspace_two_level_node **second_levels;
    /* Remember which second level we last tried to allocate a slot from */
    size_t last_second_level;
} cspace_two_level_t;

int cspace_two_level_create(struct allocman *alloc, cspace_two_level_t *cspace, struct cspace_two_level_config config);
void cspace_two_level_destroy(struct allocman *alloc, cspace_two_level_t *cspace);

seL4_CPtr _cspace_two_level_boot_alloc(struct allocman *alloc, void *_cspace, int *error);
int _cspace_two_level_alloc(struct allocman *alloc, void *_cspace, cspacepath_t *slot);
void _cspace_two_level_free(struct allocman *alloc, void *_cspace, const cspacepath_t *slot);
int _cspace_two_level_alloc_at(struct allocman *alloc, void *_cspace, seL4_CPtr slot);

cspacepath_t _cspace_two_level_make_path(void *_cspace, seL4_CPtr slot);

static inline cspace_interface_t cspace_two_level_make_interface(cspace_two_level_t *cspace) {
    return (cspace_interface_t) {
        .alloc = _cspace_two_level_alloc,
        .free = _cspace_two_level_free,
        .make_path = _cspace_two_level_make_path,
        /* We do not want to handle recursion, as it shouldn't happen */
        .properties = ALLOCMAN_DEFAULT_PROPERTIES,
        .cspace = cspace
    };
}

