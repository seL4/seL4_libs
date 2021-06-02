/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>
#include <sel4utils/gen_config.h>

#include <string.h>
#include <sel4/sel4.h>
#include <elf/elf.h>
#include <vka/capops.h>
#include <sel4utils/thread.h>
#include <sel4utils/util.h>
#include <sel4utils/mapping.h>
#include <sel4utils/elf.h>

/*
 * Convert ELF permissions into seL4 permissions.
 *
 * @param permissions elf permissions
 * @return seL4 permissions
 */
static inline seL4_CapRights_t rights_from_elf(unsigned long permissions)
{
    bool canRead = permissions & PF_R || permissions & PF_X;
    bool canWrite = permissions & PF_W;

    return seL4_CapRights_new(false, false, canRead, canWrite);
}

static int load_segment(vspace_t *loadee_vspace, vspace_t *loader_vspace,
                        vka_t *loadee_vka, vka_t *loader_vka,
                        const char *src, size_t file_size, int num_regions,
                        sel4utils_elf_region_t regions[num_regions], int region_index)
{
    int error = seL4_NoError;
    sel4utils_elf_region_t region = regions[region_index];
    size_t segment_size = region.size;
    uintptr_t dst = (uintptr_t) region.elf_vstart;
    if (file_size > segment_size) {
        ZF_LOGE("Error, file_size %zu > segment_size %zu", file_size, segment_size);
        return seL4_InvalidArgument;
    }

    /* create a slot to map a page into the loader address space */
    seL4_CPtr loader_slot;
    cspacepath_t loader_frame_cap;

    error = vka_cspace_alloc(loader_vka, &loader_slot);
    if (error) {
        ZF_LOGE("Failed to allocate cslot by loader vka: %d", error);
        return error;
    }
    vka_cspace_make_path(loader_vka, loader_slot, &loader_frame_cap);

    /* We work a page at a time */
    unsigned int pos = 0;
    while (pos < segment_size && error == seL4_NoError) {
        void *loader_vaddr = 0;
        void *loadee_vaddr = (void *)((seL4_Word)ROUND_DOWN(dst, PAGE_SIZE_4K));

        /* Find the reservation that this frame belongs to.
         * The reservation may belong to an adjacent region */
        reservation_t reservation;
        if (loadee_vaddr < region.reservation_vstart) {
            //  Have to use reservation from adjacent region
            if ((region_index - 1) < 0) {
                ZF_LOGE("Invalid regions: bad elf file.");
                error = seL4_InvalidArgument;
                continue;
            }
            reservation = regions[region_index - 1].reservation;
        } else if (loadee_vaddr + (MIN(segment_size - pos, PAGE_SIZE_4K)) >
                   (region.reservation_vstart + region.reservation_size)) {
            if ((region_index + 1) >= num_regions) {
                ZF_LOGE("Invalid regions: bad elf file.");
                error = seL4_InvalidArgument;
                continue;
            }
            reservation = regions[region_index + 1].reservation;
        } else {
            reservation = region.reservation;
        }

        /* We need to check if the frame has already been mapped by another region.
         * Currently this check is done on every frame because it is assumed to be cheap. */
        seL4_CPtr cap = vspace_get_cap(loadee_vspace, loadee_vaddr);
        if (cap == seL4_CapNull) {
            error = vspace_new_pages_at_vaddr(loadee_vspace, loadee_vaddr, 1, seL4_PageBits, reservation);
        }

        if (error != seL4_NoError) {
            ZF_LOGE("ERROR: failed to allocate frame by loadee vka: %d", error);
            continue;
        }

        /* copy the frame cap to map into the loader address space */
        cspacepath_t loadee_frame_cap;

        vka_cspace_make_path(loadee_vka, vspace_get_cap(loadee_vspace, loadee_vaddr),
                             &loadee_frame_cap);
        error = vka_cnode_copy(&loader_frame_cap, &loadee_frame_cap, seL4_AllRights);
        if (error != seL4_NoError) {
            ZF_LOGE("ERROR: failed to copy frame cap into loader cspace: %d", error);
            continue;
        }

        /* map the frame into the loader address space */
        loader_vaddr = vspace_map_pages(loader_vspace, &loader_frame_cap.capPtr, NULL, seL4_AllRights,
                                        1, seL4_PageBits, 1);
        if (loader_vaddr == NULL) {
            ZF_LOGE("failed to map frame into loader vspace.");
            error = -1;
            continue;
        }

        /* finally copy the data */
        int nbytes = PAGE_SIZE_4K - (dst & PAGE_MASK_4K);
        if (pos < file_size) {
            memcpy(loader_vaddr + (dst % PAGE_SIZE_4K), (void *)src, MIN(nbytes, file_size - pos));
        }
        /* Note that we don't need to explicitly zero frames as seL4 gives us zero'd frames */

#ifdef CONFIG_ARCH_ARM
        /* Flush the caches */
        seL4_ARM_Page_Unify_Instruction(loader_frame_cap.capPtr, 0, PAGE_SIZE_4K);
        seL4_ARM_Page_Unify_Instruction(loadee_frame_cap.capPtr, 0, PAGE_SIZE_4K);
#elif CONFIG_ARCH_RISCV
        /* Ensure that the writes to memory that may be executed become visible */
        asm volatile("fence.i" ::: "memory");
#endif

        /* now unmap the page in the loader address space */
        vspace_unmap_pages(loader_vspace, (void *) loader_vaddr, 1, seL4_PageBits, VSPACE_PRESERVE);
        vka_cnode_delete(&loader_frame_cap);

        pos += nbytes;
        dst += nbytes;
        src += nbytes;
    }

    /* clear the cslot */
    vka_cspace_free(loader_vka, loader_frame_cap.capPtr);

    return error;
}

