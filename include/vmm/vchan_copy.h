/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __VCHAN_COPY
#define __VCHAN_COPY

#define VCHAN_EVENT_IRQ 10

#define DATATYPE_INT        0
#define DATATYPE_STRING     1

/* Vchan defines */
#define VCHAN_PACKET_SIZE 4096
#define FILE_DATAPORT_MAX_SIZE 4096

#define NOWAIT_DATA_READY 1
#define NOWAIT_BUF_SPACE 2

#define VCHAN_CLIENT 0
#define VCHAN_SERVER 1

#define VCHAN_CLOSED_DATA 0
#define VCHAN_CLOSED      1
#define VCHAN_EMPTY_BUF   2
#define VCHAN_BUF_DATA    3
#define VCHAN_BUF_FULL    4

#define DPORT_KERN_ADDR 0xEE00000
#define VCHAN_DATA_TOKEN 0xDEADBEEF

/* Used in arguments referring to a vchan instance */
typedef struct vchan_ctrl {
    int domain;
    int dest;
    int port;
} vchan_ctrl_t;

/* Used in arguments referring to a vchan instance */
typedef struct vchan_alert {
    int alert;
    int dest;
    int port;
} vchan_alert_t;

/* Arguments structure used for vchan write/read actions between guest os's */
typedef struct vchan_args {
    vchan_ctrl_t v;
    void *mmap_ptr;
    int stream;
    int size;
    int mmap_phys_ptr;
} vchan_args_t;

/* Argument structure used for vchan  */
typedef struct vchan_check_args {
    vchan_ctrl_t v;
    int nowait;
    int state;
    int checktype;
} vchan_check_args_t;

/* Argument structure used for vchan connects  */
typedef struct vchan_connect {
    vchan_ctrl_t v;
    int server;
    int eventfd;
    unsigned event_mon;
} vchan_connect_t;

void volatile_copy(void *dest, void *rec, int size);

/* Used for helloworld testsuite */
#define MSG_HELLO   0
#define MSG_ACK     1
#define MSG_CONC    2
#define TEST_VCHAN_PAK_GUARD    0xBEEDEADA

typedef struct vchan_header {
    int msg_type;
    int len;
} vchan_header_t;

typedef struct vchan_packet {
    int pnum;
    int datah[4];
    int guard;
} vchan_packet_t;

#endif
