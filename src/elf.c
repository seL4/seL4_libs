/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#include <autoconf.h>

#if (defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE)

#include <string.h>
#include <sel4/sel4.h>
#include <elf/elf.h>
#include <cpio/cpio.h>
#include <vka/capops.h>
#include <sel4utils/thread.h>
#include <sel4utils/util.h>
#include <sel4utils/mapping.h>
#include <sel4utils/elf.h>

/* This library works with our cpio set up in the build system */
extern char _cpio_archive[];

/*
 * Convert ELF permissions into seL4 permissions.
 *
 * @param permissions elf permissions
 * @return seL4 permissions
 */
static inline seL4_CapRights
rights_from_elf(unsigned long permissions)
{
    seL4_CapRights result = 0;

    if (permissions & PF_R || permissions & PF_X) {
        result |= seL4_CanRead;
    }

    if (permissions & PF_W) {
        result |= seL4_CanWrite;
    }

    return result;
}

static int
load_segment(vspace_t *loadee_vspace, vspace_t *loader_vspace,
             vka_t *loadee_vka, vka_t *loader_vka,
             char *src, size_t segment_size, size_t file_size, uint32_t dst,
             reservation_t reservation)
{
    int error = seL4_NoError;

    if (file_size > segment_size) {
        LOG_ERROR("Error, file_size %zu > segment_size %zu", file_size, segment_size);
        return seL4_InvalidArgument;
    }

    /* create a slot to map a page into the loader address space */
    seL4_CPtr loader_slot;
    cspacepath_t loader_frame_cap;

    error = vka_cspace_alloc(loader_vka, &loader_slot);
    if (error) {
        LOG_ERROR("Failed to allocate cslot by loader vka: %d", error);
        return error;
    }
    vka_cspace_make_path(loader_vka, loader_slot, &loader_frame_cap);

    /* We work a page at a time */
    unsigned int pos = 0;
    while (pos < segment_size && error == seL4_NoError) {
        void *loader_vaddr = 0;
        void *loadee_vaddr = (void *) ((seL4_Word)ROUND_DOWN(dst, PAGE_SIZE_4K));

        /* create and map the frame in the loadee address space */
        error = vspace_new_pages_at_vaddr(loadee_vspace, loadee_vaddr, 1, seL4_PageBits, reservation);
        if (error != seL4_NoError) {
            LOG_ERROR("ERROR: failed to allocate frame by loadee vka: %d", error);
            continue;
        }

        /* copy the frame cap to map into the loader address space */
        cspacepath_t loadee_frame_cap;

        vka_cspace_make_path(loadee_vka, vspace_get_cap(loadee_vspace, loadee_vaddr),
                             &loadee_frame_cap);
        error = vka_cnode_copy(&loader_frame_cap, &loadee_frame_cap, seL4_AllRights);
        if (error != seL4_NoError) {
            LOG_ERROR("ERROR: failed to copy frame cap into loader cspace: %d", error);
            continue;
        }

        /* map the frame into the loader address space */
        loader_vaddr = vspace_map_pages(loader_vspace, &loader_frame_cap.capPtr, NULL, seL4_AllRights,
                                        1, seL4_PageBits, 1);
        if (loader_vaddr == NULL) {
            LOG_ERROR("failed to map frame into loader vspace.");
            error = -1;
            continue;
        }

        /* finally copy the data */
        int nbytes = PAGE_SIZE_4K - (dst & PAGE_MASK_4K);
        if (pos < file_size) {
            memcpy(loader_vaddr + (dst % PAGE_SIZE_4K), (void*)src, MIN(nbytes, file_size - pos));
        }
        /* Note that we don't need to explicitly zero frames as seL4 gives us zero'd frames */

#ifdef CONFIG_ARCH_ARM
        /* Flush the caches */
        seL4_ARM_Page_Unify_Instruction(loader_frame_cap.capPtr, 0, PAGE_SIZE_4K);
        seL4_ARM_Page_Unify_Instruction(loadee_frame_cap.capPtr, 0, PAGE_SIZE_4K);
#endif /* CONFIG_ARCH_ARM */

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

int
sel4utils_elf_num_regions(char *image_name)
{
    int i;
    unsigned long elf_size;
    char *elf_file;
    assert(image_name);
    elf_file = cpio_get_file(_cpio_archive, image_name, &elf_size);
    if (elf_file == NULL) {
        LOG_ERROR("ERROR: failed to load elf file %s", image_name);
        return 0;
    }

    int num_headers = elf_getNumProgramHeaders(elf_file);
    int loadable_headers = 0;

    for (i = 0; i < num_headers; i++) {
        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) == PT_LOAD) {
            loadable_headers++;
        }
    }
    return loadable_headers;
}

static int
make_region(vspace_t *loadee, unsigned long flags, unsigned long segment_size,
            unsigned long vaddr, sel4utils_elf_region_t *region, int anywhere)
{
    region->cacheable = 1;
    region->rights = rights_from_elf(flags);
    region->elf_vstart = (void*)PAGE_ALIGN_4K(vaddr);
    region->size = PAGE_ALIGN_4K(vaddr + segment_size - 1) + PAGE_SIZE_4K - (uint32_t)((seL4_Word)region->elf_vstart);

    if (loadee) {
        if (anywhere) {
            region->reservation = vspace_reserve_range(loadee, region->size, region->rights, 1, (void**)&region->reservation_vstart);
        } else {
            region->reservation_vstart = region->elf_vstart;
            region->reservation = vspace_reserve_range_at(loadee,
                                                          region->elf_vstart,
                                                          region->size,
                                                          region->rights,
                                                          1);
        }
        return !region->reservation.res;
    }

    return 0;
}

