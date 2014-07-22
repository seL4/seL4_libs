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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <sel4utils/util.h>
#include <sel4utils/elf.h>
#include <sel4utils/mapping.h>
#include "helpers.h"

static size_t
compute_length(int argc, char **argv)
{
    size_t length = 0;

    assert(argv != NULL);
    for (int i = 0; i < argc; i++) {
        assert(argv[i] != NULL);
        length += strlen(argv[i]) + 1;
    }

    return length;
}

static int recurse = 0;

void
sel4utils_allocated_object(void *cookie, vka_object_t object)
{
    if (recurse == 1) {
        assert(!"VSPACE RECURSION ON MALLOC, YOU ARE DEAD\n");
    }
    recurse = 1;

    sel4utils_process_t *process = (sel4utils_process_t *) cookie;

    object_node_t *node = (object_node_t *) malloc(sizeof(object_node_t));
    assert(node != NULL);
    node->object = object;

    node->next = process->allocated_object_list_head;
    process->allocated_object_list_head = node;

    recurse = 0;
}

static void
clear_objects(sel4utils_process_t *process, vka_t *vka)
{
    assert(process != NULL);
    assert(vka != NULL);

    while (process->allocated_object_list_head != NULL) {
        object_node_t *prev = process->allocated_object_list_head;

        process->allocated_object_list_head = prev->next;

        vka_free_object(vka, &prev->object);
        free(prev);
    }
}

static int
next_free_slot(sel4utils_process_t *process, cspacepath_t *dest)
{
    if (process->cspace_next_free >= (1 << process->cspace_size)) {
        LOG_ERROR("Can't allocate slot, cspace is full.\n");
        return -1;
    }

    dest->root = process->cspace.cptr;
    dest->capPtr = process->cspace_next_free;
    dest->capDepth = process->cspace_size;

    return 0;
}

static void 
allocate_next_slot(sel4utils_process_t *process)
{
    assert(process->cspace_next_free < (1 << process->cspace_size));
    process->cspace_next_free++;
}


seL4_CPtr
sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights rights, seL4_CapData_t data)
{
    cspacepath_t dest;
    if (next_free_slot(process, &dest) == -1) {
        return 0;
    }

    int error = vka_cnode_mint(&dest, &src, rights, data);
    if (error != seL4_NoError) {
        LOG_ERROR("Failed to mint cap\n");
        return 0;
    }

    /* success */
    allocate_next_slot(process);
    return dest.capPtr;
}

seL4_CPtr
sel4utils_copy_cap_to_process(sel4utils_process_t *process, cspacepath_t src)
{
    cspacepath_t dest;
    if (next_free_slot(process, &dest) == -1) {
        return 0;
    }

    int error = vka_cnode_copy(&dest, &src, seL4_AllRights);
    if (error != seL4_NoError) {
        LOG_ERROR("Failed to copy cap\n");
        return 0;
    }

    /* success */
    allocate_next_slot(process);
    return dest.capPtr;
}

