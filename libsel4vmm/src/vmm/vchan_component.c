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

#include "vmm/vmm.h"
#include "vmm/vmm_manager.h"
#include "vmm/vchan_component.h"
#include "vmm/debug.h"
#include "vmm/vchan_sharemem.h"

static libvchan_t *vchan_init(int domain, int port, int server);
static int libvchan_readwrite_action(libvchan_t *ctrl, void *data, size_t size, int stream, int action);

static camkes_vchan_con_t *vchan_comp_con = NULL;

/*
    Set up the vchan connection interface
    Currently, the number of vchan connection interfaces allowed is hardcoded to 1 per component
*/
void init_camkes_vchan(camkes_vchan_con_t *c) {
    vchan_comp_con = c;
}

/*
    Register a callback to be fired whenever a vchan event occurs
*/
int vchan_set_callback(callback_func_t cb, void *data) {
    vchan_comp_con->reg_callback(cb, data);
    return 0;
}

/*
    Create a new vchan server instance
*/
libvchan_t *libvchan_server_init(int domain, int port, size_t read_min, size_t write_min) {
    return vchan_init(domain, port, 1);
}

/*
    Create a new vchan client instance
*/
libvchan_t *libvchan_client_init(int domain, int port) {
    return vchan_init(domain, port, 0);
}

/*
    Create a new client/server instance
*/
libvchan_t *vchan_init(int domain, int port, int server) {
    if(vchan_comp_con == NULL) {
        return NULL;
    }

    libvchan_t *new_connection = malloc(sizeof(libvchan_t));
    if(new_connection == NULL) {
        return NULL;
    }

    new_connection->is_server = server;
    new_connection->server_persists = 1;
    new_connection->blocking = 1;
    new_connection->domain_num = domain;
    new_connection->port_num = port;
    new_connection->con = vchan_comp_con;

    /* Perform vchan component initialisation */
    vchan_connect_t t = {
        .v.domain = vchan_comp_con->component_dom_num,
        .v.dest = domain,
        .v.port = port,
        .server = new_connection->is_server,
    };

    if(vchan_comp_con->connect(t) < 0) {
        free(new_connection);
        return NULL;
    }

    return new_connection;
}

/*
    Reading and writing to the vchan
*/
int libvchan_write(libvchan_t *ctrl, const void *data, size_t size) {
    return libvchan_readwrite_action(ctrl, (void *) data, size, 1, VCHAN_SEND);
}

int libvchan_send(libvchan_t *ctrl, const void *data, size_t size) {
    return libvchan_readwrite_action(ctrl, (void *) data, size, 1, VCHAN_SEND);
}

int libvchan_read(libvchan_t *ctrl, void *data, size_t size) {
    return libvchan_readwrite_action(ctrl, data, size, 1, VCHAN_RECV);
}

int libvchan_recv(libvchan_t *ctrl, void *data, size_t size) {
    return libvchan_readwrite_action(ctrl, data, size, 1, VCHAN_RECV);
}

/*
    Return correct buffer for given vchan read/write action
*/
vchan_buf_t *get_vchan_buf(vchan_ctrl_t *args, camkes_vchan_con_t *c, int action) {
    if(c->data_buf == NULL) {
        ZF_LOGE("Mangled vchan connection: null data buffer\n");
        return NULL;
    }

    int buf_pos = c->get_buf(*args, action);
    /* Check that a buffer was retrieved */
    if(buf_pos < 0) {
        return NULL;
    }

    void *addr = c->data_buf;
    addr += buf_pos;
    vchan_buf_t *b = (vchan_buf_t *) (addr);

    return b;
}

/*
    Helper function
    Allows connected components to get correct vchan buffer for read/write
*/
vchan_buf_t *get_vchan_ctrl_databuf(libvchan_t *ctrl, int action) {
    vchan_ctrl_t args = {
        .domain = ctrl->con->component_dom_num,
        .dest = ctrl->domain_num,
        .port = ctrl->port_num,
    };

    return get_vchan_buf(&args, ctrl->con, action);
}

