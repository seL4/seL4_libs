/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>

#include <sel4/sel4.h>
#include <sel4platsupport/irq.h>
#include <sel4platsupport/device.h>
#include <platsupport/irq.h>
#include <platsupport/io.h>
#include <simple/simple.h>
#include <utils/util.h>
#include <vka/vka.h>
#include <vka/capops.h>

#define UNPAIRED_ID -1
#define UNALLOCATED_BADGE_INDEX -1

typedef enum irq_iface_type {
    STANDARD_IFACE,
    MINI_IFACE
} irq_iface_type_t;

typedef struct {
    /* These are always non-empty if this particular IRQ ID is in use */
    bool allocated;
    ps_irq_t irq;
    cspacepath_t handler_path;

    /* These are not always non-empty if this particular IRQ ID is in use */

    /* Driver registers these */
    irq_callback_fn_t irq_callback_fn;
    void *callback_data;

    /* User registers these */
    cspacepath_t ntfn_path;
    ntfn_id_t paired_ntfn;
    int8_t allocated_badge_index;
} irq_entry_t;

typedef struct {
    bool allocated;
    cspacepath_t root_ntfn_path;
    size_t num_irqs_bound;

    seL4_Word usable_mask;
    /* Bitfield tracking which of the bits in the badge are allocated */
    seL4_Word status_bitfield;
    /* Bitfield tracking which IRQs have arrived but not served */
    seL4_Word pending_bitfield;

    irq_id_t bound_irqs[MAX_INTERRUPTS_TO_NOTIFICATIONS];
} ntfn_entry_t;

typedef struct {
    irq_iface_type_t iface_type;
    size_t num_registered_irqs;
    size_t num_allocated_ntfns;

    size_t max_irq_ids;
    size_t max_ntfn_ids;
    size_t num_irq_bitfields;
    size_t num_ntfn_bitfields;
    /* Array of bitfields tracking which IDs have been allocated */
    seL4_Word *allocated_irq_bitfields;
    seL4_Word *allocated_ntfn_bitfields;

    irq_entry_t *irq_table;
    ntfn_entry_t *ntfn_table;
    simple_t *simple;
    vka_t *vka;
    ps_malloc_ops_t *malloc_ops;
} irq_cookie_t;

typedef struct {
    irq_cookie_t *irq_cookie;
    irq_id_t irq_id;
} ack_data_t;

static inline bool check_irq_id_is_valid(irq_cookie_t *irq_cookie, irq_id_t id)
{
    if (unlikely(id < 0 || id >= irq_cookie->max_irq_ids)) {
        return false;
    }
    return true;
}

static inline bool check_ntfn_id_is_valid(irq_cookie_t *irq_cookie, ntfn_id_t id)
{
    if (unlikely(id < 0 || id >= irq_cookie->max_ntfn_ids)) {
        return false;
    }
    return true;
}

static inline bool check_irq_id_all_allocated(irq_cookie_t *irq_cookie)
{
    if (unlikely(irq_cookie->num_registered_irqs >= irq_cookie->max_irq_ids)) {
        return true;
    }
    return false;
}

static inline bool check_ntfn_id_all_allocated(irq_cookie_t *irq_cookie)
{
    if (unlikely(irq_cookie->num_allocated_ntfns >= irq_cookie->max_ntfn_ids)) {
        return true;
    }
    return false;
}

static inline bool check_ntfn_id_is_allocated(irq_cookie_t *irq_cookie, ntfn_id_t id)
{
    if (likely(irq_cookie->ntfn_table[id].allocated)) {
        return true;
    }
    return false;
}

static inline bool check_irq_id_is_allocated(irq_cookie_t *irq_cookie, irq_id_t id)
{
    if (likely(irq_cookie->irq_table[id].allocated)) {
        return true;
    }
    return false;
}

static inline void fill_bit_in_bitfield(seL4_Word *bitfield_array, int index)
{
    int bitfield_index = index % (sizeof(seL4_Word) * CHAR_BIT);
    int array_index  = index / (sizeof(seL4_Word) * CHAR_BIT);
    bitfield_array[array_index] |= BIT(bitfield_index);
}

static inline void unfill_bit_in_bitfield(seL4_Word *bitfield_array, int index)
{
    int bitfield_index = index % (sizeof(seL4_Word) * CHAR_BIT);
    int array_index  = index / (sizeof(seL4_Word) * CHAR_BIT);
    bitfield_array[array_index] &= ~(BIT(bitfield_index));
}