void *
sel4utils_copy_args(vspace_t *current_vspace, vspace_t *target_vspace,
                    vka_t *vka, int argc, char *argv[])
{
    assert(current_vspace != target_vspace);

    size_t length = compute_length(argc, argv);
    size_t npages UNUSED = BYTES_TO_4K_PAGES(length);
    //TODO handle multiple pages of args. Implement this if you need it.
    assert(npages == 1);

    /* allocate pages in the target address space */
    vka_object_t target_page = {0};
    cspacepath_t target_cap = {0};
    cspacepath_t current_cap = {0};
    void *target_vaddr = NULL;
    void *current_vaddr = NULL;
    seL4_CPtr slot = 0;

    int error = vka_alloc_frame(vka, seL4_PageBits, &target_page);
    vka_cspace_make_path(vka, target_page.cptr, &target_cap);
    if (error) {
        target_page.cptr = 0;
        LOG_ERROR("Failed to allocate frame\n");
        goto error;
    }

    error = vka_cspace_alloc(vka, &slot);
    if (error) {
        slot = 0;
        LOG_ERROR("Failed to allocate cslot\n");
        goto error;
    }

    vka_cspace_make_path(vka, slot, &current_cap);
    error = vka_cnode_copy(&current_cap, &target_cap, seL4_AllRights);
    if (error) {
        LOG_ERROR("Failed to make a copy of frame cap\n");
        current_cap.capPtr = 0;
        goto error;
    }

    /* map the pages */
    current_vaddr = vspace_map_pages(current_vspace, &current_cap.capPtr,
                    seL4_AllRights, 1, seL4_PageBits, 1);
    if (current_vaddr == NULL) {
        LOG_ERROR("Failed to map page into current address space\n");
        goto error;
    }

    target_vaddr = vspace_map_pages(target_vspace, &target_cap.capPtr, seL4_CanRead,
                   1, seL4_PageBits, 1);
    if (target_vaddr == NULL) {
        LOG_ERROR("Failed to map page into target address space\n");
        goto error;
    }

    /* we aren't bailing now - inform caller we allocated a frame */
    vspace_maybe_call_allocated_object(current_vspace, target_page);

    /* do the copy */
    char **current_argv = current_vaddr;
    char *strings = current_vaddr + (argc * sizeof(char*));

    for (int i = 0; i < argc; i++) {
        current_argv[i] = target_vaddr + ((uint32_t) ((seL4_Word)strings) % PAGE_SIZE_4K);
        strncpy(strings, argv[i], strlen(argv[i]) + 1);
        strings += strlen(argv[i]) + 1;
    }

    /* flush & unmap */
#ifdef CONFIG_ARCH_ARM
    seL4_ARM_Page_Unify_Instruction(current_cap.capPtr, 0, PAGE_SIZE_4K);
    seL4_ARM_Page_Unify_Instruction(target_cap.capPtr, 0, PAGE_SIZE_4K);
#endif /* CONFIG_ARCH_ARM */

    vspace_unmap_pages(current_vspace, current_vaddr, 1, seL4_PageBits);

    vka_cnode_delete(&current_cap);
    vka_cspace_free(vka, slot);

    return target_vaddr;

error:
    if (current_vaddr != NULL) {
        vspace_unmap_pages(current_vspace, current_vaddr, 1, seL4_PageBits);
    }

    if (current_cap.capPtr != 0) {
        vka_cnode_delete(&current_cap);
    }

    if (slot != 0) {
        vka_cspace_free(vka, slot);
    }

    if (target_page.cptr != 0) {
        vka_free_object(vka, &target_page);
    }

    return NULL;
}

int
sel4utils_spawn_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc,
        char *argv[], int resume)
{
    char **new_process_argv = NULL;
    if (argc > 0) {
        new_process_argv = sel4utils_copy_args(vspace, &process->vspace, vka, argc, argv);
        if (new_process_argv == NULL) {
            return -1;
        }
    }

    /* On arm we write the arguments to registers */
#ifdef CONFIG_ARCH_IA32
    /* map in the first page of the stack for this thread */
    seL4_CPtr page_cap = vspace_get_cap(&process->vspace, process->thread.stack_top - PAGE_SIZE_4K);

    /* make a copy of the cap */
    seL4_CPtr page_cap_copy;
    int error = vka_cspace_alloc(vka, &page_cap_copy);
    if (error) {
        LOG_ERROR("Failed to allocate cslot\n");
        return error;
    }

    cspacepath_t src, dest;

    vka_cspace_make_path(vka, page_cap, &src);
    vka_cspace_make_path(vka, page_cap_copy, &dest);

    error = vka_cnode_copy(&dest, &src, seL4_AllRights);
    if (error) {
        LOG_ERROR("Failed to copy cap of stack page, error: %d\n", error);
        vka_cspace_free(vka, page_cap_copy);
        return error;
    }

    /* map the page in */
    void *stack = vspace_map_pages(vspace, &page_cap_copy, seL4_AllRights, 1, seL4_PageBits, 1);

    if (stack == NULL) {
        LOG_ERROR("Failed to map stack page into process loading vspace\n");
        vka_cspace_free(vka, page_cap_copy);
        return -1;
    }

    error = sel4utils_internal_start_thread(&process->thread, process->entry_point,
            (void *) ((seL4_Word)argc), (void *) new_process_argv, resume, stack + PAGE_SIZE_4K);

    /* unmap it */
    vspace_free_pages(vspace, stack, 1, seL4_PageBits);
    vka_cnode_delete(&dest);
    vka_cspace_free(vka, page_cap_copy);
#elif CONFIG_ARCH_ARM
    int error = sel4utils_internal_start_thread(&process->thread, process->entry_point,
                (void *) argc, (void *) new_process_argv, resume, process->thread.stack_top);
#endif /* CONFIG_ARCH_IA32 */

    return error;
}

