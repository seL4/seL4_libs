/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/* This file contains the configurations of lib VMM.
 *
 *     Authors:
 *         Qian Ge
 *         Adrian Danis
 */

#ifndef _LIB_VMM_CONFIG_H_
#define _LIB_VMM_CONFIG_H_

#include <autoconf.h>

/* The common driver binary image. */
#define LIB_VMM_DRIVER_IMAGE    "common_init"

/* Error code for lib vmm. */
#define LIB_VMM_SUCC   0
#define LIB_VMM_ERR  (-1)

/* Max number of Guest OS running in the system. */
#define LIB_VMM_GUEST_NUMBER    4

#define LIB_VMM_HW_BOOST

/* Cap slots configuration. */
#define LIB_VMM_CAP_INVALID               0
/* Async driver <--> host thread. Used currently for interrupts. */
#define LIB_VMM_HOST_ASYNC_MESSAGE_CAP    1
/* Guest os <--> host thread. */
#define LIB_VMM_GUEST_OS_FAULT_EP_CAP     2
/* Host thread <--> vmcs structure */
#define LIB_VMM_VCPU_CAP             3
/* The TCB of the guest thread */
#define LIB_VMM_GUEST_TCB_CAP        4

/* Serial driver <--> IO ports emulator. */
#define LIB_VMM_IO_SERIAL_CAP        5
/* Host threads <---> serial driver. */
#define LIB_VMM_DRIVER_SERIAL_CAP    6
/* Fault ep for host threads */
#define LIB_VMM_FAULT_EP_CAP         7

/* Host threads <---> PCI driver */
#define LIB_VMM_DRIVER_PCI_CAP       8

/* PCI driver <---> IO ports */
#define LIB_VMM_IO_PCI_CAP           9

/* Host threads <-----> Interrupt driver */
#define LIB_VMM_DRIVER_INTERRUPT_CAP       10
#define LIB_VMM_DRIVER_INTERRUPT_HANDLER_START 11
#define LIB_VMM_DRIVER_INTERRUPT_HANDLER_END 26


/* Badge configuration */
#define LIB_VMM_VMM_BADGE_START     1
#define LIB_VMM_GUEST_OS_BADGE_START 10
#define LIB_VMM_DRIVER_SERIAL_BADGE  20
#define LIB_VMM_DRIVER_PCI_BADGE     21
#define LIB_VMM_DRIVER_INTERRUPT_BADGE     22

/* Badge bit that indicates it has received an async message. the badge must
 * then be interpreted on a case by case basis by each driver */
#define LIB_VMM_DRIVER_ASYNC_MESSAGE_BADGE 32

/* Max length of CPIO file strings. */
#define LIB_VMM_MAX_BUFFER_LEN 256

/* Sequence number for the vmm drivers */
typedef enum {
    serial_seq = 1, 
    pci_seq, 
    interrupt_manager_seq,
    vmm_seq
} vmm_driver_seq;

#ifndef COLOUR
    #define COLOUR "\033[;1;%dm"
    #define COLOUR_R "\033[;1;31m"
    #define COLOUR_G "\033[;1;32m"
    #define COLOUR_Y "\033[;1;33m"
    #define COLOUR_B "\033[;1;34m"
    #define COLOUR_M "\033[;1;35m"
    #define COLOUR_C "\033[;1;36m"
    #define COLOUR_RESET "\033[0m"
#endif

/* Lib VMM debug output flag. */
#ifdef CONFIG_VMM_DEBUG
    #define LIB_VMM_DEBUG
#endif

/* Debug vebposity level.
 * 5 levels for debug messages:
 *    0: Always printout 
 *    1: Main entry point in a module
 *    2: 2nd level entry point in a module
 *    3: Main entry point of a function 
 *    4: Details inside a function
 */
#ifdef LIB_VMM_DEBUG
    #define LIB_VMM_DEBUG_LEVEL CONFIG_VMM_DEBUG_LEVEL
#endif

#ifdef LIB_VMM_DEBUG_LEVEL

    #define dprintf(lvl, ...) \
        do{ \
            if(lvl <= 0 || lvl < LIB_VMM_DEBUG_LEVEL){ \
                printf("%s:%d | ", __func__, __LINE__); \
                printf(__VA_ARGS__); \
            } \
        }while(0)

    #define drun(lvl, cmd) \
        do{ \
            if(lvl < LIB_VMM_DEBUG_LEVEL){ \
                cmd; \
            } \
        }while(0)

#else /* LIB_VMM_DEBUG_LEVEL */

    #define dprintf(lvl, ...) do{ /* nothing */ }while(0)
    #define drun() do{ /* nothing */ }while(0)
    
#endif /* LIB_VMM_DEBUG_LEVEL */

#ifndef panic
    #define panic(msg) \
        do{ \
            printf(COLOUR_R "!!!!!!!!!!!! LIBVMM PANIC !!!!!!!!!!!!! \n"\
                   "    @ %s: %d | %s\n" COLOUR_RESET, __func__, __LINE__, msg);\
            assert(!msg);\
            while(1);\
        }while(0)
#endif

#ifndef _UNUSED_
    #define _UNUSED_ __attribute__((__unused__))
#endif
#ifndef _MAYBE_USED_
    #define _MAYBE_UNUSED_ __attribute__((__used__))
#endif

/* Guest OS booting/ mapping configuration. */

#ifdef CONFIG_VMM_HARDWARE_VMWARE
    #define VMM_HARDWARE_VMWARE
#endif

#if defined(CONFIG_VMM_GUEST_DMA_ONE_TO_ONE)
    #define LIB_VMM_GUEST_DMA_ONE_TO_ONE
#elif defined(CONFIG_VMM_GUEST_DMA_IOMMU)
    #define LIB_VMM_GUEST_DMA_IOMMU
#elif defined(CONFIG_VMM_GUEST_DMA_NONE)
    #define LIB_VMM_GUEST_DMA_none
#else
    #error "DMA implementation not chosen. Config error."
#endif

#ifdef CONFIG_VMM_INITRD_SUPPORT
    #define LIB_VMM_INITRD_SUPPORT
#endif
#ifdef CONFIG_VMM_VESA_FRAMEBUFFER
    #define LIB_VMM_VESA_FRAMEBUFFER
#endif

#endif /* _LIB_VMM_CONFIG_H_ */


