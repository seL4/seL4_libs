/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define	sel4_error(e, str)	((e == seL4_NoError) ? (void)0 : __sel4_error(e, __FILE__, __func__, __LINE__, str))

void __sel4_error(int, const char *, const char *, int, char *);

const char *sel4_strerror(int errcode);
const char *sel4_strfault(int errcode);
extern char *sel4_errlist[];
extern char *sel4_faultlist[];
