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

#include <stdio.h>
#include <bits/errno.h>

long sys_set_thread_area(va_list ap) {
    /* As part of the initialization of the C library we need to set the
     * thread area (also knows as the TLS base) for thread local storage.
     * As we do not properly support TLS we just ignore this call. Will
     * be fine provided we do not create multiple threads (through libc)
     * or use TLS */
    return 0;
}

long sys_set_tid_address(va_list ap) {
    return -ENOSYS;
}
