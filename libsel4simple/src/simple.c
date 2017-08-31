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

#include <simple/simple_helpers.h>

bool simple_is_untyped_cap(simple_t *simple, seL4_CPtr pos)
{
    int i;

    for (i = 0; i < simple_get_untyped_count(simple); i++) {
        seL4_CPtr ut_pos = simple_get_nth_untyped(simple, i, NULL, NULL, NULL);
        if (ut_pos == pos) {
            return true;
        }
    }

    return false;
}

int simple_vka_cspace_alloc(void *data, seL4_CPtr *slot)
{
    assert(data && slot);

    simple_t *simple = data;
    seL4_CNode cnode = simple_get_cnode(simple);
    int i = 0;

    /* Keep trying to find the next free slot by seeing if we can copy something there */
    seL4_Error error = seL4_CNode_Copy(cnode, simple_get_cap_count(simple) + i, seL4_WordBits, cnode, cnode, seL4_WordBits, seL4_AllRights);
    while (error == seL4_DeleteFirst) {
        i++;
        error = seL4_CNode_Copy(cnode, simple_get_cap_count(simple) + i, seL4_WordBits, cnode, cnode, seL4_WordBits, seL4_AllRights);
    }

    if (error != seL4_NoError) {
        error = seL4_CNode_Delete(cnode, simple_get_cap_count(simple) + i, seL4_WordBits);
        return error;
    }

    error = seL4_CNode_Delete(cnode, simple_get_cap_count(simple) + i, seL4_WordBits);
    if (error != seL4_NoError) {
        return error;
    }

    *slot = simple_get_cap_count(simple) + i;
    return seL4_NoError;
}

void simple_vka_cspace_make_path(void *data, seL4_CPtr slot, cspacepath_t *path)
{
    assert(data && path);

    simple_t *simple = data;

    path->capPtr = slot;
    path->capDepth = seL4_WordBits;
    path->root = simple_get_cnode(simple);
    path->dest = simple_get_cnode(simple);
    path->offset = slot;
    path->destDepth = seL4_WordBits;
}

void simple_make_vka(simple_t *simple, vka_t *vka)
{
    vka->data = simple;
    vka->cspace_alloc = &simple_vka_cspace_alloc;
    vka->cspace_make_path = &simple_vka_cspace_make_path;
    vka->utspace_alloc = NULL;
    vka->utspace_alloc_maybe_device = NULL;
    vka->cspace_free = NULL;
    vka->utspace_free = NULL;
}

seL4_CPtr simple_last_valid_cap(simple_t *simple)
{
    seL4_CPtr largest = 0;
    int i;
    for (i = 0; i < simple_get_cap_count(simple); i++) {
        seL4_CPtr cap = simple_get_nth_cap(simple, i);
        if (cap > largest) {
            largest = cap;
        }
    }
    return largest;
}
