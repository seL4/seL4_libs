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
#include <autoconf.h>

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
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
#include <sel4utils/helpers.h>

void
sel4utils_allocated_object(void *cookie, vka_object_t object)
{
    static bool recurse = false;

    if (recurse) {
        ZF_LOGF("VSPACE RECURSION ON MALLOC, YOU ARE DEAD\n");
    }
    recurse = true;

    sel4utils_process_t *process = cookie;

    object_node_t *node = malloc(sizeof(object_node_t));
    assert(node != NULL);
    node->object = object;

    node->next = process->allocated_object_list_head;
    process->allocated_object_list_head = node;

    recurse = false;
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
    if (process->cspace_next_free >= (BIT(process->cspace_size))) {
        ZF_LOGE("Can't allocate slot, cspace is full.\n");
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
    assert(process->cspace_next_free < (BIT(process->cspace_size)));
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
        snprintf(argv[i], WORD_STRING_SIZE, "%"PRIuPTR"", arg);

    }

    va_end(args);
}

seL4_CPtr
sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights_t rights, seL4_Word data)
{
    cspacepath_t dest = { 0 };
    if (next_free_slot(process, &dest) == -1) {
        return 0;
    }

    int error = vka_cnode_mint(&dest, &src, rights, data);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to mint cap\n");
        return 0;
    }

    /* success */
    allocate_next_slot(process);
    return dest.capPtr;
}

seL4_CPtr
sel4utils_copy_path_to_process(sel4utils_process_t *process, cspacepath_t src)
{
    cspacepath_t dest = { 0 };
    if (next_free_slot(process, &dest) == -1) {
        return 0;
    }

    int error = vka_cnode_copy(&dest, &src, seL4_AllRights);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to copy cap\n");
        return 0;
    }

    /* success */
    allocate_next_slot(process);
    return dest.capPtr;
}

/* copy a cap to a process, returning the cptr in the process' cspace */
seL4_CPtr
sel4utils_copy_cap_to_process(sel4utils_process_t *process, vka_t *vka, seL4_CPtr cap)
{
    cspacepath_t path;
    vka_cspace_make_path(vka, cap, &path);
    return sel4utils_copy_path_to_process(process, path);
}

seL4_CPtr
sel4utils_move_cap_to_process(sel4utils_process_t *process, cspacepath_t src, vka_t *from_vka)
{
    cspacepath_t dest = { 0 };
    if (next_free_slot(process, &dest) == -1) {
        return 0;
    }

    int error = vka_cnode_move(&dest, &src);
    if (error != seL4_NoError) {
        ZF_LOGE("Failed to move cap\n");
        return 0;
    }
    /* If we have been passed a vka to free a cspace slot */
    if (from_vka != NULL) {
        /* free slot in previous vka */
        vka_cspace_free(from_vka, src.capPtr);
    }

    /* success */
    allocate_next_slot(process);
    return dest.capPtr;
}
int
sel4utils_stack_write(vspace_t *current_vspace, vspace_t *target_vspace,
                      vka_t *vka, void *buf, size_t len, uintptr_t *initial_stack_pointer)
{
    size_t remaining = len;
    size_t written = 0;
    uintptr_t new_stack_pointer = (*initial_stack_pointer) - len;
    uintptr_t current_dest = new_stack_pointer;
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
    *initial_stack_pointer = new_stack_pointer;
    return 0;
}

static int
sel4utils_stack_write_constant(vspace_t *current_vspace, vspace_t *target_vspace,
                               vka_t *vka, long value, uintptr_t *initial_stack_pointer)
{
    return sel4utils_stack_write(current_vspace, target_vspace, vka, &value, sizeof(value), initial_stack_pointer);
}

static int
sel4utils_stack_copy_args(vspace_t *current_vspace, vspace_t *target_vspace,
                          vka_t *vka, int argc, char *argv[], uintptr_t *dest_argv, uintptr_t *initial_stack_pointer)
{
    for (int i = 0; i < argc; i++) {
        int error = sel4utils_stack_write(current_vspace, target_vspace, vka, argv[i], strlen(argv[i]) + 1, initial_stack_pointer);
        if (error) {
            return error;
        }
        dest_argv[i] = *initial_stack_pointer;
        *initial_stack_pointer = ROUND_DOWN(*initial_stack_pointer, 4);
    }
    return 0;
}

