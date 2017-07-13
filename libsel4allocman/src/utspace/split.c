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
        assert(*head == node);
        *head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
    node->head = head;
}

static void _insert_node(struct utspace_split_node **head, struct utspace_split_node *node) {
    node->next = *head;
    node->prev = NULL;
    if (*head) {
        (*head)->prev = node;
    }
    *head = node;
    /* mark node as not allocated */
    node->head = NULL;
}

static struct utspace_split_node *_new_node(allocman_t *alloc) {
    int error;
    struct utspace_split_node *node;
    node = (struct utspace_split_node*) allocman_mspace_alloc(alloc, sizeof(*node), &error);
    if (error) {
        ZF_LOGV("Failed to allocate node of size %zu", sizeof(*node));
        return NULL;
    }
    error = allocman_cspace_alloc(alloc, &node->ut);
    if (error) {
        allocman_mspace_free(alloc, node, sizeof(*node));
        ZF_LOGV("Failed to allocate slot");
        return NULL;
    }
    return node;
}

static void _delete_node(allocman_t *alloc, struct utspace_split_node *node) {
    vka_cnode_delete(&node->ut);
    allocman_cspace_free(alloc, &node->ut);
    allocman_mspace_free(alloc, node, sizeof(*node));
}

static int _insert_new_node(allocman_t *alloc, struct utspace_split_node **head, cspacepath_t ut, uintptr_t paddr) {
    int error;
    struct utspace_split_node *node;
    node = (struct utspace_split_node*) allocman_mspace_alloc(alloc, sizeof(*node), &error);
    if (error) {
        ZF_LOGV("Failed to allocate node of size %zu", sizeof(*node));
        return 1;
    }
    node->parent = NULL;
    node->ut = ut;
    node->paddr = paddr;
    node->origin_head = head;
    _insert_node(head, node);
    return 0;
}

void utspace_split_create(utspace_split_t *split)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(split->heads); i++) {
        split->heads[i] = NULL;
        split->dev_heads[i] = NULL;
        split->dev_mem_heads[i] = NULL;
    }
}

int _utspace_split_add_uts(allocman_t *alloc, void *_split, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr, int utType) {
    utspace_split_t *split = (utspace_split_t*) _split;
    int error;
    size_t i;
    struct utspace_split_node **list;
    switch (utType) {
        case ALLOCMAN_UT_KERNEL:
            list = split->heads;
            break;
        case ALLOCMAN_UT_DEV:
            list = split->dev_heads;
            break;
        case ALLOCMAN_UT_DEV_MEM:
            list = split->dev_mem_heads;
            break;
        default:
            return -1;
    }
    for (i = 0; i < num; i++) {
        error = _insert_new_node(alloc, &list[size_bits[i]], uts[i], paddr ? paddr[i] : ALLOCMAN_NO_PADDR);
        if (error) {
            return error;
        }
    }
    return 0;
}

static int _refill_pool(allocman_t *alloc, utspace_split_t *split, struct utspace_split_node **heads, size_t size_bits, uintptr_t paddr) {
    struct utspace_split_node *node;
    struct utspace_split_node *left, *right;
    int sel4_error;
    if (paddr == ALLOCMAN_NO_PADDR) {
        /* see if pool is actually empty */
        if (heads[size_bits]) {
            return 0;
        }
    } else {
        /* see if the pool has the paddr we want */
        for (node = heads[size_bits]; node; node = node->next) {
            if (node->paddr <= paddr && paddr < node->paddr + BIT(size_bits)) {
                return 0;
            }
        }
    }
    /* ensure we are not the highest pool */
    if (size_bits >= sizeof(seL4_Word) * 8 - 2) {
        /* bugger, no untypeds bigger than us */
        ZF_LOGV("Failed to refill pool of size %zu, no larger pools", size_bits);
        return 1;
    }
    /* get something from the highest pool */
    if (_refill_pool(alloc, split, heads, size_bits + 1, paddr)) {
        /* could not fill higher pool */
        ZF_LOGV("Failed to refill pool of size %zu", size_bits);
        return 1;
    }
    if (paddr == ALLOCMAN_NO_PADDR) {
        /* use the first node for lack of a better one */
        node = heads[size_bits + 1];
    } else {
        for (node = heads[size_bits + 1]; node && !(node->paddr <= paddr && paddr < node->paddr + BIT(size_bits + 1)); node = node->next);
        /* _refill_pool should not have returned if this wasn't possible */
        assert(node);
    }
    /* allocate two new nodes */
    left = _new_node(alloc);
    if (!left) {
        ZF_LOGV("Failed to allocate left node");
        return 1;
    }
    right = _new_node(alloc);
    if (!right) {
        ZF_LOGV("Failed to allocate right node");
        _delete_node(alloc, left);
        return 1;
    }
    /* perform the first retype */
    sel4_error = seL4_Untyped_Retype(node->ut.capPtr, seL4_UntypedObject, size_bits, left->ut.root, left->ut.dest, left->ut.destDepth, left->ut.offset, 1);
    if (sel4_error != seL4_NoError) {
        _delete_node(alloc, left);
        _delete_node(alloc, right);
        /* Well this shouldn't happen */
        ZF_LOGE("Failed to retype untyped, error %d\n", sel4_error);
        return 1;
    }
    /* perform the second retype */
    sel4_error = seL4_Untyped_Retype(node->ut.capPtr, seL4_UntypedObject, size_bits, right->ut.root, right->ut.dest, right->ut.destDepth, right->ut.offset, 1);
    if (sel4_error != seL4_NoError) {
        vka_cnode_delete(&left->ut);
        _delete_node(alloc, left);
        _delete_node(alloc, right);
        /* Well this shouldn't happen */
        ZF_LOGE("Failed to retype untyped, error %d\n", sel4_error);
        return 1;
    }
    /* all is done. remove the parent and insert the children */
    _remove_node(&heads[size_bits + 1], node);
    left->parent = right->parent = node;
    left->sibling = right;
    left->origin_head = &heads[size_bits];
    right->origin_head = &heads[size_bits];
    right->sibling = left;
    if (node->paddr != ALLOCMAN_NO_PADDR) {
        left->paddr = node->paddr;
        right->paddr = node->paddr + BIT(size_bits);
    } else {
        left->paddr = right->paddr = ALLOCMAN_NO_PADDR;
    }
    /* insert in this order so that we end up pulling the untypeds off in order of contiugous
     * physical address. This makes various allocation problems slightly less likely to happen */
    _insert_node(&heads[size_bits], right);
    _insert_node(&heads[size_bits], left);
    return 0;
}

