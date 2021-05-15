/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
