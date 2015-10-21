/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdio.h>
#include <bits/errno.h>

long sys_set_thread_area(va_list ap) {
    return -ENOSYS;
}

long sys_set_tid_address(va_list ap) {
    return -ENOSYS;
}
