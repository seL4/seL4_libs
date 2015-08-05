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
#include <elf.h>
#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <stdarg.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <sel4utils/util.h>
#include <sel4utils/elf.h>
#include <sel4utils/mapping.h>
#include "helpers.h"

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


void
sel4utils_create_word_args(char strings[][WORD_STRING_SIZE], char *argv[], int argc, ...)
{
    va_list args;
    va_start(args, argc);

    for (int i = 0; i < argc; i++) {
        seL4_Word arg = va_arg(args, seL4_Word);
        argv[i] = strings[i];
        snprintf(argv[i], WORD_STRING_SIZE, "%d", arg);

    }

    va_end(args);
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

static int
sel4utils_stack_write(vspace_t *current_vspace, vspace_t *target_vspace,
                      vka_t *vka, void *buf, size_t len, uintptr_t *stack_top)
{
    size_t remaining = len;
    size_t written = 0;
    uintptr_t new_stack_top = (*stack_top) - len;
    uintptr_t current_dest = new_stack_top;
    while (remaining > 0) {
        /* How many can we write on the current page ? */
        size_t towrite = MIN(ROUND_UP(current_dest, PAGE_SIZE_4K) - current_dest, remaining);
        /* Get the cap */
        seL4_CPtr frame = vspace_get_cap(target_vspace, (void*)PAGE_ALIGN_4K(current_dest));
        if (!frame) {
            return -1;
        }
        /* map it in */
        void *mapping = sel4utils_dup_and_map(vka, current_vspace, frame, seL4_PageBits);
        if (!mapping) {
            return -1;
        }
        /* Copy the portion */
        memcpy(mapping + (current_dest % PAGE_SIZE_4K), buf + written, towrite);
        /* Unmap */
        sel4utils_unmap_dup(vka, current_vspace, mapping, seL4_PageBits);
        remaining -= towrite;
        written += towrite;
        current_dest += towrite;
    }
    *stack_top = new_stack_top;
    return 0;
}

static int
sel4utils_stack_write_constant(vspace_t *current_vspace, vspace_t *target_vspace,
                               vka_t *vka, long value, uintptr_t *stack_top)
{
    return sel4utils_stack_write(current_vspace, target_vspace, vka, &value, sizeof(value), stack_top);
}

static int
sel4utils_stack_copy_args(vspace_t *current_vspace, vspace_t *target_vspace,
                          vka_t *vka, int argc, char *argv[], uintptr_t *dest_argv, uintptr_t *stack_top)
{
    int i;
    int error;
    for (i = 0; i < argc; i++) {
        error = sel4utils_stack_write(current_vspace, target_vspace, vka, argv[i], strlen(argv[i]) + 1, stack_top);
        if (error) {
            return error;
        }
        dest_argv[i] = *stack_top;
        *stack_top = ROUND_DOWN(*stack_top, 4);
    }
    return 0;
}

int
sel4utils_spawn_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc,
                        char *argv[], int resume)
{
    uintptr_t stack_top = (uintptr_t)process->thread.stack_top - 4;
    uintptr_t new_process_argv = 0;
    int error;
    /* write all the strings into the stack */
    if (argc > 0) {
        uintptr_t dest_argv[argc];
        /* Copy over the user arguments */
        error = sel4utils_stack_copy_args(vspace, &process->vspace, vka, argc, argv, dest_argv, &stack_top);
        if (error) {
            return -1;
        }
        /* Put the new argv array on as well */
        error = sel4utils_stack_write(vspace, &process->vspace, vka, dest_argv, sizeof(dest_argv), &stack_top);
        if (error) {
            return -1;
        }
        new_process_argv = stack_top;
    }
    /* Some architectures want stack to be double word aligned */
    stack_top = ROUND_DOWN(stack_top, sizeof(int) * 2);

    /* On arm we write the arguments to registers */
#ifdef CONFIG_ARCH_IA32
    /* Write 6 words to make the argument list */
    uint32_t stack_args[6] = {0, (uint32_t)argc, (uint32_t)new_process_argv, process->thread.ipc_buffer, 0, 0};
    error = sel4utils_stack_write(vspace, &process->vspace, vka, stack_args, sizeof(stack_args), &stack_top);
    if (error) {
        return -1;
    }
#endif

    error = sel4utils_internal_start_thread(&process->thread, process->entry_point,
                                            (void *) argc, (void *) new_process_argv, resume, NULL, (void*)stack_top);

    return error;
}

