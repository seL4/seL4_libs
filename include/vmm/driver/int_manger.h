/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */
/* Interrupt management driver.
 *     Authors:
 *         Adrian Danis
 */

#ifndef __LIB_VMM_DRIVER_INT_MANGER_H__
#define __LIB_VMM_DRIVER_INT_MANGER_H__

#include "vmm/config.h"

#define LABEL_INT_MAN_GET_INTERRUPT 1
#define LABEL_INT_MAN_HAS_INTERRUPT 2

/* Entry function for the interrupt manager driver. */
void vmm_driver_interrupt_manager_run(void);

#endif /* __LIB_VMM_DRIVER_INT_MANGER_H__ */

