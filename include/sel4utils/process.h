/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef SEL4UTILS_PROCESS_H
#define SEL4UTILS_PROCESS_H

#include <autoconf.h>

#if (defined CONFIG_LIB_SEL4_VSPACE && defined CONFIG_LIB_SEL4_VKA)

#include <vka/vka.h>
#include <vspace/vspace.h>

#include <sel4utils/thread.h>
#include <sel4utils/vspace.h>
#include <sel4utils/elf.h>

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
    object_node_t *allocated_object_list_head;
    /* if the elf wasn't loaded into the address space, this describes the regions.
     * this permits lazy loading / copy on write / page sharing / whatever crazy thing
     * you want to implement */
    int num_elf_regions;
    sel4utils_elf_region_t *elf_regions;
} sel4utils_process_t;

/* sel4utils processes start with some caps in their cspace.
 * These are the caps
 */
enum sel4utils_cspace_layout {
    /*
     * The root cnode (with appropriate guard)
     */
    SEL4UTILS_CNODE_SLOT = 1,
    /* The slot on the cspace that fault_endpoint is put if
     * sel4utils_configure_process is used.
     */
    SEL4UTILS_ENDPOINT_SLOT = 2,
};

typedef struct {
    /* should we handle elf logic at all? */
    bool is_elf;
    /* if so what is the image name? */
    char *image_name;
    /* Do you want the elf image preloaded? */
    bool do_elf_load;
    
    /* otherwise what is the entry point? */
    void *entry_point;

    /* should we create a default single level cspace? */
    bool create_cspace;
    /* if so how big ? */
    int one_level_cspace_size_bits;
    
    /* otherwise what is the root cnode ?*/
    /* Note if you use a custom cspace then 
     * sel4utils_copy_cap_to_process etc will not work */
    vka_object_t cnode;

    /* do you want us to create a vspace for you? */
    bool create_vspace;
    /* if not what is the page dir, and what is the vspace */
    vspace_t *vspace;
    vka_object_t page_dir;

    /* if so, is there a regions you want left clear?*/
    sel4utils_elf_region_t *reservations;
    int num_reservations;

    /* do you want a fault endpoint created? */ 
    bool create_fault_endpoint;
    /* otherwise what is it */
    vka_object_t fault_endpoint;

    int priority;
#ifndef CONFIG_KERNEL_STABLE
    seL4_CPtr asid_pool;
#endif
} sel4utils_process_config_t;

/**
 * Copy data from one vspace to another. Data is in POSIX standard argc, argv style.
 *
 * This is intented to use when loading applications of the format:
 *
 * int main(int argc, char **argv) { };
 *
 *
 * @param current the vspace we are loading the arguments from
 * @param target  the vspace we are loading the arguments to
 * @param vka     the vka allocation interface to use when allocating pages.
 *                the pages will first be mapped into the current vspace, then unmapped and
 *                mapped into the target vspace, and left that way.
 * @param argc    the number of strings
 * @param argv    an array of c strings.
 *
 * @return NULL on failure, otherwise a pointer in the target address space where argv has
 *              been written.
 */
void *sel4utils_copy_args(vspace_t *current, vspace_t *target, vka_t *vka,
        int argc, char *argv[]);

/**
 * Start a process, and copy arguments into the processes address space.
 *
 * This is intented to use when loading applications of the format:
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
 * This is the function to use if you just want to set up a process as fast as possible.
 * It creates a simple cspace and vspace for you, allocates a fault endpoint and puts
 * it into the new cspace.
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
 * @param priority     priority to configure the process to run as.
 * @param image_name   name of the elf image to load from the cpio archive.
 *
 * @return 0 on success, -1 on error.
 */
int sel4utils_configure_process(sel4utils_process_t *process, vka_t *vka, vspace_t *vspace,
        uint8_t priority, char *image_name);

/**
 * Configure a process with more customisations (Create your own vspace, customise cspace size).
 *
 * @param process               uninitliased process struct
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
 * have mimiced their functionality.
 *
 * @param process process to copy the cap to
 * @param src     path in the current cspace to copy the cap from
 *
 * @return 0 on failure, otherwise the slot in the processes cspace.
 */
seL4_CPtr sel4utils_copy_cap_to_process(sel4utils_process_t *process, cspacepath_t src);

/**
 *
 * Mint a cap into a process' cspace.
 *
 * As above, except mint the cap with rights and data.
 *
 * @return 0 on failure, otherwise the slot in the cspace where the new cap is.
 */
seL4_CPtr sel4utils_mint_cap_to_process(sel4utils_process_t *process, cspacepath_t src, seL4_CapRights rights, seL4_CapData_t data);

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

#endif /* (defined CONFIG_LIB_SEL4_VSPACE && defined CONFIG_LIB_SEL4_VKA) */
#endif /* SEL4UTILS_PROCESS_H */