int
sel4utils_spawn_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc,
                        char *argv[], int resume)
{
    uintptr_t initial_stack_pointer = (uintptr_t)process->thread.stack_top - sizeof(seL4_Word);
    uintptr_t new_process_argv = 0;
    int error;
    /* write all the strings into the stack */
    if (argc > 0) {
        uintptr_t dest_argv[argc];
        /* Copy over the user arguments */
        error = sel4utils_stack_copy_args(vspace, &process->vspace, vka, argc, argv, dest_argv, &initial_stack_pointer);
        if (error) {
            return -1;
        }
        /* Put the new argv array on as well */
        error = sel4utils_stack_write(vspace, &process->vspace, vka, dest_argv, sizeof(dest_argv), &initial_stack_pointer);
        if (error) {
            return -1;
        }
        new_process_argv = initial_stack_pointer;
    }
    /* move the stack pointer down to a place we can write to.
     * to be compatible with as many architectures as possible
     * we need to ensure double word alignment */
    initial_stack_pointer = ALIGN_DOWN(initial_stack_pointer - sizeof(seL4_Word), STACK_CALL_ALIGNMENT);

    seL4_UserContext context = {0};
    size_t context_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    error = sel4utils_arch_init_context_with_args(process->entry_point, (void *) (uintptr_t)argc,
                                        (void *) new_process_argv,
                                        (void *) process->thread.ipc_buffer_addr, false,
                                        (void *) initial_stack_pointer,
                                        &context, vka, vspace, &process->vspace);
    if (error) {
        return error;
    }

    process->thread.initial_stack_pointer = (void *) initial_stack_pointer;

    return seL4_TCB_WriteRegisters(process->thread.tcb.cptr, resume, 0, context_size, &context);
}

int
sel4utils_spawn_process_v(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace, int argc,
                          char *argv[], int resume)
{
    /* define an envp and auxp */
    int error;
    int envc = 1;
    AUTOFREE char *ipc_buf_env = NULL;
    error = asprintf(&ipc_buf_env, "IPCBUFFER=0x%"PRIxPTR"", process->thread.ipc_buffer_addr);
    if (error == -1) {
        return -1;
    }
    AUTOFREE char *tcb_cptr_buf_env = NULL;
    error = asprintf(&tcb_cptr_buf_env, "boot_tcb_cptr=0x%"PRIxPTR"", process->dest_tcb_cptr);
    if (error == -1) {
        return -1;
    }
    char *envp[] = {ipc_buf_env, tcb_cptr_buf_env};

    if (process->dest_tcb_cptr != 0) {
        envc++;
    }

    uintptr_t initial_stack_pointer = (uintptr_t) process->thread.stack_top - sizeof(seL4_Word);

    /* Copy the elf headers */
    uintptr_t at_phdr;
    error = sel4utils_stack_write(vspace, &process->vspace, vka, process->elf_phdrs,
                                  process->num_elf_phdrs * sizeof(Elf_Phdr), &initial_stack_pointer);
    if (error) {
        return -1;
    }
    at_phdr = initial_stack_pointer;

    /* initialize of aux vectors */
    int auxc = 4;
    Elf_auxv_t auxv[5];
    auxv[0].a_type = AT_PAGESZ;
    auxv[0].a_un.a_val = process->pagesz;
    auxv[1].a_type = AT_PHDR;
    auxv[1].a_un.a_val = at_phdr;
    auxv[2].a_type = AT_PHNUM;
    auxv[2].a_un.a_val = process->num_elf_phdrs;
    auxv[3].a_type = AT_PHENT;
    auxv[3].a_un.a_val = sizeof(Elf_Phdr);
    if(process->sysinfo) {
        auxv[4].a_type = AT_SYSINFO;
        auxv[4].a_un.a_val = process->sysinfo;
        auxc++;
    }

    seL4_UserContext context = {0};

    uintptr_t dest_argv[argc];
    uintptr_t dest_envp[envc];

    /* write all the strings into the stack */
    /* Copy over the user arguments */
    error = sel4utils_stack_copy_args(vspace, &process->vspace, vka, argc, argv, dest_argv, &initial_stack_pointer);
    if (error) {
        return -1;
    }

    /* copy the environment */
    error = sel4utils_stack_copy_args(vspace, &process->vspace, vka, envc, envp, dest_envp, &initial_stack_pointer);
    if (error) {
        return -1;
    }

    /* we need to make sure the stack is aligned to a double word boundary after we push on everything else
     * below this point. First, work out how much we are going to push */
    size_t to_push = 5 * sizeof(seL4_Word) + /* constants */
                    sizeof(auxv[0]) * auxc + /* aux */
                    sizeof(dest_argv) + /* args */
                    sizeof(dest_envp); /* env */
    uintptr_t hypothetical_stack_pointer = initial_stack_pointer - to_push;
    uintptr_t rounded_stack_pointer = ALIGN_DOWN(hypothetical_stack_pointer, STACK_CALL_ALIGNMENT);
    ptrdiff_t stack_rounding = hypothetical_stack_pointer - rounded_stack_pointer;
    initial_stack_pointer -= stack_rounding;

    /* construct initial stack frame */
    /* Null terminate aux */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &initial_stack_pointer);
    if (error) {
        return -1;
    }
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &initial_stack_pointer);
    if (error) {
        return -1;
    }
    /* write aux */
    error = sel4utils_stack_write(vspace, &process->vspace, vka, auxv, sizeof(auxv[0]) * auxc, &initial_stack_pointer);
    if (error) {
        return -1;
    }
    /* Null terminate environment */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &initial_stack_pointer);
    if (error) {
        return -1;
    }
    /* write environment */
    error = sel4utils_stack_write(vspace, &process->vspace, vka, dest_envp, sizeof(dest_envp), &initial_stack_pointer);
    if (error) {
        return -1;
    }
    /* Null terminate arguments */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, 0, &initial_stack_pointer);
    if (error) {
        return -1;
    }
    /* write arguments */
    error = sel4utils_stack_write(vspace, &process->vspace, vka, dest_argv, sizeof(dest_argv), &initial_stack_pointer);
    if (error) {
        return -1;
    }
    /* Push argument count */
    error = sel4utils_stack_write_constant(vspace, &process->vspace, vka, argc, &initial_stack_pointer);
    if (error) {
        return -1;
    }

    ZF_LOGD("Starting process at %p, stack %p\n", process->entry_point, (void *) initial_stack_pointer);
    assert(initial_stack_pointer % (2 * sizeof(seL4_Word)) == 0);
    error = sel4utils_arch_init_context(process->entry_point, (void *) initial_stack_pointer, &context);
    if (error) {
        return error;
    }

    process->thread.initial_stack_pointer = (void *) initial_stack_pointer;

    /* Write the registers */
    return seL4_TCB_WriteRegisters(process->thread.tcb.cptr, resume, 0, sizeof(context) / sizeof(seL4_Word),
                                  &context);
}