static irq_id_t find_free_irq_id(irq_cookie_t *irq_cookie)
{
    for (int i = 0; i < irq_cookie->num_irq_bitfields; i++) {
        unsigned long unallocated_bitfield = ~(irq_cookie->allocated_irq_bitfields[i]);
        if (i == irq_cookie->num_irq_bitfields - 1) {
            /* Hide the bits that cannot be allocated, i.e. max_irq_ids is
             * not a multiple of sizeof(seL4_Word) */
            unsigned long num_leftover_bits = irq_cookie->num_irq_bitfields * sizeof(seL4_Word) -
                                              irq_cookie->max_irq_ids;
            unsigned long mask = MASK(num_leftover_bits);
            /* Reverse the mask, (bit 0 is the smallest ID of that bitfield) */
            mask = BSWAP_WORD(mask);

            unallocated_bitfield &= mask;
        }
        /* Check to avoid undefined behaviour of CTZL(0) */
        if (likely(unallocated_bitfield)) {
            return CTZL(unallocated_bitfield) + (i * sizeof(seL4_Word) * CHAR_BIT);
        }
    }
    return -1;
}

static ntfn_id_t find_free_ntfn_id(irq_cookie_t *irq_cookie)
{
    for (int i = 0; i < irq_cookie->num_ntfn_bitfields; i++) {
        unsigned long unallocated_bitfield = ~(irq_cookie->allocated_ntfn_bitfields[i]);
        if (i == irq_cookie->num_irq_bitfields - 1) {
            /* Hide the bits that cannot be allocated, i.e. max_irq_ids is
             * not a multiple of sizeof(seL4_Word) */
            unsigned long num_leftover_bits = irq_cookie->num_ntfn_bitfields * sizeof(seL4_Word) -
                                              irq_cookie->max_ntfn_ids;
            unsigned long mask = MASK(num_leftover_bits);
            /* Reverse the mask, (bit 0 is the smallest ID of that bitfield) */
            mask = BSWAP_WORD(mask);

            unallocated_bitfield &= mask;
        }
        /* Check to avoid undefined behaviour of CTZL(0) */
        if (likely(unallocated_bitfield)) {
            return CTZL(unallocated_bitfield) + (i * sizeof(seL4_Word) * CHAR_BIT);
        }
    }
    return -1;
}

static int find_free_ntfn_badge_index(ntfn_entry_t *ntfn_entry)
{
    unsigned long unallocated_bitfield = ~(ntfn_entry->status_bitfield);
    /* Mask the bits that we can use */
    unallocated_bitfield &= ntfn_entry->usable_mask;
    /* Check to avoid undefined behaviour of CTZL(0) */
    if (unlikely(!unallocated_bitfield)) {
        return -1;
    }
    return CTZL(unallocated_bitfield);
}

