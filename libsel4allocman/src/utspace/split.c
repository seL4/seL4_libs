/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>
#include <allocman/utspace/split.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <string.h>

static void _remove_node(struct utspace_split_node **head, struct utspace_split_node *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        *head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
    /* mark node as allocated */
    node->allocated = 1;
}

static void _insert_node(struct utspace_split_node **head, struct utspace_split_node *node) {
    node->next = *head;
    node->prev = NULL;
    if (*head) {
        (*head)->prev = node;
    }
    *head = node;
    /* mark node as not allocated */
    node->allocated = 0;
}

static struct utspace_split_node *_new_node(allocman_t *alloc) {
    int error;
    struct utspace_split_node *node;
    node = (struct utspace_split_node*) allocman_mspace_alloc(alloc, sizeof(*node), &error);
    if (error) {
        return NULL;
    }
    error = allocman_cspace_alloc(alloc, &node->ut);
    if (error) {
        allocman_mspace_free(alloc, node, sizeof(*node));
        return NULL;
    }
    return node;
}

static void _delete_node(allocman_t *alloc, struct utspace_split_node *node) {
    vka_cnode_delete(&node->ut);
    allocman_cspace_free(alloc, &node->ut);
    allocman_mspace_free(alloc, node, sizeof(*node));
}

static int _insert_new_node(allocman_t *alloc, struct utspace_split_node **head, cspacepath_t ut, uint32_t paddr) {
    int error;
    struct utspace_split_node *node;
    node = (struct utspace_split_node*) allocman_mspace_alloc(alloc, sizeof(*node), &error);
    if (error) {
        return 1;
    }
    node->parent = NULL;
    node->ut = ut;
    node->paddr = paddr;
    _insert_node(head, node);
    return 0;
}

void utspace_split_create(utspace_split_t *split)
{
    int i;
    for (i = 0; i < 32; i++) {
        split->heads[i] = NULL;
    }
}

int _utspace_split_add_uts(allocman_t *alloc, void *_split, uint32_t num, const cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr) {
    utspace_split_t *split = (utspace_split_t*) _split;
    int error;
    uint32_t i;
    for (i = 0; i < num; i++) {
        error = _insert_new_node(alloc, &split->heads[size_bits[i]], uts[i], paddr ? paddr[i] : 0);
        if (error) {
            return error;
        }
    }
    return 0;
}

static int _refill_pool(allocman_t *alloc, utspace_split_t *split, uint32_t size_bits) {
    struct utspace_split_node *node;
    struct utspace_split_node *left, *right;
    int sel4_error;
    /* see if pool is actually empty */
    if (split->heads[size_bits]) {
        return 0;
    }
    /* ensure we are not the highest pool */
    if (size_bits >= 30) {
        /* bugger, no untypeds bigger than us */
        return 1;
    }
    /* get something from the highest pool */
    if (_refill_pool(alloc, split, size_bits + 1)) {
        /* could not fill higher pool */
        return 1;
    }
    /* use the first node for lack of a better one */
    node = split->heads[size_bits + 1];
    /* allocate two new nodes */
    left = _new_node(alloc);
    if (!left) {
        return 1;
    }
    right = _new_node(alloc);
    if (!right) {
        _delete_node(alloc, left);
        return 1;
    }
    /* perform the first retype */
#if defined(CONFIG_KERNEL_STABLE)
    sel4_error = seL4_Untyped_RetypeAtOffset(node->ut.capPtr, seL4_UntypedObject, 0, size_bits, left->ut.root, left->ut.dest, left->ut.destDepth, left->ut.offset, 1);
#else
    sel4_error = seL4_Untyped_Retype(node->ut.capPtr, seL4_UntypedObject, size_bits, left->ut.root, left->ut.dest, left->ut.destDepth, left->ut.offset, 1);
#endif
    if (sel4_error != seL4_NoError) {
        _delete_node(alloc, left);
        _delete_node(alloc, right);
        /* Well this shouldn't happen */
        return 1;
    }
    /* perform the second retype */
#if defined(CONFIG_KERNEL_STABLE)
    sel4_error = seL4_Untyped_RetypeAtOffset(node->ut.capPtr, seL4_UntypedObject, BIT(size_bits), size_bits, right->ut.root, right->ut.dest, right->ut.destDepth, right->ut.offset, 1);
#else
    sel4_error = seL4_Untyped_Retype(node->ut.capPtr, seL4_UntypedObject, size_bits, right->ut.root, right->ut.dest, right->ut.destDepth, right->ut.offset, 1);
#endif
    if (sel4_error != seL4_NoError) {
        vka_cnode_delete(&left->ut);
        _delete_node(alloc, left);
        _delete_node(alloc, right);
        /* Well this shouldn't happen */
        return 1;
    }
    /* all is done. remove the parent and insert the children */
    _remove_node(&split->heads[size_bits + 1], node);
    left->parent = right->parent = node;
    left->sibling = right;
    right->sibling = left;
    if (node->paddr) {
        left->paddr = node->paddr;
        right->paddr = node->paddr + BIT(size_bits);
    } else {
        left->paddr = right->paddr = 0;
    }
    /* insert in this order so that we end up pulling the untypeds off in order of contiugous
     * physical address. This makes various allocation problems slightly less likely to happen */
    _insert_node(&split->heads[size_bits], right);
    _insert_node(&split->heads[size_bits], left);
    return 0;
}

