/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* defining _GNU_SOURCE to make certain constants appear in muslc. This is rather hacky */
#define _GNU_SOURCE

#include <autoconf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>

#include <vspace/vspace.h>

#include <sel4utils/util.h>
#include <sel4utils/mapping.h>

/* If we have a nonzero static morecore then we are just doing dodgy hacky morecore */
#if CONFIG_LIB_SEL4_MUSLC_SYS_MORECORE_BYTES > 0

/*
 * Statically allocated morecore area.
 *
 * This is rather terrible, but is the simplest option without a
 * huge amount of infrastructure.
 */
char morecore_area[CONFIG_LIB_SEL4_MUSLC_SYS_MORECORE_BYTES];

/* Pointer to free space in the morecore area. */
static uintptr_t morecore_base = (uintptr_t) &morecore_area;
static uintptr_t morecore_top = (uintptr_t) &morecore_area[CONFIG_LIB_SEL4_MUSLC_SYS_MORECORE_BYTES];

/* Actual morecore implementation
   returns 0 if failure, returns newbrk if success.
*/

long
sys_brk(va_list ap)
{

    uintptr_t ret;
    uintptr_t newbrk = va_arg(ap, uintptr_t);

    /*if the newbrk is 0, return the bottom of the heap*/
    if (!newbrk) {
        ret = morecore_base;
    } else if (newbrk < morecore_top && newbrk > (uintptr_t)&morecore_area[0]) {
        ret = morecore_base = newbrk;
    } else {
        ret = 0;
    }

    return ret;
}

/* Large mallocs will result in muslc calling mmap, so we do a minimal implementation
   here to support that. We make a bunch of assumptions in the process */
long 
sys_mmap2(va_list ap)
{
    void *addr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);
    int prot = va_arg(ap, int);
    int flags = va_arg(ap, int);
    int fd = va_arg(ap, int);
    off_t offset = va_arg(ap, off_t);
    (void)addr;
    (void)prot;
    (void)fd;
    (void)offset;
    if (flags & MAP_ANONYMOUS) {
        /* Steal from the top */
        uintptr_t base = morecore_top - length;
        if (base < morecore_base) {
            return -ENOMEM;
        }
        morecore_top = base;
        return base;
    }
    assert(!"not implemented");
    return -ENOMEM;
}

long 
sys_mremap(va_list ap)
{
    assert(!"not implemented");
    return -ENOMEM;
}

#else

/* dynamic morecore based on a vspace. These need to be defined somewhere (probably in the
 * main function of your app. And setup something like
    sel4utils_reserve_range_no_alloc(&vspace, &muslc_brk_reservation_memory, BRK_VIRTUAL_SIZE, seL4_AllRights, 1, &muslc_brk_reservation_start);
    muslc_this_vspace = &vspace;
    muslc_brk_reservation.res = &muslc_brk_reservation_memory;

    In the case that you need dynamic morecore for some apps and static for others, just
    define more_core area in your app itself and set the global morecore_area before calling malloc.

    We have to do this because the build system builds the same library for each app.
  */

vspace_t *muslc_this_vspace = NULL;
reservation_t muslc_brk_reservation = {.res = NULL};
void *muslc_brk_reservation_start = NULL;

char *morecore_area = NULL;
size_t morecore_size = 0;
static uintptr_t morecore_base = 0;
static uintptr_t morecore_top = 0;

static uintptr_t brk_start;

static long
sys_brk_static(va_list ap)
{
    uintptr_t ret;
    uintptr_t newbrk = va_arg(ap, uintptr_t);

    /*if the newbrk is 0, return the bottom of the heap*/
    if (morecore_base == 0) {
        if (morecore_size == 0) {
            LOG_ERROR("Warning: static morecore size is 0");
        }
        morecore_base = (uintptr_t) morecore_area;
        morecore_top = (uintptr_t) &morecore_area[morecore_size];
    }

    if (!newbrk) {
        ret = morecore_base;
    } else if (newbrk < morecore_top && newbrk > (uintptr_t)&morecore_area[0]) {
        ret = morecore_base = newbrk;
    } else {
        ret = 0;
    }

    return ret;
}

