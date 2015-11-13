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
#include <allocman/utspace/trickle.h>
#include <allocman/allocman.h>
#include <allocman/util.h>
#include <sel4/sel4.h>

#include <vka/object.h>

#ifdef CONFIG_KERNEL_STABLE

static inline size_t _make_bitmap(size_t bits) {
    /* avoid shift by int max in BIT/MASK code */
    if (BIT(bits) == sizeof(size_t)) {
        return -1;
    } else {
        /* shift the result up so that the high bits are set */
        return MASK(BIT(bits - 1)) << (CONFIG_WORD_SIZE - BIT(bits - 1));
    }
}

static inline void _insert_node(struct utspace_trickle_node **head, struct utspace_trickle_node *node)
{
    if (*head) {
        (*head)->prev = node;
    }
    node->next = *head;
    node->prev = NULL;
    *head = node;
}

static inline void _remove_node(struct utspace_trickle_node **head, struct utspace_trickle_node *node)
{
    if (node->next) {
        node->next->prev = node->prev;
    }
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        *head = node->next;
    }
}

static inline seL4_Word _make_cookie(struct utspace_trickle_node *node, size_t offset)
{
    size_t int_node = (size_t) node;
    assert( (int_node % (CONFIG_WORD_SIZE)) == 0);
    assert(offset < CONFIG_WORD_SIZE);
    return int_node | offset;
}

static inline struct utspace_trickle_node *_cookie_to_node(seL4_Word cookie) {
    cookie -= cookie % (CONFIG_WORD_SIZE);
    return (struct utspace_trickle_node*)cookie;
}

static inline size_t _cookie_to_offset(seL4_Word cookie)
{
    return cookie % (CONFIG_WORD_SIZE);
}

static inline struct utspace_trickle_node *_make_node(struct allocman *alloc, int *error) {
    uintptr_t addr = (uintptr_t)allocman_mspace_alloc(alloc, sizeof(utspace_trickle_node) * 2 - sizeof(size_t), error);
    struct utspace_trickle_node *node;
    if (*error) {
        return NULL;
    }
    node = _cookie_to_node(addr + CONFIG_WORD_SIZE);
    node->padding = addr;
    return node;
}

static inline void _free_node(struct allocman *alloc, struct utspace_trickle_node *node)
{
    allocman_mspace_free(alloc, (void*)node->padding, sizeof(utspace_trickle_node) * 2 - sizeof(size_t));
}

int _utspace_trickle_add_uts(allocman_t *alloc, void *_trickle, size_t num, const cspacepath_t *uts, size_t *size_bits, uintptr_t *paddr) {
    utspace_trickle_t *trickle = (utspace_trickle_t*) _trickle;
    struct utspace_trickle_node *nodes[num];
    cspacepath_t *uts_copy[num];
    int error;
    size_t i;
    for (i = 0; i < num; i++) {
        nodes[i] = _make_node(alloc, &error);
        if (error) {
            for (i--; i >= 0; i--) {
                _free_node(alloc, nodes[i]);
                allocman_mspace_free(alloc, uts_copy[i], sizeof(cspacepath_t));
            }
            return error;
        }
        uts_copy[i] = allocman_mspace_alloc(alloc, sizeof(cspacepath_t), &error);
        if (error) {
            _free_node(alloc, nodes[i]);
            for (i--; i >= 0; i--) {
                _free_node(alloc, nodes[i]);
                allocman_mspace_free(alloc, uts_copy[i], sizeof(cspacepath_t));
            }
        }
    }
    for (i = 0; i < num; i++) {
        *uts_copy[i] = uts[i];
        nodes[i]->ut = uts_copy[i];
        nodes[i]->offset = 0;
        nodes[i]->paddr = paddr[i];
        nodes[i]->parent_cookie = 0;
        nodes[i]->next = nodes[i]->prev = NULL;
        /* Start with only 1 thing free */
        nodes[i]->bitmap = BIT(31);
        nodes[i]->bitmap_bits = 1;
        _insert_node(&trickle->heads[size_bits[i]], nodes[i]);
    }
    return 0;
}

void utspace_trickle_create(utspace_trickle_t *trickle)
{
    uint32_t i;
    for (i = 0; i < CONFIG_WORD_SIZE; i++) {
        trickle->heads[i] = NULL;
    }
}

