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

#define UART1_PADDR     0x13810000
#define UART_PADDR      UART1_PADDR

#define ULCON       0x0000 /* line control */
#define UCON        0x0004 /* control */
#define UFCON       0x0008 /* fifo control */
#define UMCON       0x000C /* modem control */
#define UTRSTAT     0x0010 /* TX/RX status */
#define UERSTAT     0x0014 /* RX error status */
#define UFSTAT      0x0018 /* FIFO status */
#define UMSTAT      0x001C /* modem status */
#define UTXH        0x0020 /* TX buffer */
#define URXH        0x0024 /* RX buffer */
#define UBRDIV      0x0028 /* baud rate divisor */
#define UFRACVAL    0x002C /* divisor fractional value */
#define UINTP       0x0030 /* interrupt pending */
#define UINTSP      0x0034 /* interrupt source pending */
#define UINTM       0x0038 /* interrupt mask */


#define UART_REG(x)    ((volatile seL4_Word *)(uart_page + (x)))

/* ULCON */
#define WORD_LENGTH_8   (3<<0)

/* UTRSTAT */
#define TX_EMPTY        (1<<2)
#define TXBUF_EMPTY     (1<<1)
#define RXBUF_READY     (1<<0)


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
    /* Wait for serial to become ready. */
    while ( (*UART_REG(UTRSTAT) & TXBUF_EMPTY) == 0 );

    /* Write out the next character. */
    *UART_REG(UTXH) = (c & 0xff);
    if(c == '\n')
        __plat_putchar('\r');
}

int
__plat_getchar(void)
{
    if ( (*UART_REG(UTRSTAT) & RXBUF_READY)){
        return (unsigned char)*UART_REG(URXH);
    } else {
        return EOF;
    }
}
