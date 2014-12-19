/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __VMM_VCHAN_COMMON_INTF
#define __VMM_VCHAN_COMMON_INTF

#define VMM_MAGIC 'V'
#define NUM_VMM_OPS 50

/* vmm_args_t */
#define VMM_MANAGER_OPEN    0
#define VMM_MANAGER_CLOSE   1
#define VMM_PROBE           6
#define VMM_NUM_OF_DOM      7
#define VMM_LIST_DOM        8
#define VMM_CONNECT         14
#define VMM_DISCONNECT      15
#define VMM_TESTS           16

/* vchan_args_T */
#define VCHAN_SEND          19
#define VCHAN_RECV          20

/* vchan_check_args_t */
#define VMM_GUEST_NUM       23
#define SEL4_VCHAN_CONNECT  24
#define SEL4_VCHAN_CLOSE    25
#define SEL4_VCHAN_STATE    26
#define SEL4_VCHAN_WAIT     27
#define SEL4_VCHAN_BUF      28

#define DATATYPE_INT        0

#define DRIVER_NAME "vmm_manager"

/* Arguments used in initial vmcall call */
#define DRIVER_ARGS_MAX_SIZE 256
#define VMM_MANAGER_TOKEN 0xfabbdad

typedef int vmm_datatype_t;

/* User Level - Macros for interacting with the vmm_driver ioctl */
#define VM_IOCTL_CMD(cmd, sz) _IOR(VMM_MAGIC, cmd, sz)

/* Main structure used for a hypervisor call */
typedef struct vmcall_args {
    int cmd; /* Used for verifying that a vmcall is from a virtual linux driver */
    void *data; /* Pointer to a further data struct that is manipulated in a vmcall */
    int phys_data;
    int err;
    unsigned size;
} vmcall_args_t;

/* Used for passing arguments to the vchan linux driver */
typedef struct ioctl_arg {
    unsigned size;
    void *ptr;
} ioctl_arg_t;

/* Argument structure used for query commands to the hypervisor */
typedef struct vmm_args {
    char ret_data[DRIVER_ARGS_MAX_SIZE];
    vmm_datatype_t datatype;
} vmm_args_t;

#endif
