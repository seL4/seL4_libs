/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _UTILS_PROFILE_H_
#define _UTILS_PROFILE_H_

#include <sel4utils/util.h>
#include <stdint.h>

#define PROFILE_VAR_TYPE_INT32 1
#define PROFILE_VAR_TYPE_INT64 2

typedef struct profile_var {
    int type;
    void *var;
    const char *varname;
    const char *description;
} profile_var_t;

#define __WATCH_VAR(_type, _var, _description, _unique) \
    static profile_var_t profile_##_unique \
        __attribute__((used)) __attribute__((section("_profile_var"))) \
        = {.type = _type, .var = &_var, .varname = #_var, .description = _description}

#define _WATCH_VAR32(var, description, unique) \
    compile_time_assert(profile_size_##unique, sizeof(var) == 4); \
    __WATCH_VAR(PROFILE_VAR_TYPE_INT32, var, description, unique)

#define WATCH_VAR32(var, description) \
    _WATCH_VAR32(var, description, __LINE__)

#define _WATCH_VAR64(var, description, unique) \
    compile_time_assert(profile_size_##unique, sizeof(var) == 8); \
    __WATCH_VAR(PROFILE_VAR_TYPE_INT64, var, description, unique)

#define WATCH_VAR64(var, description) \
    _WATCH_VAR64(var, description, __LINE__)

typedef void (*profile_callback32)(uint32_t value, const char *varname, const char *descrption, void *cookie);
typedef void (*profile_callback64)(uint64_t value, const char *varname, const char *descrption, void *cookie);

/* Sample functions that just output the vars using printf. cookie is ignored */
void profile_print32(uint32_t value, const char *varname, const char *description, void *cookie);
void profile_print64(uint64_t value, const char *varname, const char *description, void *cookie);

/* Iterates over all profile variables and calls back the specified function(s)
 * with the current value. */
void profile_scrape(profile_callback32 callback32, profile_callback64 callback64, void *cookie);

#define _PSTART_TIME(x) uint32_t __begin_time##x = read_ccnt()
#define _PEND_TIME(x) uint32_t __end_time##x = read_ccnt()
#define _PTIME_DIFF(x) (__end_time##x - __begin_time##x)
#define PINC(name) PADD(name, 1)
#define PADDTIMEDIFF(name) PADD(name, PTIMEDIFF(name))
#define PADDTIME(name) PEND_TIME(name); PADDTIMEDIFF(name)
#define PTIMEDIFF(x) _PTIME_DIFF(x)

#ifdef PROFILE
#define PVARUINT32(name, desc) static uint32_t name = 0; WATCH_VAR32(name, desc);
#define PVARUINT64(name, desc) static uint64_t name = 0; WATCH_VAR64(name, desc);
#define PADD(name, x) do {name += (x); } while(0)
#define PSTART_TIME(x) _PSTART_TIME(x)
#define PEND_TIME(x) _PEND_TIME(x)
#else
#define PVARUINT32(name, desc)
#define PVARUINT64(name, desc)
#define PADD(name, x) do { } while(0)
#define PSTART_TIME(x) do { } while(0)
#define PEND_TIME(x) do { } while(0)
#endif

#endif
