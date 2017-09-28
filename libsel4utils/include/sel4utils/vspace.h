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

/*
 * An implementation of the vspace virtual memory allocation interface, using a two
 * level page table.
 *
 * This implementation expects malloc to work, although it only uses it for small amounts of
 * memory.
 *
 * Stack size is constant and be configured in menuconfig.
 *
 * Stacks are allocated with 1 guard page between them, but they are contiguous in the vmem
 * region.
 *
 * Allocation starts at 0x00001000.
 *
 * This library will allow you to map over anything that it doesn't know about, so
 * make sure you tell it if you do any external mapping.
 *
 */
#pragma once

#include <autoconf.h>

#include <vspace/vspace.h>
#include <vka/vka.h>
#include <sel4utils/util.h>
#include <sel4utils/sel4_arch/vspace.h>

/* These definitions are only here so that you can take the size of them.
 * TOUCHING THESE DATA STRUCTURES IN ANY WAY WILL BREAK THE WORLD
 */

#define VSPACE_LEVEL_SIZE BIT(VSPACE_LEVEL_BITS)

typedef struct vspace_mid_level {
    /* there is a clear optimization that could be done where instead of always pointing to a
     * sub table, there is the option of pointing directly to a page. This allows more
     * efficient memory usage for book keeping large pages */
    uintptr_t table[VSPACE_LEVEL_SIZE];
} vspace_mid_level_t;

typedef struct vspace_bottom_level {
    seL4_CPtr cap[VSPACE_LEVEL_SIZE];
    uintptr_t cookie[VSPACE_LEVEL_SIZE];
} vspace_bottom_level_t;

typedef int(*sel4utils_map_page_fn)(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights_t rights, int cacheable, size_t size_bits);

struct sel4utils_res {
    uintptr_t start;
    uintptr_t end;
    seL4_CapRights_t rights;
    int cacheable;
    int malloced;
    struct sel4utils_res *next;
};

typedef struct sel4utils_res sel4utils_res_t;

typedef struct sel4utils_alloc_data {
    seL4_CPtr vspace_root;
    vka_t *vka;
    vspace_mid_level_t *top_level;
    uintptr_t next_bootstrap_vaddr;
    uintptr_t last_allocated;
    vspace_t *bootstrap;
    sel4utils_map_page_fn map_page;
    sel4utils_res_t *reservation_head;
} sel4utils_alloc_data_t;

static inline sel4utils_res_t *
reservation_to_res(reservation_t res)
{
    return (sel4utils_res_t *) res.res;
}

/**
 * This is a mostly internal function for constructing a vspace. Allows a vspace to be created
 * with an arbitrary function to invoke for the mapping of pages. This is useful if you want
 * a vspace manager, but you do not want to use seL4 page directories
 *
 * @param loader                  vspace of the current process, used to allocate
 *                                virtual memory book keeping.
 * @param new_vspace              uninitialised vspace struct to populate.
 * @param data                    uninitialised vspace data struct to populate.
 * @param vka                     initialised vka that this virtual memory allocator will use to
 *                                allocate pages and pagetables. This allocator will never invoke free.
 * @param vspace_root             root object for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 * @param map_page                Function that will be called to map seL4 pages
 *
 * @return 0 on success.
 */
int
sel4utils_get_vspace_with_map(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                              vka_t *vka, seL4_CPtr vspace_root,
                              vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie, sel4utils_map_page_fn map_page);

/**
 * Initialise a vspace allocator for a new address space (not the current one).
 *
 * @param loader                  vspace of the current process, used to allocate
 *                                virtual memory book keeping.
 * @param new_vspace              uninitialised vspace struct to populate.
 * @param data                    uninitialised vspace data struct to populate.
 * @param vka                     initialised vka that this virtual memory allocator will use to
 *                                allocate pages and pagetables. This allocator will never invoke free.
 * @param vspace_root             root object for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 *
 * @return 0 on success.
 */
int sel4utils_get_vspace(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                         vka_t *vka, seL4_CPtr vspace_root, vspace_allocated_object_fn allocated_object_fn,
                         void *allocated_object_cookie);

#ifdef CONFIG_VTX
/**
 * Initialise a vspace allocator for an EPT address space
 *
 * @param loader                  vspace of the current process, used to allocate
 *                                virtual memory book keeping.
 * @param new_vspace              uninitialised vspace struct to populate.
 * @param vka                     initialised vka that this virtual memory allocator will use to
 *                                allocate pages and pagetables. This allocator will never invoke free.
 * @param ept                     EPT page directory for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 *
 * @return 0 on success.
 */
int sel4utils_get_vspace_ept(vspace_t *loader, vspace_t *new_vspace, vka_t *vka,
                             seL4_CPtr ept, vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie);
#endif /* CONFIG_VTX */

/**
 * Initialise a vspace allocator for the current address space (this is intended
 * for use a task that is not the root task but has no vspace, ie one loaded by the capDL loader).
 *
 * @param vspace                  uninitialised vspace struct to populate.
 * @param data                    uninitialised vspace data struct to populate.
 * @param vka                     initialised vka that this virtual memory allocator will use to
 *                                allocate pages and pagetables. This allocator will never invoke free.
 * @param vspace_root             root object for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 * @param existing_frames         a NULL terminated list of virtual addresses for 4K frames that are
 *                                already allocated. For larger frames, just pass in the virtual
 *                                address range in 4K addresses. This will prevent the allocator
 *                                from overriding these frames.
 *
 * @return 0 on succes.
 *
 */
int sel4utils_bootstrap_vspace(vspace_t *vspace, sel4utils_alloc_data_t *data,
                               seL4_CPtr vspace_root, vka_t *vka,
                               vspace_allocated_object_fn allocated_object_fn, void *allocated_object_cookie,
                               void *existing_frames[]);

