/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <simple/simple_helpers.h>

int simple_is_untyped_cap(simple_t *simple, seL4_CPtr pos)
{
    int i;

    for (i = 0; i < simple_get_untyped_count(simple); i++) {
        seL4_CPtr ut_pos = simple_get_nth_untyped(simple, i, NULL, NULL, NULL);
        if (ut_pos == pos) {
            return 1;
        }
    }

    return 0;
}

seL4_Error simple_copy_caps(simple_t *simple, seL4_CNode cspace, int copy_untypeds)
{
    int i;
    seL4_Error error = seL4_NoError;

    for (i = 0; i < simple_get_cap_count(simple); i++) {
        seL4_CPtr pos = simple_get_nth_cap(simple, i);
        /* Don't copy this, put the cap to the new cspace */
        if (pos == seL4_CapInitThreadCNode) {
            continue;
        }

        /* If we don't want to copy untypeds and it is one move on */
        if (!copy_untypeds && simple_is_untyped_cap(simple, pos)) {
            continue;
        }

        error = seL4_CNode_Copy(
                    cspace, pos, seL4_WordBits,
                    seL4_CapInitThreadCNode, pos, seL4_WordBits,
                    seL4_AllRights);
        /* Don't error on the low cap numbers, we might have tried to copy a control cap */
        if (error && i > 10) {
            return error;
        }
    }

    return error;
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