static long
sys_brk_dynamic(va_list ap)
{

    uintptr_t ret;
    uintptr_t newbrk = va_arg(ap, uintptr_t);
    if (!muslc_this_vspace || !muslc_brk_reservation.res || !muslc_brk_reservation_start) {
        LOG_ERROR("Need to assign vspace for sys_brk to work!\n");
        assert(muslc_this_vspace && muslc_brk_reservation.res && muslc_brk_reservation_start);
        return 0;
    }

    /*if the newbrk is 0, return the bottom of the heap*/
    if (newbrk == 0) {
        brk_start = (uintptr_t)muslc_brk_reservation_start;
        ret = brk_start;
    } else {
        /* try and map pages until this point */
        while (brk_start < newbrk) {
            int error = vspace_new_pages_at_vaddr(muslc_this_vspace, (void*) brk_start, 1,
                        seL4_PageBits, muslc_brk_reservation);
            if (error) {
                LOG_ERROR("Mapping new pages to extend brk region failed\n");
                return 0;
            }
            brk_start += PAGE_SIZE_4K;
        }
        ret = brk_start;
    }
    return ret;
}

long
sys_brk(va_list ap)
{
    if (morecore_area != NULL) {
        return sys_brk_static(ap);
    } else if (muslc_this_vspace != NULL) {
        return sys_brk_dynamic(ap);
    } else {
        LOG_ERROR("You need to define either morecore_area or the muslc*");
        LOG_ERROR("global variables to use malloc\n");
        assert(morecore_area != NULL || muslc_this_vspace != NULL);
        return 0;
    }
}


/* Large mallocs will result in muslc calling mmap, so we do a minimal implementation
   here to support that. We make a bunch of assumptions in the process */
static long
sys_mmap2_static(va_list ap)
{
    void *addr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);
    int prot = va_arg(ap, int);
    int flags = va_arg(ap, int);
    int fd = va_arg(ap, int);
    off_t offset = va_arg(ap, off_t);
    (void)addr;
    (void)prot;
    (void)fd;
    (void)offset;
    if (flags & MAP_ANONYMOUS) {
        /* Steal from the top */
        uintptr_t base = morecore_top - length;
        if (base < morecore_base) {
            return -ENOMEM;
        }
        morecore_top = base;
        return base;
    }
    assert(!"not implemented");
    return -ENOMEM;
}


static long
sys_mmap2_dynamic(va_list ap)
{
    void *addr = va_arg(ap, void*);
    size_t length = va_arg(ap, size_t);
    int prot = va_arg(ap, int);
    int flags = va_arg(ap, int);
    int fd = va_arg(ap, int);
    off_t offset = va_arg(ap, off_t);
    (void)addr;
    (void)prot;
    (void)fd;
    (void)offset;
    if (!muslc_this_vspace || !muslc_brk_reservation.res || !muslc_brk_reservation_start) {
        LOG_ERROR("Need to assign vspace for sys_brk to work!\n");
        assert(muslc_this_vspace && muslc_brk_reservation.res && muslc_brk_reservation_start);
        return 0;
    }
    if (flags & MAP_ANONYMOUS) {
        /* determine how many pages we need */
        uint32_t pages = ROUND_UP(length, PAGE_SIZE_4K) / PAGE_SIZE_4K;
        void *ret = vspace_new_pages(muslc_this_vspace, seL4_AllRights, pages, seL4_PageBits);
        return (long)ret;
    }
    assert(!"not implemented");
    return -ENOMEM;
}

long
sys_mmap2(va_list ap)
{
    if (morecore_area != NULL) {
        return sys_mmap2_static(ap);
    } else if (muslc_this_vspace != NULL) {
        return sys_mmap2_dynamic(ap);
    } else {
        LOG_ERROR("mmap requires morecore_area or muslc* vars to be initialised\n");
        assert(morecore_area != NULL || muslc_this_vspace != NULL);
        return 0;
    }
}

