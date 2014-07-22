/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * This implements a virtual address space that proxies calls two different
 * vspaces to ensure that pages in one are mapped into the other. This allows
 * a server to efficiently read/write memory from a client. No attempt is made
 * to construct contiguous regions in the server when contiguous regions are
 * created in the client.
 * Currently cannot correctly handle stacks (since their size is unknown to us)
 * and cacheability attributes (currently assume all cached mappings)
 */
#ifndef _UTILS_CLIENT_SERVER_VSPACE_H
#define _UTILS_CLIENT_SERVER_VSPACE_H

#include <autoconf.h>

#if defined CONFIG_LIB_SEL4_VSPACE && defined CONFIG_LIB_SEL4_VKA

#include <vspace/vspace.h>
#include <vka/vka.h>

/**
 * Initialize a new client server vspace
 *
 * @param vspace vspace struct to initialize
 * @param vka vka for future allocations
 * @param server vspace to duplicate all mappings into
 * @param client vspace to directly proxy calls to
 *
 * @return 0 on success
 */
int sel4utils_get_cs_vspace(vspace_t *vspace, vka_t *vka, vspace_t *server, vspace_t *client);

/**
 * Translate virtual address in client to one in the server.
 * This is only for that address! Pages are not contiguous
 *
 * @param vspace vspace to do translation with
 * @param addr addr to lookup in the client
 *
 * @return Address in the server vspace, or NULL if not found
 */
void *sel4utils_cs_vspace_translate(vspace_t *vspace, void *addr);

/**
 * Perform a callback for every contiguous range of bytes in the server vspace
 * for the range provided in the client vspace
 *
 * @param vspace vspace to do translation in
 * @param addr Address in the client to start translation from
 * @param len length of the range in the client
 * @param proc Callback function that will be called back with ranges of address in the server after
 *  page boundary splits. If non zero is returned the loop is stopped and the error returned
 * @param cookie Cookie to be passed to the callback function
 *
 * @return Result of callback if non zero occured, internal error code if mapping not found, zero otherwise
 */
int sel4utils_cs_vspace_for_each(vspace_t *vspace, void *addr, uint32_t len,
        int (*proc)(void* addr, uint32_t len, void *cookie),
        void *cookie);

#endif /* CONFIG_LIB_SEL4_VSPACE && CONFIG_LIB_SEL4_VKA */
#endif /* _UTILS_CLIENT_SERVER_VSPACE_H */
