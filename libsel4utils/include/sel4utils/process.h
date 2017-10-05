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

#include <sel4utils/thread.h>
#include <sel4utils/process_config.h>
#include <sel4utils/vspace.h>
#include <sel4utils/elf.h>
#include <sel4platsupport/timer.h>

#define WORD_STRING_SIZE ((CONFIG_WORD_SIZE / 3) + 1)

typedef struct object_node object_node_t;

struct object_node {
    vka_object_t object;
    object_node_t *next;
};

typedef struct {
    vka_object_t pd;
    vspace_t vspace;
    sel4utils_alloc_data_t data;
    vka_object_t cspace;
    uint32_t cspace_size;
    uint32_t cspace_next_free;
    sel4utils_thread_t thread;
    vka_object_t fault_endpoint;
    void *entry_point;
    uintptr_t sysinfo;
    /* cptr (with respect to the process cnode) of the tcb of the first thread (0 means not supplied) */
    seL4_CPtr dest_tcb_cptr;
    seL4_Word pagesz;
    object_node_t *allocated_object_list_head;
    /* ELF headers that describe the sections of the loaded image (at least as they
     * existed at load time). Is different to the elf_regions, which have reservations,
     * these are the original headers from the elf and include nonloaded information regions */
    int num_elf_phdrs;
    Elf_Phdr *elf_phdrs;
    /* if the elf wasn't loaded into the address space, this describes the regions.
     * this permits lazy loading / copy on write / page sharing / whatever crazy thing
     * you want to implement */
    int num_elf_regions;
    sel4utils_elf_region_t *elf_regions;
    bool own_vspace;
    bool own_cspace;
    bool own_ep;
} sel4utils_process_t;

/* sel4utils processes start with some caps in their cspace.
 * These are the caps
 */
enum sel4utils_cspace_layout {
    /* no cap in NULL */
    SEL4UTILS_NULL_SLOT = 0,
    /*
     * The root cnode (with appropriate guard)
     */
    SEL4UTILS_CNODE_SLOT = 1,
    /* The slot on the cspace that fault_endpoint is put if
     * sel4utils_configure_process is used.
     */
    SEL4UTILS_ENDPOINT_SLOT = 2,

    /* The page directory slot */
    SEL4UTILS_PD_SLOT = 3,

    /* the slot for the asid pool that this thread is in and can create threads
     * in. 0 if this kernel does not support asid pools */
    SEL4UTILS_ASID_POOL_SLOT = 4,

    /* the slot for this processes tcb */
    SEL4UTILS_TCB_SLOT = 5,

    /* the slot for this processes sc */
    SEL4UTILS_SCHED_CONTEXT_SLOT = 6,

    /* The slot for this processes reply object */
    SEL4UTILS_REPLY_SLOT = 7,

    /* First free slot in the cspace configured by sel4utils */
    SEL4UTILS_FIRST_FREE = 8
};

/**
 * Start a process, and copy arguments into the processes address space.
 *
 * This is intended to use when loading applications of the format:
 *
 * int main(int argc, char **argv) { };
 *
 * The third argument (in r2 for arm, on stack for ia32) will be the address of the ipc buffer.
 * This is intended to be loaded by the crt into the data word of the ipc buffer,
 * or you can just add it to your function prototype.
 *
 * Add the following to your crt to use it this way:
 *
 * arm: str r2, [r2, #484]
 * ia32: popl %ebp
 *       movl %ebp, 484(%ebp)
 *
 * @param process initialised sel4utils process struct.
 * @param vka     vka interface to use for allocation of frames.
 * @param vspace  the current vspace.
 * @param argc    the number of arguments.
 * @param argv    a pointer to an array of strings in the current vspace.
 * @param resume  1 to start the process, 0 to leave suspended.
 *
 * @return -1 on error, 0 on success.
 *
 */
int sel4utils_spawn_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace,
                            int argc, char *argv[], int resume);

/**
 * Start a process, and copy arguments into the processes address space.
 *
 * This is intended for use when loading applications that have a System V ABI compliant
 * entry point. This means that the entry point should *not* be a 'main' that is expecting
 * argc and argv in the form as passed here, but should be an _start routine that will
 * take a stack frame with the arguments on it and construct an appropriate invocation
 * to a main function. The stack frame passed to the entry point will have an auxv, envp
 * and argv
 *
 * @param process initialised sel4utils process struct.
 * @param vka     vka interface to use for allocation of frames.
 * @param vspace  the current vspace.
 * @param argc    the number of arguments.
 * @param argv    a pointer to an array of strings in the current vspace.
 * @param resume  1 to start the process, 0 to leave suspended.
 *
 * @return -1 on error, 0 on success.
 *
 */
int sel4utils_spawn_process_v(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace,
                              int argc, char *argv[], int resume);