static int irq_set_ntfn_common(irq_cookie_t *irq_cookie, ntfn_id_t ntfn_id, irq_id_t irq_id,
                               seL4_Word *ret_badge)
{
    irq_entry_t *irq_entry = &(irq_cookie->irq_table[irq_id]);

    /* Check if the IRQ is already bound to something */
    if (irq_entry->paired_ntfn > UNPAIRED_ID) {
        return -EINVAL;
    }

    ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[ntfn_id]);

    /* Check if we have space to bound another IRQ to this notification */
    if (ntfn_entry->num_irqs_bound >= MAX_INTERRUPTS_TO_NOTIFICATIONS) {
        return -ENOSPC;
    }

    /* Find an empty space (unclaimed bit in the badge) for the IRQ */
    int badge_index = find_free_ntfn_badge_index(ntfn_entry);
    if (badge_index == -1) {
        return -ENOSPC;
    }

    /* Allocate a CSpace slot and mint the root notification object with badge */
    cspacepath_t mint_path = {0};
    int error = vka_cspace_alloc_path(irq_cookie->vka, &mint_path);
    if (error) {
        return -ENOMEM;
    }

    seL4_Word badge = BIT(badge_index);
    error = vka_cnode_mint(&mint_path, &(ntfn_entry->root_ntfn_path), seL4_AllRights, badge);
    if (error) {
        vka_cspace_free_path(irq_cookie->vka, mint_path);
        return -ENOMEM;
    }

    /* Bind the notification with the handler now */
    error = seL4_IRQHandler_SetNotification(irq_entry->handler_path.capPtr, mint_path.capPtr);
    if (error) {
        ZF_LOGE("Failed to set a notification with the IRQ handler");
        error = vka_cnode_delete(&mint_path);
        ZF_LOGF_IF(error, "Failed to cleanup after a failed IRQ set ntfn operation");
        vka_cspace_free_path(irq_cookie->vka, mint_path);
        return -EFAULT;
    }

    /* Acknowledge the handler so interrupts can arrive on the notification */
    error = seL4_IRQHandler_Ack(irq_entry->handler_path.capPtr);
    if (error) {
        ZF_LOGE("Failed to ack an IRQ handler");
        error = seL4_IRQHandler_Clear(irq_entry->handler_path.capPtr);
        ZF_LOGF_IF(error, "Failed to unpair a notification after a failed IRQ set ntfn operation");
        error = vka_cnode_delete(&mint_path);
        ZF_LOGF_IF(error, "Failed to cleanup after a failed IRQ set ntfn operation");
        vka_cspace_free_path(irq_cookie->vka, mint_path);
        return -EFAULT;
    }

    if (ret_badge) {
        *ret_badge = badge;
    }

    /* Fill in the bookkeeping information */
    ntfn_entry->num_irqs_bound++;
    ntfn_entry->status_bitfield |= badge;
    ntfn_entry->bound_irqs[badge_index] = irq_id;

    irq_entry->ntfn_path = mint_path;
    irq_entry->paired_ntfn = ntfn_id;
    irq_entry->allocated_badge_index = badge_index;

    return 0;
}

static irq_id_t irq_register_common(irq_cookie_t *irq_cookie, ps_irq_t irq, irq_callback_fn_t callback,
                                    void *callback_data)
{
    if (check_irq_id_all_allocated(irq_cookie)) {
        return -EMFILE;
    }

    if (!callback) {
        return -EINVAL;
    }

    irq_id_t free_id = find_free_irq_id(irq_cookie);
    if (free_id == -1) {
        /* Probably wouldn't get here, as we checked above already */
        ZF_LOGE("Failed to find a free IRQ id");
        return -EMFILE;
    }

    /* Allocate a path for the IRQ handler object */
    cspacepath_t irq_handler_path = {0};
    vka_cspace_alloc_path(irq_cookie->vka, &irq_handler_path);

    /* Create an IRQ handler object for this IRQ */
    int err = sel4platsupport_copy_irq_cap(irq_cookie->vka, irq_cookie->simple, &irq, &irq_handler_path);
    if (err) {
        /* Give a slightly ambigious message as we don't want to leak implementation details */
        ZF_LOGE("Failed to register an IRQ");
        vka_cspace_free_path(irq_cookie->vka, irq_handler_path);
        return -EFAULT;
    }

    irq_entry_t *irq_entry = &(irq_cookie->irq_table[free_id]);
    irq_entry->allocated = true;
    irq_entry->irq = irq;
    irq_entry->handler_path = irq_handler_path;
    irq_entry->irq_callback_fn = callback;
    irq_entry->callback_data = callback_data;

    irq_cookie->num_registered_irqs++;
    fill_bit_in_bitfield(irq_cookie->allocated_irq_bitfields, free_id);

    return free_id;
}

static void provide_ntfn_common(irq_cookie_t *irq_cookie, seL4_CPtr ntfn, seL4_Word usable_mask,
                                ntfn_id_t allocated_id)
{
    cspacepath_t ntfn_path = {0};
    vka_cspace_make_path(irq_cookie->vka, ntfn, &ntfn_path);

    /* Clear the notification entry and then fill in bookkeeping information */
    ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[allocated_id]);
    memset(ntfn_entry, 0, sizeof(ntfn_entry_t));
    ntfn_entry->allocated = true;
    ntfn_entry->root_ntfn_path = ntfn_path;
    ntfn_entry->usable_mask = usable_mask;

    irq_cookie->num_allocated_ntfns++;
    fill_bit_in_bitfield(irq_cookie->allocated_ntfn_bitfields, allocated_id);
}