int
sel4utils_spawn_process_v(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc,
                          char *argv[], int resume)
{
    /* define an envp and auxp */
    int envc = 1;
    char ipc_buf_env[30];
    sprintf(ipc_buf_env, "IPCBUFFER=0x%x", process->thread.ipc_buffer_addr);
    char *envp[] = {ipc_buf_env};
    int auxc = process->sysinfo ? 1 : 0;
#if defined(CONFIG_ARCH_IA32) || defined(CONFIG_ARCH_ARM)
    Elf32_auxv_t auxv[] = { {.a_type = AT_SYSINFO, .a_un = {process->sysinfo}}};
#elif defined(CONFIG_X86_64)
    Elf64_auxv_t auxv[] = { {.a_type = AT_SYSINFO, .a_un = {process->sysinfo}}};
#else
#error Not defined
#endif
    seL4_UserContext context;
    memset(&context, 0, sizeof(context));
    /* write all the strings into the stack */
    uintptr_t stack_top = (uintptr_t)process->thread.stack_top - 4;
    uintptr_t dest_argv[argc];
    uintptr_t dest_envp[envc];
    int error;
    /* Copy over the user arguments */
    error = sel4utils_stack_copy_args(vspace, &process->vspace, vka, argc, argv, dest_argv, &stack_top);
    if (error) {
        return -1;
    }
    /* copy the environment */
    error = sel4utils_stack_copy_args(vspace, &process->vspace, vka, envc, envp, dest_envp, &stack_top);
    if (error) {
        return -1;
    }

#if defined(CONFIG_ARCH_IA32) || defined(CONFIG_ARCH_ARM) || defined(CONFIG_X86_64)
    /* construct initial stack frame */
    /* Null terminate aux */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &stack_top);
    if (error) {
        return -1;
    }
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &stack_top);
    if (error) {
        return -1;
    }
    /* write aux */
    error = sel4utils_stack_write(vspace, &process->vspace, vka, auxv, sizeof(auxv[0]) * auxc, &stack_top);
    if (error) {
        return -1;
    }
    /* Null terminate environment */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &stack_top);
    if (error) {
        return -1;
    }
    /* write environment */
    error = sel4utils_stack_write(vspace, &process->vspace, vka, dest_envp, sizeof(dest_envp), &stack_top);
    if (error) {
        return -1;
    }
    /* Null terminate arguments */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &stack_top);
    if (error) {
        return -1;
    }
    /* write arguments */
    error = sel4utils_stack_write(vspace, &process->vspace, vka, dest_argv, sizeof(dest_argv), &stack_top);
    if (error) {
        return -1;
    }
    /* Push argument count */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, argc, &stack_top);
    if (error) {
        return -1;
    }
#else
#error Not implemented yet
#endif

#if defined(CONFIG_ARCH_IA32)
    /* No atexit pointer */
    context.edx = 0;
    context.esp = stack_top;
    context.gs = IPCBUF_GDT_SELECTOR;
    context.eip = (seL4_Word)process->entry_point;
#elif defined(CONFIG_X86_64)
    context.rdx = 0;
    context.rsp = stack_top;
    context.gs = IPCBUF_GDT_SELECTOR;
    context.rip = (seL4_Word)process->entry_point;
#elif defined(CONFIG_ARCH_ARM)
    context.sp = stack_top;
    context.pc = (seL4_Word)process->entry_point;
#else
#error Not implemented yet
#endif

    /* Write the registers */
    error = seL4_TCB_WriteRegisters(process->thread.tcb.cptr, resume, 0, sizeof(context) / sizeof(seL4_Word), &context);
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
        if (region->reservation.res == NULL) {
            LOG_ERROR("Failed to create region\n");
            for (int j = i - 1; j >= 0; j--) {
                vspace_free_reservation(vspace, regions[i].reservation);
            }
            return -1;
        }
    }

    return 0;
}

#ifndef CONFIG_KERNEL_STABLE
static int
assign_asid_pool(seL4_CPtr asid_pool, seL4_CPtr pd)
{

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
              seL4_CapData_t cspace_root_data)
{
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
    process->own_vspace = config.create_vspace;
    if (config.create_vspace) {
        error = vka_alloc_vspace_root(vka, &process->pd);
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

    process->own_cspace = config.create_cspace;
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

        process->sysinfo = sel4utils_elf_get_vsyscall(config.image_name);
    } else {
        process->entry_point = config.entry_point;
        process->sysinfo = config.sysinfo;
    }

    /* create the thread, do this *after* elf-loading so that we don't clobber
     * the required virtual memory*/
    error = sel4utils_configure_thread(vka, spawner_vspace, &process->vspace, SEL4UTILS_ENDPOINT_SLOT,
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
        if (process->vspace.data != 0) {
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
    if (process->own_cspace) {
        vka_free_object(vka, &process->cspace);
    }

    /* tear down the vspace */
    if (process->own_vspace) {
        vspace_tear_down(&process->vspace, VSPACE_FREE);
        /* free any objects created by the vspace */
        clear_objects(process, vka);
    }

    /* destroy the endpoint */
    if (process->fault_endpoint.cptr != 0) {
        vka_free_object(vka, &process->fault_endpoint);
    }

    /* destroy the page directory */
    if (process->own_vspace) {
        vka_free_object(vka, &process->pd);
    }
}

#endif /*(defined CONFIG_LIB_SEL4_VKA && defined CONFIG_LIB_SEL4_VSPACE)*/