/**
 * Initialise a vspace allocator for the current address space (this is intended
 * for use by the root task). Take details of existing frames from bootinfo.
 *
 * @param vspace                  uninitialised vspace struct to populate.
 * @param data                    uninitialised vspace data struct to populate.
 * @param vka                     initialised vka that this virtual memory allocator will use to
 *                                allocate pages and pagetables. This allocator will never invoke free.
 * @param info                    seL4 boot info
 * @param vspace_root             root object for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 *
 * @return 0 on succes.
 *
 */
int sel4utils_bootstrap_vspace_with_bootinfo(vspace_t *vspace, sel4utils_alloc_data_t *data,
                                             seL4_CPtr vspace_root,
                                             vka_t *vka, seL4_BootInfo *info, vspace_allocated_object_fn allocated_object_fn,
                                             void *allocated_object_cookie);

/* Wrapper function that configures a vspaceator such that all allocated objects are not
 * tracked.
 */
static inline int
sel4utils_get_vspace_leaky(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
                           vka_t *vka, seL4_CPtr vspace_root)
{
    return sel4utils_get_vspace(loader, new_vspace, data, vka, vspace_root,
                                (vspace_allocated_object_fn) NULL, NULL);
}

#ifdef CONFIG_VTX
static inline int
sel4utils_get_vspace_ept_leaky(vspace_t *loader, vspace_t *new_vspace,
                               vka_t *vka, seL4_CPtr vspace_root)
{
    return sel4utils_get_vspace_ept(loader, new_vspace, vka, vspace_root,
                                    (vspace_allocated_object_fn) NULL, NULL);
}
#endif /* CONFIG_VTX */

static inline int
sel4utils_bootstrap_vspace_with_bootinfo_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr vspace_root,
                                               vka_t *vka, seL4_BootInfo *info)
{
    return sel4utils_bootstrap_vspace_with_bootinfo(vspace, data, vspace_root, vka, info, NULL, NULL);
}

static inline int
sel4utils_bootstrap_vspace_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr vspace_root, vka_t *vka,
                                 void *existing_frames[])
{
    return sel4utils_bootstrap_vspace(vspace, data, vspace_root, vka, NULL, NULL, existing_frames);
}

/**
 * Attempts to create a new vspace reservation. Function behaves similarly to vspace_reserve_range
 * except a reservation struct is passed in, instead of being malloc'ed. This is intended to be
 * used during bootstrapping where malloc has not yet been setup.
 * Reservations created with this function should *only* be freed with sel4utils_reserve_range_at_no_alloc
 *
 * Result will be aligned to 4K.
 *
 * @param vspace the virtual memory allocator to use.
 * @param reservation Allocated reservation struct to fill out
 * @param bytes the size in bytes to map.
 * @param rights the rights to map the pages in with in this reservation
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 * @param vaddr the virtual address of the reserved range will be returned here.
 *
 * @return 0 on success
 */
int sel4utils_reserve_range_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, size_t size,
                                     seL4_CapRights_t rights, int cacheable, void **result);

/*
 * @see sel4utils_reserve_range_no_alloc, however result is aligned to size_bits.
 */
int sel4utils_reserve_range_no_alloc_aligned(vspace_t *vspace, sel4utils_res_t *reservation,
                                             size_t size, size_t size_bits, seL4_CapRights_t rights, int cacheable, void **result);

/**
 * Attempts to create a new vspace reservation. Function behaves similarly to vspace_reserve_range_at
 * except a reservation struct is passed in, instead of being malloc'ed. This is intended to be
 * used during bootstrapping where malloc has not yet been setup.
 * Reservations created with this function should *only* be freed with sel4utils_reserve_range_at_no_alloc
 *
 * @param vspace the virtual memory allocator to use.
 * @param reservation Allocated reservation struct to fill out
 * @param vaddr the virtual address to start the range at.
 * @param bytes the size in bytes to map.
 * @param rights the rights to map the pages in with in this reservatio
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 *
 * @return 0 on success
 */
int sel4utils_reserve_range_at_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, void *vaddr,
                                        size_t size, seL4_CapRights_t rights, int cacheable);

/**
 * Move and/or resizes a reservation in any direction, allowing for both start and/or end address to
 * be changed. Any overlapping region will keep their reservation, any non-overlapping regions will
 * be unreserved. Does not make any mapping changes.
 * @param vspace the virtual memory allocator to use.
 * @param reservation the reservation to move.
 * @param vaddr the start virtual address to move the reservation to.
 * @param bytes the size in bytes of new reservation.
 * @return 0 on success, -1 if region start could not be moved, -2 if end could not be moved.
 */
int sel4utils_move_resize_reservation(vspace_t *vspace, reservation_t reservation, void *vaddr,
                                      size_t bytes);

/*
 * Copy the code and data segment (the image effectively) from current vspace
 * into clone vspace. The clone vspace should be initialised.
 *
 * @param current the vspace to copy from.
 * @param clone the vspace to copy to.
 * @param reservation the previously established reservation in clone to copy.
 * @return 0 on success.
 */
int sel4utils_bootstrap_clone_into_vspace(vspace_t *current, vspace_t *clone, reservation_t reserve);

/**
 * Get the bounds of _executable_start and _end.
 *
 * @param va_start return va_start.
 * @param va_end return va_end.
 */
void sel4utils_get_image_region(uintptr_t *va_start, uintptr_t *va_end);

/**
 *
 * @return the physical address that vaddr is mapped to.
 *         VKA_NO_PADDR if there is no mapping
 */
uintptr_t sel4utils_get_paddr(vspace_t *vspace, void *vaddr, seL4_Word type, seL4_Word size_bits);