static irq_cookie_t *new_irq_ops_common(vka_t *vka, simple_t *simple, irq_interface_config_t irq_config,
                                        ps_malloc_ops_t *malloc_ops, irq_iface_type_t iface_type)
{
    int err = 0;
    /* Figure out how many bitfields we need to keep track of the allocation status of the IDs */
    size_t bits_in_seL4_Word = sizeof(seL4_Word) * CHAR_BIT;
    size_t num_irq_bitfields = ALIGN_UP(irq_config.max_irq_ids, bits_in_seL4_Word) / sizeof(seL4_Word);
    size_t num_ntfn_bitfields = ALIGN_UP(irq_config.max_ntfn_ids, bits_in_seL4_Word) / sizeof(seL4_Word);

    irq_cookie_t *cookie = 0;
    err = ps_calloc(malloc_ops, 1, sizeof(irq_cookie_t), (void **) &cookie);
    if (err) {
        ZF_LOGE("Failed to allocate %zu bytes for cookie", sizeof(irq_cookie_t));
        goto error;
    }

    /* Allocate the IRQ bookkeeping array, and set default values for some of the members */
    err = ps_calloc(malloc_ops, 1, sizeof(irq_entry_t) * irq_config.max_irq_ids, (void **) & (cookie->irq_table));
    if (err) {
        ZF_LOGE("Failed to allocate IRQ bookkeeping array");
        goto error;
    }
    for (int i = 0; i < irq_config.max_irq_ids; i++) {
        cookie->irq_table[i].paired_ntfn = UNPAIRED_ID;
        cookie->irq_table[i].allocated_badge_index = UNALLOCATED_BADGE_INDEX;
    }

    /* Allocate the notification bookkeeping array, and set default values for some of the members */
    err = ps_calloc(malloc_ops, 1, sizeof(ntfn_entry_t) * irq_config.max_ntfn_ids,
                    (void **) & (cookie->ntfn_table));
    if (err) {
        ZF_LOGE("Failed to allocate notification bookkeeping array");
        goto error;
    }
    for (int i = 0; i < irq_config.max_ntfn_ids; i++) {
        memset(cookie->ntfn_table[i].bound_irqs, UNPAIRED_ID, sizeof(irq_id_t) * MAX_INTERRUPTS_TO_NOTIFICATIONS);
    }

    err = ps_calloc(malloc_ops, 1, num_irq_bitfields * sizeof(seL4_Word),
                    (void **) & (cookie->allocated_irq_bitfields));
    if (err) {
        ZF_LOGE("Failed to allocate the IRQ bitfields");
        goto error;
    }
    err = ps_calloc(malloc_ops, 1, num_ntfn_bitfields * sizeof(seL4_Word),
                    (void **) & (cookie->allocated_ntfn_bitfields));
    if (err) {
        ZF_LOGE("Failed to allocate the notification bitfields");
        goto error;
    }

    cookie->iface_type = iface_type;
    cookie->simple = simple;
    cookie->vka = vka;
    cookie->malloc_ops = malloc_ops;
    cookie->max_irq_ids = irq_config.max_irq_ids;
    cookie->max_ntfn_ids = irq_config.max_ntfn_ids;
    cookie->num_irq_bitfields = num_irq_bitfields;
    cookie->num_ntfn_bitfields = num_ntfn_bitfields;

    return cookie;

error:
    if (cookie) {
        if (cookie->irq_table) {
            ps_free(malloc_ops, sizeof(irq_entry_t) * irq_config.max_irq_ids, cookie->irq_table);
        }

        if (cookie->ntfn_table) {
            ps_free(malloc_ops, sizeof(ntfn_entry_t) * irq_config.max_ntfn_ids,
                    cookie->ntfn_table);
        }

        if (cookie->allocated_irq_bitfields) {
            ps_free(malloc_ops, sizeof(seL4_Word) * num_irq_bitfields, cookie->allocated_irq_bitfields);
        }

        ps_free(malloc_ops, sizeof(irq_cookie_t), cookie);
    }

    return NULL;
}