int
sel4utils_configure_process(sel4utils_process_t *process, vka_t *vka,
                            vspace_t *vspace, const char *image_name)
{
    sel4utils_process_config_t config = process_config_default(image_name, seL4_CapInitThreadASIDPool);
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
            ZF_LOGE("Failed to create region\n");
            for (int j = i - 1; j >= 0; j--) {
                vspace_free_reservation(vspace, regions[i].reservation);
            }
            return -1;
        }
    }

    return 0;
}

static seL4_CPtr
get_asid_pool(seL4_CPtr asid_pool)
{
    if (asid_pool == 0) {
        ZF_LOGW("This method will fail if run in a thread that is not in the root server cspace\n");
        asid_pool = seL4_CapInitThreadASIDPool;
    }

    return asid_pool;
}

static seL4_CPtr
assign_asid_pool(seL4_CPtr asid_pool, seL4_CPtr pd)
{
    int error = seL4_ARCH_ASIDPool_Assign(get_asid_pool(asid_pool), pd);
    if (error) {
        ZF_LOGE("Failed to assign asid pool\n");
    }

    return error;
}

static int
create_cspace(vka_t *vka, int size_bits, sel4utils_process_t *process,
              seL4_Word cspace_root_data, seL4_CPtr asid_pool)
{
    /* create a cspace */
    int error = vka_alloc_cnode_object(vka, size_bits, &process->cspace);
    if (error) {
        ZF_LOGE("Failed to create cspace: %d\n", error);
        return error;
    }

    process->cspace_size = size_bits;
    /* first slot is always 1, never allocate 0 as a cslot */
    process->cspace_next_free = 1;

    /*  mint the cnode cap into the process cspace */
    cspacepath_t src;
    vka_cspace_make_path(vka, process->cspace.cptr, &src);
    UNUSED seL4_CPtr slot = sel4utils_mint_cap_to_process(process, src, seL4_AllRights, cspace_root_data);
    assert(slot == SEL4UTILS_CNODE_SLOT);

    /* copy fault endpoint cap into process cspace */
    if (process->fault_endpoint.cptr != 0) {
        vka_cspace_make_path(vka, process->fault_endpoint.cptr, &src);
        slot = sel4utils_copy_path_to_process(process, src);
        assert(slot == SEL4UTILS_ENDPOINT_SLOT);
    } else {
        /* no fault endpoint, update slot so next will work */
        allocate_next_slot(process);
    }

    /* copy page directory cap into process cspace */
    vka_cspace_make_path(vka, process->pd.cptr, &src);
    slot = sel4utils_copy_path_to_process(process, src);
    assert(slot == SEL4UTILS_PD_SLOT);

    if (!config_set(CONFIG_X86_64)) {
        vka_cspace_make_path(vka, get_asid_pool(asid_pool), &src);
        slot = sel4utils_copy_path_to_process(process, src);
    } else {
        allocate_next_slot(process);
    }
    assert(slot == SEL4UTILS_ASID_POOL_SLOT);

    return 0;
}