/**
 * Load an array of regions into a vspace.
 *
 * The region array passed in won't be mutated by this function or functions it calls.
 * State in the vspaces and vkas will be mutated to track resources used.
 * If this function fails, any allocated and mapped frames will not be freed.
 *
 * @param loadee_vspace target vspace to map frames into.
 * @param loader_vspace vspace of the caller.  Frames are temporarily mapped into this to init with
 *                      elf data from elf file.
 * @param loadee_vka target vka
 * @param loader_vka caller vka
 * @param elf_file pointer to elf object
 * @param num_regions total number of segments/regions to load.
 * @param regions region array containing segment info.
 *
 * @return 0 on success.
 */
static int load_segments(vspace_t *loadee_vspace, vspace_t *loader_vspace,
                         vka_t *loadee_vka, vka_t *loader_vka, const elf_t *elf_file,
                         int num_regions, sel4utils_elf_region_t regions[num_regions])
{
    for (int i = 0; i < num_regions; i++) {
        int segment_index = regions[i].segment_index;
        const char *source_addr = elf_getProgramSegment(elf_file, segment_index);
        if (source_addr == NULL) {
            return 1;
        }
        size_t file_size = elf_getProgramHeaderFileSize(elf_file, segment_index);

        int error = load_segment(loadee_vspace, loader_vspace, loadee_vka, loader_vka,
                                 source_addr, file_size, num_regions, regions, i);
        if (error) {
            return error;
        }
    }
    return 0;
}

static bool is_loadable_section(const elf_t *elf_file, int index)
{
    return elf_getProgramHeaderType(elf_file, index) == PT_LOAD;
}

static int count_loadable_regions(const elf_t *elf_file)
{
    int num_headers = elf_getNumProgramHeaders(elf_file);
    int loadable_headers = 0;

    for (int i = 0; i < num_headers; i++) {
        /* Skip non-loadable segments (such as debugging data). */
        if (is_loadable_section(elf_file, i)) {
            loadable_headers++;
        }
    }
    return loadable_headers;
}

int sel4utils_elf_num_regions(const elf_t *elf_file)
{
    return count_loadable_regions(elf_file);
}

/**
 * Create reservations for regions in a target vspace.
 *
 * The region position and size fields should have already been calculated by prepare_reservations.
 *
 * @param loadee the vspace to load into.
 * @param total_regions the size of the regions array
 * @param regions the array of regions.
 * @param anywhere some legacy parameter that throws away the vspace address.  It is supposedly to support
                   loading a segment for inspection rather than execution.
 *
 * @return 0 on success.
 */
