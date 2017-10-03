/* @TAG(CUSTOM) *//*
 * The Qubes OS Project, http://www.qubes-os.org
 *
 * Copyright (C) 2010  Rafal Wojtczuk  <rafal@invisiblethingslab.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
typedef int EVTCHN;

struct libvchan;
typedef struct libvchan libvchan_t;

/**
* Set up a vchan, including granting pages
* @return structure, or NULL in case of an error
*/
libvchan_t *libvchan_server_init(int domain, int port, size_t read_min, size_t write_min);

/**
* Connect to an existing vchan. Note: you can reconnect to an existing vchan
* safely, however no locking is performed, so you must prevent multiple clients
* from connecting to a single server.
*/
libvchan_t *libvchan_client_init(int domain, int port);

/**
* Waits for reads or writes to unblock, or for a close
*/
int libvchan_wait(libvchan_t *ctrl);

 /** Amount of data ready to read, in bytes */
int libvchan_data_ready(libvchan_t *ctrl);

/** Amount of data it is possible to send without blocking */
int libvchan_buffer_space(libvchan_t *ctrl);

/**
* Packet-based receive: always reads exactly $size bytes.
* @param ctrl The vchan control structure
* @param data Buffer for data that was read
* @param size Size of the buffer and amount of data to read
* @return -1 on error, 0 if nonblocking and insufficient data is available, or $size
*/
int libvchan_recv(libvchan_t *ctrl, void *data, size_t size);

/**
* Packet-based send: send entire buffer if possible
* @param ctrl The vchan control structure
* @param data Buffer for data to send
* @param size Size of the buffer and amount of data to send
* @return -1 on error, 0 if nonblocking and insufficient space is available, or $size
*/
int libvchan_send(libvchan_t *ctrl, const void *data, size_t size);

/**
* Stream-based receive: reads as much data as possible.
* @param ctrl The vchan control structure
* @param data Buffer for data that was read
* @param size Size of the buffer
* @return -1 on error, otherwise the amount of data read (which may be zero if
* the vchan is nonblocking)
*/
int libvchan_read(libvchan_t *ctrl, void *data, size_t size);

/**
* Stream-based send: send as much data as possible.
* @param ctrl The vchan control structure
* @param data Buffer for data to send
* @param size Size of the buffer
* @return -1 on error, otherwise the amount of data sent (which may be zero if
* the vchan is nonblocking)
*/
int libvchan_write(libvchan_t *ctrl, const void *data, size_t size);

/**
* Query the state of the vchan shared page:
* return 0 when one side has called libxenvchan_close() or crashed
* return 1 when both sides are open
* return 2 [server only] when no client has yet connected
*/
int libvchan_is_open(libvchan_t *ctrl);

/// returns nonzero if the peer has closed connection
int libvchan_is_eof(libvchan_t *ctrl);

/**
* Returns the event file descriptor for this vchan. When this FD is readable,
* libvchan() will not block, and the state of the vchan has changed since
* the last invocation of libvchan().
*/
int libvchan_fd_for_select(libvchan_t *ctrl);

/**
 * Notify the peer of closing.
 */
void libvchan_close(libvchan_t *ctrl);

