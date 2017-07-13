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

#include <assert.h>
#include <autoconf.h>
#include <sel4/sel4.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vka/cspacepath_t.h>
#include <vka/vka.h>

#ifndef CONFIG_LIB_SEL4_VKA_DEBUG_LIVE_SLOTS_SZ
#define CONFIG_LIB_SEL4_VKA_DEBUG_LIVE_SLOTS_SZ 0
#endif
#ifndef CONFIG_LIB_SEL4_VKA_DEBUG_LIVE_OBJS_SZ
#define CONFIG_LIB_SEL4_VKA_DEBUG_LIVE_SLOTS_SZ 0
#endif

/* Kconfig-set sizes for buffers to track live slots and objects. */
static size_t live_slots_sz = CONFIG_LIB_SEL4_VKA_DEBUG_LIVE_SLOTS_SZ;
static size_t live_objs_sz = CONFIG_LIB_SEL4_VKA_DEBUG_LIVE_OBJS_SZ;

typedef struct {

    /* The underlying allocator that we call to effect allocations. This is
     * effectively our debugging target if we're troubleshooting problems with
     * the allocator itself.
     */
    vka_t *underlying;

    /* Metadata related to currently live CSlots. The live_slots list contains
     * non-null entries representing currently live CSlots. Note that these may
     * be interspersed with null (dead) entries.
     */
    seL4_CPtr *live_slots;
    size_t live_slots_sz;

    /* Metadata related to currently live objects. The live_objs list contains
     * live entries with non-0 cookie members, potentially interspersed with
     * dead entries with 0 cookies. The other members are used for confirming
     * that a caller is freeing an object in the same way they allocated it.
     */
    struct obj {
        seL4_Word type;
        seL4_Word size_bits;
        seL4_Word cookie;
    } *live_objs;
    size_t live_objs_sz;

} state_t;

/* This is called when we encounter an allocator/caller error. */
#define fatal(args...) do { \
        fprintf(stderr, "VKA debug: " args); \
        fprintf(stderr, "\n"); \
        abort(); \
    } while (0)

/* This is called when we encounter a non-fatal problem. */
#define warn(args...) do { \
        fprintf(stderr, "VKA debug: " args); \
        fprintf(stderr, "\n"); \
    } while (0)

/* Track a slot that has just become live. */
static void track_slot(state_t *state, seL4_CPtr slot)
{
    assert(state != NULL);

    if (state->live_slots_sz == 0) {
        /* Disable tracking if we have no buffer. */
        return;
    }

    if (slot == 0) {
        fatal("allocator attempted to hand out the null slot");
    }

    /* Search the current list of live slots for this slot in case it's
     * currently live. Pretty slow to do this on every allocate, but we're
     * debugging so hey.
     */
    unsigned int available = 0;
    for (unsigned int i = 0; i < state->live_slots_sz; i++) {
        if (state->live_slots[i] == slot) {
            fatal("allocator attempted to hand out slot %lu that is currently "
                  "in use", (long)slot);
        } else if (available == i && state->live_slots[i] != 0) {
            available++;
        }
    }

    if (available == state->live_slots_sz) {
        /* The entire live slot list is full. */
        warn("ran out of space for tracking slots; disabling tracking");
        state->live_slots_sz = 0;
    } else {
        state->live_slots[available] = slot;
    }
}

static int cspace_alloc(void *data, seL4_CPtr *res)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;
    vka_t *v = s->underlying;
    int result = v->cspace_alloc(v->data, res);
    if (result == 0 && res != NULL) {
        track_slot(s, *res);
    }
    return result;
}

/* Stop tracking a slot that is now dead. */
static void untrack_slot(state_t *state, seL4_CPtr slot)
{
    assert(state != NULL);

    for (unsigned int i = 0; i < state->live_slots_sz; i++) {
        if (state->live_slots[i] == slot) {
            /* Found it. */
            state->live_slots[i] = 0;
            return;
        }
    }
    fatal("attempt to free slot %lu that was not live (double free?)", (long)slot);
}

static void cspace_free(void *data, seL4_CPtr slot)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;

    if (slot != 0) {
        /* Let's assume malloc semantics, i.e. that it's always OK to free
         * NULL. Therefore the caller may have called this with 0 even if they
         * were never handed out 0. We should ignore trying to untrack this.
         */
        untrack_slot(s, slot);
    }
    vka_t *v = s->underlying;
    v->cspace_free(v->data, slot);
}

/* No instrumentation required for this one. Just invoke the underlying
 * allocator.
 */
static void cspace_make_path(void *data, seL4_CPtr slot, cspacepath_t *res)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;
    vka_t *v = s->underlying;
    v->cspace_make_path(v->data, slot, res);
}

/* Track an object that has just become live. Logic is essentially the same as
 * track_slot. Refer to the comments in it for explanations.
 */
