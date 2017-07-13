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

#include <platsupport/plat/serial.h>

#if defined(CONFIG_LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM1)
    #define DEFAULT_SERIAL_IOPORT SERIAL_CONSOLE_COM1_PORT
    #define DEFAULT_SERIAL_INTERRUPT SERIAL_CONSOLE_COM1_IRQ
#elif defined(CONFIG_LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM2)
    #define DEFAULT_SERIAL_IOPORT SERIAL_CONSOLE_COM2_PORT
    #define DEFAULT_SERIAL_INTERRUPT SERIAL_CONSOLE_COM2_IRQ
#elif defined(CONFIG_LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM3)
    #define DEFAULT_SERIAL_IOPORT SERIAL_CONSOLE_COM3_PORT
    #define DEFAULT_SERIAL_INTERRUPT SERIAL_CONSOLE_COM3_IRQ
#elif defined(CONFIG_LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM4)
    #define DEFAULT_SERIAL_IOPORT SERIAL_CONSOLE_COM4_PORT
    #define DEFAULT_SERIAL_INTERRUPT SERIAL_CONSOLE_COM4_IRQ
#elif defined(CONFIG_LIB_PLAT_SUPPORT_SERIAL_TEXT_EGA)
    /* Don't define a port for the EGA alphanumeric mode device */
#else
    #define DEFAULT_SERIAL_IOPORT SERIAL_CONSOLE_COM1_PORT
    #define DEFAULT_SERIAL_INTERRUPT SERIAL_CONSOLE_COM1_IRQ
#endif
