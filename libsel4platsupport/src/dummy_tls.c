/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <stdint.h>
#include <utils/attribute.h>

/* What we want to achieve here is a set of symbols (_tdata_start, _tdata_end and _tbss_end)
 * that, if not already defined, get defined to be all the same value. We do not care what that
 * value is, just that it's the same, such that any calculation on the size of the sections
 * these values represent comes out to 0. To do this we define a zero sized symbol, and then
 * declare each of our three symbols as a weak alias of that one */

static char dummy_tls[0];

extern uintptr_t _tdata_start WEAK_ALIAS(dummy_tls);
extern uintptr_t _tdata_end WEAK_ALIAS(dummy_tls);
extern uintptr_t _tbss_end WEAK_ALIAS(dummy_tls);