static int sel4platsupport_irq_unregister(void *cookie, irq_id_t irq_id)
{
    irq_cookie_t *irq_cookie = cookie;

    if (!check_irq_id_is_valid(irq_cookie, irq_id)) {
        return -EINVAL;
    }

    if (!check_irq_id_is_allocated(irq_cookie, irq_id)) {
        return -EINVAL;
    }

    irq_entry_t *irq_entry = &(irq_cookie->irq_table[irq_id]);

    if (irq_entry->paired_ntfn > UNPAIRED_ID) {
        /* Clear the handler */
        int error = seL4_IRQHandler_Clear(irq_entry->handler_path.capPtr);
        if (error) {
            /* Give a slightly ambigious message as we don't want to leak implementation details */
            ZF_LOGE("Failed to unregister an IRQ");
            return -EFAULT;
        }

        /* Delete the notification */
        vka_cnode_delete(&(irq_entry->ntfn_path));
        vka_cspace_free_path(irq_cookie->vka, irq_entry->ntfn_path);

        /* Clear the necessary information in the notification array */
        ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[irq_entry->paired_ntfn]);
        ntfn_entry->status_bitfield &= ~BIT(irq_entry->allocated_badge_index);
        ntfn_entry->pending_bitfield &= ~BIT(irq_entry->allocated_badge_index);
        ntfn_entry->bound_irqs[irq_entry->allocated_badge_index] = UNPAIRED_ID;
        ntfn_entry->num_irqs_bound--;
    }

    /* Delete the handler */
    vka_cnode_delete(&(irq_entry->handler_path));
    vka_cspace_free_path(irq_cookie->vka, irq_entry->handler_path);

    /* Zero-out the entire entry */
    memset(irq_entry, 0, sizeof(irq_entry_t));
    /* Reset parts of the entry */
    irq_entry->paired_ntfn = UNPAIRED_ID;
    irq_entry->allocated_badge_index = UNALLOCATED_BADGE_INDEX;

    irq_cookie->num_registered_irqs--;
    unfill_bit_in_bitfield(irq_cookie->allocated_irq_bitfields, irq_id);

    return 0;
}

/* The register function for the standard IRQ interface */
static irq_id_t sel4platsupport_irq_register(void *cookie, ps_irq_t irq, irq_callback_fn_t callback,
                                             void *callback_data)
{
    irq_cookie_t *irq_cookie = cookie;

    return irq_register_common(irq_cookie, irq, callback, callback_data);
}

/* The register function for the mini IRQ interface */
static irq_id_t sel4platsupport_irq_register_mini(void *cookie, ps_irq_t irq, irq_callback_fn_t callback,
                                                  void *callback_data)
{
    irq_cookie_t *irq_cookie = cookie;

    irq_id_t assigned_id = irq_register_common(irq_cookie, irq, callback, callback_data);
    if (assigned_id < 0) {
        /* Contains the error code if < 0 */
        return assigned_id;
    }

    /* Pair this IRQ with the only notification */
    int error = irq_set_ntfn_common(irq_cookie, MINI_IRQ_INTERFACE_NTFN_ID, assigned_id, NULL);
    if (error) {
        ZF_LOGF_IF(sel4platsupport_irq_unregister(irq_cookie, assigned_id),
                   "Failed to clean-up a failed operation");
        return error;
    }

    return assigned_id;
}

static int sel4platsupport_irq_acknowledge(void *ack_data)
{
    if (!ack_data) {
        return -EINVAL;
    }

    int ret = 0;

    ack_data_t *data = ack_data;
    irq_cookie_t *irq_cookie = data->irq_cookie;
    irq_id_t irq_id = data->irq_id;

    if (!check_irq_id_is_valid(irq_cookie, irq_id)) {
        ret = -EINVAL;
        goto exit;
    }

    if (!check_irq_id_is_allocated(irq_cookie, irq_id)) {
        ret = -EINVAL;
        goto exit;
    }

    irq_entry_t *irq_entry = &(irq_cookie->irq_table[irq_id]);
    int error = seL4_IRQHandler_Ack(irq_entry->handler_path.capPtr);
    if (error) {
        ZF_LOGE("Failed to acknowledge IRQ");
        ret = -EFAULT;
        goto exit;
    }

exit:
    ps_free(irq_cookie->malloc_ops, sizeof(ack_data_t), data);

    return ret;
}

int sel4platsupport_new_irq_ops(ps_irq_ops_t *irq_ops, vka_t *vka, simple_t *simple,
                                irq_interface_config_t irq_config, ps_malloc_ops_t *malloc_ops)
{
    if (!irq_ops || !vka || !simple || !malloc_ops) {
        return -EINVAL;
    }

    if (irq_config.max_irq_ids == 0 || irq_config.max_ntfn_ids == 0) {
        return -EINVAL;
    }

    irq_cookie_t *cookie = new_irq_ops_common(vka, simple, irq_config, malloc_ops, STANDARD_IFACE);
    if (!cookie) {
        return -ENOMEM;
    }

    /* Fill in the actual IRQ ops structure now */
    irq_ops->cookie = (void *) cookie;
    irq_ops->irq_register_fn = sel4platsupport_irq_register;
    irq_ops->irq_unregister_fn = sel4platsupport_irq_unregister;

    return 0;
}

