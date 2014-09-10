/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef SEL4UTILS_ELF_H
#define SEL4UTILS_ELF_H

#include <autoconf.h>

#if (defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE)

#include <vka/vka.h>

#include <vspace/vspace.h>

typedef struct sel4utils_elf_region {
    seL4_CapRights rights;
    /* These two vstarts may differ if the elf was not mapped 1to1. Such an elf is not
     * runnable, but allows it to be loaded into a vspace where it is not intended to be run */
    void *elf_vstart;
    void *reservation_vstart;
    uint32_t size;
    reservation_t reservation;
    int cacheable;
} sel4utils_elf_region_t;

/**
 * Load an elf file into a vspace.
 *
 * The loader vspace and vka allocation will be preserved (no extra cslots or objects or vaddrs
 * will leak from this function), even in the case of an error.
 *
 * The loadee vspace and vka will alter: cslots will be allocated for each frame to be
 * mapped into the address space and frames will be allocated. In case of failure the entire
 * virtual address space is left in the state where it failed.
 *
 * @param loadee the vspace to load the elf file into
 * @param loader the vspace we are loading from
 * @param loadee_vka allocator to use for allocation in the loadee vspace
 * @param loader_vka allocator to use for loader vspace. Can be the same as loadee_vka.
 * @param image_name name of the image in the cpio archive to load.
 * @param regions Optional array for list of regions to be placed. Assumed to be the correct
                  size as reported by a call to sel4utils_elf_num_regions
 * @param mapanywhere If true allows the elf to be loaded anywhere in the vspace. Regions will
                      still be contiguous
 *
 * @return The entry point of the new process, NULL on error
 */
void *
sel4utils_elf_load_record_regions(vspace_t *loadee, vspace_t *loader, vka_t *loadee_vka,
        vka_t *loader_vka, char *image_name, sel4utils_elf_region_t *regions, int mapanywhere);

/**
 * Wrapper for sel4utils_elf_load_record_regions. Does not record/perform resevations and
 * maps into the correct virtual addresses
 *
 * @param loadee the vspace to load the elf file into
 * @param loader the vspace we are loading from
 * @param loadee_vka allocator to use for allocation in the loadee vspace
 * @param loader_vka allocator to use for loader vspace. Can be the same as loadee_vka.
 * @param image_name name of the image in the cpio archive to load.
 *
 * @return The entry point of the new process, NULL on error
 */
void *
sel4utils_elf_load(vspace_t *loadee, vspace_t *loader, vka_t *loadee_vka,
                   vka_t *loader_vka, char *image_name);

/**
 * Parses an elf file but does not actually load it. Merely reserves the regions in the vspace
 * for where the elf segments would go. This is used for lazy loading / copy on write

 * @param loadee the vspace to reserve the elf regions in
 * @param image_name name of the image in the cpio archive to parse.
 * @param regions Array for list of regions to be placed. Assumed to be the correct
                  size as reported by a call to sel4utils_elf_num_regions
 *
 * @return The entry point of the elf, NULL on error
 */
void *
sel4utils_elf_reserve(vspace_t *loadee, char *image_name, sel4utils_elf_region_t *regions);

/**
 * Parses an elf file and returns the number of loadable regions. The result of this
 * is used to calculate the number of regions to pass to sel4utils_elf_reserve and
 * sel4utils_elf_load_record_regions
 *
 * @param image_name name of the image in the cpio archive to inspect
 * @return Number of loadable regions in the elf
 */
int
sel4utils_elf_num_regions(char *image_name);

/**
 * Looks for the __vsyscall section in an elf file and returns the value. This
 * is used to set the __sysinfo value when launching the elf
 *
 * @param image_name name of the image in the cpio archive to inspect
 *
 * @return Address of vsyscall function or 0 if not found
 */
uintptr_t sel4utils_elf_get_vsyscall(char *image_name);

#endif /* (defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE) */
#endif /* SEL4UTILS_ELF_H */