/**
 * This is the function to use if you just want to set up a process as fast as possible.
 * It creates a simple cspace and vspace for you, allocates a fault endpoint and puts
 * it into the new cspace. The process will start at priority 0.
 *
 * If CONFIG_RT is enabled, it will not have a scheduling context.
 *
 * It loads the elf file into the new vspace, parses the elf file and stores the entry point
 * into process->entry_point.
 *
 * It uses the same vka for both allocations - the new process will not have an allocator.
 *
 * The process will have just one thread.
 *
 * Otherwise, use it as a model for loading processes and use the elf_load.
 *
 * WARNING: when launching processes on ia32 please ensure your calling convention
 * matches that documented in sel4utils_start_thread. Otherwise your arguments
 * won't come through correctly.
 *
 * @param process      uninitialised process struct.
 * @param vka          allocator to use to allocate objects.
 * @param vspace vspace allocator for the current vspace.
 * @param image_name   name of the elf image to load from the cpio archive.
 *
 * @return 0 on success, -1 on error.
 */
int sel4utils_configure_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace,
                                const char *image_name);

/**
 * Configure a process with more customisations (Create your own vspace, customise cspace size).
 *
 * @param process               uninitialised process struct
 * @param vka                   allocator to use to allocate objects.
 * @param spawner_vspace        vspace to use to allocate virtual memory in the current address space.
 * @param config process config.
 *
 * @return 0 on success, -1 on error.
 */
int sel4utils_configure_process_custom(sel4utils_process_t *process, vka_t *target_vka,
                                       vspace_t *spawner_vspace, sel4utils_process_config_t config);

/**
 * Copy a cap into a process' cspace.
 *
 * This will only work if you configured the process using one of the above functions, or
 * have mimicked their functionality.
 *
 * @param process process to copy the cap to
 * @param src     path in the current cspace to copy the cap from
 *
 * @return 0 on failure, otherwise the slot in the processes cspace.
 */
seL4_CPtr sel4utils_copy_path_to_process(sel4utils_process_t *process, cspacepath_t src);

/**
 * Copy a cap into a process' cspace.
 *
 * This will only work if you configured the process using one of the above functions, or
 * have mimicked their functionality.
 *
 * @param process process to copy the cap to
 * @param vka     vka that can translate the cap into a cspacepath_t.
 * @param cap     cap location in your own cspace.
 *
 * @return 0 on failure, otherwise the slot in the processes cspace.
 */
seL4_CPtr sel4utils_copy_cap_to_process(sel4utils_process_t *process, vka_t *vka, seL4_CPtr cap);

/**
 * Move a cap into a process' cspace.
 *
 * This will only work if you configured the process using one of the above functions, or
 * have mimicked their functionality.  Additionally the VKA that is passed in to have the
 * slot freed can only do this if it is tracking that slot.  IE when moving one of the initial
 * caps provided by bootinfo such as the IRQ Control Cap the VKA may not be able to free the slot.
 * Passing in NULL for the VKA will result in no slot being freed.
 *
 * @param process process to move the cap to
 * @param src     path in the current cspace to move the cap from
 * @param vka     the allocator that owns the cslot the cap is currently in, so it may be freed after the move.
 *                if NULL, then the original slot will not be freed.
 *
 * @return 0 on failure, otherwise the slot in the processes cspace.
 */
seL4_CPtr sel4utils_move_cap_to_process(sel4utils_process_t *process, cspacepath_t src, vka_t *from_vka);

/**
 *
 * Mint a cap into a process' cspace.
 *
 * As above, except mint the cap with rights and data.
 *
 * @return 0 on failure, otherwise the slot in the cspace where the new cap is.
 */
seL4_CPtr sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights_t rights, seL4_Word data);

/**
 * Destroy a process.
 *
 * This will free everything possible associated with a process and teardown the vspace.
 *
 * @param process process to destroy
 * @param vka allocator used to allocate objects for this process
 */
void sel4utils_destroy_process(sel4utils_process_t *process, vka_t *vka);

/*
 * sel4utils default allocated object function for vspaces.
 *
 * Stores a list of allocated objects in the process struct and frees them
 * when sel4utils_destroy_process is called.
 */
void sel4utils_allocated_object(void *cookie, vka_object_t object);

/*
 * Create c-formatted argument list to pass to a process from arbitrarily long amount of words.
 *
 * @param strings empty 2d array of chars to populate with word strings.
 * @param argv empty 1d array of char pointers which will be set up with pointers to
 *             strings in strings.
 * @param argc number of words
 * @param ... list of words to create arguments from.
 *
 */
void sel4utils_create_word_args(char strings[][WORD_STRING_SIZE], char *argv[], int argc, ...);

/*
 * Get the initial cap equivalent for a sel4utils created process.
 * This will only work if sel4utils created the processes cspace.
 *
 * @param cap initial cap index from sel4/bootinfo_types.h
 * @return the corresponding cap in a sel4utils launched process.
 */
seL4_CPtr sel4utils_process_init_cap(void *data, seL4_CPtr cap);

/*
 * Helper function for copying timer IRQs and Device untyped caps into another process.
 *
 * @param to        struct to store new cap info
 * @param from      struct containing current cap info
 * @param vka       vka to create cnodes
 * @param process   target process to copy caps too
 *
 * @return 0 on success.
 */
int sel4utils_copy_timer_caps_to_process(timer_objects_t *to, timer_objects_t *from, vka_t *vka, sel4utils_process_t *process);
