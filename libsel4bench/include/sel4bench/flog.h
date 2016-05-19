/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

#include <autoconf.h>
#include <sel4bench/sel4bench.h>
#include <stdlib.h>
#include <utils/util.h>

typedef struct {
    ccnt_t *results;
    size_t length;
    size_t next;
    ccnt_t start;
} flog_t;

#ifdef CONFIG_FLOG
static inline flog_t *
flog_init(ccnt_t *results, int length) 
{
    flog_t *flog = calloc(1, sizeof(flog_t));
    if (unlikely(flog == NULL)) {
        ZF_LOGE("Failed to allocate flog");
        return NULL;
    }

    flog->results = results;
    flog->length = length;

    return flog;
}

static inline void
flog_end(flog_t *flog) {
    ccnt_t end;
    SEL4BENCH_READ_CCNT(end);
    if (likely(flog->start != 0 && flog->next < flog->length)) {
        flog->results[flog->next] = end - flog->start;
        flog->next++;
    }
}

static inline void
flog_start(flog_t *flog) {
    SEL4BENCH_READ_CCNT(flog->start);
}

static inline void
flog_free(flog_t *flog)
{
    free(flog);
}

#else

#define flog_init(r, l) NULL
#define flog_end(f)
#define flog_start(f)
#define flog_free(f)

#endif /* CONFIG_FLOG */
