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
#pragma once

#include <autoconf.h>

#include <vka/vka.h>

#include <vspace/vspace.h>
#include <elf.h>

#if CONFIG_WORD_SIZE == 64
#define Elf_Phdr Elf64_Phdr
#elif CONFIG_WORD_SIZE == 32
#define Elf_Phdr Elf32_Phdr
#else
#error "Word size unsupported"
#endif /* CONFIG_WORD_SIZE */


typedef struct sel4utils_elf_region {
    seL4_CapRights_t rights;
    /* These two vstarts may differ if the elf was not mapped 1to1. Such an elf is not
     * runnable, but allows it to be loaded into a vspace where it is not intended to be run.
     * This is also as reported by the elf file. */
    void *elf_vstart;
    /* Start of the reservation. This will be 4k aligned */
    void *reservation_vstart;
    /* Size of the elf segment as reported by elf file */
    uint32_t size;
    /* Size of the reservation.  This will be a multple of 4k */
    size_t reservation_size;
    reservation_t reservation;
    int cacheable;
    /* Index of this elf segment in the section header */
    int segment_index;
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
                                  vka_t *loader_vka, const char *image_name, sel4utils_elf_region_t *regions, int mapanywhere);

/**
 * Wrapper for sel4utils_elf_load_record_regions. Does not record/perform reservations and
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
                   vka_t *loader_vka, const char *image_name);

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
sel4utils_elf_reserve(vspace_t *loadee, const char *image_name, sel4utils_elf_region_t *regions);

/**
 * Parses an elf file and returns the number of loadable regions. The result of this
 * is used to calculate the number of regions to pass to sel4utils_elf_reserve and
 * sel4utils_elf_load_record_regions
 *
 * @param image_name name of the image in the cpio archive to inspect
 * @return Number of loadable regions in the elf
 */
int
sel4utils_elf_num_regions(const char *image_name);

/**
 * Looks for the __vsyscall section in an elf file and returns the value. This
 * is used to set the __sysinfo value when launching the elf
 *
 * @param image_name name of the image in the cpio archive to inspect
 *
 * @return Address of vsyscall function or 0 if not found
 */
uintptr_t sel4utils_elf_get_vsyscall(const char *image_name);

/**
 * Finds the section_name section in an elf file and returns the address.
 *
 * @param image_name name of the image in the cpio archive to inspect
 *
 * @param section_name name of the section to find
 *
 * @param section_size optional pointer to uint64_t to return the section size
 *
 * @return Address of section or 0 if not found
 */
uintptr_t sel4utils_elf_get_section(const char *image_name, const char *section_name, uint64_t* section_size);

/**
 * Parses an elf file and returns the number of phdrs. The result of this
 * can be used prior to a call to sel4utils_elf_read_phdrs
 *
 * @param image_name name of the image in the cpio archive to inspect
 * @return Number of phdrs in the elf
 */
uint32_t sel4utils_elf_num_phdrs(const char *image_name);

/**
 * Parse an elf file and retrieve all the phdrs
 *
 * @param image_name name of the image in the cpio archive to inspect
 * @param max_phdrs Maximum number of phdrs to retrieve
 * @param phdrs Array to store the loaded phdrs into
 *
 * @return Number of phdrs retrieved
 */
void sel4utils_elf_read_phdrs(const char *image_name, uint32_t max_phdrs, Elf_Phdr *phdrs);

