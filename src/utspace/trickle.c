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

static inline uint32_t _make_bitmap(int bits) {
    /* avoid shift by 32 in BIT/MASK code */
    if (bits == 6) {
        return -1;
    } else {
        /* shift the result up so that the high bits are set */
        return MASK(BIT(bits - 1)) << (32 - BIT(bits - 1));
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

static inline uint32_t _make_cookie(struct utspace_trickle_node *node, uint32_t offset)
{
    uint32_t int_node = (uint32_t) node;
    assert( (int_node & MASK(5)) == 0);
    assert(offset < BIT(5));
    return int_node | offset;
}

static inline struct utspace_trickle_node *_cookie_to_node(uint32_t cookie) {
    return (struct utspace_trickle_node*)(cookie & (uint32_t)(~MASK(5)));
}

static inline uint32_t _cookie_to_offset(uint32_t cookie)
{
    return cookie & MASK(5);
}

static inline struct utspace_trickle_node *_make_node(struct allocman *alloc, int *error) {
    compile_time_assert(trickle_struct_size, sizeof(struct utspace_trickle_node) == 36);
    uint32_t addr = (uint32_t)allocman_mspace_alloc(alloc, 68, error);
    uint32_t ret;
    struct utspace_trickle_node *node;
    if (*error) {
        return NULL;
    }
    ret = (addr + 32) & ((uint32_t)~MASK(5));
    node = (struct utspace_trickle_node*)ret;
    node->padding = addr;
    return node;
}

static inline void _free_node(struct allocman *alloc, struct utspace_trickle_node *node)
{
    allocman_mspace_free(alloc, (void*)node->padding, 68);
}

int _utspace_trickle_add_uts(allocman_t *alloc, void *_trickle, uint32_t num, const cspacepath_t *uts, uint32_t *size_bits, uint32_t *paddr) {
    utspace_trickle_t *trickle = (utspace_trickle_t*) _trickle;
    struct utspace_trickle_node *nodes[num];
    cspacepath_t *uts_copy[num];
    int error;
    int i;
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
    for (i = 0; i < 32; i++) {
        trickle->heads[i] = NULL;
    }
}

static int _refill_pool(struct allocman *alloc, utspace_trickle_t *trickle, uint32_t size_bits)
{
    uint32_t i;
    int error;
    struct utspace_trickle_node *node;
    uint32_t cookie;
    node = _make_node(alloc, &error);
    if (error) {
        return error;
    }
    /* Check if there are untypeds >= 5 size_bits from us */
    for (i = size_bits + 5 > 31 ? 31 : size_bits + 5; i < 32; i++) {
        if (trickle->heads[i]) {
            i = size_bits + 5;
            break;
        }
    }
    if (i == 32) {
        /* Search for the biggest one near us */
        for (i = size_bits + 5 > 31 ? 31 : size_bits + 5; i > size_bits; i--) {
            if (trickle->heads[i]) {
                break;
            }
        }
    }
    if (i != size_bits) {
        cookie = _utspace_trickle_alloc(alloc, trickle, i, seL4_UntypedObject, NULL, &error);
        if (!error) {
            struct utspace_trickle_node *parent = _cookie_to_node(cookie);
            uint32_t offset = _cookie_to_offset(cookie);
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

uint32_t _utspace_trickle_alloc(struct allocman *alloc, void *_trickle, uint32_t size_bits, seL4_Word type, const cspacepath_t *slot, int *error)
{
    uint32_t sel4_size_bits;
    int _error;
    utspace_trickle_t *trickle = (utspace_trickle_t*)_trickle;
    struct utspace_trickle_node *node;
    uint32_t offset;
    uint32_t mem_offset;
    /* get size of untyped call */
    sel4_size_bits = get_sel4_object_size(type, size_bits);
    if (size_bits != vka_get_object_size(type, sel4_size_bits) || size_bits == 0) {
        SET_ERROR(error, 1);
        return 0;
    }
    assert(size_bits < 32);
    if (!trickle->heads[size_bits]) {
        _error = _refill_pool(alloc, trickle, size_bits);
        if (_error) {
            SET_ERROR(error, _error);
            return 0;
        }
    }
    node = trickle->heads[size_bits];
    offset = CLZ(node->bitmap);
    mem_offset = node->offset + (offset << size_bits);
    if (slot) {
        _error = seL4_Untyped_RetypeAtOffset(node->ut->capPtr, type, mem_offset, sel4_size_bits, slot->root, slot->dest, slot->destDepth, slot->offset, 1);
        if (_error != seL4_NoError) {
            SET_ERROR(error, 1);
            return 0;
        }
    }
    node->bitmap &= ~BIT(31 - offset);
    if (node->bitmap == 0) {
        _remove_node(&trickle->heads[size_bits], node);
    }
    SET_ERROR(error, 0);
    return _make_cookie(node, offset);
}

void _utspace_trickle_free(struct allocman *alloc, void *_trickle, uint32_t cookie, uint32_t size_bits)
{
    utspace_trickle_t *trickle = (utspace_trickle_t*)_trickle;
    struct utspace_trickle_node *node = _cookie_to_node(cookie);
    uint32_t offset = _cookie_to_offset(cookie);
    int in_list = !(node->bitmap == 0);
    node->bitmap |= BIT(31 - offset);
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

uint32_t _utspace_trickle_paddr(void *_trickle, uint32_t cookie, uint32_t size_bits) {
    struct utspace_trickle_node *node = _cookie_to_node(cookie);
    uint32_t offset = _cookie_to_offset(cookie);
    return node->paddr + (offset << size_bits);
}

#endif
