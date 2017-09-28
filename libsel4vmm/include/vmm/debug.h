/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <autoconf.h>
#include "vmm/vmm.h"

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

#ifdef CONFIG_LIB_VMM_DEBUG

    #define DPRINTF(lvl, ...) \
        do{ \
            if(lvl <= 0 || lvl < CONFIG_LIB_VMM_DEBUG_LEVEL){ \
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

#else /* CONFIG_LIB_VMM_DEBUG */

    #define DPRINTF(lvl, ...) do{ /* nothing */ }while(0)
    #define drun() do{ /* nothing */ }while(0)

#endif /* CONFIG_LIB_VMM_DEBUG */

#ifndef panic
    #define panic(msg) \
        do{ \
            printf(COLOUR_R "!!!!!!!!!!!! LIBVMM PANIC !!!!!!!!!!!!! \n"\
                   "    @ %s: %d | %s\n" COLOUR_RESET, __func__, __LINE__, msg);\
            assert(!msg);\
            while(1);\
        }while(0)
#endif

void vmm_print_guest_context(int, vmm_vcpu_t*);

