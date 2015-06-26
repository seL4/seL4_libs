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

#include <simple/simple.h>
#include <sel4platsupport/io.h>
#ifdef ARCH_ARM
#include <platsupport/clock.h>
#include <platsupport/mux.h>
#endif
#include <utils/util.h>
#include <sel4utils/util.h>
#include <sel4utils/mapping.h>

#include <vspace/vspace.h>
#include <vka/capops.h>

#include <stdlib.h>

typedef struct io_mapping {
    /* 1 if we mapped this into the vspace ourselves, or 0
     * if simple just gave us the vaddr */
    int was_mapped;
    /* address we returned to the user */
    void *returned_addr;
    /* base address of the mapping with respect to the vspace */
    void *mapped_addr;
    int num_pages;
    int page_size;
    seL4_CPtr *caps;
    struct io_mapping *next, *prev;
} io_mapping_t;

typedef struct sel4platsupport_io_mapper_cookie {
    vspace_t vspace;
    simple_t simple;
    vka_t vka;
    io_mapping_t *head;
} sel4platsupport_io_mapper_cookie_t;

static io_mapping_t *new_unmapped_node(void *addr)
{
    io_mapping_t *ret = (io_mapping_t*)malloc(sizeof(*ret));
    if (!ret) {
        return NULL;
    }
    *ret = (io_mapping_t) {
        .was_mapped = 0,
         .returned_addr = addr,
          .next = NULL,
           .prev = NULL,
            .caps = NULL,
             .mapped_addr = NULL
    };
    return ret;
}

static void
_insert_node(sel4platsupport_io_mapper_cookie_t *io_mapper, io_mapping_t *node)
{
    node->prev = NULL;
    node->next = io_mapper->head;
    if (io_mapper->head) {
        io_mapper->head->prev = node;
    }
    io_mapper->head = node;
}

static io_mapping_t *
_find_node(sel4platsupport_io_mapper_cookie_t *io_mapper, void *returned_addr)
{
    io_mapping_t *current;
    for (current = io_mapper->head; current; current = current->next) {
        if (current->returned_addr == returned_addr) {
            return current;
        }
    }
    return NULL;
}

static void
_remove_node(sel4platsupport_io_mapper_cookie_t *io_mapper, io_mapping_t *node)
{
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        assert(io_mapper->head == node);
        io_mapper->head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
}

static void
_free_node(io_mapping_t *node)
{
    if (node->caps) {
        free(node->caps);
    }
    free(node);
}

static void
_remove_free_node(sel4platsupport_io_mapper_cookie_t *io_mapper, io_mapping_t *node)
{
    _remove_node(io_mapper, node);
    _free_node(node);
}

static void *
sel4platsupport_map_paddr_with_page_size(sel4platsupport_io_mapper_cookie_t *io_mapper, uintptr_t paddr, size_t size, int page_size_bits, int cached)
{

    vka_t *vka = &io_mapper->vka;
    vspace_t *vspace = &io_mapper->vspace;
    simple_t *simple = &io_mapper->simple;

    /* search at start of page */
    int page_size = BIT(page_size_bits);
    uintptr_t start = ROUND_DOWN(paddr, page_size);
    uint32_t offset = paddr - start;
    size += offset;

    /* calculate number of pages */
    uint32_t num_pages = ROUND_UP(size, page_size) >> page_size_bits;
    assert(num_pages << page_size_bits >= size);

    seL4_CPtr *frames = (seL4_CPtr*)malloc(sizeof(*frames) * num_pages);
    if (!frames) {
        LOG_ERROR("Failed to allocate array of size %d", sizeof(*frames) * num_pages);
        return NULL;
    }
    io_mapping_t *node = (io_mapping_t*)malloc(sizeof(*node));
    if (!node) {
        LOG_ERROR("Failed to malloc of size %d", sizeof(*node));
        free(frames);
        return NULL;
    }

    /* get all of the physical frame caps */
    for (uint32_t i = 0; i < num_pages; i++) {
        /* allocate a cslot */
        int error = vka_cspace_alloc(vka, &frames[i]);
        if (error) {
            LOG_ERROR("cspace alloc failed");
            assert(error == 0);
            /* we don't clean up as everything has gone to hell */
            return NULL;
        }

        /* create a path */
        cspacepath_t path;
        vka_cspace_make_path(vka, frames[i], &path);

        error = simple_get_frame_cap(simple, (void*)start + (i * page_size), page_size_bits, &path);

        if (error) {
            /* free this slot, and then do general cleanup of the rest of the slots.
             * this avoids a needless seL4_CNode_Delete of this slot, as there is no
             * cap in it */
            vka_cspace_free(vka, frames[i]);
            num_pages = i;
            goto error;
        }

    }

    /* Now map the frames in */
    void *vaddr = vspace_map_pages(vspace, frames, NULL, seL4_AllRights, num_pages, page_size_bits, cached);
    if (vaddr) {
        /* fill out and insert the node */
        *node = (io_mapping_t) {
            .mapped_addr = vaddr,
             .returned_addr = vaddr + offset,
              .num_pages = num_pages,
               .page_size = page_size_bits,
                .caps = frames
        };
        _insert_node(io_mapper, node);
        return vaddr + offset;
    }
error:
    for (uint32_t i = 0; i < num_pages; i++) {
        cspacepath_t path;
        vka_cspace_make_path(vka, frames[i], &path);
        vka_cnode_delete(&path);
        vka_cspace_free(vka, frames[i]);
    }
    free(frames);
    free(node);
    return NULL;
}

