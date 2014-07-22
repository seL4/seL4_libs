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

#include <assert.h>
#include <sel4/sel4.h>
#include "../../plat_internal.h"

#include <autoconf.h>

#ifdef CONFIG_LIB_SEL4_PLAT_SUPPORT_X86_SERIAL_PUTCHAR

#if defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_SERIAL_PORT_X86_COM4)
#  define SERIAL_CONSOLE_PORT 0x2e8
#elif defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_SERIAL_PORT_X86_COM2)
#  define SERIAL_CONSOLE_PORT 0x2f8
#elif defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_SERIAL_PORT_X86_COM3)
#  define SERIAL_CONSOLE_PORT 0x3e8
#elif defined(CONFIG_LIB_SEL4_PLAT_SUPPORT_SERIAL_PORT_X86_COM1)
#  define SERIAL_CONSOLE_PORT 0x3f8
#else
#  error Serial port not set
#endif

/*
 * Port offsets
 * W    - write
 * R    - read 
 * RW   - read and write
 * DLAB - Alternate register function bit
 */
#define SERIAL_THR  0 /* Transmitter Holding Buffer (W ) DLAB = 0 */
#define SERIAL_RBR  0 /* Receiver Buffer            (R ) DLAB = 0 */ 
#define SERIAL_DLL  0 /* Divisor Latch Low Byte     (RW) DLAB = 1 */
#define SERIAL_IER  1 /* Interrupt Enable Register  (RW) DLAB = 0 */
#define SERIAL_DLH  1 /* Divisor Latch High Byte    (RW) DLAB = 1 */
#define SERIAL_IIR  2 /* Interrupt Identification   (R ) */ 
#define SERIAL_FCR  2 /* FIFO Control Register      (W ) */
#define SERIAL_LCR  3 /* Line Control Register      (RW) */
#define SERIAL_MCR  4 /* Modem Control Register     (RW) */
#define SERIAL_LSR  5 /* Line Status Register       (R ) */
#define SERIAL_MSR  6 /* Modem Status Register      (R ) */
#define SERIAL_SR   7 /* Scratch Register           (RW) */
#define CONSOLE(label) ((SERIAL_CONSOLE_PORT) + (SERIAL_##label))
#define SERIAL_DLAB (1<<7)

#ifdef CONFIG_X86_64

#define seL4_IA32_IOPort_Out8   seL4_X64_IOPort_Out8
#define seL4_IA32_IOPort_In8    seL4_X64_IOPort_In8
typedef seL4_X64_IOPort_In8_t   seL4_IA32_IOPort_In8_t;

#endif  /* end of X86_64 */

void
__plat_serial_input_init_IRQ(void)
{
}

void
__plat_serial_init(void)
{
    /* clear DLAB - Divisor Latch Access Bit */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(LCR), 0x00 & ~SERIAL_DLAB);
    /* disable generating interrupts */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(IER), 0x00);

    /* set DLAB to*/
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(LCR), 0x00 | SERIAL_DLAB);
    /* set low byte of divisor to 0x01 = 115200 baud */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(DLL), 0x01);
    /* set high byte of divisor to 0x00 */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(DLH), 0x00);

    /* line control register: set 8 bit, no parity, 1 stop bit; clear DLAB */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(LCR), 0x03 & ~SERIAL_DLAB);
    /* modem control register: set DTR/RTS/OUT2 */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(MCR), 0x0b);

    /* clear receiver port */
    seL4_IA32_IOPort_In8(seL4_CapIOPort, CONSOLE(RBR));
    /* clear line status port */
    seL4_IA32_IOPort_In8(seL4_CapIOPort, CONSOLE(LSR));
    /* clear modem status port */
    seL4_IA32_IOPort_In8(seL4_CapIOPort, CONSOLE(MSR));
}

void
__plat_putchar(int c)
{
    seL4_IA32_IOPort_In8_t res;

    /* Wait for serial to become ready. */
    do {
        res = seL4_IA32_IOPort_In8(seL4_CapIOPort, CONSOLE(LSR));
        assert(res.error == 0);
    } while (!(res.result & (1<<5)));

    /* Write out the next character. */
    seL4_IA32_IOPort_Out8(seL4_CapIOPort, CONSOLE(THR), c);
    if(c == '\n')
        __plat_putchar('\r');
}

int
__plat_getchar(void)
{
    seL4_IA32_IOPort_In8_t res;

    /* Wait for character to arrive */
    do {
        res = seL4_IA32_IOPort_In8(seL4_CapIOPort, CONSOLE(LSR));
        assert(res.error == 0);
    }while(!(res.result & (1<<0)));

    /* retrieve character */
    res = seL4_IA32_IOPort_In8(seL4_CapIOPort, CONSOLE(RBR));
    assert(res.error == 0);

    return res.result;
}

#endif