static int create_reservations(vspace_t *loadee, size_t total_regions, sel4utils_elf_region_t regions[total_regions],
                               int anywhere)
{
    for (int i = 0; i < total_regions; i++) {
        if (regions[i].reservation_size == 0) {
            ZF_LOGD("Empty reservation detected. This should indicate that this segments"
                    "data is entirely stored in other section reservations.");
            continue;
        }
        if (anywhere) {
            regions[i].reservation = vspace_reserve_range(loadee, regions[i].reservation_size,
                                                          regions[i].rights, regions[i].cacheable, (void **)&regions[i].reservation_vstart);
        } else {
            regions[i].reservation = vspace_reserve_range_at(loadee,
                                                             regions[i].reservation_vstart,
                                                             regions[i].reservation_size,
                                                             regions[i].rights,
                                                             regions[i].cacheable);
        }
        if (regions[i].reservation.res == NULL) {
            ZF_LOGE("Failed to make reservation: %p, %zd", regions[i].reservation_vstart, regions[i].reservation_size);
            return -1;
        }
    }
    return 0;

}

/**
 * Function for deciding whether a frame needs to be moved to a different reservation.
 *
 * Mapping permissions are set for a whole reservation.  With adjacent segments of different
 * permissions, we need to give the shared frame mapping to the reservation with the more permissive
 * permissions.  Currently this assumes that every region will have read permissions, and the frame
 * only needs to be moved if the lower region doesn't have write permissions and the upper one does.
 *
 * @param a CapRights for reservation a.
 * @param b CapRights for reservation b.
 * @param result whether the frame should be moved.
 *
 * @return 0 on success.
 */
static int cap_writes_check_move_frame(seL4_CapRights_t a, seL4_CapRights_t b, bool *result)
{
    if (!seL4_CapRights_get_capAllowRead(a) || !seL4_CapRights_get_capAllowRead(b)) {
        ZF_LOGE("Regions do not have read rights.");
        return -1;
    }
    if (!seL4_CapRights_get_capAllowWrite(a) && seL4_CapRights_get_capAllowWrite(b)) {
        *result = true;
        return 0;
    }
    *result = false;
    return 0;
}

/**
 * Prepares a list of regions to have reservations reserved by a vspace.
 *
 * Iterates through a region array in ascending order.
 * For each region it tries to place a reservation that contains the segment.
 * Reservations are rounded up to 4k alignments.  This means that the previous reservation
 * may overlap with the start of the current region.  When this occurs, the last frame of the
 * previous reservation may need to be moved to the current reservation.  This is decided based
 * on the reservation permissions by the cap_writes_check_move_frame function.
 * If the frame doesn't need to be moved, then this reservation starts from the first unreserved
 * frame of the segment.  When the segment is eventually loaded, the frames may need to be mapped
 * from from other segment's reservations.
 *
 * @param total_regions total number of regions in array.
 * @param regions array of regions sorted in ascending order.
 *
 * @return 0 on success.
 */
static int prepare_reservations(size_t total_regions, sel4utils_elf_region_t regions[total_regions])
{
    uintptr_t prev_res_start = 0;
    size_t prev_res_size = 0;
    seL4_CapRights_t prev_rights = seL4_NoRights;
    for (int i = 0; i < total_regions; i++) {
        uintptr_t current_res_start = PAGE_ALIGN_4K((uintptr_t)regions[i].elf_vstart);
        uintptr_t current_res_top = ROUND_UP((uintptr_t)regions[i].elf_vstart + regions[i].size, PAGE_SIZE_4K);
        size_t current_res_size = current_res_top - current_res_start;
        assert(current_res_size % PAGE_SIZE_4K == 0);
        seL4_CapRights_t current_rights = regions[i].rights;

        if ((prev_res_start + prev_res_size) > current_res_start) {
            /* This segment shares a frame with the previous segment */
            bool should_move;
            int error = cap_writes_check_move_frame(prev_rights, current_rights, &should_move);
            if (error) {
                /* Comparator function failed. Return error. */
                return -1;
            }
            if (should_move) {
                /* Frame needs to be moved from the last reservation into this one */
                ZF_LOGF_IF(i == 0, "Should not need to adjust first element in list");
                ZF_LOGF_IF(regions[i - 1].reservation_size < PAGE_SIZE_4K, "Invalid previous region");
                regions[i - 1].reservation_size -= PAGE_SIZE_4K;
            } else {
                /* Frame stays in previous reservation and we update our reservation start address and size */
                current_res_start = ROUND_UP((prev_res_start + prev_res_size) + 1, PAGE_SIZE_4K);
                current_res_size = current_res_top - current_res_start;
                ZF_LOGF_IF(ROUND_UP(regions[i].size, PAGE_SIZE_4K) - current_res_size == PAGE_SIZE_4K,
                           "Regions shouldn't overlap by more than a single 4k frame");
            }
        }
        /* Record this reservation layout */
        regions[i].reservation_size = current_res_size;
        regions[i].reservation_vstart = (void *)current_res_start;
        prev_res_size = current_res_size;
        prev_res_start = current_res_start;
        prev_rights = current_rights;
    }
    return 0;
}