int
sel4utils_configure_process(sel4utils_process_t *process, vka_t *vka,
        vspace_t *vspace, uint8_t priority, char *image_name)
{
    sel4utils_process_config_t config = {
        .is_elf = true,
        .image_name = image_name,
        .do_elf_load = true,
        .create_cspace = true,
        .one_level_cspace_size_bits = CONFIG_SEL4UTILS_CSPACE_SIZE_BITS,
        .create_vspace = true,
        .create_fault_endpoint = true,
        .priority = priority,
#ifndef CONFIG_KERNEL_STABLE
        .asid_pool = seL4_CapInitThreadASIDPool,
#endif 
    };

    return sel4utils_configure_process_custom(process, vka, vspace, config);
}

static int 
create_reservations(vspace_t *vspace, int num, sel4utils_elf_region_t regions[])
{
    for (int i = 0; i < num; i++) {
        sel4utils_elf_region_t *region = &regions[i];
        region->reservation = vspace_reserve_range_at(vspace, region->elf_vstart, 
            region->size, region->rights, region->cacheable);
        if (region == NULL) {
            LOG_ERROR("Failed to create region\n");
            for (int j = i - 1; j >= 0; j--) {
                free(regions[i].reservation);
            }
            return -1;
        }
    }

    return 0;
}

#ifndef CONFIG_KERNEL_STABLE
static int 
assign_asid_pool(seL4_CPtr asid_pool, seL4_CPtr pd) {

    int error;

    if (asid_pool == 0) {
        LOG_INFO("This method will fail if run in a thread that is not in the root server cspace\n");
        asid_pool = seL4_CapInitThreadASIDPool;
    }

    error = seL4_ARCH_ASIDPool_Assign(asid_pool, pd);    
    if (error) {
        LOG_ERROR("Failed to assign asid pool\n");
    }

    return error;
}
#endif

static int
create_cspace(vka_t *vka, int size_bits, sel4utils_process_t *process, 
        seL4_CapData_t cspace_root_data) {
    int error;
    
    /* create a cspace */
    error = vka_alloc_cnode_object(vka, size_bits, &process->cspace);
    if (error) {
        LOG_ERROR("Failed to create cspace: %d\n", error);
        return error;
    }
    
    process->cspace_size = size_bits;
    /* first slot is always 1, never allocate 0 as a cslot */
    process->cspace_next_free = 1;

    /*  mint the cnode cap into the process cspace */
    cspacepath_t src;
    vka_cspace_make_path(vka, process->cspace.cptr, &src);
    error = sel4utils_mint_cap_to_process(process, src, seL4_AllRights, cspace_root_data);
    assert(error == SEL4UTILS_CNODE_SLOT);

    return 0;
}

static int
create_fault_endpoint(vka_t *vka, sel4utils_process_t *process) 
{
    /* create a fault endpoint and put it into the cspace */
    int error = vka_alloc_endpoint(vka, &process->fault_endpoint);
    if (error) {
        LOG_ERROR("Failed to allocate fault endpoint: %d\n", error);
        return error;
    }
   
    cspacepath_t src;
    vka_cspace_make_path(vka, process->fault_endpoint.cptr, &src);
    error = sel4utils_copy_cap_to_process(process, src);

    if (error == 0) {
        LOG_ERROR("Failed to move allocated endpoint: %d\n", error);
        return error;
    }
    assert(error == SEL4UTILS_ENDPOINT_SLOT);

    return 0;
}


