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

#include <allocman/allocman.h>
#include <allocman/util.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sel4/sel4.h>
#include <vka/capops.h>
#include <sel4utils/util.h>

static int _refill_watermark(allocman_t *alloc);

static inline int _can_alloc(struct allocman_properties properties, size_t alloc_depth, size_t free_depth)
{
    int in_alloc = alloc_depth > 0;
    int in_free = free_depth > 0;
    return (properties.alloc_can_alloc || !in_alloc) && (properties.free_can_alloc || !in_free);
}

static inline int _can_free(struct allocman_properties properties, size_t alloc_depth, size_t free_depth)
{
    int in_alloc = alloc_depth > 0;
    int in_free = free_depth > 0;
    return (properties.alloc_can_free || !in_alloc) && (properties.free_can_free || !in_free);
}

/* Signals an operation is being started, and returns whether
   this is the root operation, or a dependent call */
static int _start_operation(allocman_t *alloc)
{
    int ret = !alloc->in_operation;
    alloc->in_operation = 1;
    return ret;
}

static inline void _end_operation(allocman_t *alloc, int root)
{
    alloc->in_operation = !root;
    /* Anytime we end an operation we need to make sure we have watermark
       resources */
    if (root) {
        _refill_watermark(alloc);
    }
}

static void allocman_mspace_queue_for_free(allocman_t *alloc, void *ptr, size_t bytes) {
    if (alloc->num_freed_mspace_chunks == alloc->desired_freed_mspace_chunks) {
        assert(!"Out of space to store free'd objects. Leaking memory");
        return;
    }
    alloc->freed_mspace_chunks[alloc->num_freed_mspace_chunks] =
        (struct allocman_freed_mspace_chunk) {ptr, bytes};
    alloc->num_freed_mspace_chunks++;
}

static void allocman_cspace_queue_for_free(allocman_t *alloc, const cspacepath_t *path) {
    if (alloc->num_freed_slots == alloc->desired_freed_slots) {
        assert(!"Out of space to store free'd objects. Leaking memory");
        return;
    }
    alloc->freed_slots[alloc->num_freed_slots] = *path;
    alloc->num_freed_slots++;
}

static void allocman_utspace_queue_for_free(allocman_t *alloc, seL4_Word cookie, size_t size_bits) {
    if (alloc->num_freed_utspace_chunks == alloc->desired_freed_utspace_chunks) {
        assert(!"Out of space to store free'd objects. Leaking memory");
        return;
    }
    alloc->freed_utspace_chunks[alloc->num_freed_utspace_chunks] =
        (struct allocman_freed_utspace_chunk) {size_bits, cookie};
    alloc->num_freed_utspace_chunks++;
}

/* this nasty macro prevents code duplication for the free functions. Unfortunately I can think of no other
 * way of allowing the number of arguments to the 'free' function in the body to be parameterized */