/*
    Perform a vchan read/write action into a given buffer
     This function is intended for non Init components, Init components have a different method
*/
int libvchan_readwrite_action(libvchan_t *ctrl, void *data, size_t size, int stream, int action) {
    int *update;
    vchan_buf_t *b = get_vchan_ctrl_databuf(ctrl, action);
    if(b == NULL) {
        return -1;
    }

    /*
        How data is stored in a given vchan buffer

        Position of data in buffer is given by
            (either b->write_pos or b->read_pos) % VCHAN_BUF_SIZE
            read_pos is incremented by x when x bytes are read from the buffer
            write_pos is incremented by x when x bytes are read from the buffer

        Amount of bytes in buffer is given by the difference between read_pos and write_pos
            if write_pos > read_pos, there is data yet to be read
    */
    size_t filled = abs(b->write_pos - b->read_pos);
    if(action == VCHAN_SEND) {
        while(filled == VCHAN_BUF_SIZE) {
            ctrl->con->wait();
            filled = abs(b->write_pos - b->read_pos);
        }

        if(stream) {
            size = MIN(VCHAN_BUF_SIZE - filled, size);
        } else if(size > VCHAN_BUF_SIZE - filled) {
            return -1;
        }

        update = &b->write_pos;
    } else {
        while(filled == 0) {
            ctrl->con->wait();
            filled = abs(b->write_pos - b->read_pos);
        }

        if(stream) {
            size = MIN(filled, size);
        } else if(size > filled) {
            return -1;
        }

        update = &b->read_pos;
    }

    /*
        Because this buffer is circular,
            data may have to wrap around to the start of the buffer
            This is achieved by doing two copies, one to buffer end
            And one at start of buffer for remaining data

        E.g if buffer size = 12
        and if write pos = 7 && number of bytes to write = 8

        Start:
                write_pos
                    V
            [oooooooooooo]
        End:
            write_pos
                V
            [xxxooooxxxxx]

    */
    off_t start = (*update % VCHAN_BUF_SIZE);
    off_t remain = 0;

    if(start + size > VCHAN_BUF_SIZE) {
        remain = (start + size) - VCHAN_BUF_SIZE;
        size -= remain;
    }

    void *dbuf = &b->sync_data;

    if(action == VCHAN_SEND) {
        memcpy(dbuf + start, data, size);
        memcpy(dbuf, data + size, remain);
    } else {
        memcpy(data, ((void *) dbuf) + start, size);
        memcpy(data + size, dbuf, remain);
    }
    __sync_synchronize();

    /*
        Update either the read byte counter or the written byte counter
            With how much was written or read
    */
    *update += (size + remain);

    ctrl->con->alert();

    return (size + remain);
}

/*
    Wait for data to arrive to a component from a given vchan
*/
int libvchan_wait(libvchan_t *ctrl) {
    vchan_buf_t *b = get_vchan_ctrl_databuf(ctrl, VCHAN_RECV);
    assert(b != NULL);

    size_t filled = abs(b->write_pos - b->read_pos);
    while(filled == 0) {
        ctrl->con->wait();
        b = get_vchan_ctrl_databuf(ctrl, VCHAN_RECV);
        filled = abs(b->write_pos - b->read_pos);
    }

    return 0;
}

void libvchan_close(libvchan_t *ctrl) {
    /* Perform vchan component initialisation */
    vchan_connect_t t = {
        .v.domain = ctrl->con->component_dom_num,
        .v.dest = ctrl->domain_num,
        .v.port = ctrl->port_num,
        .server = ctrl->is_server,
    };

    ctrl->con->disconnect(t);
    free(ctrl);
    ctrl = NULL;
}

int libvchan_is_open(libvchan_t *ctrl) {
    vchan_ctrl_t args = {
        .domain = ctrl->con->component_dom_num,
        .dest = ctrl->domain_num,
        .port = ctrl->port_num,
    };

    return ctrl->con->status(args);
}

/*
    How much data can be read from the vchan
*/
int libvchan_data_ready(libvchan_t *ctrl) {
    vchan_buf_t *b = get_vchan_ctrl_databuf(ctrl, VCHAN_RECV);
    size_t filled = abs(b->write_pos - b->read_pos);
    return filled;
}

/*
    How much data can be written to the vchan
*/
int libvchan_buffer_space(libvchan_t *ctrl) {
    vchan_buf_t *b = get_vchan_ctrl_databuf(ctrl, VCHAN_SEND);
    size_t filled = abs(b->write_pos - b->read_pos);
    return VCHAN_BUF_SIZE - filled;
}
