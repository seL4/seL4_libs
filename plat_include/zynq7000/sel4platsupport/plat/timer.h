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

#define TRIPPLE_TIMER0_PADDR    0xF8001000
#define TRIPPLE_TIMER0_IRQ0     42
#define TRIPPLE_TIMER0_IRQ1     43
#define TRIPPLE_TIMER0_IRQ2     44

#define TRIPPLE_TIMER1_PADDR    0xF8002000
#define TRIPPLE_TIMER1_IRQ0     69
#define TRIPPLE_TIMER1_IRQ1     70
#define TRIPPLE_TIMER1_IRQ2     71

#define DEFAULT_TIMER_PADDR     TRIPPLE_TIMER0_PADDR
#define DEFAULT_TIMER_INTERRUPT TRIPPLE_TIMER0_IRQ0


#endif /* _SEL4PLATSUPPORT_PLAT_TIMER_H */
