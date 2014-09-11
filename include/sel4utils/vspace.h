/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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
 * Stacks are allocated with 1 guard page between them, but they are continguous in the vmem
 * region.
 *
 * Allocation starts at 0x00001000.
 *
 * This library will allow you to map over anything that it doesn't know about, so
 * make sure you tell it if you do any external mapping.
 *
 */
#ifndef _UTILS_VSPACE_H
#define _UTILS_VSPACE_H

#include <autoconf.h>

#if defined CONFIG_LIB_SEL4_VSPACE && defined CONFIG_LIB_SEL4_VKA

#include <vspace/vspace.h>
#include <vka/vka.h>
#include <sel4utils/util.h>

/* These definitions are only here so that you can take the size of them.
 * TOUCHING THESE DATA STRUCTURES IN ANY WAY WILL BREAK THE WORLD
 * */
#ifdef CONFIG_X86_64
#define VSPACE_LEVEL_BITS 9
#else
#define VSPACE_LEVEL_BITS 10
#endif

#define VSPACE_LEVEL_SIZE BIT(VSPACE_LEVEL_BITS)

typedef struct bottom_level {
    seL4_CPtr bottom_level[VSPACE_LEVEL_SIZE];
    uint32_t  cookies[VSPACE_LEVEL_SIZE];
} bottom_level_t;

typedef int(*sel4utils_map_page_fn)(vspace_t *vspace, seL4_CPtr cap, void *vaddr, seL4_CapRights rights, int cacheable, size_t size_bits);

struct sel4utils_res {
    void *start;
    void *end;
    seL4_CapRights rights;
    int cacheable;
    int malloced;
    struct sel4utils_res *next;
};

typedef struct sel4utils_res sel4utils_res_t;

typedef struct sel4utils_alloc_data {
    seL4_CPtr page_directory;
    vka_t *vka;
    bottom_level_t **top_level;
    void *next_bottom_level_vaddr;
    void *last_allocated;
    vspace_t *bootstrap;
    sel4utils_map_page_fn map_page;
    sel4utils_res_t *reservation_head;
} sel4utils_alloc_data_t;

static inline sel4utils_res_t *
reservation_to_res(reservation_t res) {
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
 * @param page_directory          page directory for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 * @param map_page                Function that will be called to map seL4 pages
 *
 * @return 0 on success.
 */
int
sel4utils_get_vspace_with_map(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
        vka_t *vka, seL4_CPtr page_directory,
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
 * @param page_directory          page directory for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 *
 * @return 0 on success.
 */
int sel4utils_get_vspace(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
        vka_t *vka, seL4_CPtr page_directory, vspace_allocated_object_fn allocated_object_fn,
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
 * @param page_directory          page directory for the new vspace.
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
        seL4_CPtr page_directory, vka_t *vka,
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
 * @param page_directory          page directory for the new vspace.
 * @param allocated_object_fn     function to call when objects are allocated. Can be null.
 * @param allocated_object_cookie cookie passed when the above function is called. Can be null.
 *
 * @return 0 on succes.
 *
 */
int sel4utils_bootstrap_vspace_with_bootinfo(vspace_t *vspace, sel4utils_alloc_data_t *data,
        seL4_CPtr page_directory,
        vka_t *vka, seL4_BootInfo *info, vspace_allocated_object_fn allocated_object_fn,
        void *allocated_object_cookie);

/* Wrapper function that configures a vspaceator such that all allocated objects are not
 * tracked.
 */
static inline int
sel4utils_get_vspace_leaky(vspace_t *loader, vspace_t *new_vspace, sel4utils_alloc_data_t *data,
        vka_t *vka, seL4_CPtr page_directory)
{
    return sel4utils_get_vspace(loader, new_vspace, data, vka, page_directory,
            (vspace_allocated_object_fn) NULL, NULL);
}

#ifdef CONFIG_VTX
static inline int
sel4utils_get_vspace_ept_leaky(vspace_t *loader, vspace_t *new_vspace,
        vka_t *vka, seL4_CPtr page_directory)
{
    return sel4utils_get_vspace_ept(loader, new_vspace, vka, page_directory,
            (vspace_allocated_object_fn) NULL, NULL);
}
#endif /* CONFIG_VTX */

static inline int
sel4utils_bootstrap_vspace_with_bootinfo_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr page_directory,
        vka_t *vka, seL4_BootInfo *info)
{
    return sel4utils_bootstrap_vspace_with_bootinfo(vspace, data, page_directory, vka, info, NULL, NULL);
}

static inline int
sel4utils_bootstrap_vspace_leaky(vspace_t *vspace, sel4utils_alloc_data_t *data, seL4_CPtr page_directory, vka_t *vka,
        void *existing_frames[])
{
    return sel4utils_bootstrap_vspace(vspace, data, page_directory, vka, NULL, NULL, existing_frames);
}

/**
 * Attempts to create a new vspace reservation. Function behaves similarly to vspace_reserve_range
 * except a reservation struct is passed in, instead of being malloc'ed. This is intended to be
 * used during bootstrapping where malloc has not yet been setup.
 * Reservations created with this function should *only* be freed with sel4utils_reserve_range_at_no_alloc
 *
 * @param vspace the virtual memory allocator to use.
 * @param reservation Allocated reservation struct to fill out
 * @param bytes the size in bytes to map.
 * @param rights the rights to map the pages in with in this reservation
 * @param cacheable 1 if the pages should be mapped with cacheable attributes. 0 for DMA.
 * @param vaddr the virtual address of the reserved range will be returned here.
 *
 * @param Returns 0 on success
 */
int sel4utils_reserve_range_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, size_t size,
        seL4_CapRights rights, int cacheable, void **result);

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
 * @param Returns 0 on success
 */
int sel4utils_reserve_range_at_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation, void *vaddr,
        size_t size, seL4_CapRights rights, int cacheable);

/**
 * Frees a reservation. Function behaves similarly to vspace_free_reservation, except
 * it should only be called on reservations that were created with sel4utils_reserve_range_no_alloc
 * and sel4utils_reserve_range_at_no_alloc
 * @param vspace the virtual memory allocator to use.
 * @param reservation the reservation to free.
 */
void sel4utils_free_reservation_no_alloc(vspace_t *vspace, sel4utils_res_t *reservation);

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
void sel4utils_get_image_region(seL4_Word *va_start, seL4_Word *va_end);

#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA */
#endif /* _UTILS_VSPACE_H */