static void track_obj(state_t *state, seL4_Word type, seL4_Word size_bits,
                      seL4_Word cookie)
{
    assert(state != NULL);

    if (state->live_objs_sz == 0) {
        return;
    }

    if (cookie == 0) {
        fatal("allocator attempted to hand out an object with no cookie");
    }

    unsigned int available = 0;
    for (unsigned int i = 0; i < state->live_objs_sz; i++) {
        if (state->live_objs[i].cookie == cookie) {
            fatal("allocator attempted to hand out an object with a cookie %zu "
                  "that is currently in use", cookie);
        } else if (available == i && state->live_objs[i].cookie != 0) {
            available++;
        }
    }

    if (available == state->live_objs_sz) {
        warn("ran out of space for tracking objects; disabling tracking");
        state->live_objs_sz = 0;
    } else {
        state->live_objs[available].type = type;
        state->live_objs[available].size_bits = size_bits;
        state->live_objs[available].cookie = cookie;
    }
}

static int utspace_alloc(void *data, const cspacepath_t *dest, seL4_Word type,
                         seL4_Word size_bits, seL4_Word *res)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;

    /* At this point I guess we could check that dest is a path that was
     * previously returned from cspace_make_path, but there seems to be nothing
     * in the interface that spells out that dest needs to have originated from
     * this allocator and not another co-existing one.
     */

    vka_t *v = s->underlying;
    int result = v->utspace_alloc(v->data, dest, type, size_bits, res);
    if (result == 0 && res != NULL) {
        track_obj(s, type, size_bits, *res);
    }
    return result;
}

static int utspace_alloc_maybe_device(void *data, const cspacepath_t *dest, seL4_Word type,
                         seL4_Word size_bits, bool can_use_dev, seL4_Word *res)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;

    vka_t *v = s->underlying;
    int result = v->utspace_alloc_maybe_device(v->data, dest, type, size_bits, can_use_dev, res);
    if (result == 0 && res != NULL) {
        track_obj(s, type, size_bits, *res);
    }
    return result;
}

static int utspace_alloc_at(void *data, const cspacepath_t *dest, seL4_Word type,
                            seL4_Word size_bits, uintptr_t paddr, seL4_Word *cookie)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;

    /* At this point I guess we could check that dest is a path that was
     * previously returned from cspace_make_path, but there seems to be nothing
     * in the interface that spells out that dest needs to have originated from
     * this allocator and not another co-existing one.
     */

    vka_t *v = s->underlying;
    int result = v->utspace_alloc_at(v->data, dest, type, size_bits, paddr, cookie);
    if (result == 0 && cookie != NULL) {
        track_obj(s, type, size_bits, *cookie);
    }
    return result;
}

/* Stop tracking an object that is now dead. */
static void untrack_obj(state_t *state, seL4_Word type, seL4_Word size_bits,
                        seL4_Word cookie)
{
    assert(state != NULL);

    for (unsigned int i = 0; i < state->live_objs_sz; i++) {
        if (state->live_objs[i].cookie == cookie) {
            if (state->live_objs[i].type != type) {
                fatal("attempt to free object with type %d that was allocated "
                    "with type %d", (int)type, (int)state->live_objs[i].type);
            }
            if (state->live_objs[i].size_bits != size_bits) {
                fatal("attempt to free object with size %d that was allocated "
                    "with size %d", (int)size_bits, (int)state->live_objs[i].size_bits);
            }
            state->live_objs[i].cookie = 0;
            return;
        }
    }
    fatal("attempt to free object %lu that was not live (double free?)",
          (long)cookie);
}

static void utspace_free(void *data, seL4_Word type, seL4_Word size_bits,
                         seL4_Word target)
{
    assert(data != NULL);

    state_t *s = (state_t*)data;
    vka_t *v = s->underlying;
    if (target != 0) {
        /* Again, assume malloc semantics that freeing NULL is fine. */
        untrack_obj(s, type, size_bits, target);
    }
    v->utspace_free(v->data, type, size_bits, target);
}

int vka_init_debugvka(vka_t *vka, vka_t *tracee)
{
    assert(vka != NULL);

    state_t *s = (state_t*)malloc(sizeof(state_t));
    if (s == NULL) {
        goto fail;
    }
    s->underlying = tracee;
    s->live_slots = NULL;
    s->live_objs = NULL;

    s->live_slots_sz = live_slots_sz;
    if (live_slots_sz > 0) {
        s->live_slots = (seL4_CPtr*)malloc(sizeof(seL4_CPtr) * live_slots_sz);
        if (s->live_slots == NULL) {
            goto fail;
        }
    }

    s->live_objs_sz = live_objs_sz;
    if (live_objs_sz > 0) {
        s->live_objs = (struct obj*)malloc(sizeof(struct obj) * live_objs_sz);
        if (s->live_objs == NULL) {
            goto fail;
        }
    }

    vka->data = (void*)s;
    vka->cspace_alloc = cspace_alloc;
    vka->cspace_make_path = cspace_make_path;
    vka->utspace_alloc = utspace_alloc;
    vka->utspace_alloc_maybe_device = utspace_alloc_maybe_device;
    vka->utspace_alloc_at = utspace_alloc_at;
    vka->cspace_free = cspace_free;
    vka->utspace_free = utspace_free;

    return 0;

fail:
    if (s != NULL) {
        if (s->live_slots != NULL) {
            free(s->live_slots);
        }
        if (s->live_objs != NULL) {
            free(s->live_objs);
        }
        free(s);
    }
    return -1;
}
