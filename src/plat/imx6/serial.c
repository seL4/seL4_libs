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

#define UART5_PADDR    0x021F4000
#define UART4_PADDR    0x021F0000
#define UART3_PADDR    0x021EC000
#define UART2_PADDR    0x021E8000
#define UART1_PADDR    0x02020000

#define UART_PADDR     UART2_PADDR
#define UART_REG(x)    ((volatile seL4_Word *)(uart_page + (x)))
#define UART_SR1_TRDY  13
#define UART_SR1_RRDY  9

#define URXD  0x00 /* UART Receiver Register */
#define UTXD  0x40 /* UART Transmitter Register */
#define UCR1  0x80 /* UART Control Register 1 */
#define UCR2  0x84 /* UART Control Register 2 */
#define UCR3  0x88 /* UART Control Register 3 */
#define UCR4  0x8c /* UART Control Register 4 */
#define UFCR  0x90 /* UART FIFO Control Register */
#define USR1  0x94 /* UART Status Register 1 */
#define USR2  0x98 /* UART Status Register 2 */
#define UESC  0x9c /* UART Escape Character Register */
#define UTIM  0xa0 /* UART Escape Timer Register */
#define UBIR  0xa4 /* UART BRM Incremental Register */
#define UBMR  0xa8 /* UART BRM Modulator Register */
#define UBRC  0xac /* UART Baud Rate Counter Register */
#define ONEMS 0xb0 /* UART One Millisecond Register */
#define UTS   0xb4 /* UART Test Register */



#define UART_URXD_READY_MASK (1 << 15)
#define UART_BYTE_MASK       0xFF

#define UART_SR2_TXFIFO_EMPTY 14
#define UART_SR2_RXFIFO_RDR    0

static void* uart_page;

void
__plat_serial_input_init_IRQ(void)
{
    /* Turn on receiver interrupt. */
    *UART_REG(UCR1) |= (1 << 9);
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
    while (!(*UART_REG(USR2) & BIT(UART_SR2_TXFIFO_EMPTY)));

    /* Write out the next character. */
    *UART_REG(UTXD) = c;
    if(c == '\n')
        __plat_putchar('\r');
}

int
__plat_getchar(void)
{
    seL4_Word reg = 0;
    int character = 0;

    while (!(*UART_REG(USR2) & BIT(UART_SR2_RXFIFO_RDR)));

    reg = *UART_REG(URXD);
    if (reg & UART_URXD_READY_MASK) {
        character = reg & UART_BYTE_MASK;
    }

    return character;
}