/**
 * Reads segment data out of elf file and creates region list.
 *
 * The region array must have the correct size as calculated by count_loadable_regions.
 *
 * @param elf_file pointer to start of elf_file
 * @param total_regions total number of loadable segments.
 * @param regions array of regions.
 *
 * @return 0 on success.
 */
static int read_regions(const elf_t *elf_file, size_t total_regions, sel4utils_elf_region_t regions[total_regions])
{
    int num_headers = elf_getNumProgramHeaders(elf_file);
    int region_id = 0;
    for (int i = 0; i < num_headers; i++) {

        /* Skip non-loadable segments (such as debugging data). */
        if (is_loadable_section(elf_file, i)) {
            sel4utils_elf_region_t *region = &regions[region_id];
            /* Fetch information about this segment. */

            region->cacheable = 1;
            region->rights = rights_from_elf(elf_getProgramHeaderFlags(elf_file, i));
            // elf_getProgramHeaderMemorySize should just return `uintptr_t`
            region->elf_vstart = (void *) elf_getProgramHeaderVaddr(elf_file, i);
            region->size = elf_getProgramHeaderMemorySize(elf_file, i);
            region->segment_index = i;
            region_id++;
        }
    }
    if (region_id != total_regions) {
        ZF_LOGE("Did not correctly read all regions.");
        return 1;
    }

    return 0;
}

/**
 * Compare function for ordering regions. Passed to sglib quick sort.
 *
 * Sort is based on base address.  This assumes segments do not overlap.
 * There is likely a chance that quick sort won't terminate if segments overlap.
 *
 * @param a region a
 * @param b region b
 *
 * @return 1 for a > b, -1 for b > a
 */
static int compare_regions(sel4utils_elf_region_t a, sel4utils_elf_region_t b)
{
    if (a.elf_vstart + a.size <= b.elf_vstart) {
        return -1;
    } else if (b.elf_vstart + b.size <= a.elf_vstart) {
        return 1;
    } else {
        ZF_LOGF("Bad elf file: segments overlap");
        return 0;
    }
}

/**
 * Parse an elf file and create reservations in a target vspace for all loadable segments.
 *
 * Reads segment layout data out of elf file and stores in elf_region array.
 * Then sorts the array, then plans reservations based on segment layout.
 * Finally creates reservations in vspace.
 *
 * @param loadee vspace to create reservations in.
 * @param elf_file pointer to elf file.
 * @param num_regions number of regions in array as calculated by count_loadable_regions.
 * @param regions region array.
 * @param mapanywhere throw away vspace positioning if set to 1.
 *
 * @return 0 on success.
 */
static int elf_reserve_regions_in_vspace(vspace_t *loadee, const elf_t *elf_file,
                                         int num_regions, sel4utils_elf_region_t regions[num_regions], int mapanywhere)
{
    int error = read_regions(elf_file, num_regions, regions);
    if (error) {
        ZF_LOGE("Failed to read regions");
        return error;
    }

    /* Sort region list */
    SGLIB_ARRAY_SINGLE_QUICK_SORT(sel4utils_elf_region_t, regions, num_regions, compare_regions);

    error = prepare_reservations(num_regions, regions);
    if (error) {
        ZF_LOGE("Failed to prepare reservations");
        return error;
    }
    error = create_reservations(loadee, num_regions, regions, mapanywhere);
    if (error) {
        ZF_LOGE("Failed to create reservations");
        return error;
    }
    return 0;
}

static void *entry_point(const elf_t *elf_file)
{
    uint64_t entry_point = elf_getEntryPoint(elf_file);
    if ((uint32_t)(entry_point >> 32) != 0) {
        ZF_LOGE("ERROR: this code hasn't been tested for 64bit!");
        return NULL;
    }
    assert(entry_point != 0);
    return (void *)(seL4_Word)entry_point;

}


