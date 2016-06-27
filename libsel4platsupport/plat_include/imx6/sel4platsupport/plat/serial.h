/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#pragma once

#include <platsupport/plat/serial.h>

#if defined(CONFIG_PLAT_SABRE)
    #define DEFAULT_SERIAL_PADDR UART2_PADDR
    #define DEFAULT_SERIAL_INTERRUPT UART2_IRQ
#elif defined(CONFIG_PLAT_WANDQ)
    #define DEFAULT_SERIAL_PADDR UART1_PADDR
    #define DEFAULT_SERIAL_INTERRUPT UART1_IRQ
#endif