static struct utspace_split_node **find_head_for_paddr(struct utspace_split_node **head, uintptr_t paddr, size_t size_bits) {
    int i;
    for (i = 0; i < CONFIG_WORD_SIZE; i++) {
        struct utspace_split_node *node;
        for (node = head[i]; node; node = node->next) {
            if (node->paddr <= paddr && paddr + BIT(size_bits) <= node->paddr + BIT(i)) {
                return head;
            }
        }
    }return NULL;
}

seL4_Word _utspace_split_alloc(allocman_t *alloc, void *_split, size_t size_bits, seL4_Word type, const cspacepath_t *slot, uintptr_t paddr, bool canBeDev, int *error)
{
    utspace_split_t *split = (utspace_split_t*)_split;
    size_t sel4_size_bits;
    int sel4_error;
    struct utspace_split_node *node;
    /* get size of untyped call */
    sel4_size_bits = get_sel4_object_size(type, size_bits);
    if (size_bits != vka_get_object_size(type, sel4_size_bits) || size_bits == 0) {
        SET_ERROR(error, 1);
        return 0;
    }
    struct utspace_split_node **head = NULL;
    /* if we're allocating at a particular paddr then we will just trawl through every pool
     * and see if we can find out which one has what we want */
    if (paddr != ALLOCMAN_NO_PADDR) {
        if (canBeDev) {
            head = find_head_for_paddr(split->dev_heads, paddr, size_bits);
            if (!head) {
                head = find_head_for_paddr(split->dev_mem_heads, paddr, size_bits);
            }
        }
        if (!head) {
            head = find_head_for_paddr(split->heads, paddr, size_bits);
        }
        if (!head) {
            SET_ERROR(error, 1);
            ZF_LOGE("Failed to find any untyped capable of creating an object at address %p", (void*)paddr);
            return 0;
        }
        if (_refill_pool(alloc, split, head, size_bits, paddr)) {
            /* out of memory? */
            SET_ERROR(error, 1);
            ZF_LOGV("Failed to refill pool to allocate object of size %zu", size_bits);
            return 0;
        }
        /* search for the node we want to use. We have the advantage of knowing that
         * due to objects being size aligned that the base paddr of the untyped will
         * be exactly the paddr we want */
        for (node = head[size_bits]; node && node->paddr != paddr; node = node->next);
        /* _refill_pool should not have returned if this wasn't possible */
        assert(node);
    } else {
        /* if we can use device memory then preference allocating from there */
        if (canBeDev) {
            if (_refill_pool(alloc, split, split->dev_mem_heads, size_bits, ALLOCMAN_NO_PADDR)) {
                /* out of memory? Try fall through */
                ZF_LOGV("Failed to refill device memory pool to allocate object of size %zu", size_bits);
                ZF_LOGV("Trying regular untyped pool");
            } else {
                head = split->dev_mem_heads;
            }

        }
        if (!head) {
            head = split->heads;
            if (_refill_pool(alloc, split, head, size_bits, ALLOCMAN_NO_PADDR)) {
                /* out of memory? */
                SET_ERROR(error, 1);
                ZF_LOGV("Failed to refill pool to allocate object of size %zu", size_bits);
                return 0;
            }
        }
        /* use the first node for lack of a better one */
        node = head[size_bits];
    }
    /* Perform the untyped retype */
    sel4_error = seL4_Untyped_Retype(node->ut.capPtr, type, sel4_size_bits, slot->root, slot->dest, slot->destDepth, slot->offset, 1);
    if (sel4_error != seL4_NoError) {
        /* Well this shouldn't happen */
        ZF_LOGE("Failed to retype untyped, error %d\n", sel4_error);
        SET_ERROR(error, 1);
        return 0;
    }
    /* remove the node */
    _remove_node(&head[size_bits], node);
    SET_ERROR(error, 0);
    /* return the node as a cookie */
    return (seL4_Word)node;
}

void _utspace_split_free(allocman_t *alloc, void *_split, seL4_Word cookie, size_t size_bits)
{
    utspace_split_t *split = (utspace_split_t*)_split;
    struct utspace_split_node *node = (struct utspace_split_node*)cookie;
    struct utspace_split_node *parent = node->parent;
    /* see if our sibling is also free */
    if (parent && !node->sibling->head) {
        /* remove sibling from free list */
        _remove_node(node->sibling->origin_head, node->sibling);
        /* delete both of us */
        _delete_node(alloc, node->sibling);
        _delete_node(alloc, node);
        /* put the parent back in */
        _utspace_split_free(alloc, split, (seL4_Word) parent, size_bits + 1);
    } else {
        /* just put ourselves back in */
        _insert_node(node->head, node);
    }
}

uintptr_t _utspace_split_paddr(void *_split, seL4_Word cookie, size_t size_bits)
{
    struct utspace_split_node *node = (struct utspace_split_node*)cookie;
    return node->paddr;
}
