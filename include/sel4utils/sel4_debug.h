/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _SEL4_DEBUG_H_
#define _SEL4_DEBUG_H_

#define	sel4_error(e, str)	((e == seL4_NoError) ? (void)0 : __sel4_error(e, __FILE__, __func__, __LINE__, str))

void __sel4_error(int, const char *, const char *, int, char *);

extern char *sel4_errlist[];

#endif /* _SEL4_DEBUG_ */
