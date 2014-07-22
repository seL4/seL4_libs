/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* The main header file for seL4 VMM.
 *     Authors:
 *         Qian Ge
 */

#ifndef _LIB_VMM_PLAT_SYS_H_
#define _LIB_VMM_PLAT_SYS_H_

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
#include <pci/virtual_pci.h>
#include "vmm/config.h"
#include "vmm/io.h"
#include <simple/simple.h>

/* Define badge for a PCI device. This rule is defined by seL4. */
#define PLAT_PCI_DEV_BADGE(id, bus, dev, fun)\
    (seL4_CapData){.raw = ((id) << 16) + ((bus) << 8) + ((dev) << 3) + (fun)}

/* Macros related with hardware platform. */
#define LIB_VMM_MAX_PCI_DEVICES 16
#define LIB_VMM_MAX_MEM_REGIONS 10
#define LIB_VMM_MAX_DEV_REGIONS 64

/* Macros related with software config. */
#define LIB_VMM_MAX_IO_HANDLERS 32

/* Macros related with hardware simulation config. */
#ifdef LIB_VMM_IOAPIC_ENABLE 
    #define LIB_VMM_EXT_INT         24
#else
    #define LIB_VMM_EXT_INT         16
#endif

/* CNode size for threads, this cnode contains 2^12 = 4096 slots. */
#define LIB_VMM_CNODE_SIZE       12

/* Thread priorities. */
#define LIB_VMM_DRIVER_PRIORITY     220
#define LIB_VMM_VMM_PRIORITY        210
#define LIB_VMM_GUEST_PRIORITY      200

/* Software flags. */
#define LIB_VMM_VMM_THREAD      1
#define LIB_VMM_GUEST_OS_THREAD 2
#define LIB_VMM_DRIVER_THREAD   3

/* VMM helper macros. */
#if CONFIG_VMM_USE_4M_FRAMES
#define LIB_VMM_FRAME_CAPS            1024
#define LIB_VMM_GUEST_PAGE_SIZE       (1 << seL4_4MBits)
#define LIB_VMM_GUEST_PAGE_MASK       0xffc00000
#define LIB_VMM_GUEST_STACK_SIZE      (1 << 5)
#else
#define LIB_VMM_FRAME_CAPS            (1024 * 1024)
#define LIB_VMM_GUEST_PAGE_SIZE       (1 << seL4_PageBits)
#define LIB_VMM_GUEST_PAGE_MASK       0xfffff000
#define LIB_VMM_GUEST_STACK_SIZE      (1 << 5)
#endif

#define LIB_VMM_HOST_PAGE_SIZE        (1 << seL4_PageBits)
#define LIB_VMM_HOST_PAGE_MASK        0xfffff000


#define LIB_VMM_STACK_PAGE_MASK       0xfffff000
#define LIB_VMM_STACK_PAGES           8

/* Attributes of guest OS page directory.
 * Present, write, user, page size 4M. */
#define LIB_VMM_GUEST_PAGE_DIR_ATTR    0x87  

/* Maximum number of IO port ranges per guest. Each device that is allowed on the guest that
 * requires access to an IO port memory range uses an entry here to do it. So this number limits the
 * number of IO port requiring devices the guest can have. */
#define LIB_VMM_GUEST_MAX_IOPORT_RANGES 128

/* Memory segment regions. */
typedef struct guest_mem_area {
   seL4_Word start; 
   seL4_Word end;
   uint16_t type;
   seL4_Word perm;
} guest_mem_area_t;


/* Resource list for threads that are booted by boot.c. */
typedef struct thread_rec {
    seL4_CPtr tcb;
    seL4_CPtr async_vmm_ep;
    seL4_CPtr vmm_ep;
    seL4_CPtr irq_ep;
    seL4_CPtr vcpu;          /* VCPU cap of guest thread. */
    seL4_CPtr pd;            /* EPT_PD for guest OS, PD for VMM*/
    seL4_CPtr cnode;
    seL4_CPtr ipc_cap;
    seL4_Word ipc_buffer;    /* IPC buffer vaddr (host). */
    seL4_Word stack_top;     /* Stack top vaddr (host). */
    seL4_Word badge_no;      /* Badge used by this thread. */

    vspace_t vspace;         /* Vspace management proxy. */
    sel4utils_alloc_data_t vspace_data;
    seL4_Word mem_start; 

    seL4_CPtr frame_caps[LIB_VMM_FRAME_CAPS]; /* Frame caps mapped into this vspace. */
    unsigned int flag;       /* Thread type. */
    char *elf_name;
    char *elf_params;
    char *thread_name;
    seL4_Word elf_entry;
 
    uint32_t n_mem_areas;
    guest_mem_area_t *mem_areas;

    uint32_t n_guest_bootinfo_mem_areas;
    guest_mem_area_t *guest_bootinfo_mem_areas;

    uint32_t guest_mem_start;
    int32_t guest_relocation_offset;
    seL4_Word guest_kernel_addr;
    seL4_Word guest_kernel_vaddr;
    seL4_Word guest_page_dir; /* Guest page directory address (for Linux). */
    seL4_Word guest_boot_info;
    seL4_Word guest_cmd_line;
    char *guest_initrd_addr;
    uint32_t guest_initrd_size;
    char *guest_initrd_filename;

    seL4_CPtr pci_iospace_caps[LIB_VMM_MAX_PCI_DEVICES];
    int num_pci_iospace;

    ioport_range_t guest_ioport_map[LIB_VMM_GUEST_MAX_IOPORT_RANGES];

    struct thread_rec *guest_host_thread;
} thread_rec_t;

/* Resource list for platform */
typedef struct plat_net {
    /* Network for host threads. */
    seL4_CPtr pci_ep;
    seL4_CPtr serial_ep;
    seL4_CPtr interrupt_ep;

    /* Fault endpoint for host threads*/
    seL4_CPtr fault_ep;
} plat_net_t;


/* Internal interfaces. */
void vmm_plat_print_thread_context(char *, seL4_UserContext *);

/* Handler entry point for init thread. Starts processing guest VM exists and distributing
 * then to the rest of the VMM threads. Does not return. */
void vmm_plat_fault_wait(void);

/* Create a guest OS thread. */
void vmm_plat_init_guest_thread(thread_rec_t *, char *, char *, thread_rec_t *, seL4_Word);
void vmm_plat_run_guest_thread(thread_rec_t *);


/* Create a host thread. */
void vmm_plat_init_host_thread(thread_rec_t *, unsigned int flag, char *, seL4_Word, char *);
void vmm_plat_run_host_thread(thread_rec_t *, vmm_driver_seq);


/* External interfaces. */

/* Initialise the VMM platform. Should be the first function to be called. */
void vmm_plat_init(simple_t *simple, uint32_t num_guest_os, char *guest_kernel, char *guest_initrd, void **existing_frames);

/* Initialise the VMM host driver threads. */
void vmm_plat_boot_serial(thread_rec_t *);
void vmm_plat_boot_pci(thread_rec_t *);
void vmm_plat_boot_interrupt_manager(thread_rec_t *, thread_rec_t *);

/* Init and start a VMM host thread. */
void vmm_plat_start_vmm(thread_rec_t *);

/* Init and start a guest OS thread. */
void vmm_plat_start_guest_os(thread_rec_t *thread, char *kernel_image, char *initrd_image,
                             thread_rec_t *host); 

#endif 
