/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <sel4utils/profile.h>
#include <stdio.h>
#include <inttypes.h>

/*
 *  __start_SECTION_NAME and __stop_SECTION_NAME are magic symbols inserted by the gcc
 *  linker
 */
extern profile_var_t __start__profile_var[];
extern profile_var_t __stop__profile_var[];

void profile_print32(uint32_t value, const char *varname, const char *description, void *cookie)
{
    printf("%s: %"PRIu32" %s\n", varname, value, description);
}
void profile_print64(uint64_t value, const char *varname, const char *description, void *cookie)
{
    printf("%s: %"PRIu64" %s\n", varname, value, description);
}

void profile_scrape(profile_callback32 callback32, profile_callback64 callback64, void *cookie)
{
    for (profile_var_t *i = __start__profile_var; i < __stop__profile_var; i++) {
        switch (i->type) {
        case PROFILE_VAR_TYPE_INT32:
            callback32(*(uint32_t*)i->var, i->varname, i->description, cookie);
            break;
        case PROFILE_VAR_TYPE_INT64:
            callback64(*(uint64_t*)i->var, i->varname, i->description, cookie);
            break;
        default:
            ZF_LOGE("Unknown profile var. Probable memory corruption or linker failure!");
            break;
        }
    }
}

void profile_reset(void)
{
    for (profile_var_t *i = __start__profile_var; i < __stop__profile_var; i++) {
        switch (i->type) {
        case PROFILE_VAR_TYPE_INT32:
            *(uint32_t*)i->var = 0;
            break;
        case PROFILE_VAR_TYPE_INT64:
            *(uint64_t*)i->var = 0;
            break;
        default:
            ZF_LOGE("Unknown profile var. Probable memory corruption or linker failure!");
            break;
        }
    }
}
