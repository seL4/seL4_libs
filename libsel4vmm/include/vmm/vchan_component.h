/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include "libvchan.h"
#include "vchan_sharemem.h"
#include "vchan_copy.h"

typedef void (*callback_func_t)(void *);

typedef struct camkes_vchan_con {
    /* Domain */
    int component_dom_num;
    void *data_buf;

    /* Function Pointers */
    int (*connect)(vchan_connect_t);
    int (*disconnect)(vchan_connect_t);
    intptr_t (*get_buf)(vchan_ctrl_t, int);
    int (*status)(vchan_ctrl_t);
    int (*alert_status)(vchan_ctrl_t);

    void (*wait)(void);
    void (*alert)(void);
    int (*poll)(void);

    int (*reg_callback)(callback_func_t, void *);
} camkes_vchan_con_t;

struct libvchan {
    int is_server;
    int server_persists;
    int blocking;
    int domain_num, port_num;

    camkes_vchan_con_t *con;
};

void init_camkes_vchan(camkes_vchan_con_t *c);

int vchan_set_callback(callback_func_t cb, void *data);

vchan_buf_t *get_vchan_buf(vchan_ctrl_t *args, camkes_vchan_con_t *c, int action);

void vevent_wait(void);
int vevent_poll(void);
int vevent_reg_callback(void (*callback)(void*), void *arg);