void *
sel4utils_elf_reserve(vspace_t *loadee, char *image_name, sel4utils_elf_region_t *regions)
{
    unsigned long elf_size;
    char *elf_file = cpio_get_file(_cpio_archive, image_name, &elf_size);
    if (elf_file == NULL) {
        LOG_ERROR("ERROR: failed to load elf file %s", image_name);
        return NULL;
    }

    int num_headers = elf_getNumProgramHeaders(elf_file);
    int region = 0;

    for (int i = 0; i < num_headers; i++) {
        unsigned long flags, segment_size, vaddr;

        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) == PT_LOAD) {
            /* Fetch information about this segment. */
            segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
            vaddr = elf_getProgramHeaderVaddr(elf_file, i);
            flags = elf_getProgramHeaderFlags(elf_file, i);
            if (make_region(loadee, flags, segment_size, vaddr, &regions[region], 0)) {
                for (region--; region >= 0; region--) {
                    vspace_free_reservation(loadee, regions[region].reservation);
                    regions[region].reservation.res = NULL;
                }
                LOG_ERROR("Failed to create reservation");
                return NULL;
            }
            region++;
        }
    }

    uint64_t entry_point = elf_getEntryPoint(elf_file);
    if ((uint32_t) (entry_point >> 32) != 0) {
        LOG_ERROR("ERROR: this code hasn't been tested for 64bit!");
        return NULL;
    }
    assert(entry_point != 0);
    return (void*)(seL4_Word)entry_point;
}

void *
sel4utils_elf_load_record_regions(vspace_t *loadee, vspace_t *loader, vka_t *loadee_vka, vka_t *loader_vka, char *image_name, sel4utils_elf_region_t *regions, int mapanywhere)
{
    unsigned long elf_size;
    char *elf_file = cpio_get_file(_cpio_archive, image_name, &elf_size);
    if (elf_file == NULL) {
        LOG_ERROR("ERROR: failed to load elf file %s", image_name);
        return NULL;
    }

    int num_headers = elf_getNumProgramHeaders(elf_file);
    int error = seL4_NoError;
    int region_count = 0;

    uint64_t entry_point = elf_getEntryPoint(elf_file);
    if ((uint32_t) (entry_point >> 32) != 0) {
        LOG_ERROR("ERROR: this code hasn't been tested for 64bit!");
        return NULL;
    }
    assert(entry_point != 0);

    for (int i = 0; i < num_headers; i++) {
        char *source_addr;
        unsigned long flags, file_size, segment_size, vaddr;

        /* Skip non-loadable segments (such as debugging data). */
        if (elf_getProgramHeaderType(elf_file, i) == PT_LOAD) {
            /* Fetch information about this segment. */
            source_addr = elf_file + elf_getProgramHeaderOffset(elf_file, i);
            file_size = elf_getProgramHeaderFileSize(elf_file, i);
            segment_size = elf_getProgramHeaderMemorySize(elf_file, i);
            vaddr = elf_getProgramHeaderVaddr(elf_file, i);
            flags = elf_getProgramHeaderFlags(elf_file, i);
            /* make reservation */
            sel4utils_elf_region_t region;
            error = make_region(loadee, flags, segment_size, vaddr, &region, mapanywhere);
            if (error) {
                LOG_ERROR("Failed to reserve region");
                break;
            }
            unsigned long offset = vaddr - PAGE_ALIGN_4K(vaddr);
            /* Copy it across to the vspace */
            LOG_INFO(" * Loading segment %08x-->%08x", (int)vaddr, (int)(vaddr + segment_size));
            error = load_segment(loadee, loader, loadee_vka, loader_vka, source_addr,
                                 segment_size, file_size, offset + (uint32_t)((seL4_Word)region.reservation_vstart), region.reservation);
            if (error) {
                LOG_ERROR("Failed to load segment");
                break;
            }
            /* record the region if requested */
            if (regions) {
                regions[region_count] = region;
            } else {
                vspace_free_reservation(loadee, region.reservation);
            }
            /* increment the count of the number of loaded regions */
            region_count++;
        }
    }

    return error == seL4_NoError ? (void*)(seL4_Word)entry_point : NULL;
}

uintptr_t sel4utils_elf_get_vsyscall(char *image_name)
{
    unsigned long elf_size;
    char *elf_file = cpio_get_file(_cpio_archive, image_name, &elf_size);
    if (elf_file == NULL) {
        LOG_ERROR("ERROR: failed to lookup elf file %s", image_name);
        return 0;
    }
    /* See if we can find the __vsyscall section */
    void *addr = elf_getSectionNamed(elf_file, "__vsyscall");
    if (addr) {
        /* Hope everyting is good and just dereference it */
        return *(uintptr_t*)addr;
    } else {
        return 0;
    }
}

void *
sel4utils_elf_load(vspace_t *loadee, vspace_t *loader, vka_t *loadee_vka, vka_t *loader_vka, char *image_name)
{
    return sel4utils_elf_load_record_regions(loadee, loader, loadee_vka, loader_vka, image_name, NULL, 0);
}

#endif /* (defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE) */
