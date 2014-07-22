/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _LIB_VMM_DRIVERS_H_
#define _LIB_VMM_DRIVERS_H_

/* Entry functions for drivers. */
void vmm_driver_pci_run(void);
void vmm_driver_serial_run(void);
void vmm_driver_interrupt_manager_run(void);

#endif 






