/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
/*
 * Provide serial UART glue to hardware. This file is not used on debug
 * kernels, as we use a seL4 debug syscall to print to the serial console.
 */

#include <sel4/sel4.h>
#include "../../plat_internal.h"
#include <stdio.h>

#define UART_PADDR     0x16640000

#define USR       0x0008 /* Status register */
#define UCR       0x0010 /* Control register */
#define UTF       0x0070 /* TX fifo */
#define UNTX      0x0040 /* Number of bytes to send */

#define USR_TXRDY             (1U << 2)
#define USR_TXEMP             (1U << 3)

#define CMD_TXRDY_RESET       (3U << 8)

#define UART_REG(x)    ((volatile uint32_t *)(uart_page + (x)))


static void* uart_page;

void
__plat_serial_input_init_IRQ(void)
{
}

void
__plat_serial_init(void)
{
    uart_page = __map_device_page(UART_PADDR, 12);
}

void
__plat_putchar(int c)
{
    while(!(*UART_REG(USR) & USR_TXEMP));

    *UART_REG(UNTX) = 1;
    *UART_REG(UTF) = c & 0xff;
    if(c == '\n')
        __plat_putchar('\r');
}

int
__plat_getchar(void)
{
    return EOF;
}