static int _refill_pool(struct allocman *alloc, utspace_trickle_t *trickle, size_t size_bits)
{
    size_t i;
    int error;
    struct utspace_trickle_node *node;
    seL4_Word cookie;
    node = _make_node(alloc, &error);
    if (error) {
        return error;
    }
    /* Check if there are untypeds >= 5 size_bits from us */
    for (i = size_bits + 5 > CONFIG_WORD_SIZE - 1 ? CONFIG_WORD_SIZE - 1 : size_bits + 5; i < CONFIG_WORD_SIZE; i++) {
        if (trickle->heads[i]) {
            i = size_bits + 5;
            break;
        }
    }
    if (i == CONGIG_WORD_SIZE) {
        /* Search for the biggest one near us */
        for (i = size_bits + 5 > CONFIG_WORD_SIZE - 1 ? CONFIG_WORD_SIZE : size_bits + 5; i > size_bits; i--) {
            if (trickle->heads[i]) {
                break;
            }
        }
    }
    if (i != size_bits) {
        cookie = _utspace_trickle_alloc(alloc, trickle, i, seL4_UntypedObject, NULL, &error);
        if (!error) {
            struct utspace_trickle_node *parent = _cookie_to_node(cookie);
            size_t offset = _cookie_to_offset(cookie);
            node->ut = parent->ut;
            node->offset = parent->offset + (offset << (i));
            if (parent->paddr) {
                node->paddr = parent->paddr + (offset << (i));
            } else {
                node->paddr = 0;
            }
            node->parent_cookie = cookie;
            node->bitmap_bits = i - size_bits + 1;
            node->bitmap = _make_bitmap(node->bitmap_bits);
            node->next = node->prev = NULL;
            _insert_node(&trickle->heads[size_bits], node);
            return 0;
        }
    }
    _free_node(alloc, node);
    return 1;
}

seL4_Word _utspace_trickle_alloc(struct allocman *alloc, void *_trickle, size_t size_bits, seL4_Word type, const cspacepath_t *slot, int *error)
{
    size_t sel4_size_bits;
    int _error;
    utspace_trickle_t *trickle = (utspace_trickle_t*)_trickle;
    struct utspace_trickle_node *node;
    size_t offset;
    size_t mem_offset;
    /* get size of untyped call */
    sel4_size_bits = get_sel4_object_size(type, size_bits);
    if (size_bits != vka_get_object_size(type, sel4_size_bits) || size_bits == 0) {
        SET_ERROR(error, 1);
        return 0;
    }
    assert(size_bits < CONFIG_WORD_SIZE);
    if (!trickle->heads[size_bits]) {
        _error = _refill_pool(alloc, trickle, size_bits);
        if (_error) {
            SET_ERROR(error, _error);
            return 0;
        }
    }
    node = trickle->heads[size_bits];
    offset = CLZL(node->bitmap);
    mem_offset = node->offset + (offset << size_bits);
    if (slot) {
        _error = seL4_Untyped_RetypeAtOffset(node->ut->capPtr, type, mem_offset, sel4_size_bits, slot->root, slot->dest, slot->destDepth, slot->offset, 1);
        if (_error != seL4_NoError) {
            SET_ERROR(error, 1);
            return 0;
        }
    }
    node->bitmap &= ~BIT(CONFIG_WORD_SIZE - 1 - offset);
    if (node->bitmap == 0) {
        _remove_node(&trickle->heads[size_bits], node);
    }
    SET_ERROR(error, 0);
    return _make_cookie(node, offset);
}

void _utspace_trickle_free(struct allocman *alloc, void *_trickle, seL4_Word cookie, size_t size_bits)
{
    utspace_trickle_t *trickle = (utspace_trickle_t*)_trickle;
    struct utspace_trickle_node *node = _cookie_to_node(cookie);
    size_t offset = _cookie_to_offset(cookie);
    int in_list = !(node->bitmap == 0);
    node->bitmap |= BIT(CONFIG_WORD_SIZE - 1 - offset);
    if (node->bitmap == _make_bitmap(node->bitmap_bits)) {
        if (node->parent_cookie) {
            if (in_list) {
                _remove_node(&trickle->heads[size_bits], node);
            }
            _utspace_trickle_free(alloc, trickle, node->parent_cookie, size_bits + node->bitmap_bits - 1);
            _free_node(alloc, node);
        } else if (!in_list) {
            _insert_node(&trickle->heads[size_bits], node);
        }
    } else if (!in_list) {
        _insert_node(&trickle->heads[size_bits], node);
    }
}

uintptr_t _utspace_trickle_paddr(void *_trickle, seL4_Word cookie, size_t size_bits) {
    struct utspace_trickle_node *node = _cookie_to_node(cookie);
    size_t offset = _cookie_to_offset(cookie);
    return node->paddr + (offset << size_bits);
}

#endif