void *sel4utils_elf_reserve(vspace_t *loadee, const elf_t *elf_file, sel4utils_elf_region_t *regions)
{
    /* Count number of loadable segments */
    int num_regions = count_loadable_regions(elf_file);

    /* Create reservations in vspace using internal functions */
    int error = elf_reserve_regions_in_vspace(loadee, elf_file, num_regions, regions, 0);
    if (error) {
        ZF_LOGE("Failed to reserve regions");
        return NULL;
    }

    /* Return entry point */
    return entry_point(elf_file);
}

void *sel4utils_elf_load_record_regions(vspace_t *loadee, vspace_t *loader, vka_t *loadee_vka, vka_t *loader_vka,
                                        const elf_t *elf_file, sel4utils_elf_region_t *regions, int mapanywhere)
{
    /* Calculate number of loadable regions.  Use stack array if one wasn't passed in */
    int num_regions = count_loadable_regions(elf_file);
    bool clear_at_end = false;
    sel4utils_elf_region_t stack_regions[num_regions];
    if (regions == NULL) {
        regions = stack_regions;
        clear_at_end = true;
    }

    /* Create reservations */
    int error = elf_reserve_regions_in_vspace(loadee, elf_file, num_regions, regions, mapanywhere);
    if (error) {
        ZF_LOGE("Failed to reserve regions");
        return NULL;
    }

    /* Load Map reservations and load in elf data */
    error = load_segments(loadee, loader, loadee_vka, loader_vka, elf_file, num_regions, regions);
    if (error) {
        ZF_LOGE("Failed to load segments");
        return NULL;
    }

    /* Clean up reservations if we allocated our own array */
    if (clear_at_end) {
        for (int i = 0; i < num_regions; i++) {
            if (regions[i].reservation_size > 0) {
                vspace_free_reservation(loadee, regions[i].reservation);
            }
        }
    }

    /* Return entry point */
    return entry_point(elf_file);
}

uintptr_t sel4utils_elf_get_vsyscall(const elf_t *elf_file)
{
    uintptr_t *addr = (uintptr_t *)sel4utils_elf_get_section(elf_file, "__vsyscall", NULL);
    if (addr) {
        return *addr;
    } else {
        return 0;
    }
}

uintptr_t sel4utils_elf_get_section(const elf_t *elf_file, const char *section_name, uint64_t *section_size)
{
    /* See if we can find the section */
    size_t section_id;
    const void *addr = elf_getSectionNamed(elf_file, section_name, &section_id);
    if (addr) {
        if (section_size != NULL) {
            *section_size = elf_getSectionSize(elf_file, section_id);
        }
        return (uintptr_t) addr;
    } else {
        return 0;
    }
}

void *sel4utils_elf_load(vspace_t *loadee, vspace_t *loader, vka_t *loadee_vka, vka_t *loader_vka,
                         const elf_t *elf_file)
{
    return sel4utils_elf_load_record_regions(loadee, loader, loadee_vka, loader_vka, elf_file, NULL, 0);
}

uint32_t sel4utils_elf_num_phdrs(const elf_t *elf_file)
{
    return elf_getNumProgramHeaders(elf_file);
}

void sel4utils_elf_read_phdrs(const elf_t *elf_file, size_t max_phdrs, Elf_Phdr *phdrs)
{
    size_t num_phdrs = elf_getNumProgramHeaders(elf_file);
    for (size_t i = 0; i < num_phdrs && i < max_phdrs; i++) {
        phdrs[i] = (Elf_Phdr) {
            .p_type = elf_getProgramHeaderType(elf_file, i),
            .p_offset = elf_getProgramHeaderOffset(elf_file, i),
            .p_vaddr = elf_getProgramHeaderVaddr(elf_file, i),
            .p_paddr = elf_getProgramHeaderPaddr(elf_file, i),
            .p_filesz = elf_getProgramHeaderFileSize(elf_file, i),
            .p_memsz = elf_getProgramHeaderMemorySize(elf_file, i),
            .p_flags = elf_getProgramHeaderFlags(elf_file, i),
            .p_align = elf_getProgramHeaderAlign(elf_file, i)
        };
    }
}