uint32_t _utspace_split_alloc(allocman_t *alloc, void *_split, uint32_t size_bits, seL4_Word type, const cspacepath_t *slot, int *error)
{
    utspace_split_t *split = (utspace_split_t*)_split;
    uint32_t sel4_size_bits;
    int sel4_error;
    struct utspace_split_node *node;
    /* get size of untyped call */
    sel4_size_bits = get_sel4_object_size(type, size_bits);
    if (size_bits != vka_get_object_size(type, sel4_size_bits) || size_bits == 0) {
        SET_ERROR(error, 1);
        return 0;
    }
    /* make sure we have an available untyped */
    if (_refill_pool(alloc, split, size_bits)) {
        /* out of memory? */
        SET_ERROR(error, 1);
        return 0;
    }
    /* use the first node for lack of a better one */
    node = split->heads[size_bits];
    /* Perform the untyped retype */
#if defined(CONFIG_KERNEL_STABLE)
    sel4_error = seL4_Untyped_RetypeAtOffset(node->ut.capPtr, type, 0, sel4_size_bits, slot->root, slot->dest, slot->destDepth, slot->offset, 1);
#else
    sel4_error = seL4_Untyped_Retype(node->ut.capPtr, type, sel4_size_bits, slot->root, slot->dest, slot->destDepth, slot->offset, 1);
#endif
    if (sel4_error != seL4_NoError) {
        /* Well this shouldn't happen */
        SET_ERROR(error, 1);
        return 0;
    }
    /* remove the node */
    _remove_node(&split->heads[size_bits], node);
    SET_ERROR(error, 0);
    /* return the node as a cookie */
    return (uint32_t)node;
}

void _utspace_split_free(allocman_t *alloc, void *_split, uint32_t cookie, uint32_t size_bits)
{
    utspace_split_t *split = (utspace_split_t*)_split;
    struct utspace_split_node *node = (struct utspace_split_node*)cookie;
    struct utspace_split_node *parent = node->parent;
    /* see if our sibling is also free */
    if (parent && !node->sibling->allocated) {
        /* remove sibling from free list */
        _remove_node(&split->heads[size_bits], node->sibling);
        /* delete both of us */
        _delete_node(alloc, node->sibling);
        _delete_node(alloc, node);
        /* put the parent back in */
        _utspace_split_free(alloc, split, (uint32_t) parent, size_bits + 1);
    } else {
        /* just put ourselves back in */
        _insert_node(&split->heads[size_bits], node);
    }
}

uint32_t _utspace_split_paddr(void *_split, uint32_t cookie, uint32_t size_bits)
{
    struct utspace_split_node *node = (struct utspace_split_node*)cookie;
    return node->paddr;
}

