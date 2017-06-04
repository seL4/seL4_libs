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

/* This file contains functionality for debugging heap corruption issues. It is
 * styled after Electric Fence [0], but we can't check as thoroughly as eFence
 * can because we don't have support for mapping/unmapping/mprotecting regions.
 * The basic concept is that we stick a 'canary' value before and after
 * allocated memory. During free, we check that these canaries haven't been
 * overwritten, which would imply buffer underflow or overflow. This checking
 * is limited in that we don't catch heap corruption until memory is actually
 * freed.
 *
 * Essentially any allocated region actually looks like this:
 *   p           q         r
 *   ↓           ↓         ↓
 *   |canary|size|memory...|canary|
 *
 * Internally we deal in 'unboxed' pointers, like p. Externally the caller is
 * given 'boxed' pointers, like q. We effectively play the same tricks malloc
 * does to store some book keeping information surrounding the malloced memory.
 * The 'size' stored is the size the user requested, not the size including
 * the canaries etc. We need to track this in order to calculate the pointer r
 * on free.
 *
 * In addition to checking for corruption within the heap, this functionality
 * also tracks pointers that have been allocated. If you try and free (or
 * realloc) a pointer that was never allocated, it will be detected.
 *
 * To use this, you will need to instruct the linker to wrap your calls to
 * allocation functions. You will need to append something like the following
 * to your LD_FALGS:
 *  -Wl,--wrap=malloc -Wl,--wrap=free -Wl,--wrap=realloc -Wl,--wrap=calloc
 *
 * Ensure you do not call any functions that malloc or free memory within this
 * file or you will infinitely recurse.
 */

#include <assert.h>
#include <autoconf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* for size_t */
#include <string.h>

/* Maximum alignment of a data type. The malloc spec requires that returned
 * pointers are aligned to this.
 */
#define MAX_ALIGNMENT (sizeof(void*) * 2) /* bytes */

/* Random bits to OR into the pre- and post-canary values so we're not storing
 * an exact pointer value. Note that these should be less than MAX_ALIGNMENT so
 * that we're ORing into known 0 bits. Preserving the pointer is not actually
 * necessary so nothing will break if you change these values to any random
 * word-sized value.
 */
#define PRE_EXTRA_BITS  0x7
#define POST_EXTRA_BITS 0x3

/* Algorithms for calculating a canary value to use before and after the
 * allocated region. The actual algorithm used here is more or less irrelevant
 * as long as it is deterministic.
 */
static uintptr_t pre_canary(void *ptr)
{
    return ((uintptr_t)ptr) | PRE_EXTRA_BITS;
}
static uintptr_t post_canary(void *ptr)
{
    return ((uintptr_t)ptr) | POST_EXTRA_BITS;
}

typedef struct {
    uintptr_t canary;
    size_t size;
} __attribute__((aligned(MAX_ALIGNMENT))) metadata_t;

/* A `uintptr_t` that is not naturally aligned. It is necessary to explicitly
 * tag unaligned pointers because, e.g., on ARM the compiler is free to
 * transform `memcpy` calls into a variant that assumes its inputs are aligned
 * and performs word-sized accesses. See the following for more information:
 *   http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/4972.html
 */
typedef uintptr_t unaligned_uintptr_t __attribute__((aligned(1)));

/* Whether we've already encountered heap corruption. See uses of this below,
 * where we abandon all instrumentation if we've hit an error. The reasoning
 * behind this is that error reporting functions may call malloc.
 */
static int err = 0;

/* Adjust a size that is about to be passed to the real allocation functions in
 * order to account for our instrumentation.
 */
static size_t adjust_size(size_t size)
{
    return size + sizeof(metadata_t) + sizeof(uintptr_t);
}

/* Wrap a (just-allocated) region with canary values. */
static void *box(void *ptr, size_t size)
{
    if (ptr == NULL) {
        return ptr;
    }

    metadata_t *pre = (metadata_t*)ptr;
    ptr += sizeof(*pre);
    unaligned_uintptr_t *post = ptr + size;

    /* Write the leading canary. */
    pre->canary = pre_canary(ptr);
    pre->size = size;

    /* Use memcpy for the trailing canary so we don't need to care about
     * alignment.
     */
    uintptr_t canary = post_canary(ptr);
    memcpy(post, &canary, sizeof(canary));

    return ptr;
}

/* Throw an error resulting from a bad user invocation. */
#define error(args...) do { \
        assert(!err); /* We should not already be handling an error. */ \
        err = 1; \
        fprintf(stderr, args); \
        abort(); \
    } while (0)