static long
sys_mremap_dynamic(va_list ap)
{


    void *old_address = va_arg(ap, void*);
    size_t old_size = va_arg(ap, size_t);
    size_t new_size = va_arg(ap, size_t);
    int flags = va_arg(ap, int);
    UNUSED void *new_address_arg;

    assert(flags == MREMAP_MAYMOVE);
    assert(IS_ALIGNED_4K(old_size));
    assert(IS_ALIGNED_4K((uint32_t) old_address));
    assert(IS_ALIGNED_4K(new_size));
    /* we currently only support remaping to size >= original */
    assert(new_size >= old_size);

    if (flags & MREMAP_FIXED) {
        new_address_arg = va_arg(ap, void*);
    }

    /* first find all the old caps */
    int num_pages = old_size >> seL4_PageBits;
    seL4_CPtr caps[num_pages];
    uint32_t cookies[num_pages];
    int i;
    for (i = 0; i < num_pages; i++) {
        void *vaddr = old_address + i * BIT(seL4_PageBits);
        caps[i] = vspace_get_cap(muslc_this_vspace, vaddr);
        cookies[i] = vspace_get_cookie(muslc_this_vspace, vaddr);
    }
    /* unmap the previous mapping */
    vspace_unmap_pages(muslc_this_vspace, old_address, num_pages, seL4_PageBits, VSPACE_PRESERVE);
    /* reserve a new region */
    int error;
    void *new_address;
    int new_pages = new_size >> seL4_PageBits;
    reservation_t reservation = vspace_reserve_range(muslc_this_vspace, new_pages * PAGE_SIZE_4K, seL4_AllRights, 1, &new_address);
    if (!reservation.res) {
        LOG_ERROR("Failed to make reservation for remap\n");
        goto restore;
    }
    /* map all the existing pages into the reservation */
    error = vspace_map_pages_at_vaddr(muslc_this_vspace, caps, cookies, new_address, num_pages, seL4_PageBits, reservation);
    if (error) {
        LOG_ERROR("Mapping existing pages into new reservation failed\n");
        vspace_free_reservation(muslc_this_vspace, reservation);
        goto restore;
    }
    /* create any new pages */
    error = vspace_new_pages_at_vaddr(muslc_this_vspace, new_address + num_pages * PAGE_SIZE_4K, new_pages - num_pages, seL4_PageBits, reservation);
    if (error) {
        LOG_ERROR("Creating new pages for remap region failed\n");
        vspace_unmap_pages(muslc_this_vspace, new_address, num_pages, seL4_PageBits, VSPACE_PRESERVE);
        vspace_free_reservation(muslc_this_vspace, reservation);
        goto restore;
    }
    /* free the reservation book keeping */
    vspace_free_reservation(muslc_this_vspace, reservation);
    return (long)new_address;
restore:
    /* try and recreate the original mapping */
    reservation = vspace_reserve_range_at(muslc_this_vspace, old_address, num_pages * PAGE_SIZE_4K, seL4_AllRights, 1);
    assert(reservation.res);
    error = vspace_map_pages_at_vaddr(muslc_this_vspace, caps, cookies, old_address,
            num_pages, seL4_PageBits, reservation);
    assert(!error);
    return -ENOMEM;
}


static long
sys_mremap_static(va_list ap)
{
    assert(!"not implemented");
    return -ENOMEM;
}

long
sys_mremap(va_list ap)
{
    if (morecore_area != NULL) {
        return sys_mremap_static(ap);
    } else if (muslc_this_vspace != NULL) {
        return sys_mremap_dynamic(ap);
    } else {
        LOG_ERROR("mrepmap requires morecore_area or muslc* vars to be initialised\n");
        assert(morecore_area != NULL || muslc_this_vspace != NULL);
        return 0;
    }
}


#endif