int sel4utils_configure_process_custom(sel4utils_process_t *process, vka_t *vka,
        vspace_t *spawner_vspace, sel4utils_process_config_t config)
{
    int error;
    sel4utils_alloc_data_t * data = NULL;
    memset(process, 0, sizeof(sel4utils_process_t));
    seL4_CapData_t cspace_root_data = seL4_CapData_Guard_new(0, 
            seL4_WordBits - config.one_level_cspace_size_bits);

    /* create a page directory */
    if (config.create_vspace) {
        error = vka_alloc_page_directory(vka, &process->pd);
        if (error) {
            LOG_ERROR("Failed to allocate page directory for new process: %d\n", error);
            goto error;
        }

#ifndef CONFIG_KERNEL_STABLE
#ifndef CONFIG_X86_64
        /* assign an asid pool */
        if (assign_asid_pool(config.asid_pool, process->pd.cptr) != seL4_NoError) {
            goto error;
        }
#endif /* end of !CONFIG_X86_64 */
#endif /* CONFIG_KERNEL_STABLE */
    } else {
        process->pd = config.page_dir;
    }

    if (config.create_cspace) {
        if (create_cspace(vka, config.one_level_cspace_size_bits, process, cspace_root_data) != 0) {
            goto error;
        }
    } else {
        process->cspace = config.cnode;
    }

    if (config.create_fault_endpoint) {
        if (create_fault_endpoint(vka, process) != 0) {
            goto error;
        }
    } else {
        process->fault_endpoint = config.fault_endpoint;
    }

    /* create a vspace */
    if (config.create_vspace) {
        sel4utils_get_vspace(spawner_vspace, &process->vspace, &process->data, vka, process->pd.cptr,
                sel4utils_allocated_object, (void *) process);
      
        if (config.num_reservations > 0) {
            if (create_reservations(&process->vspace, config.num_reservations, 
                        config.reservations)) {
                goto error;
            }
        }
    } else {
        memcpy(&process->vspace, config.vspace, sizeof(process->vspace));
    }

    /* finally elf load */
    if (config.is_elf) {
        if (config.do_elf_load) {
            process->entry_point = sel4utils_elf_load(&process->vspace, spawner_vspace, vka, vka, config.image_name);
        } else {
            process->num_elf_regions = sel4utils_elf_num_regions(config.image_name);
            process->elf_regions = calloc(process->num_elf_regions, sizeof(*process->elf_regions));
            if (!process->elf_regions) {
                LOG_ERROR("Failed to allocate memory for elf region information");
                goto error;
            }
            process->entry_point = sel4utils_elf_reserve(&process->vspace, config.image_name, process->elf_regions);
        }
    
        if (process->entry_point == NULL) {
            LOG_ERROR("Failed to load elf file\n");
            goto error;
        }
    } else {
        process->entry_point = config.entry_point;
    }

    /* create the thread, do this *after* elf-loading so that we don't clobber
     * the required virtual memory*/
    error = sel4utils_configure_thread(vka, &process->vspace, SEL4UTILS_ENDPOINT_SLOT,
            config.priority, process->cspace.cptr, cspace_root_data, &process->thread);

    if (error) {
        LOG_ERROR("ERROR: failed to configure thread for new process %d\n", error);
        goto error;
    }

    return 0;

error:
    /* try to clean up */
    if (config.create_fault_endpoint && process->fault_endpoint.cptr != 0) {
        vka_free_object(vka, &process->fault_endpoint);
    }

    if (config.create_cspace && process->cspace.cptr != 0) {
        vka_free_object(vka, &process->cspace);
    }

    if (config.create_vspace && process->pd.cptr != 0) {
        vka_free_object(vka, &process->pd);
        if (process->vspace.page_directory != 0) {
            LOG_ERROR("Could not clean up vspace\n");
        }
    }

    if (process->elf_regions) {
        free(process->elf_regions);
    }

    if (data != NULL) {
        free(data);
    }

    return -1;
}

void
sel4utils_destroy_process(sel4utils_process_t *process, vka_t *vka)
{
    /* delete all of the caps in the cspace */
    for (int i = 1; i < process->cspace_next_free; i++) {
        cspacepath_t path;
        path.root = process->cspace.cptr;
        path.capPtr = i;
        path.capDepth = process->cspace_size;
        vka_cnode_delete(&path);
    }

    /* destroy the thread */
    sel4utils_clean_up_thread(vka, &process->vspace, &process->thread);

    /* destroy the cnode */
    vka_free_object(vka, &process->cspace);
    
    /* free any objects created by the vspace */
    clear_objects(process, vka);

    /* destroy the endpoint */
    if (process->fault_endpoint.cptr != 0) {
        vka_free_object(vka, &process->fault_endpoint);
    }

    /* destroy the page directory */
    vka_free_object(vka, &process->pd);
}

#endif /*(defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE)*/