/* Unwrap a canary-endowed region into the original allocated pointer. */
static void *unbox(void *ptr, void *ret_addr)
{
    if (ptr == NULL) {
        return ptr;
    }

    metadata_t *pre = (metadata_t*)(ptr - sizeof(*pre));
    unaligned_uintptr_t *post = ptr + pre->size;

    /* Check the leading canary (underflow). */
    if (pre->canary != pre_canary(ptr)) {
        error("Leading corruption in heap memory pointed to by %p (called "
              "prior to %p)\n", ptr, ret_addr);
    }

    /* Check the trailing canary with memcmp so we don't need to care about
     * alignment. If the memcmp VM faults here, it's likely that you underran
     * your buffer and overwrote the 'size' metadata, causing this function to
     * derive an incorrect 'post' pointer.
     */
    uintptr_t canary = post_canary(ptr);
    if (memcmp(post, &canary, sizeof(canary)) != 0) {
        error("Buffer overflow in heap memory pointed to by %p (called prior "
              "to %p)\n", ptr, ret_addr);
    }

    return (void*)pre;
}

/* Buffer for tracking currently live heap pointers. This is used to detect
 * when the user attempts to free an invalid pointer. Note that we always track
 * *boxed* pointers as these are the ones seen by the user.
 */
#ifndef CONFIG_LIBSEL4DEBUG_ALLOC_BUFFER_ENTRIES
#define CONFIG_LIBSEL4DEBUG_ALLOC_BUFFER_ENTRIES 128
#endif
static uintptr_t alloced[CONFIG_LIBSEL4DEBUG_ALLOC_BUFFER_ENTRIES];

/* Track the given heap pointer as currently live. */
static void track(void *ptr)
{
    if (sizeof(alloced) == 0 || ptr == NULL) {
        /* Disable tracking if we have no buffer and never track NULL. */
        return;
    }
    for (unsigned int i = 0; i < sizeof(alloced) / sizeof(uintptr_t); i++) {
        if (alloced[i] == 0) {
            /* Found a free slot. */
            alloced[i] = (uintptr_t)ptr;
            return;
        }
    }
    /* Failed to find a free slot. */
    error("Exhausted pointer tracking buffer; try increasing "
          "CONFIG_LIBSEL4DEBUG_ALLOC_BUFFER_ENTRIES value\n");
}

/* Stop tracking the given pointer (mark it as dead). */
static void untrack(void *ptr, void *ret_addr)
{
    if (sizeof(alloced) == 0 || ptr == NULL) {
        /* Ignore tracking if we have no buffer or are freeing NULL. */
        return;
    }
    for (unsigned int i = 0; i < sizeof(alloced) / sizeof(uintptr_t); i++) {
        if (alloced[i] == (uintptr_t)ptr) {
            /* Found it. */
            alloced[i] = 0;
            return;
        }
    }
    /* Failed to find it. */
    error("Attempt to free pointer %p that was never malloced (called prior "
          "to %p)\n", ptr, ret_addr);
}

/* Wrapped functions that will be exported to us from libmuslc. */
void *__real_malloc(size_t size);
void __real_free(void *ptr);
void *__real_calloc(size_t num, size_t size);
void *__real_realloc(void *ptr, size_t size);

/* Actual allocation wrappers follow. */

void *__wrap_malloc(size_t size)
{
    if (err) {
        return __real_malloc(size);
    }

    size_t new_size = adjust_size(size);
    void *ptr = __real_malloc(new_size);
    ptr = box(ptr, size);
    track(ptr);
    return ptr;
}

void __wrap_free(void *ptr)
{
    if (err) {
        __real_free(ptr);
        return;
    }

    void *ret = __builtin_extract_return_addr(__builtin_return_address(0));

    untrack(ptr, ret);

    /* Write garbage all over the region we were handed back to try to expose
     * use-after-free bugs. If we fault while doing this, it probably means the
     * user underran their buffer and overwrote the 'size' metadata.
     */
    metadata_t *pre = (metadata_t*)(ptr - sizeof(*pre));
    for (unsigned int i = 0; i < pre->size; i++) {
        ((char*)ptr)[i] ^= (char)~i;
    }

    ptr = unbox(ptr, ret);
    __real_free(ptr);
}

void *__wrap_calloc(size_t num, size_t size)
{
    if (err) {
        return __real_calloc(num, size);
    }

    size_t sz = adjust_size(num * size);
    size_t new_num = sz / size;
    if (sz % size != 0) {
        new_num++;
    }

    void *ptr = __real_calloc(new_num, size);
    ptr = box(ptr, num * size);
    track(ptr);
    return ptr;
}

void *__wrap_realloc(void *ptr, size_t size)
{
    if (err) {
        return __real_realloc(ptr, size);
    }

    void *ret = __builtin_extract_return_addr(__builtin_return_address(0));

    untrack(ptr, ret);
    ptr = unbox(ptr, ret);
    size_t new_size = adjust_size(size);
    ptr = __real_realloc(ptr, new_size);
    ptr = box(ptr, size);
    track(ptr);
    return ptr;
}