static int
create_fault_endpoint(vka_t *vka, sel4utils_process_t *process)
{
    /* create a fault endpoint and put it into the cspace */
    int error = vka_alloc_endpoint(vka, &process->fault_endpoint);
    process->own_ep = true;
    if (error) {
        ZF_LOGE("Failed to allocate fault endpoint: %d\n", error);
        return error;
    }

    return 0;
}

int
sel4utils_configure_process_custom(sel4utils_process_t *process, vka_t *vka,
                                   vspace_t *spawner_vspace, sel4utils_process_config_t config)
{
    int error;
    sel4utils_alloc_data_t * data = NULL;
    memset(process, 0, sizeof(sel4utils_process_t));
    seL4_Word cspace_root_data = api_make_guard_skip_word(seL4_WordBits - config.one_level_cspace_size_bits);

    /* create a page directory */
    process->own_vspace = config.create_vspace;
    if (config.create_vspace) {
        error = vka_alloc_vspace_root(vka, &process->pd);
        if (error) {
            ZF_LOGE("Failed to allocate page directory for new process: %d\n", error);
            goto error;
        }

        /* assign an asid pool */
        if (!config_set(CONFIG_X86_64) &&
              assign_asid_pool(config.asid_pool, process->pd.cptr) != seL4_NoError) {
            goto error;
        }
    } else {
        process->pd = config.page_dir;
    }

    if (config.create_fault_endpoint) {
        if (create_fault_endpoint(vka, process) != 0) {
            goto error;
        }
    } else {
        process->fault_endpoint = config.fault_endpoint;
    }

    process->own_cspace = config.create_cspace;
    if (config.create_cspace) {
        if (create_cspace(vka, config.one_level_cspace_size_bits, process, cspace_root_data,
                          config.asid_pool) != 0) {
            goto error;
        }
    } else {
        process->cspace = config.cnode;
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
                ZF_LOGE("Failed to allocate memory for elf region information");
                goto error;
            }
            process->entry_point = sel4utils_elf_reserve(&process->vspace, config.image_name, process->elf_regions);
        }

        if (process->entry_point == NULL) {
            ZF_LOGE("Failed to load elf file\n");
            goto error;
        }

        process->sysinfo = sel4utils_elf_get_vsyscall(config.image_name);

        /* Retrieve the ELF phdrs */
        process->num_elf_phdrs = sel4utils_elf_num_phdrs(config.image_name);
        process->elf_phdrs = calloc(process->num_elf_phdrs, sizeof(Elf_Phdr));
        if (!process->elf_phdrs) {
            ZF_LOGE("Failed to allocate memory for elf phdr information");
            goto error;
        }
        sel4utils_elf_read_phdrs(config.image_name, process->num_elf_phdrs, process->elf_phdrs);
    } else {
        process->entry_point = config.entry_point;
        process->sysinfo = config.sysinfo;
    }

    /* select the default page size of machine this process is running on */
    process->pagesz = PAGE_SIZE_4K;

    /* create the thread, do this *after* elf-loading so that we don't clobber
     * the required virtual memory*/
    sel4utils_thread_config_t thread_config = {0};
    thread_config = thread_config_cspace(thread_config, process->cspace.cptr, cspace_root_data);
    if (config_set(CONFIG_KERNEL_RT)) {
        /* on the MCS kernel, use the fault endpoint in the current cspace */
        thread_config = thread_config_fault_endpoint(thread_config, process->fault_endpoint.cptr);
    } else if (process->fault_endpoint.cptr != 0) {
        /* on the master kernel, the fault ep must be in the cspace of the process */
        thread_config = thread_config_fault_endpoint(thread_config, SEL4UTILS_ENDPOINT_SLOT);
    }
    thread_config.sched_params = config.sched_params;
    thread_config.create_reply = config.create_cspace;
    error = sel4utils_configure_thread_config(vka, spawner_vspace, &process->vspace, thread_config,
                                              &process->thread);
    if (error) {
        ZF_LOGE("ERROR: failed to configure thread for new process %d\n", error);
        goto error;
    }

    /* copy tcb cap to cspace */
    if (config.create_cspace) {
        cspacepath_t src;
        vka_cspace_make_path(vka, process->thread.tcb.cptr, &src);
        UNUSED seL4_CPtr slot = sel4utils_copy_path_to_process(process, src);
        assert(slot == SEL4UTILS_TCB_SLOT);
        process->dest_tcb_cptr = SEL4UTILS_TCB_SLOT;
    } else {
        process->dest_tcb_cptr = config.dest_cspace_tcb_cptr;
    }

    if (config.create_cspace) {
        if(config_set(CONFIG_KERNEL_RT)) {
            seL4_CPtr UNUSED slot = sel4utils_copy_cap_to_process(process, vka, process->thread.sched_context.cptr);
            assert(slot == SEL4UTILS_SCHED_CONTEXT_SLOT);
            slot = sel4utils_copy_cap_to_process(process, vka, process->thread.reply.cptr);
            assert(slot == SEL4UTILS_REPLY_SLOT);
        } else {
            /* skip the sc slot */
            allocate_next_slot(process);
            /* skip the reply object slot */
            allocate_next_slot(process);
        }
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
            ZF_LOGE("Could not clean up vspace\n");
        }
    }

    if (process->elf_regions) {
        free(process->elf_regions);
    }

    if (process->elf_phdrs) {
        free(process->elf_phdrs);
    }

    if (data != NULL) {
        free(data);
    }

    return -1;
}

