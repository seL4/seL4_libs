/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*header file for serial driver*/
#ifndef __LIB_VMM_DRIVER_SERIAL_H__
#define __LIB_VMM_DRIVER_SERIAL_H__

#include "vmm/config.h"

#ifdef LIB_VMM_HW_BEAST
#  define SERIAL_CONSOLE_PORT 0x2f8
#endif


#ifdef LIB_VMM_HW_BOOST
#  define SERIAL_CONSOLE_PORT 0x3f8
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


/*entry function for serial driver*/
void vmm_driver_serial_run(void);

#endif

