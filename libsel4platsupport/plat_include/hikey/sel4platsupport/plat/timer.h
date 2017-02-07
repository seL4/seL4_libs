/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _SEL4PLATSUPPORT_PLAT_TIMER_H
#define _SEL4PLATSUPPORT_PLAT_TIMER_H

#include <sel4platsupport/timer.h>
#include <platsupport/plat/timer.h>

#define DEFAULT_TIMER_PADDR DMTIMER0_PADDR
#define DEFAULT_TIMER_INTERRUPT DMTIMER0_INTERRUPT

seL4_timer_t *sel4platsupport_get_timer(enum timer_id id,
                                        vka_t *vka,
                                        vspace_t *vspace,
                                        simple_t *simple,
                                        seL4_CPtr notification);

/** This function stands distinct from sel4platsupport_get_timer, but is
 * conceptually not very different from it: it's really a special case getter
 * for a specific pseudo-device.
 *
 * To get an exact idea of how the pseudo-device works, see virtual_upcounter.c
 * in util_libs' libplatsupport.
 *
 * In short though, the virtual upcounter is a combination of a 1Hz upcounter
 * and one microsecond resolution downcounter. Since the Hikey doesn't provide
 * any high resolution upcounters, we use the virtual upcounter as an emulated
 * high-resolution upcounter.
 *
 * The virtual upcounter only provides timestamps, and not timeout events.
 * The underlying seL4_libs libsel4platsupport code for this function
 * presumes to use the RTC0 upcounter and the DMTIMER2 downcounter. If you plan
 * to use the virtual upcounter, you are strongly cautioned to avoid also
 * using those two devices separately, and to leave them to the virtual
 * upcounter.
 *
 * Furthermore, since two DMTIMERs share the same MMIO frame in pairs, the use
 * of DMTIMER2 implies that you should also avoid using DMTIMER3. At the
 * hardware level it's completely okay to use DMTIMER3, because the two devices
 * are distinct and have their own registers.
 *
 * But at the software level, since they both use the same 4K physical MMIO
 * frame, you may run into problems with vka_alloc_frame_at(), since you will
 * naturally end up requesting the same physical frame address twice.
 *
 * @return Handle to a sel4_timer_t which can provide timestamps.
 * @param vka Pointer to an initialized VKA
 * @param vspace Pointer to an initialized vspace
 * @param simple Pointer to an initialized simple
 * @param notification You can pass seL4_CapNull here, because the virtual
 *        upcounter doesn't use IRQs.
 */
seL4_timer_t *
sel4platsupport_hikey_get_vupcounter_timer(vka_t *vka, vspace_t *vspace,
                                           simple_t *simple,
                                           UNUSED seL4_CPtr notification);

#endif /* _SEL4PLATSUPPORT_PLAT_TIMER_H */