void
sel4utils_destroy_process(sel4utils_process_t *process, vka_t *vka)
{
    /* destroy the cnode */
    if (process->own_cspace) {
        cspacepath_t path;
        vka_cspace_make_path(vka, process->cspace.cptr, &path);
        /* need to revoke the cnode to remove any self references that would keep the object
         * alive when we try to delete it */
        vka_cnode_revoke(&path);
        vka_free_object(vka, &process->cspace);
    }


    /* destroy the thread */
    sel4utils_clean_up_thread(vka, &process->vspace, &process->thread);

    /* tear down the vspace */
    if (process->own_vspace) {
        vspace_tear_down(&process->vspace, VSPACE_FREE);
        /* free any objects created by the vspace */
        clear_objects(process, vka);
    }

    /* destroy the endpoint */
    if (process->own_ep && process->fault_endpoint.cptr != 0) {
        vka_free_object(vka, &process->fault_endpoint);
    }

    /* destroy the page directory */
    if (process->own_vspace) {
        vka_free_object(vka, &process->pd);
    }

    /* Free elf information */
    if (process->elf_regions) {
        free(process->elf_regions);
    }

    if (process->elf_phdrs) {
        free(process->elf_phdrs);
    }
}

seL4_CPtr
sel4utils_process_init_cap(void *data, seL4_CPtr cap)
{
    switch (cap) {
    case seL4_CapInitThreadTCB:
        return SEL4UTILS_TCB_SLOT;
    case seL4_CapInitThreadCNode:
        return SEL4UTILS_CNODE_SLOT;
    case seL4_CapInitThreadVSpace:
        return SEL4UTILS_PD_SLOT;
    case seL4_CapInitThreadASIDPool:
        return SEL4UTILS_ASID_POOL_SLOT;
#ifdef CONFIG_KERNEL_RT
    case seL4_CapInitThreadSC:
        return SEL4UTILS_SCHED_CONTEXT_SLOT;
#endif
    default:
        ZF_LOGE("sel4utils does not copy this cap (%zu) to new processes", cap);
        return seL4_CapNull;
    }
};

int
sel4utils_copy_timer_caps_to_process(timer_objects_t *to, timer_objects_t *from, vka_t *vka, sel4utils_process_t *process)
{
    if (to == NULL || from == NULL || vka == NULL || process == NULL) {
        ZF_LOGE("Invalid argument (is null): to: %p, from: %p, vka: %p, process: %p", to, from, vka, process);
        return EINVAL;
    }
    /* struct deep copy */
     *to = *from;

    /* copy irq caps */
    for (size_t i = 0; i < to->nirqs; i++) {
        to->irqs[i].handler_path.capPtr = sel4utils_copy_cap_to_process(process,
                vka, from->irqs[i].handler_path.capPtr);
    }

    /* copy pmem ut frame caps */
    for (size_t i = 0; i < to->nobjs; i++) {
        to->objs[i].obj.cptr = sel4utils_copy_cap_to_process(process,
                vka, from->objs[i].obj.cptr);
    }
    return 0;
}