int sel4platsupport_new_mini_irq_ops(ps_irq_ops_t *irq_ops, vka_t *vka, simple_t *simple,
                                     ps_malloc_ops_t *malloc_ops, seL4_CPtr ntfn, seL4_Word usable_mask)
{
    if (!irq_ops || !vka || !simple || !malloc_ops || !usable_mask) {
        return -EINVAL;
    }

    if (ntfn == seL4_CapNull) {
        return -EINVAL;
    }

    /* Get the number of bits that we can use in the badge,
     * this is the amount of interrupts that can be registered at a given time */
    size_t bits_in_mask = POPCOUNTL(usable_mask);

    /* max_ntfn_ids is kinda irrelevant in the mini interface, but set it anyway */
    irq_interface_config_t irq_config = { .max_irq_ids = bits_in_mask, .max_ntfn_ids = 1 };

    irq_cookie_t *cookie = new_irq_ops_common(vka, simple, irq_config, malloc_ops, MINI_IFACE);
    if (!cookie) {
        return -ENOMEM;
    }

    /* Fill in the actual IRQ ops structure now */
    irq_ops->cookie = (void *) cookie;
    irq_ops->irq_register_fn = sel4platsupport_irq_register_mini;
    irq_ops->irq_unregister_fn = sel4platsupport_irq_unregister;

    /* Provide the ntfn */
    provide_ntfn_common(cookie, ntfn, usable_mask, MINI_IRQ_INTERFACE_NTFN_ID);

    return 0;
}

ntfn_id_t sel4platsupport_irq_provide_ntfn(ps_irq_ops_t *irq_ops, seL4_CPtr ntfn, seL4_Word usable_mask)
{
    if (!irq_ops || ntfn == seL4_CapNull || !usable_mask) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (irq_cookie->iface_type == MINI_IFACE) {
        ZF_LOGE("Trying to use %s with the mini IRQ Interface", __func__);
        return -EPERM;
    }

    if (check_ntfn_id_all_allocated(irq_cookie)) {
        return -EMFILE;
    }

    ntfn_id_t free_id = find_free_ntfn_id(irq_cookie);
    if (free_id == -1) {
        return -EMFILE;
    }

    provide_ntfn_common(irq_cookie, ntfn, usable_mask, free_id);

    return free_id;
}

int sel4platsupport_irq_provide_ntfn_with_id(ps_irq_ops_t *irq_ops, seL4_CPtr ntfn,
                                             seL4_Word usable_mask, ntfn_id_t id_hint)
{
    if (!irq_ops || ntfn == seL4_CapNull || !usable_mask) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (irq_cookie->iface_type == MINI_IFACE) {
        ZF_LOGE("Trying to use %s with the mini IRQ Interface", __func__);
        return -EPERM;
    }

    if (check_ntfn_id_all_allocated(irq_cookie)) {
        return -EMFILE;
    }

    if (irq_cookie->ntfn_table[id_hint].allocated) {
        return -EADDRINUSE;
    }

    provide_ntfn_common(irq_cookie, ntfn, usable_mask, id_hint);

    return 0;
}