static void *
sel4platsupport_get_vaddr_with_page_size(sel4platsupport_io_mapper_cookie_t *io_mapper, uintptr_t paddr, size_t size, int page_size_bits)
{
    simple_t *simple = &io_mapper->simple;

    /* search at start of page */
    int page_size = BIT(page_size_bits);
    uintptr_t start = ROUND_DOWN(paddr, page_size);
    uint32_t offset = paddr - start;
    size += offset;

    /* calculate number of pages */
    uint32_t num_pages = ROUND_UP(size, page_size) >> page_size_bits;
    assert(num_pages << page_size_bits >= size);

    void *first_vaddr = simple_get_frame_vaddr(simple, (void*)start, page_size_bits);
    if (!first_vaddr) {
        return NULL;
    }
    for (uint32_t i = 1; i < num_pages; i++) {
        void *vaddr = simple_get_frame_vaddr(simple, (void*)start + (i * page_size), page_size_bits);
        if (first_vaddr + (i * page_size) != vaddr) {
            return NULL;
        }
    }
    io_mapping_t *node = new_unmapped_node(first_vaddr + offset);
    if (!node) {
        LOG_ERROR("Failed to allocate node to track mapping");
        return NULL;
    }
    _insert_node(io_mapper, node);
    return first_vaddr + offset;
}

static void *
sel4platsupport_map_paddr(void *cookie, uintptr_t paddr, size_t size, int cached, ps_mem_flags_t flags)
{
    (void)flags; // we don't support these
    /* The simple interface supports two ways of getting physical addresses.
     * Unfortunately it tends that precisely one of them will actually be
     * implemented.
     * One gives us the cap and we have to map it in, the other gives us the
     * mapped address. We try the getting the cap technique first as that gives
     * us better control since we can ensure the correct caching policy.
     * If that fails then we attempt to get the mappings and ensure that they
     * are in contiguous virtual address if there is more than one.
     * In both cases we will try and use the largest frame size possible */
    sel4platsupport_io_mapper_cookie_t* io_mapper = (sel4platsupport_io_mapper_cookie_t*)cookie;


    int frame_size_index = 0;
    /* find the largest reasonable frame size */
    while (frame_size_index + 1 < NUM_SEL4_PAGE_SIZES) {
        if (size >> sel4_supported_page_sizes[frame_size_index + 1] == 0) {
            break;
        }
        frame_size_index++;
    }

    /* try mapping in this and all smaller frame sizes until something works */
    for (int i = frame_size_index; i >= 0; i--) {
        void *result = sel4platsupport_map_paddr_with_page_size(io_mapper, paddr, size, sel4_supported_page_sizes[i], cached);
        if (result) {
            return result;
        }
    }

    /* try the get_frame_vaddr technique */
    for (int i = frame_size_index; i >= 0; i--) {
        void *result = sel4platsupport_get_vaddr_with_page_size(io_mapper, paddr, size, sel4_supported_page_sizes[i]);
        if (result) {
            return result;
        }
    }

    /* shit out of luck */
    LOG_ERROR("Failed to find a way to map address 0x%x", (uint32_t)paddr);
    return NULL;
}

static void
sel4platsupport_unmap_vaddr(void *cookie, void *vaddr, size_t size)
{
    (void)size;
    sel4platsupport_io_mapper_cookie_t* io_mapper = (sel4platsupport_io_mapper_cookie_t*)cookie;

    io_mapping_t *mapping;

    mapping = _find_node(io_mapper, vaddr);
    if (!mapping) {
        LOG_ERROR("Tried to unmap vaddr 0x%x, which was never mapped in", (uint32_t)vaddr);
        assert(mapping);
        return;
    }
    if (!mapping->was_mapped) {
        /* this vaddr was given directly from simple, so nothing to unmap */
        _remove_free_node(io_mapper, mapping);
        return;
    }

    vspace_t *vspace = &io_mapper->vspace;
    vka_t *vka = &io_mapper->vka;

    vspace_unmap_pages(vspace, mapping->mapped_addr, mapping->num_pages, mapping->page_size,
                       VSPACE_PRESERVE);

    for (int i = 0; i < mapping->num_pages; i++) {
        cspacepath_t path;
        vka_cspace_make_path(vka, mapping->caps[i], &path);
        vka_cnode_delete(&path);
        vka_cspace_free(vka, mapping->caps[i]);
    }

    _remove_free_node(io_mapper, mapping);
}

int
sel4platsupport_new_io_mapper(simple_t simple, vspace_t vspace, vka_t vka, ps_io_mapper_t *io_mapper)
{
    sel4platsupport_io_mapper_cookie_t *cookie;
    cookie = (sel4platsupport_io_mapper_cookie_t*)malloc(sizeof(*cookie));
    if (!cookie) {
        LOG_ERROR("Failed to allocate %d bytes", sizeof(*cookie));
        return -1;
    }
    *cookie = (sel4platsupport_io_mapper_cookie_t) {
        .vspace = vspace,
         .simple = simple,
          .vka = vka
    };
    *io_mapper = (ps_io_mapper_t) {
        .cookie = cookie,
         .io_map_fn = sel4platsupport_map_paddr,
          .io_unmap_fn = sel4platsupport_unmap_vaddr
    };
    return 0;
}

int
sel4platsupport_new_io_ops(simple_t simple, vspace_t vspace, vka_t vka, ps_io_ops_t *io_ops)
{
    int err;
    err = sel4platsupport_new_io_mapper(simple, vspace, vka, &io_ops->io_mapper);
    if (err) {
        return err;
    }
#ifdef ARCH_ARM
    /* We don't consider these as failures */
    err = clock_sys_init(io_ops, &io_ops->clock_sys);
    (void)err;
    err = mux_sys_init(io_ops, &io_ops->mux_sys);
    (void)err;
#endif
    return 0;
}