#define ALLOCMAN_FREE(alloc,space,...) do { \
    int root; \
    assert(alloc->have_##space); \
    if (!_can_free(alloc->space.properties, alloc->space##_alloc_depth, alloc->space##_free_depth)) { \
        allocman_##space##_queue_for_free(alloc, __VA_ARGS__); \
        return; \
    } \
    root = _start_operation(alloc); \
    alloc->space##_free_depth++; \
    alloc->space.free(alloc, alloc->space.space, __VA_ARGS__); \
    alloc->space##_free_depth--; \
    _end_operation(alloc, root); \
} while(0)

void allocman_cspace_free(allocman_t *alloc, const cspacepath_t *slot)
{
    ALLOCMAN_FREE(alloc, cspace, slot);
}

void allocman_mspace_free(allocman_t *alloc, void *ptr, size_t bytes)
{
    ALLOCMAN_FREE(alloc, mspace, ptr, bytes);
}

void allocman_utspace_free(allocman_t *alloc, seL4_Word cookie, size_t size_bits)
{
    ALLOCMAN_FREE(alloc, utspace, cookie, size_bits);
}

static void *_try_watermark_mspace(allocman_t *alloc, size_t size, int *_error)
{
    size_t i;
    for (i = 0; i < alloc->num_mspace_chunks; i++) {
        if (alloc->mspace_chunk[i].size == size) {
            if (alloc->mspace_chunk_count[i] > 0) {
                void *ret = alloc->mspace_chunks[i][--alloc->mspace_chunk_count[i]];
                SET_ERROR(_error, 0);
                alloc->used_watermark = 1;
                return ret;
            }
        }
    }
    SET_ERROR(_error, 1);
    return NULL;
}

static int _try_watermark_cspace(allocman_t *alloc, cspacepath_t *slot)
{
    if (alloc->num_cspace_slots == 0) {
        return 1;
    }
    alloc->used_watermark = 1;
    *slot = alloc->cspace_slots[--alloc->num_cspace_slots];
    return 0;
}

static seL4_Word _try_watermark_utspace(allocman_t *alloc, size_t size_bits, seL4_Word type, const cspacepath_t *path, int *_error)
{
    size_t i;

    for (i = 0; i < alloc->num_utspace_chunks; i++) {
        if (alloc->utspace_chunk[i].size_bits == size_bits && alloc->utspace_chunk[i].type == type) {
            if (alloc->utspace_chunk_count[i] > 0) {
                struct allocman_utspace_allocation result = alloc->utspace_chunks[i][alloc->utspace_chunk_count[i] - 1];
                int error;
                /* Need to perform a cap move */
                error = vka_cnode_move(path, &result.slot);
                if (error != seL4_NoError) {
                    SET_ERROR(_error, 1);
                    return 0;
                }
                alloc->used_watermark = 1;
                alloc->utspace_chunk_count[i]--;
                allocman_cspace_free(alloc, &result.slot);
                SET_ERROR(_error, 0);
                return result.cookie;
            }
        }
    }
    SET_ERROR(_error, 1);
    return 0;
}

static void *_allocman_mspace_alloc(allocman_t *alloc, size_t size, int *_error, int use_watermark)
{
    int root_op;
    void *ret;
    int error;
    /* see if we have an allocator installed yet*/
    if (!alloc->have_mspace) {
        SET_ERROR(_error, 1);
        return 0;
    }
    /* Check that we are permitted to cspace_alloc here */
    if (!_can_alloc(alloc->mspace.properties, alloc->mspace_alloc_depth, alloc->mspace_free_depth)) {
        if (use_watermark) {
            ret = _try_watermark_mspace(alloc, size, _error);
            if (!ret) {
                ZF_LOGI("Failed to fullfill recursive allocation from watermark, size %zu\n", size);
            }
            return ret;
        } else {
            SET_ERROR(_error, 1);
            return 0;
        }
    }
    root_op = _start_operation(alloc);
    /* Attempt the allocation */
    alloc->mspace_alloc_depth++;
    ret = alloc->mspace.alloc(alloc, alloc->mspace.mspace, size, &error);
    alloc->mspace_alloc_depth--;
    if (!error) {
        _end_operation(alloc, root_op);
        SET_ERROR(_error, 0);
        return ret;
    }
    /* We encountered some fail. We will try and allocate from the watermark pool.
       Does not matter what the error or outcome is, just propogate back up*/
    if (use_watermark) {
        ret = _try_watermark_mspace(alloc, size, _error);
        if (!ret) {
            ZF_LOGI("Regular mspace alloc failed, and watermark also failed. for size %zu\n", size);
        }
        _end_operation(alloc, root_op);
        return ret;
    } else {
        _end_operation(alloc, root_op);
        SET_ERROR(_error, 1);
        return NULL;
    }
}

static int _allocman_cspace_alloc(allocman_t *alloc, cspacepath_t *slot, int use_watermark)
{
    int root_op;
    int error;
    /* see if we have an allocator installed yet*/
    if (!alloc->have_cspace) {
        return 1;
    }
    /* Check that we are permitted to cspace_alloc here */
    if (!_can_alloc(alloc->cspace.properties, alloc->cspace_alloc_depth, alloc->cspace_free_depth)) {
        if (use_watermark) {
            int ret = _try_watermark_cspace(alloc, slot);
            if (ret) {
                ZF_LOGI("Failed to allocate cslot from watermark\n");
            }
            return ret;
        } else {
            return 1;
        }
    }
    root_op = _start_operation(alloc);
    /* Attempt the allocation */
    alloc->cspace_alloc_depth++;
    error = alloc->cspace.alloc(alloc, alloc->cspace.cspace, slot);
    alloc->cspace_alloc_depth--;
    if (!error) {
        _end_operation(alloc, root_op);
        return 0;
    }
    /* We encountered some fail. We will try and allocate from the watermark pool.
       Does not matter what the error or outcome is, just propogate back up*/
    if (use_watermark) {
        error = _try_watermark_cspace(alloc, slot);
        if (error) {
            ZF_LOGI("Regular cspace alloc failed, and failed from watermark\n");
        }
        _end_operation(alloc, root_op);
        return error;
    } else {
        _end_operation(alloc, root_op);
        return 1;
    }
}

static seL4_Word _allocman_utspace_alloc(allocman_t *alloc, size_t size_bits, seL4_Word type, const cspacepath_t *path, uintptr_t paddr, bool canBeDev, int *_error, int use_watermark)
{
    int root_op;
    int error;
    seL4_Word ret;
    /* see if we have an allocator installed yet*/
    if (!alloc->have_utspace) {
        SET_ERROR(_error,1);
        return 0;
    }
    /* Check that we are permitted to utspace_alloc here */
    if (!_can_alloc(alloc->utspace.properties, alloc->utspace_alloc_depth, alloc->utspace_free_depth)) {
        if (use_watermark && paddr == ALLOCMAN_NO_PADDR) {
            ret = _try_watermark_utspace(alloc, size_bits, type, path, _error);
            if (ret == 0) {
                ZF_LOGI("Failed to allocate utspace from watermark. size %zu type %ld\n", size_bits, (long)type);
            }
            return ret;
        } else {
            SET_ERROR(_error, 1);
            return 0;
        }
    }
    root_op = _start_operation(alloc);
    /* Attempt the allocation */
    alloc->utspace_alloc_depth++;
    ret = alloc->utspace.alloc(alloc, alloc->utspace.utspace, size_bits, type, path, paddr, canBeDev, &error);
    alloc->utspace_alloc_depth--;
    if (!error) {
        _end_operation(alloc, root_op);
        SET_ERROR(_error, error);
        return ret;
    }
    /* We encountered some fail. We will try and allocate from the watermark pool.
       Does not matter what the error or outcome is, just propogate back up*/
    if (use_watermark && paddr == ALLOCMAN_NO_PADDR) {
        ret = _try_watermark_utspace(alloc, size_bits, type, path, _error);
        _end_operation(alloc, root_op);
        if (ret == 0) {
            ZF_LOGI("Regular utspace alloc failed and not watermark for size %zu type %ld\n", size_bits, (long)type);
        }
        return ret;
    } else {
        _end_operation(alloc, root_op);
        SET_ERROR(_error, 1);
        return 0;
    }
}

void *allocman_mspace_alloc(allocman_t *alloc, size_t size, int *_error)
{
    return _allocman_mspace_alloc(alloc, size, _error, 1);
}

int allocman_cspace_alloc(allocman_t *alloc, cspacepath_t *slot)
{
    return _allocman_cspace_alloc(alloc, slot, 1);
}

seL4_Word allocman_utspace_alloc_at(allocman_t *alloc, size_t size_bits, seL4_Word type, const cspacepath_t *path, uintptr_t paddr, bool canBeDev, int *_error)
{
    return _allocman_utspace_alloc(alloc, size_bits, type, path, paddr, canBeDev, _error, 1);
}

static int _refill_watermark(allocman_t *alloc)
{
    int found_empty_pool;
    int did_allocation;
    size_t i;
    if (alloc->refilling_watermark || !alloc->used_watermark) {
        return 0;
    }
    alloc->refilling_watermark = 1;

    /* Run in a loop refilling our resources. We need a loop as refilling
       one resource may require another watermark resource to be used. It is up
       to the allocators to prove that this process results in a consistent
       increase in the watermark pool, and hence will terminate. Need to be
       very careful with re-entry in this loop, as our watermark resources
       may change anytime we perform an allocation. We try and allocate evenly
       across all the resources types since typically we are only refilling
       a single object from each resource anyway, so the performance will be
       the same, and if we aren't we are boot strapping and I'm not convinced
       that all allocations orders are equivalent in this case */
    int limit = 0;
    do {
        found_empty_pool = 0;
        did_allocation = 0;
        while (alloc->num_freed_slots > 0) {
            cspacepath_t slot = alloc->freed_slots[--alloc->num_freed_slots];
            allocman_cspace_free(alloc, &slot);
            /* a free is like an allocation in that we have made some progress */
            did_allocation = 1;
        }
        while (alloc->num_freed_mspace_chunks > 0) {
            struct allocman_freed_mspace_chunk chunk = alloc->freed_mspace_chunks[--alloc->num_freed_mspace_chunks];
            allocman_mspace_free(alloc, chunk.ptr, chunk.size);
            did_allocation = 1;
        }
        while (alloc->num_freed_utspace_chunks > 0) {
            struct allocman_freed_utspace_chunk chunk = alloc->freed_utspace_chunks[--alloc->num_freed_utspace_chunks];
            allocman_utspace_free(alloc, chunk.cookie, chunk.size_bits);
            did_allocation = 1;
        }
        if (alloc->num_cspace_slots < alloc->desired_cspace_slots) {
            int error;
            found_empty_pool = 1;
            cspacepath_t slot;
            error = _allocman_cspace_alloc(alloc, &slot, 0);
            if (!error) {
                alloc->cspace_slots[alloc->num_cspace_slots++] = slot;
                did_allocation = 1;
            }
        }
        for (i = 0; i < alloc->num_utspace_chunks; i++) {
            if (alloc->utspace_chunk_count[i] < alloc->utspace_chunk[i].count) {
                cspacepath_t slot;
                seL4_Word cookie;
                int error;
                /* First grab a slot */
                found_empty_pool = 1;
                error = allocman_cspace_alloc(alloc, &slot);
                if (!error) {
                    /* Now try to allocate */
                    cookie = _allocman_utspace_alloc(alloc, alloc->utspace_chunk[i].size_bits, alloc->utspace_chunk[i].type, &slot, ALLOCMAN_NO_PADDR, false, &error, 0);
                    if (!error) {
                        alloc->utspace_chunks[i][alloc->utspace_chunk_count[i]].cookie = cookie;
                        alloc->utspace_chunks[i][alloc->utspace_chunk_count[i]].slot = slot;
                        alloc->utspace_chunk_count[i]++;
                        did_allocation = 1;
                    } else {
                        /* Give the slot back */
                        allocman_cspace_free(alloc, &slot);
                    }
                }
            }
        }
        for (i = 0 ; i < alloc->num_mspace_chunks; i++) {
            if (alloc->mspace_chunk_count[i] < alloc->mspace_chunk[i].count) {
                void *result;
                int error;
                found_empty_pool = 1;
                result = _allocman_mspace_alloc(alloc, alloc->mspace_chunk[i].size, &error, 0);
                if (!error) {
                    alloc->mspace_chunks[i][alloc->mspace_chunk_count[i]++] = result;
                    did_allocation = 1;
                }
            }
        }
        limit++;
    } while (found_empty_pool && did_allocation && limit < 4);

    alloc->refilling_watermark = 0;
    if (!found_empty_pool) {
        alloc->used_watermark = 0;
    }
    return found_empty_pool;
}

int allocman_create(allocman_t *alloc, struct mspace_interface mspace) {
    /* zero out the struct */
    memset(alloc, 0, sizeof(allocman_t));

    alloc->mspace = mspace;
    alloc->have_mspace = 1;

    return 0;
}

int allocman_fill_reserves(allocman_t *alloc) {
    int full;
    int root = _start_operation(alloc);
    /* force the reserves to be checked */
    alloc->used_watermark = 1;
    /* attempt to fill */
    full = _refill_watermark(alloc);
    _end_operation(alloc, root);
    return full;
}

#define ALLOCMAN_ATTACH(alloc, space, interface) do { \
    int root = _start_operation(alloc); \
    assert(root); \
    if (alloc->have_##space) { \
        /* an untyped allocator has already been attached, bail */ \
        LOG_ERROR("Alocate of type " #space " is already attached"); \
        return 1; \
    } \
    alloc->space = interface; \
    alloc->have_##space = 1; \
    _end_operation(alloc, root); \
    return 0; \
}while(0)

int allocman_attach_utspace(allocman_t *alloc, struct utspace_interface utspace) {
    ALLOCMAN_ATTACH(alloc, utspace, utspace);
}

int allocman_attach_cspace(allocman_t *alloc, struct cspace_interface cspace) {
    ALLOCMAN_ATTACH(alloc, cspace, cspace);
}

static int resize_array(allocman_t *alloc, size_t num, void **array, size_t *size, size_t *count, size_t item_size) {
    int root = _start_operation(alloc);
    void *new_array;
    int error;

    assert(root);

    /* allocate new array */
    new_array = allocman_mspace_alloc(alloc, item_size * num, &error);
    if (!!error) {
        return error;
    }

    /* if we have less than before. throw an error */
    while (num < (*count)) {
        return -1;
    }

    /* copy any existing slots and free the old array, but avoid using a null array */
    if ((*array)) {
        memcpy(new_array, (*array), item_size * (*count));
        allocman_mspace_free(alloc, (*array), item_size * (*size));
    }

    /* switch the new array in */
    (*array) = new_array;
    (*size) = num;

    alloc->used_watermark = 1;
    _end_operation(alloc, root);
    return error;
}

static int resize_slots_array(allocman_t *alloc, size_t num, cspacepath_t **slots, size_t *size, size_t *count) {
    return resize_array(alloc, num, (void**)slots, size, count, sizeof(cspacepath_t));
}

int allocman_configure_cspace_reserve(allocman_t *alloc, size_t num) {
    return resize_slots_array(alloc, num, &alloc->cspace_slots, &alloc->desired_cspace_slots, &alloc->num_cspace_slots);
}

int allocman_configure_max_freed_slots(allocman_t *alloc, size_t num) {
    return resize_slots_array(alloc, num, &alloc->freed_slots, &alloc->desired_freed_slots, &alloc->num_freed_slots);
}

int  allocman_configure_max_freed_memory_chunks(allocman_t *alloc, size_t num) {
    return resize_array(alloc, num, (void**)&alloc->freed_mspace_chunks, &alloc->desired_freed_mspace_chunks, &alloc->num_freed_mspace_chunks, sizeof(struct allocman_freed_mspace_chunk));
}

int  allocman_configure_max_freed_untyped_chunks(allocman_t *alloc, size_t num) {
    return resize_array(alloc, num, (void**)&alloc->freed_utspace_chunks, &alloc->desired_freed_utspace_chunks, &alloc->num_freed_utspace_chunks, sizeof(struct allocman_freed_utspace_chunk));
}

int allocman_configure_utspace_reserve(allocman_t *alloc, struct allocman_utspace_chunk chunk) {
    int root = _start_operation(alloc);
    size_t i;
    struct allocman_utspace_chunk *new_chunk;
    size_t *new_counts;
    struct allocman_utspace_allocation **new_chunks;
    struct allocman_utspace_allocation *new_alloc;
    int error;
    /* ensure this chunk hasn't already been added. would be nice to handle both decreasing and
     * icnreasing reservations, but I cannot see the use case for that */
    for (i = 0; i < alloc->num_utspace_chunks; i++) {
        if (alloc->utspace_chunk[i].size_bits == chunk.size_bits && alloc->utspace_chunk[i].type == chunk.type) {
            return 1;
        }
    }
    /* tack this chunk on */
    new_chunk = allocman_mspace_alloc(alloc, sizeof(struct allocman_utspace_chunk) * (alloc->num_utspace_chunks + 1), &error);
    if (error) {
        return error;
    }
    new_counts = allocman_mspace_alloc(alloc, sizeof(size_t) * (alloc->num_utspace_chunks + 1), &error);
    if (error) {
        allocman_mspace_free(alloc, new_chunk, sizeof(struct allocman_utspace_chunk) * (alloc->num_utspace_chunks + 1));
        return error;
    }
    new_chunks = allocman_mspace_alloc(alloc, sizeof(struct allocman_utspace_allocation *) * (alloc->num_utspace_chunks + 1), &error);
    if (error) {
        allocman_mspace_free(alloc, new_chunk, sizeof(struct allocman_utspace_chunk) * (alloc->num_utspace_chunks + 1));
        allocman_mspace_free(alloc, new_counts, sizeof(size_t) * (alloc->num_utspace_chunks + 1));
        return error;
    }
    new_alloc = allocman_mspace_alloc(alloc, sizeof(struct allocman_utspace_allocation) * chunk.count, &error);
    if (error) {
        allocman_mspace_free(alloc, new_chunk, sizeof(struct allocman_utspace_chunk) * (alloc->num_utspace_chunks + 1));
        allocman_mspace_free(alloc, new_counts, sizeof(size_t) * (alloc->num_utspace_chunks + 1));
        allocman_mspace_free(alloc, new_chunks, sizeof(struct allocman_utspace_allocation *) * (alloc->num_utspace_chunks + 1));
        return error;
    }
    if (alloc->num_utspace_chunks > 0) {
        memcpy(new_chunk, alloc->utspace_chunk, sizeof(struct allocman_utspace_chunk) * alloc->num_utspace_chunks);
        memcpy(new_counts, alloc->utspace_chunk_count, sizeof(size_t) * alloc->num_utspace_chunks);
        memcpy(new_chunks, alloc->utspace_chunks, sizeof(struct allocman_utspace_allocation *) * alloc->num_utspace_chunks);
        allocman_mspace_free(alloc, alloc->utspace_chunk, sizeof(struct allocman_utspace_chunk) * alloc->num_utspace_chunks);
        allocman_mspace_free(alloc, alloc->utspace_chunk_count, sizeof(size_t) * alloc->num_utspace_chunks);
        allocman_mspace_free(alloc, alloc->utspace_chunks, sizeof(struct allocman_utspace_allocation *) * alloc->num_utspace_chunks);
    }
    new_chunk[alloc->num_utspace_chunks] = chunk;
    new_counts[alloc->num_utspace_chunks] = 0;
    new_chunks[alloc->num_utspace_chunks] = new_alloc;
    alloc->utspace_chunk = new_chunk;
    alloc->utspace_chunk_count = new_counts;
    alloc->utspace_chunks = new_chunks;
    alloc->num_utspace_chunks++;
    alloc->used_watermark = 1;
    _end_operation(alloc, root);
    return 0;
}

int allocman_configure_mspace_reserve(allocman_t *alloc, struct allocman_mspace_chunk chunk) {
    int root = _start_operation(alloc);
    size_t i;
    struct allocman_mspace_chunk *new_chunk;
    size_t *new_counts;
    void ***new_chunks;
    void **new_alloc;
    int error;
    /* ensure this chunk hasn't already been added. would be nice to handle both decreasing and
     * icnreasing reservations, but I cannot see the use case for that */
    for (i = 0; i < alloc->num_mspace_chunks; i++) {
        if (alloc->mspace_chunk[i].size == chunk.size) {
            return 1;
        }
    }
    /* tack this chunk on */
    new_chunk = allocman_mspace_alloc(alloc, sizeof(struct allocman_mspace_chunk) * (alloc->num_mspace_chunks + 1), &error);
    if (error) {
        return error;
    }
    new_counts = allocman_mspace_alloc(alloc, sizeof(size_t) * (alloc->num_mspace_chunks + 1), &error);
    if (error) {
        allocman_mspace_free(alloc, new_chunk, sizeof(struct allocman_mspace_chunk) * (alloc->num_mspace_chunks + 1));
        return error;
    }
    new_chunks = allocman_mspace_alloc(alloc, sizeof(void **) * (alloc->num_mspace_chunks + 1), &error);
    if (error) {
        allocman_mspace_free(alloc, new_chunk, sizeof(struct allocman_mspace_chunk) * (alloc->num_mspace_chunks + 1));
        allocman_mspace_free(alloc, new_counts, sizeof(size_t) * (alloc->num_mspace_chunks + 1));
        return error;
    }
    new_alloc = allocman_mspace_alloc(alloc, sizeof(void *) * chunk.count, &error);
    if (error) {
        allocman_mspace_free(alloc, new_chunk, sizeof(struct allocman_mspace_chunk) * (alloc->num_mspace_chunks + 1));
        allocman_mspace_free(alloc, new_counts, sizeof(size_t) * (alloc->num_mspace_chunks + 1));
        allocman_mspace_free(alloc, new_chunks, sizeof(void **) * (alloc->num_mspace_chunks + 1));
        return error;
    }
    if (alloc->num_mspace_chunks > 0) {
        memcpy(new_chunk, alloc->mspace_chunk, sizeof(struct allocman_mspace_chunk) * alloc->num_mspace_chunks);
        memcpy(new_counts, alloc->mspace_chunk_count, sizeof(size_t) * alloc->num_mspace_chunks);
        memcpy(new_chunks, alloc->mspace_chunks, sizeof(void **) * alloc->num_mspace_chunks);
        allocman_mspace_free(alloc, alloc->mspace_chunk, sizeof(struct allocman_mspace_chunk) * alloc->num_mspace_chunks);
        allocman_mspace_free(alloc, alloc->mspace_chunk_count, sizeof(size_t) * alloc->num_mspace_chunks);
        allocman_mspace_free(alloc, alloc->mspace_chunks, sizeof(void **) * alloc->num_mspace_chunks);
    }
    new_chunk[alloc->num_mspace_chunks] = chunk;
    new_counts[alloc->num_mspace_chunks] = 0;
    new_chunks[alloc->num_mspace_chunks] = new_alloc;
    alloc->mspace_chunk = new_chunk;
    alloc->mspace_chunk_count = new_counts;
    alloc->mspace_chunks = new_chunks;
    alloc->num_mspace_chunks++;
    alloc->used_watermark = 1;
    _end_operation(alloc, root);
    return 0;
}


int allocman_add_untypeds_from_timer_objects(allocman_t *alloc, timer_objects_t *to) {
    int error = 0;
    for (size_t i = 0; i < to->nobjs; i++) {
        cspacepath_t path = allocman_cspace_make_path(alloc, to->objs[i].obj.cptr);
        error = allocman_utspace_add_uts(alloc, 1, &path, &to->objs[i].obj.size_bits,
                                        (uintptr_t *) &to->objs[i].region.base_addr,
                                        ALLOCMAN_UT_DEV);
        if (error) {
            ZF_LOGE("Failed to add ut to allocman");
            return error;
        }
    }
    return 0;
}