int sel4platsupport_irq_return_ntfn(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                                    seL4_CPtr *ret_cptr)
{
    if (!irq_ops) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (irq_cookie->iface_type == MINI_IFACE) {
        ZF_LOGE("Trying to use %s with the mini IRQ Interface", __func__);
        return -EPERM;
    }

    if (!check_ntfn_id_is_valid(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    if (!check_ntfn_id_is_allocated(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[ntfn_id]);

    if (ntfn_entry->num_irqs_bound > 0) {
        unsigned long allocated_bits = ntfn_entry->status_bitfield;
        while (allocated_bits) {
            unsigned long index = CTZL(allocated_bits);
            irq_entry_t *irq_entry = &(irq_cookie->irq_table[ntfn_entry->bound_irqs[index]]);
            seL4_IRQHandler_Clear(irq_entry->handler_path.capPtr);
            int error = vka_cnode_delete(&(irq_entry->ntfn_path));
            ZF_LOGF_IF(error, "Failed to delete a minted notification");
            irq_entry->ntfn_path = (cspacepath_t) {
                0
            };
            irq_entry->paired_ntfn = UNPAIRED_ID;
            irq_entry->allocated_badge_index = UNALLOCATED_BADGE_INDEX;

            allocated_bits &= ~BIT(index);
        }
    }

    if (ret_cptr) {
        *ret_cptr = ntfn_entry->root_ntfn_path.capPtr;
    }

    /* Zero out the entire entry */
    memset(ntfn_entry, 0, sizeof(ntfn_entry_t));
    /* Reset the bound_irqs array for the entry */
    memset(ntfn_entry->bound_irqs, UNPAIRED_ID, sizeof(irq_id_t) * MAX_INTERRUPTS_TO_NOTIFICATIONS);

    irq_cookie->num_allocated_ntfns--;
    unfill_bit_in_bitfield(irq_cookie->allocated_ntfn_bitfields, ntfn_id);

    return 0;
}

int sel4platsupport_irq_set_ntfn(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id, irq_id_t irq_id, seL4_Word *ret_badge)
{
    if (!irq_ops) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (irq_cookie->iface_type == MINI_IFACE) {
        ZF_LOGE("Trying to use %s with the mini IRQ Interface", __func__);
        return -EPERM;
    }

    if (!check_ntfn_id_is_valid(irq_cookie, ntfn_id) ||
        !check_ntfn_id_is_allocated(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    if (!check_irq_id_is_valid(irq_cookie, irq_id) ||
        !check_irq_id_is_allocated(irq_cookie, irq_id)) {
        return -EINVAL;
    }

    return irq_set_ntfn_common(irq_cookie, ntfn_id, irq_id, ret_badge);
}

int sel4platsupport_irq_unset_ntfn(ps_irq_ops_t *irq_ops, irq_id_t irq_id)
{
    if (!irq_ops) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (irq_cookie->iface_type == MINI_IFACE) {
        ZF_LOGE("Trying to use %s with the mini IRQ Interface", __func__);
        return -EPERM;
    }

    if (!check_irq_id_is_valid(irq_cookie, irq_id) ||
        !check_irq_id_is_allocated(irq_cookie, irq_id)) {
        return -EINVAL;
    }

    irq_entry_t *irq_entry = &(irq_cookie->irq_table[irq_id]);

    /* Check if the IRQ is bound to a notification */
    if (irq_entry->paired_ntfn == UNPAIRED_ID) {
        return -EINVAL;
    }

    /* Unbind the notification from the handler */
    int error = seL4_IRQHandler_Clear(irq_entry->handler_path.capPtr);
    if (error) {
        ZF_LOGE("Failed to unpair a notification");
        return -EFAULT;
    }

    /* Delete the minted notification and free the path */
    vka_cnode_delete(&(irq_entry->ntfn_path));
    vka_cspace_free_path(irq_cookie->vka, irq_entry->ntfn_path);

    /* Free the allocated badge index and update the bookkeeping in the notification array and the irq array */
    ntfn_id_t paired_id = irq_entry->paired_ntfn;
    ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[paired_id]);
    ntfn_entry->status_bitfield &= ~BIT(irq_entry->allocated_badge_index);
    ntfn_entry->pending_bitfield &= ~BIT(irq_entry->allocated_badge_index);
    ntfn_entry->num_irqs_bound--;
    ntfn_entry->bound_irqs[irq_entry->allocated_badge_index] = UNPAIRED_ID;

    irq_entry->ntfn_path = (cspacepath_t) {
        0
    };
    irq_entry->paired_ntfn = UNPAIRED_ID;
    irq_entry->allocated_badge_index = UNALLOCATED_BADGE_INDEX;

    return 0;
}

static bool perform_callback(irq_cookie_t *irq_cookie, irq_id_t irq_id, unsigned long badge_bit)
{
    irq_entry_t *irq_entry = &(irq_cookie->irq_table[irq_id]);
    irq_callback_fn_t callback = irq_entry->irq_callback_fn;

    /* Check if callback was registered, if so, then run it */
    if (callback) {
        ack_data_t *ack_data = NULL;
        int error = ps_calloc(irq_cookie->malloc_ops, 1, sizeof(ack_data_t), (void **) &ack_data);
        ZF_LOGF_IF(error, "Failed to allocate memory for a cookie for the acknowledge function");
        *ack_data = (ack_data_t) {
            .irq_cookie = irq_cookie, .irq_id = irq_id
        };
        callback(irq_entry->callback_data,
                 sel4platsupport_irq_acknowledge, ack_data);
        return true;
    }
    return false;
}

int sel4platsupport_irq_handle(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id, seL4_Word handle_mask)
{
    if (!irq_ops) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (!check_ntfn_id_is_valid(irq_cookie, ntfn_id) ||
        !check_ntfn_id_is_allocated(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[ntfn_id]);

    /* Just in case, but probably should throw an error at the user for passing in bits that
     * we dont' handle */
    unsigned long unchecked_bits = handle_mask & ntfn_entry->usable_mask;

    while (unchecked_bits) {
        unsigned long bit_index = CTZL(unchecked_bits);
        irq_id_t paired_irq_id = ntfn_entry->bound_irqs[bit_index];
        bool callback_called = perform_callback(irq_cookie, paired_irq_id, bit_index);
        if (callback_called && ntfn_entry->pending_bitfield & BIT(bit_index)) {
            /* Unset the bit, we've performed the callback for that interrupt */
            ntfn_entry->pending_bitfield &= ~BIT(bit_index);
        }
        unchecked_bits &= ~BIT(bit_index);
    }

    return 0;
}

static void serve_irq(irq_cookie_t *irq_cookie, ntfn_id_t id, seL4_Word mask,
                      seL4_Word badge, seL4_Word *ret_leftover_bits)
{
    seL4_Word served_mask = 0;

    ntfn_entry_t *ntfn_entry = &(irq_cookie->ntfn_table[id]);

    /* Mask out the bits the are not relevant to us */
    unsigned long unchecked_bits = badge & ntfn_entry->usable_mask;
    /* Also check the interrupts that were leftover and not served */
    unchecked_bits |= ntfn_entry->pending_bitfield;

    while (unchecked_bits) {
        unsigned long bit_index = CTZL(unchecked_bits);

        if (likely(BIT(bit_index) & mask)) {
            irq_id_t paired_irq_id = ntfn_entry->bound_irqs[bit_index];
            if (perform_callback(irq_cookie, paired_irq_id, bit_index)) {
                /* Record that this particular IRQ was served */
                served_mask |= BIT(bit_index);
            }
        }

        unchecked_bits &= ~BIT(bit_index);
    }

    /* Record the IRQs that were not served but arrived, note that we don't want to
     * override the leftover IRQs still inside the pending bitfield */
    ntfn_entry->pending_bitfield ^= badge & ntfn_entry->usable_mask & ~(served_mask);

    /* Write bits that are leftover */
    if (ret_leftover_bits) {
        *ret_leftover_bits = badge & ~(served_mask);
    }
}

int sel4platsupport_irq_wait(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                             seL4_Word wait_mask,
                             seL4_Word *ret_leftover_bits)
{
    if (!irq_ops) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (!check_ntfn_id_is_valid(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    if (!check_ntfn_id_is_allocated(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    seL4_CPtr ntfn = irq_cookie->ntfn_table[ntfn_id].root_ntfn_path.capPtr;

    seL4_Word badge = 0;

    /* Wait on the notification object */
    seL4_Wait(ntfn, &badge);

    serve_irq(irq_cookie, ntfn_id, wait_mask, badge, ret_leftover_bits);

    return 0;

}

int sel4platsupport_irq_poll(ps_irq_ops_t *irq_ops, ntfn_id_t ntfn_id,
                             seL4_Word poll_mask,
                             seL4_Word *ret_leftover_bits)
{
    if (!irq_ops) {
        return -EINVAL;
    }

    irq_cookie_t *irq_cookie = irq_ops->cookie;

    if (!check_ntfn_id_is_valid(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    if (!check_ntfn_id_is_allocated(irq_cookie, ntfn_id)) {
        return -EINVAL;
    }

    seL4_CPtr ntfn = irq_cookie->ntfn_table[ntfn_id].root_ntfn_path.capPtr;

    seL4_Word badge = 0;

    /* Poll the notification object */
    seL4_Poll(ntfn, &badge);

    serve_irq(irq_cookie, ntfn_id, poll_mask, badge, ret_leftover_bits);

    return 0;
}
